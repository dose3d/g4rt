# RT Plan converter

`dcm2dat` converts DICOM RT Plan beams and control points into the project's
plain-text `.dat` plan format. One output file is produced for every selected
control point.

## Quick start

Build the converter:

```bash
cmake --build build --target dcm2dat
```

Convert all beams and control points:

```bash
mkdir -p output
build/app/dcm2dat \
  --File plan.dcm \
  --OutputDir output \
  --fieldCentre true
```

The output directory must exist before the converter is started.

## Field centralization

With `--fieldCentre true`, the converter calculates the geometric centroid of
the open field. The field is treated as a union of rectangular leaf apertures,
so irregular fields are weighted by their actual open area.

The target centroid is randomized independently in both axes within
`[-30, +30] mm`. This introduces small positional variation while keeping the
field close to `(0, 0)`.

- Along the leaf-travel axis, all endpoints are translated continuously and
  the randomized target is reached exactly.
- Along the leaf-row axis, movement is limited to complete rows. The converter
  selects the valid row shift closest to the randomized target.
- The 60-row MLC geometry uses 5 mm outer rows and 2.5 mm inner rows, matching
  `MlcSimplified`. Other row counts use a uniform 2.5 mm fallback pitch.
- Closed fields or inconsistent MLC banks are reported and left unchanged.

The resulting centroid is printed for each processed control point:

```text
Field geometric centre [mm]: -7.5,12.34
```

## Useful options

| Option | Meaning |
| --- | --- |
| `--File`, `-f` | Input DICOM RT Plan file (required) |
| `--OutputDir`, `-o` | Existing output directory (required) |
| `--fieldCentre` | Enable randomized geometric centering |
| `--nBeams`, `-b` | Limit the number of processed beams |
| `--nCtrlPts`, `-c` | Limit control points per beam |
| `--nParticles` | Particle count written to each output plan |
| `--fieldConstrain W,H` | Save only fields smaller than the given limits in mm |

Run `build/app/dcm2dat --help` for the complete command-line help.
