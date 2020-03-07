print("This is a test")
if (solver.loadGeometry("Fibres/NonSquare.bmp")) then
	solver.solve("solver.lua")
end

if (solver.loadGeometry("Fibres/LNSiO2Waveguide.bmp")) then
	solver.solve("solver.lua")
end