print("This is a test")

local solve1
local solve2

--if (solver.loadGeometry("Fibres/NonSquare.bmp")) then
	--solve1 = solver.solve("solver.lua")
--end


config1 = {
	n_points = 50,
	size_of_structure = 1.2 ,--um
	max_index_red = 2.2623,
	max_index_green = 1.45,
	max_index_blue = 1.0,
	max_neff_guess = 3.26,
	wavelength = 0.75, --um 
	geometry_filename = "Fibres/LNSiO2Waveguide.bmp",
	timers = false,
	num_modes = 10,
}


if (solver.loadGeometry("Fibres/LNSiO2Waveguide.bmp")) then
	solve2 = solver.solve(config1)
end

if solve1 then
	print(solve1)
	mode = solver.display(solve1)
	print("Mode: "..mode)
end

if solve2 then
	print(solve2)
	solver.display(solve2)
end

