import pandas as pd
import numpy as np
import pyvista as pv

# 🔹 wczytanie danych (ignorujemy komentarze)
path = "/home/geant4/workspace/d3df_g4rt/output/srunet3d_4x4x2_64x64x64_tps_plan1_1/sim/tps_plan1/tps_plan1_d3ddetector_voxel.csv"
# path = "/home/geant4/workspace/d3df_g4rt/output/srunet3d_4x4x2_64x64x64_tps_plan1_1/sim/tps_plan1/tps_plan1_d3ddetector_cell.csv"
df = pd.read_csv(path, comment='#')

# 🔹 wyciągnięcie kolumn
x = df['X [mm]'].values
y = df['Y [mm]'].values
z = df['Z [mm]'].values
dose = df['Dose [Gy]'].values

# 🔹 normalizacja dawki
max_dose = dose.max()
dose_norm = dose / max_dose

# 🔹 filtr: usuwamy zero-dose
mask = dose > 0
x, y, z, dose_norm = x[mask], y[mask], z[mask], dose_norm[mask]

# 🔹 voxel sizey
# voxel_size = 0.985
voxel_size = 0.985

# 🔹 tworzymy punkty
points = np.column_stack((x, y, z))

# 🔹 tworzymy "cube glyph" (voxel)
cube = pv.Cube(x_length=voxel_size,
               y_length=voxel_size,
               z_length=voxel_size)

# 🔹 przypisujemy dane
point_cloud = pv.PolyData(points)
point_cloud["dose"] = dose_norm

# 🔹 tworzymy voxele
glyphs = point_cloud.glyph(scale=False, orient=False, geom=cube)

# 🔹 plotter
plotter = pv.Plotter()
plotter.add_mesh(
    glyphs,
    scalars="dose",
    cmap="magma",
    clim=[dose_norm.min(), 1.0],
    log_scale=True
)

print("Dawka znormalizowana:", dose_norm)

plotter.add_axes()
plotter.show_grid()

plotter.show(interactive=True)