#pragma once
#include <string>

struct MainConfig
{
	MainConfig()
	{

	}
	bool m_timers = false; //display timers or not
	short m_screenWidth = 800; //width of the screen, height will be calculated based on input geometry
	std::string m_colorMapPath = "jet.dat"; //path to the current colormap
	std::string m_solverPath = "solver.lua";
	std::string m_sweepPath = "sweep.lua";
	std::string m_outputDataPath = "Output_Data";
	std::string m_scriptPath = "script.lua";
	short m_width;
	short m_height;
};

struct SolverConfig
{
	short m_width; //width of the sim window, these are gotten from the file size
	short m_height; //height of the sim window, these are gotton from the file size
	short m_points; //number of points the structure takes up
	double m_size; //size of the structure
	double m_maxIndexRed; //red channel max index
	double m_maxIndexBlue; //blue channel max index
	double m_maxIndexGreen; //green channel max index
	double m_neffGuess; //inital guess for shift invert eigen solver
	double m_wavelength; //wavelength of the light
	std::string m_fileName; //location of geometry
	bool m_timers; //timers for solver or not
	short m_modes; //number of modes
};

struct SweepConfig
{
	double m_overlapConfidence; //required overlap to accept mode
	double m_start; //start wavelength
	double m_end; //end wavelength
	short m_nPoints; //number of points in sweep
	short m_startingMode; //the mode which the program will use for the first overlap comparison
	std::string m_outputData; //output data filename
	std::string m_redProfile; //refractive index profile of the red channel
	std::string m_greenProfile; //refractive index profile of the red channel
	std::string m_blueProfile; //refractive index profile of the red channel
};