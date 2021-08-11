import Solver
import numpy as np
import matplotlib.pyplot as plt

sweep_start_wavelength = 1.0
sweep_end_wavelength = 0.1
sweep_steps = 20

"""
config = {
	"n_points" : 75,
	"size_of_structure" : 40.2e-2 , #um
	"max_index_red" : 1.0,
	"max_index_green" : 1.45,
	"max_index_blue" : 1.0,
	"max_neff_guess" : 0.9999, #3.26,
	"wavelength" : sweep_start_wavelength, #um 
	"geometry_filename" : "Fibres/hollow_core3blocky.bmp",
	"timers" : False,
	"num_modes" : 50,
}
"""

config = {
    "n_points" : 50,
    "size_of_structure" : 1.2, #um
    "max_index_red" : 2.2623,
    "max_index_green" : 1.45,
    "max_index_blue" : 1.0,
    "max_neff_guess" : 3.26,
    "wavelength" : 0.75, #um 
    "geometry_filename" : "Fibres/LNSiO2Waveguide.bmp",
    "timers" : False,
    "num_modes" : 10,
}

wavelengths = np.linspace(sweep_start_wavelength, sweep_end_wavelength, sweep_steps)
neff = []

previous_solution = None
previous_mode = -1

for wavelength in wavelengths:
    config["wavelength"] = wavelength
    solution = Solver.solve(config)
    
    
    if previous_solution == None:
        success, previous_mode = solution.display()

        if not success:
            print("Something went wrong.")
            break

    else:
        overlap, previous_mode = solution.overlap(previous_mode, previous_solution)
        print("Overlap {} on mode {}".format(overlap, previous_mode))

        if overlap < 0.8:
            success, previous_mode = solution.display()

            if not success or previous_mode == -1:
                break
            
            print("Mode {} selected".format(previous_mode))

    previous_solution = solution
    print(previous_mode)
    neff.append(solution.getEffectiveIndex(previous_mode))
    print("neff: {}".format(neff[-1]))
        

previous_solution.display()
plt.plot(wavelengths[:len(neff)], neff, "k-")
plt.show()


#result = Solver.solve(config)

#success, selected_mode = result.display()

#overlap, bestMode = result.overlap(selected_mode, result)

#print("Overlap {}, Mode {}, Original Mode {}".format(overlap, bestMode, selected_mode))

#if success:
    #print("Selected Mode: {}".format(selected_mode))