#pragma once
#include "Field.h"
#include "Config.h"
#include <vector>
struct Solve
{
	Field field;
	MainConfig mainConfig;
	SolverConfig solverConfig;
	std::vector<double> permativityVector;
};