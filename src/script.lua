
function get_step(start, stop, points)
	local diff = start - stop

	local step = diff / ( points - 1)

	return step
end

function write_data(data)
	file = io.open("data.csv", "w+")

	if file then
		file:write("wavelength,neff\n")

		for i, v in pairs(data) do
			file:write(tostring(v.wavelength)..","..tostring(v.neff).."\n")
		end

		file:close()
	end
end


config = {
	n_points = 75,
	size_of_structure = 40.2e-2 ,--um
	max_index_red = 1.0,
	max_index_green = 1.45,
	max_index_blue = 1.0,
	max_neff_guess = 0.9999,--3.26,
	wavelength = 1.25e-2, --um 
	geometry_filename = "Fibres/hollow_core3blocky.bmp",
	timers = false,
	num_modes = 50,
}

local data = {}

local points = 20
local sweep_end_wavelength = 1.1e-2
local step = get_step(config.wavelength, sweep_end_wavelength, points)

previous_solve = solver.solve(config)
previous_mode = solver.display(previous_solve)
print("Mode "..tostring(previous_mode).." selected.")

for i = 1, (points - 1), 1 do

	config.wavelength = config.wavelength - step
	print("Wavelength: "..config.wavelength)
	local current_solve = solver.solve(config)

	best_overlap, previous_mode = solver.closestOverlap(previous_mode, previous_solve, current_solve)
	local neff = solver.getEffectiveIndex(previous_mode, current_solve)

	print("Best Mode "..previous_mode.." neff: "..neff.." overlap: "..best_overlap)

	data[i] = {
		wavelength = config.wavelength,
		neff = neff,
	}
	
	if best_overlap < 0.7 then
		break
	end

	previous_solve = current_solve
	
end

write_data(data)


