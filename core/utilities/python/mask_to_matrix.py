from loguru import logger
import numpy as np
import matplotlib.pyplot as plt
import argparse
import os
import polars as pl


# ============================================================
# Geometry / grid constants
# ============================================================
NUM_LEAVES = 26
LEAF_HEIGHT_MM = 2.5

FULL_FIELD_MM = 65.0
HALF_FIELD_MM = FULL_FIELD_MM / 2.0
X_MIN_MM = -HALF_FIELD_MM
X_MAX_MM = HALF_FIELD_MM

PIXEL_MM = 0.1
FULL_SIZE = int(round(FULL_FIELD_MM / PIXEL_MM))      # 650
ROW_REPEAT = int(round(LEAF_HEIGHT_MM / PIXEL_MM))    # 25
SMALL_SIZE = 64

if FULL_SIZE != NUM_LEAVES * ROW_REPEAT:
    raise ValueError(
        f"Inconsistent geometry: FULL_SIZE={FULL_SIZE}, "
        f"NUM_LEAVES*ROW_REPEAT={NUM_LEAVES * ROW_REPEAT}"
    )


# ============================================================
# Helpers
# ============================================================
def add_suffix_before_extension(path: str, suffix: str) -> str:
    root, ext = os.path.splitext(path)
    return f"{root}{suffix}{ext}"


def write_matrix_csv(path: str, matrix: np.ndarray) -> None:
    if np.issubdtype(matrix.dtype, np.integer) or np.issubdtype(matrix.dtype, np.bool_):
        np.savetxt(path, matrix, delimiter=",", fmt="%d")
    else:
        np.savetxt(path, matrix, delimiter=",", fmt="%.6f")


def build_polars_df(matrix: np.ndarray) -> pl.DataFrame:
    cols = [f"c{i:04d}" for i in range(matrix.shape[1])]
    return pl.DataFrame(data=matrix.tolist(), schema=cols, orient='row')


def parse_mlc_segments(lines: list[str]) -> list[list[float]]:
    idx = next((i for i, ln in enumerate(lines) if ln.strip().startswith('# MLC')), None)
    if idx is None:
        raise ValueError("No MLC section found")

    segments = []
    for line_no, ln in enumerate(lines[idx + 1:], start=idx + 2):
        txt = ln.strip()

        if not txt:
            break
        if txt.startswith('#'):
            break

        txt = txt.split('#', 1)[0].strip()
        parts = [p.strip() for p in txt.split(',') if p.strip()]

        if len(parts) != 2:
            logger.warning(f"Skipping malformed MLC line {line_no}: {ln!r}")
            continue

        try:
            y1, y2 = map(float, parts)
        except ValueError:
            logger.warning(f"Skipping non-numeric MLC line {line_no}: {ln!r}")
            continue

        segments.append(sorted([y1, y2]))

    logger.debug(f"Found {len(segments)} raw MLC segments")
    return segments


def normalize_to_26_segments(segments: list[list[float]]) -> list[list[float]]:
    if len(segments) > NUM_LEAVES:
        start_row = max((len(segments) - NUM_LEAVES) // 2, 0)
        logger.warning(
            f"MLC section has {len(segments)} rows; trimming to central {NUM_LEAVES}"
        )
        return segments[start_row:start_row + NUM_LEAVES]

    if len(segments) < NUM_LEAVES:
        logger.warning(
            f"MLC section has only {len(segments)} rows; padding with closed leaves to {NUM_LEAVES}"
        )
        padded = segments[:]
        while len(padded) < NUM_LEAVES:
            padded.append([0.0, 0.0])
        return padded

    return segments


def build_leaf_level_matrix(segments: list[list[float]]) -> np.ndarray:
    """
    Build a 26 x 650 matrix.
    Columns represent 0.1 mm cells in [-32.5 mm, 32.5 mm), so there are 650 cells.
    """
    rows = []

    for start, end in segments:
        row = np.zeros(FULL_SIZE, dtype=np.uint8)

        if not np.isclose(start, end):
            # Continuous interval [start, end) mapped onto cell indices [i0:i1)
            i0 = int(np.floor((start - X_MIN_MM) / PIXEL_MM))
            i1 = int(np.ceil((end - X_MIN_MM) / PIXEL_MM))

            i0 = max(0, min(FULL_SIZE, i0))
            i1 = max(0, min(FULL_SIZE, i1))

            if i1 > i0:
                row[i0:i1] = 1

        rows.append(row)

    matrix = np.vstack(rows)

    if matrix.shape != (NUM_LEAVES, FULL_SIZE):
        raise ValueError(f"Unexpected leaf matrix shape: {matrix.shape}")

    return matrix


def expand_leaf_matrix_to_full_resolution(leaf_matrix: np.ndarray) -> np.ndarray:
    """
    26 x 650 -> 650 x 650
    Each 2.5 mm leaf becomes 25 rows of 0.1 mm each.
    """
    full = np.repeat(leaf_matrix, ROW_REPEAT, axis=0)

    if full.shape != (FULL_SIZE, FULL_SIZE):
        raise ValueError(f"Unexpected full matrix shape: {full.shape}")

    return full.astype(np.uint8)


def size_mm_to_px(size_mm: float, pixel_mm: float = PIXEL_MM, rounding: str = "round") -> int:
    value = size_mm / pixel_mm

    if rounding == "round":
        px = int(np.round(value))
    elif rounding == "floor":
        px = int(np.floor(value))
    elif rounding == "ceil":
        px = int(np.ceil(value))
    else:
        raise ValueError(f"Unknown rounding mode: {rounding}")

    if px <= 0:
        raise ValueError(f"Computed non-positive size in px: {px}")
    if px > FULL_SIZE:
        raise ValueError(
            f"Requested crop {px}px exceeds available full field {FULL_SIZE}px"
        )

    return px


def crop_center_square(matrix: np.ndarray, size_px: int) -> np.ndarray:
    h, w = matrix.shape
    if size_px > h or size_px > w:
        raise ValueError(
            f"Requested crop {size_px}x{size_px} is larger than matrix {h}x{w}"
        )

    start_y = (h - size_px) // 2
    start_x = (w - size_px) // 2

    return matrix[start_y:start_y + size_px, start_x:start_x + size_px]


def area_resample_weights(in_size: int, out_size: int) -> np.ndarray:
    """
    Exact weights for area/box resampling.
    Each output row sums to 1.0.
    """
    src_edges = np.arange(in_size + 1, dtype=np.float64)
    dst_edges = np.linspace(0.0, float(in_size), out_size + 1, dtype=np.float64)

    W = np.zeros((out_size, in_size), dtype=np.float32)

    for i in range(out_size):
        a = dst_edges[i]
        b = dst_edges[i + 1]
        width = b - a

        j_start = int(np.floor(a))
        j_end = int(np.ceil(b))

        for j in range(j_start, j_end):
            if j < 0 or j >= in_size:
                continue

            left = max(a, src_edges[j])
            right = min(b, src_edges[j + 1])
            overlap = right - left

            if overlap > 0:
                W[i, j] = overlap / width

    return W


def downsample_area_binary(matrix: np.ndarray,
                           out_h: int,
                           out_w: int,
                           threshold: float = 0.5) -> np.ndarray:
    """
    Professional area/box downsampling with antialiasing, then thresholded to binary.
    For masks this is more stable than naive subsampling.
    """
    mat = matrix.astype(np.float32)
    Wr = area_resample_weights(matrix.shape[0], out_h)
    Wc = area_resample_weights(matrix.shape[1], out_w)
    downsampled = Wr @ mat @ Wc.T
    return (downsampled >= threshold).astype(np.uint8)


def save_preview_png_from_leaf_matrix(leaf_matrix: np.ndarray,
                                      out_png: str,
                                      crop_px: int | None = None) -> None:
    """
    Preserve the original geometry preview based on 26 leaves.
    Optionally overlay the central crop square.
    """
    nrows, ncols = leaf_matrix.shape
    total_h = nrows * LEAF_HEIGHT_MM
    half_h = total_h / 2.0

    fig = plt.figure(figsize=(6, 6))
    ax_mm = fig.add_axes([0.1, 0.1, 0.85, 0.85])
    ax_mm.patch.set_visible(False)

    x_lefts = X_MIN_MM + np.arange(ncols) * PIXEL_MM

    for r in range(nrows):
        row = leaf_matrix[r]
        indices = np.where(row == 1)[0]

        y0_mm = r * LEAF_HEIGHT_MM - half_h
        h_mm = LEAF_HEIGHT_MM

        if indices.size:
            x0 = x_lefts[indices[0]]
            x1 = x_lefts[indices[-1]] + PIXEL_MM

            ax_mm.add_patch(plt.Rectangle(
                (x0, y0_mm),
                x1 - x0,
                h_mm,
                facecolor='pink',
                alpha=0.95,
                edgecolor=None,
                clip_on=False
            ))

    red_box = plt.Rectangle(
        (X_MIN_MM, -half_h),
        X_MAX_MM - X_MIN_MM,
        total_h,
        fill=False,
        edgecolor='red',
        linewidth=0.95,
        clip_on=False
    )
    ax_mm.add_patch(red_box)

    if crop_px is not None:
        crop_mm = crop_px * PIXEL_MM
        half_crop_mm = crop_mm / 2.0
        crop_box = plt.Rectangle(
            (-half_crop_mm, -half_crop_mm),
            crop_mm,
            crop_mm,
            fill=False,
            edgecolor='green',
            linewidth=0.9,
            linestyle='-',
            clip_on=False,
        )
        ax_mm.add_patch(crop_box)

    nums = [-23, -11, 1, 13]
    for a in nums:
        for b in nums:
            blue_box = plt.Rectangle(
                (a, b),
                10,
                10,
                fill=False,
                edgecolor='blue',
                linewidth=0.5,
                linestyle='--',
                clip_on=False
            )
            ax_mm.add_patch(blue_box)

    ax_mm.set_xlim(X_MIN_MM - 2.5, X_MAX_MM + 2.5)
    ax_mm.set_ylim(-half_h - 2.5, half_h + 2.5)

    ax_mm.set_xlabel('X position (mm)')
    ax_mm.set_ylabel('Y position (mm)')
    ax_mm.set_aspect('equal', anchor='C')

    fig.savefig(out_png, dpi=450, bbox_inches='tight')
    plt.close(fig)
    logger.info(f"Saved PNG: {out_png}")


# ============================================================
# Main conversion function
# ============================================================
def mask_to_matrix(plan_file: str,
                   out_csv: str = None,
                   out_pickle: str = None,
                   out_png: str = None,
                   crop_size_mm: float = 64.00,
                   crop_rounding: str = "round",
                   binary_threshold: float = 0.5) -> pl.DataFrame:
    logger.info(f"Processing file: {plan_file}")

    try:
        with open(plan_file, 'r') as f:
            lines = f.read().splitlines()
        logger.debug(f"Read {len(lines)} lines")
    except Exception as e:
        logger.error(f"Cannot open {plan_file}: {e}")
        raise

    # 1) Parse MLC and normalize to 26 leaves
    segments = parse_mlc_segments(lines)
    segments = normalize_to_26_segments(segments)

    # 2) Build 26 x 650 leaf matrix
    leaf_matrix = build_leaf_level_matrix(segments)

    # 3) Expand to full 650 x 650 binary matrix
    full_matrix = expand_leaf_matrix_to_full_resolution(leaf_matrix)

    # 4) Central square crop, e.g. 42.56 mm -> 426 px -> 42.6 mm actual size
    crop_px = size_mm_to_px(crop_size_mm, PIXEL_MM, crop_rounding)
    crop_actual_mm = crop_px * PIXEL_MM
    cropped_matrix = crop_center_square(full_matrix, crop_px)

    # 5) Professional area-downsample to 64 x 64, then threshold back to binary
    small_binary_matrix = downsample_area_binary(
        cropped_matrix,
        SMALL_SIZE,
        SMALL_SIZE,
        threshold=binary_threshold,
    )

    logger.info(
        f"Requested square: {crop_size_mm:.3f} mm, rounding={crop_rounding} "
        f"-> {crop_px} px -> actual {crop_actual_mm:.3f} mm"
    )
    logger.debug(f"Leaf matrix shape: {leaf_matrix.shape}")
    logger.debug(f"Full matrix shape: {full_matrix.shape}")
    logger.debug(f"Cropped matrix shape: {cropped_matrix.shape}")
    logger.debug(f"Small binary matrix shape: {small_binary_matrix.shape}")

    cropped_matrix = np.rot90(cropped_matrix, k=3, axes=(0, 1))
    cropped_matrix = np.flip(cropped_matrix, axis=1)

    small_binary_matrix = np.rot90(small_binary_matrix, k=3, axes=(0, 1))
    small_binary_matrix = np.flip(small_binary_matrix, axis=1)
    

    # Return the main exported matrix as a DataFrame
    df = build_polars_df(cropped_matrix)

    if out_csv:
        write_matrix_csv(out_csv, cropped_matrix.astype(np.uint8))
        logger.info(f"Saved CSV ({crop_px}x{crop_px}): {out_csv}")

        out_csv_64 = add_suffix_before_extension(out_csv, "_64")
        write_matrix_csv(out_csv_64, small_binary_matrix.astype(np.uint8))
        logger.info(f"Saved CSV (64x64, binary): {out_csv_64}")

    if out_pickle:
        np.save(out_pickle, cropped_matrix.astype(np.uint8))
        logger.info(f"Saved NPY ({crop_px}x{crop_px}): {out_pickle}")

        out_pickle_64 = add_suffix_before_extension(out_pickle, "_64")
        np.save(out_pickle_64, small_binary_matrix.astype(np.uint8))
        logger.info(f"Saved NPY (64x64, binary): {out_pickle_64}")

    if out_png:
        save_preview_png_from_leaf_matrix(leaf_matrix, out_png, crop_px=crop_px)

    return df


# ============================================================
# CLI
# ============================================================
def main():
    parser = argparse.ArgumentParser(
        description="Convert .dat mask files to square binary matrices"
    )
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-i', '--input_file', help='Single .dat file')
    group.add_argument('-d', '--input_dir', help='Directory of .dat files')

    parser.add_argument('-o', '--out_dir', required=True, help='Output folder')
    parser.add_argument('--out_csv', action='store_true', help='Save CSV')
    parser.add_argument('--out_pickle', action='store_true', help='Save .npy')
    parser.add_argument('--out_png', action='store_true', help='Save PNG preview')

    parser.add_argument(
        '--crop_mm',
        type=float,
        default=64.00,
        help='Central square size in mm to export, e.g. 42.56'
    )
    parser.add_argument(
        '--crop_rounding',
        choices=['round', 'floor', 'ceil'],
        default='round',
        help='How to convert crop_mm to pixel count'
    )
    parser.add_argument(
        '--binary_threshold',
        type=float,
        default=0.5,
        help='Threshold applied after area downsampling to 64x64'
    )

    args = parser.parse_args()

    files = []
    if args.input_file:
        files.append(args.input_file)
    else:
        for root, _, fnames in os.walk(args.input_dir):
            for f in fnames:
                if f.lower().endswith('.dat'):
                    files.append(os.path.join(root, f))

    for fpath in files:
        base = os.path.splitext(os.path.basename(fpath))[0]
        odir = os.path.join(args.out_dir, base)
        os.makedirs(odir, exist_ok=True)

        crop_px = size_mm_to_px(args.crop_mm, PIXEL_MM, args.crop_rounding)

        mask_to_matrix(
            fpath,
            out_csv=os.path.join(odir, f"{base}_mask_{crop_px}.csv") if args.out_csv else None,
            out_pickle=os.path.join(odir, f"{base}_mask_{crop_px}.npy") if args.out_pickle else None,
            out_png=os.path.join(odir, base + '_mask.png') if args.out_png else None,
            crop_size_mm=args.crop_mm,
            crop_rounding=args.crop_rounding,
            binary_threshold=args.binary_threshold,
        )


if __name__ == '__main__':
    main()
