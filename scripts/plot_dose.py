import pandas as pd
import matplotlib.pyplot as plt

file = "/home/geant4/workspace/github/g4rt/output/water_phantom_2/sim/cp10x10/cp10x10_watertank_voxel.csv"
# Load the CSV file
data = pd.read_csv(file)

# Extract relevant columns
z_values = data["Z [mm]"]
dose_values = data["Dose [Gy]"]

dose_values = dose_values/dose_values.max()

# Plot
plt.figure(figsize=(8, 5))
plt.scatter(z_values, dose_values, color='b', label='Dose Profile')
plt.xlabel("Z [mm]")
plt.ylabel("Dose [Gy]")
plt.title("Dose Profile along Z-axis")
plt.grid(True)
plt.legend()
plt.show()