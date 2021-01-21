#pragma once
#include "Config.h"
#include <vector>
#include "luaextern.h"
class Program
{
public:
	enum ProgramType
	{
		SCRIPT,
		SINGLE,
		SWEEP
	};

	enum Error
	{
		NO_SOLVER_CONFIG,
		NO_SWEEP_CONFIG,
		NO_COLOR_MAP,
		NO_GEOMETRY
	};

	Program(const MainConfig& config, const ProgramType type);
	void doScript();
	void doSingle();
	void doSweep();
	int returnWithErrors();
	static Program* instance();
	MainConfig& getMainConfig();
private:
	bool failed();
	void printError(const Error error);
	void loadBoth();
	void loadSingle();
	const ProgramType m_type;
	MainConfig m_config;
	SolverConfig m_solverConfig;
	SweepConfig m_sweepConfig;
	std::vector<Error> m_errors;

	static Program* global_program;
};

inline bool Program::failed()
{
	return m_errors.size() > 0;
}

inline void Program::loadSingle()
{
	if (!loadSolverConfig(m_config.m_solverPath, m_solverConfig))
	{
		m_errors.push_back(Error::NO_SOLVER_CONFIG);
	}
}

inline void Program::loadBoth()
{
	if (!loadSolverConfig(m_config.m_solverPath, m_solverConfig))
	{
		m_errors.push_back(Error::NO_SOLVER_CONFIG);
	}
	if (!loadSweepConfig(m_config.m_sweepPath, m_sweepConfig))
	{
		m_errors.push_back(Error::NO_SWEEP_CONFIG);
	}
}

inline Program* Program::instance()
{
	return global_program;
}

inline MainConfig& Program::getMainConfig()
{
	return m_config;
}