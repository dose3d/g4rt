import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import argparse

# Parametry twardego podziału
INNER_LEAF_HEIGHT = 25.0
OUTER_LEAF_HEIGHT = 50.0
INNER_LEAF_COUNT = 35  # liczba wewnętrznych listków

def load_mlc_dat(filepath):
    """
    Load MLC pairs (Y1, Y2) from .dat file, starting **after** '# MLC:' line.
    Returns:
        bank_a, bank_b
    """
    bank_a = []
    bank_b = []
    read_mlc = False

    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith('# MLC'):
                read_mlc = True
                continue
            if not read_mlc:
                continue
            if line.startswith('#'):
                continue
            parts = line.split(',')
            if len(parts) < 2:
                continue
            bank_a.append(float(parts[0]))
            bank_b.append(float(parts[1]))
    return bank_a, bank_b

def get_leaf_boundaries(n_leaves):
    """
    Tworzy listę y-boundaries dla wszystkich liści:
    - Środkowe listki (INNER_LEAF_COUNT) -> INNER_LEAF_HEIGHT
    - Zewnętrzne listki -> OUTER_LEAF_HEIGHT
    """
    boundaries = [0.0]
    n_outer_each_side = (n_leaves - INNER_LEAF_COUNT) // 2
    for i in range(n_leaves):
        if i < n_outer_each_side or i >= n_leaves - n_outer_each_side:
            h = OUTER_LEAF_HEIGHT  # zewnętrzne
        else:
            h = INNER_LEAF_HEIGHT  # wewnętrzne
        boundaries.append(boundaries[-1] + h)
    # Wycentrowanie wokół 0
    total_height = boundaries[-1]
    boundaries = [y - total_height/2 for y in boundaries]
    return boundaries

def visualize_mlc(bank_a, bank_b):
    n_leaves = len(bank_a)
    leaf_boundaries = get_leaf_boundaries(n_leaves)

    # Skala X i Y poprawiona, aby zawsze mieć kwadrat
    x_min = min(bank_a + bank_b)
    x_max = max(bank_a + bank_b)
    y_min = min(leaf_boundaries)
    y_max = max(leaf_boundaries)
    x_center = (x_min + x_max) / 2
    y_center = (y_min + y_max) / 2
    max_range = max(x_max - x_min, y_max - y_min) + 40  # dodajemy margines 20 mm z każdej strony
    x_min, x_max = x_center - max_range/2, x_center + max_range/2
    y_min, y_max = y_center - max_range/2, y_center + max_range/2

    fig, ax = plt.subplots(figsize=(10,10))
    ax.add_patch(plt.Rectangle((x_min, y_min), max_range, max_range,
                               facecolor='royalblue', alpha=0.8, zorder=1))

    # rysowanie liści
    for i in range(n_leaves):
        y_low = leaf_boundaries[i]
        y_high = leaf_boundaries[i+1]
        if bank_a[i] < bank_b[i]:
            ax.fill_between([bank_a[i], bank_b[i]], y_low, y_high,
                            color='white', edgecolor='black', linewidth=0.5, zorder=2)

    # linie graniczne liści
    for i in range(n_leaves+1):
        y = leaf_boundaries[i]
        ax.plot([x_min, x_max], [y, y], 'k-', linewidth=0.3, alpha=0.5, zorder=3)

    ax.axvline(0, color='red', linestyle='--', linewidth=1.5, alpha=0.7, zorder=4)
    ax.axhline(0, color='red', linestyle='--', linewidth=1.5, alpha=0.7, zorder=4)

    ax.set_xlim(x_min, x_max)
    ax.set_ylim(y_min, y_max)
    ax.set_xlabel('X Position [mm]', fontsize=12, fontweight='bold')
    ax.set_ylabel('Y Position [mm]', fontsize=12, fontweight='bold')
    ax.set_title('MLC Aperture (Beam\'s Eye View)', fontsize=14, fontweight='bold')
    ax.set_aspect('equal')
    ax.grid(True, alpha=0.3, linestyle='--', color='gray', zorder=0)

    leaf_height_values = [y2 - y1 for y1, y2 in zip(leaf_boundaries[:-1], leaf_boundaries[1:])]
    aperture_area = sum(max(b-a,0)*h for a,b,h in zip(bank_a, bank_b, leaf_height_values))
    stats_text = f"""MLC STATISTICS

Leaves: {n_leaves}
Aperture: {aperture_area/100:.1f} cm²

X range: [{min(bank_a):.1f}, {max(bank_b):.1f}] mm
Max opening: {max([b-a for a,b in zip(bank_a, bank_b)]):.1f} mm
Min opening: {min([b-a for a,b in zip(bank_a, bank_b)]):.1f} mm
"""
    ax.text(0.98,0.98,stats_text, transform=ax.transAxes,
            fontsize=9, verticalalignment='top', horizontalalignment='right',
            fontfamily='monospace',
            bbox=dict(boxstyle='round', facecolor='lightyellow', alpha=0.9, edgecolor='black'),
            zorder=5)

    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Visualize MLC aperture from .dat file, only after # MLC header")
    parser.add_argument("filepath", type=str, help="Path to MLC .dat file")
    args = parser.parse_args()

    bank_a, bank_b = load_mlc_dat(args.filepath)
    print(f"Loaded {len(bank_a)} leaf pairs from {args.filepath}")

    visualize_mlc(bank_a, bank_b)