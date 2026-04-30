import pandas as pd
import numpy as np
import pyvista as pv

# 🔹 wczytanie danych
path = "/home/geant4/workspace/d3df_g4rt/output/srunet3d_4x4x2_64x64x64_tps_plan1_1/sim/tps_plan1/tps_plan1_d3ddetector_cell.csv"
df = pd.read_csv(path, comment='#')

# 🔹 kolumny
x = df['X [mm]'].values
y = df['Y [mm]'].values
z = df['Z [mm]'].values
labels = df['Label'].values

# 🔹 filtr tylko niepuste Label
mask = labels != ""
x, y, z, labels = x[mask], y[mask], z[mask], labels[mask]

# 🔹 voxel size
voxel_size = 0.985*10
half_size = voxel_size/2

# 🔹 plotter
plotter = pv.Plotter()

# 🔹 iterujemy po każdym voxel
for xi, yi, zi, label in zip(x, y, z, labels):
    # cube voxel
    cube = pv.Cube(center=(xi, yi, zi),
                   x_length=voxel_size,
                   y_length=voxel_size,
                   z_length=voxel_size)
    plotter.add_mesh(cube, color="lightblue", opacity=0.5, show_edges=True)
    
    # 🔹 tekst na ścianie przedniej (np. +y)
    text_pos = (xi, yi + half_size + 0.1, zi)  # lekko ponad ścianą voxel
    plotter.add_point_labels(
        np.array([text_pos]),
        [label],
        font_size=10,
        point_size=0,
        render_points_as_spheres=False
    )

# 🔹 osie i siatka
plotter.add_axes()
plotter.show_grid()
plotter.show(interactive=True)