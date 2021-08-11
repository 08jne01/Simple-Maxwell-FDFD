#include "Program.h"
#include "FileHandler.h"
#include "FieldSolver.h"
#include "FieldViewer.h"
#include "FieldHandler.h"
#include <iostream>
#include "PythonHooks.h"

Program* Program::global_program = nullptr;

Program::Program
(
	const MainConfig& config,
	ProgramType type
):
	m_config(config),
	m_type(type)
{
	if (!global_program)
	{
		global_program = this;
	}
	else
	{
		std::cout << "Warning multiple instances of program!" << std::endl;
	}

	type = SCRIPT;

	//Singlets
	FileHandler fileHandler(m_config);
	FieldHandler fieldHandler;

	switch (type)
	{
	case SCRIPT:
		doScript();
		return;
	case SINGLE:
		doSingle();
		return;
	case SWEEP:
		doSweep();
		return;
	}
}

void Program::doScript()
{
	if (failed())
	{
		return;
	}


	runPythonScript( m_config.m_scriptPath.c_str() );

	////else do script
	//LuaScript script( m_config.m_scriptPath );
	//
	//script.executeScript();
}

void Program::doSingle()
{
	loadSingle();
	if (failed())
	{
		return;
	}

	if (!FileHandler::instance()->loadGeometry(m_solverConfig.m_fileName))
	{
		m_errors.push_back(Error::NO_GEOMETRY);
		return;
	}

	FieldSolver solver(m_config, m_solverConfig, FileHandler::instance()->getGeometry());
	Field field;
	solver.solve(field);
	FieldViewer viewer(m_config, m_solverConfig, field, solver.getPermativityVector());
	if (!viewer.mainLoop())
	{
		m_errors.push_back(Error::NO_COLOR_MAP);
		return;
	}
}

void Program::doSweep()
{
 	loadSingle();
	LuaSweepConfig sweepConfig(m_config.m_sweepPath, m_sweepConfig);
	if (!sweepConfig.loadConfig())
	{
		m_errors.push_back(Error::NO_SWEEP_CONFIG);
		return;
	}
	
	std::cout << sweepConfig.evaluateIndex(1.5, LuaSweepConfig::Color::RED) << std::endl;

	
	if (failed())
	{
		return;
	}

	if (!FileHandler::instance()->loadGeometry(m_solverConfig.m_fileName))
	{
		m_errors.push_back(Error::NO_GEOMETRY);
	}
}

void Program::printError(const Error error)
{
	switch (error)
	{
	case NO_SOLVER_CONFIG:
		std::cout << "ERROR: No solver config was loaded." << std::endl;
		break;
	case NO_SWEEP_CONFIG:
		std::cout << "ERROR: No sweep config was loaded." << std::endl;
		break;
	case NO_COLOR_MAP:
		std::cout << "ERROR: No colour map was loaded." << std::endl;
		break;
	case NO_GEOMETRY:
		std::cout << "ERROR: No geometry was loaded." << std::endl;
		break;
	default:
		std::cout << "ERROR: Unknown." << std::endl;
	}
}

int Program::returnWithErrors()
{
	if (!failed())
	{
		return EXIT_SUCCESS;
	}

	for (int i = 0; i < m_errors.size(); i++)
	{
		printError(m_errors[i]);
	}
	return EXIT_FAILURE;
}