import pyvista as pv

plotter = pv.Plotter()

plotter.add_mesh(pv.Sphere())
plotter.render()    
plotter.show(auto_close=False)

print(pv.Report())
