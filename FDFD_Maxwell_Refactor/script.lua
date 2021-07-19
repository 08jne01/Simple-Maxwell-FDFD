config = {
	n_points = 10,
	size_of_structure = 10.0,--um
	max_index_red = 1.0,
	max_index_green = 1.45,
	max_index_blue = 1.0,
	max_neff_guess = nil,--3.26,
	wavelength = 1.55, --um 
	geometry_filename = "Fibres/Fibre160.bmp",
	timers = true,
	num_modes = 10,
}


function main()
	solve = solver.solve(config)
	success, mode = solver.display(solve)
	print("Mode Selected: "..mode)

	if success then
		success = solver.outputField(mode, solve, "FieldData"..string.format("%d", mode)..".csv")
	end
end


