overlap_confidence = 0.8
start_wavelength = 0.5
end_wavelength = 1.5
n_sweep_points = 10
starting_mode = 0
output_data_filename = "Output_Data/Sweep0.dat"
red_index_profile = function(wavelength)
	return 3.0*wavelength
end
green_index_profile = function(wavelength)
	return 2.0
end
blue_index_profile = function(wavelength)
	return 1.0
end