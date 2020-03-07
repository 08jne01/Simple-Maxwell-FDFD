extern "C"
{
#include "lua/include/lua.h"
#include "lua/include/lauxlib.h"
#include "lua/include/lualib.h"
}
#include <iostream>
#include "luaextern.h"
#include "Config.h"
#include "FieldSolver.h"
#include "FileHandler.h"
#include "FieldViewer.h"
#include "Program.h"

//TODO:
// - fix the mess below
// - make this instancable for fucks sake

//#ifdef _WIN32
//#pragma comment(lib, "../include/lua/liblua53.a")
//#else
#pragma comment(lib, "../include/lua64/lua53.lib")
//#endif



static lua_State* LScript;
static lua_State* LConfig;
static lua_State* LFunctions;
static bool luaFailed = false;

static bool luaOkay(lua_State* L, const int status);
static bool initLua(lua_State** L, const std::string& filename, bool func);
static void closeLua(lua_State* L);

//special lua functions
static int luaLoadGeometry(lua_State* L);
static int luaSolve(lua_State* L);
static int luaSweep(lua_State* L);

//get global lua vars
static int getInt(lua_State* L, const char* var);
static double getDouble(lua_State* L, const char* var);
static std::string getString(lua_State* L, const char* var);
static bool getBool(lua_State* L, const char* var);
static void addModule(lua_State* L, const luaL_Reg* funcs, const char* name);
static bool luaCallFunction(lua_State* L, char* functionName, int numParams, int numReturns);
static double callWavelength(double wavelength, const char* name);

static const luaL_Reg solverFunctions[] = {
	{"loadGeometry", luaLoadGeometry},
	{"solve", luaSolve},
	{NULL, NULL}
};

void addModule(lua_State* L, const luaL_Reg* funcs, const char* name)
{
	lua_newtable(L);
	luaL_setfuncs(L, funcs, 0);
	lua_pushvalue(L, -1);
	lua_setglobal(L, name);
}

bool runScript(const std::string& filename)
{
	initLua(&LScript, filename, true);
	closeLua(LScript);
	return !luaFailed;
}

bool loadMainConfig(const std::string& filename, MainConfig& config)
{
	if (initLua(&LConfig, filename, false))
	{
		config.m_timers = getBool(LConfig, "timers");
		config.m_screenWidth = getInt(LConfig, "screen_width");
		config.m_colorMapPath = getString(LConfig, "color_map_path");
		config.m_solverPath = getString(LConfig, "solver_path");
		config.m_sweepPath = getString(LConfig, "sweep_path");
		config.m_outputDataPath = getString(LConfig, "output_data_folder");
	}
	closeLua(LConfig);
	return !luaFailed;
}

bool loadSolverConfig(const std::string& filename, SolverConfig& config)
{
	if (initLua(&LConfig, filename, false))
	{
		config.m_points = getInt(LConfig, "n_points");
		config.m_size = getDouble(LConfig, "size_of_structure");
		config.m_maxIndexRed = getDouble(LConfig, "max_index_red");
		config.m_maxIndexGreen = getDouble(LConfig, "max_index_green");
		config.m_maxIndexBlue = getDouble(LConfig, "max_index_blue");
		config.m_neffGuess = getDouble(LConfig, "max_neff_guess");
		config.m_wavelength = getDouble(LConfig, "wavelength");
		config.m_fileName = getString(LConfig, "geometry_filename");
		config.m_timers = getBool(LConfig, "timers");
		config.m_modes = getInt(LConfig, "num_modes");
	}
	closeLua(LConfig);
	return !luaFailed;
}

bool loadSweepConfig(const std::string& filename, SweepConfig& config)
{
	if (initLua(&LFunctions, filename, false))
	{
		config.m_overlapConfidence = getDouble(LFunctions, "overlap_confidence");
		config.m_start = getDouble(LFunctions, "start_wavelength");
		config.m_end = getDouble(LFunctions, "end_wavelength");
		config.m_nPoints = getInt(LFunctions, "n_sweep_points");
		config.m_startingMode = getInt(LFunctions, "starting_mode");
		config.m_outputData = getString(LFunctions, "output_data_filename");
		//config.m_redProfile = getString(LConfig, "red_index_profile");
		//config.m_greenProfile = getString(LConfig, "green_index_profile");
		//config.m_blueProfile = getString(LConfig, "blue_index_profile");
	}
	return !luaFailed;
}

double evaluateSweepFunction(double wavelength, int color)
{
	switch (color)
	{
	case 0:
		return callWavelength(wavelength, "red_index_profile");
	case 1:
		return callWavelength(wavelength, "green_index_profile");
	case 2:
		return callWavelength(wavelength, "blue_index_profile");
	}
	return 1.0;
}

double callWavelength(double wavelength, const char* name)
{
	lua_getglobal(LFunctions, name);
	if (lua_isfunction(LFunctions, -1))
	{
		lua_pushnumber(LFunctions, wavelength);
		lua_pcall(LFunctions, 1, 1, 0);
		if (lua_isnumber(LFunctions, -1))
		{
			return lua_tonumber(LFunctions, -1);
		}
		else
		{
			return 1.0;
		}
	}
	else
	{
		lua_pop(LFunctions, 1);
	}
	return 1.0;
}

void closeSweepConfig()
{
	closeLua(LFunctions);
}

int getInt(lua_State* L, const char* var)
{
	lua_getglobal(L, var);
	if (lua_isnumber(L, -1))
	{
		return (int)lua_tonumber(L, -1);
	}
	else if (lua_isnil(L, -1))
	{
		std::cout << "Variable " << var << " does not exist." << std::endl;
		luaFailed = true;
		return 0;
	}
	else
	{
		std::cout << "Variable " << var << " is not an integer." << std::endl;
		luaFailed = true;
		return 0;
	}
}

double getDouble(lua_State* L, const char* var)
{
	lua_getglobal(L, var);
	if (lua_isnumber(L, -1))
	{
		return (double)lua_tonumber(L, -1);
	}
	else if (lua_isnil(L, -1))
	{
		std::cout << "Variable " << var << " does not exist." << std::endl;
		luaFailed = true;
		return 0.0;
	}
	else
	{
		std::cout << "Value " << var << " is not an float." << std::endl;
		luaFailed = true;
		return 0.0;
	}
}

std::string getString(lua_State* L, const char* var)
{
	lua_getglobal(L, var);
	if (lua_isstring(L, -1))
	{
		return lua_tostring(L, -1);
	}
	else if (lua_isnil(L, -1))
	{
		std::cout << "Variable " << var << " does not exist." << std::endl;
		luaFailed = true;
		return "";
	}
	else
	{
		std::cout << "Value " << var << " is not an string." << std::endl;
		luaFailed = true;
		return "";
	}
}

bool getBool(lua_State* L, const char* var)
{
	lua_getglobal(L, var);
	if (lua_isboolean(L,-1))
	{
		return lua_toboolean(L, -1);
	}
	else
	{
		std::cout << "Value " << var << " is not an boolean." << std::endl;
		luaFailed = true;
		return false;
	}
}

bool luaCallFunction(lua_State* L, char* functionName, int numParams, int numReturns)
{
	if (lua_getglobal(L, functionName))
	{
		if (lua_isfunction(L, -1))
		{
			if (luaOkay(L, lua_pcall(L, numParams, numReturns, 0)))
			{
				return true;
			}
		}
	}
	return false;
}

bool initLua(lua_State** L, const std::string& filename, bool func)
{
	luaFailed = false;
	*L = luaL_newstate();
	luaL_openlibs(*L); //allow standard libs
	if (func)
	{
		addModule(*L, solverFunctions, "solver");
	}
	return luaOkay(*L, luaL_dofile(*L, filename.c_str()));
}

void closeLua(lua_State* L)
{
	lua_close(L);
}

bool luaOkay(lua_State* L, const int status)
{
	if (status != LUA_OK)
	{
		std::cout << lua_tostring(L, -1) << std::endl;
		luaFailed = true;
		return false;
	}
	return true;
}

int luaLoadGeometry(lua_State* L)
{
	const char* str = lua_tostring(L, -1);
	bool success = FileHandler::instance()->loadGeometry(str);
	lua_pushboolean(L, success);
	return 1;
}

int luaSolve(lua_State* L)
{
	const char* str = lua_tostring(L, -1);
	SolverConfig config;
	loadSolverConfig(str, config);
	FieldSolver solver(Program::instance()->getMainConfig(), config, FileHandler::instance()->getGeometry());
	Field field;
	solver.solve(field);
	FieldViewer viewer(Program::instance()->getMainConfig(), config, field, solver.getPermativityVector());
	lua_pushnumber(L, viewer.mainLoop());
	return 1;
}

int luaSweep(lua_State* L)
{

	return 0;
}

LuaVM::LuaVM(const std::string& filename)
{
	L() = luaL_newstate();
	luaL_openlibs(L());
	m_luaFailed = !luaOkay(L(), luaL_dofile(L(), filename.c_str()));
}

LuaVM::~LuaVM()
{
	lua_close(L());
}

LuaMainConfig::LuaMainConfig(const std::string& filename, MainConfig& config):
	LuaVM(filename), m_config(config)
{

}

bool LuaMainConfig::loadConfig()
{
	if (!failed())
	{
		m_config.m_timers = getBool("timers");
		m_config.m_screenWidth = getInt("screen_width");
		m_config.m_colorMapPath = getString("color_map_path");
		m_config.m_solverPath = getString("solver_path");
		m_config.m_sweepPath = getString("sweep_path");
		m_config.m_outputDataPath = getString("output_data_folder");
	}

	return !failed();
}

LuaSolverConfig::LuaSolverConfig(const std::string& filename, SolverConfig& config) :
	LuaVM(filename), m_config(config)
{

}

bool LuaSolverConfig::loadConfig()
{
	if (!failed())
	{
		m_config.m_points = getInt("n_points");
		m_config.m_size = getDouble("size_of_structure");
		m_config.m_maxIndexRed = getDouble("max_index_red");
		m_config.m_maxIndexGreen = getDouble("max_index_green");
		m_config.m_maxIndexBlue = getDouble("max_index_blue");
		m_config.m_neffGuess = getDouble("max_neff_guess");
		m_config.m_wavelength = getDouble("wavelength");
		m_config.m_fileName = getString("geometry_filename");
		m_config.m_timers = getBool("timers");
		m_config.m_modes = getInt("num_modes");
	}

	return !failed();
}

LuaSweepConfig::LuaSweepConfig(const std::string& filename, SweepConfig& config):
	LuaVM(filename), m_config(config)
{

}

bool LuaSweepConfig::loadConfig()
{
	if (!failed())
	{
		m_config.m_overlapConfidence = getDouble("overlap_confidence");
		m_config.m_start = getDouble("start_wavelength");
		m_config.m_end = getDouble("end_wavelength");
		m_config.m_nPoints = getInt("n_sweep_points");
		m_config.m_startingMode = getInt("starting_mode");
		m_config.m_outputData = getString("output_data_filename");
	}
	return !failed();
}

double LuaSweepConfig::evaluateIndex(double wavelength, Color color)
{
	const char* func;

	switch (color)
	{
	case Color::RED:
		func = "red_index_profile";
		break;
	case Color::GREEN:
		func = "green_index_profile";
		break;
	case Color::BLUE:
		func = "blue_index_profile";
		break;
	default:
		func = "unknown_function";
	}

	if (lua_getglobal(L(), func))
	{
		lua_pushnumber(L(), wavelength);
		if (luaOkay(L(), lua_pcall(L(), 1, 1, 0)))
		{
			if (lua_isnumber(L(), -1))
			{
				return (double)lua_tonumber(L(), -1);
			}
			else
			{
				std::cout << "Index function must return a float!" << std::endl;
			}
		}	
	}
	failed(true);
	return 1.0;
}



double LuaVM::getDouble(const char* var)
{
	lua_getglobal(L(), var);
	if (lua_isnumber(L(), -1))
	{
		return (double)lua_tonumber(L(), -1);
	}
	else
	{
		std::cout << "Value " << var << " is not an float." << std::endl;
		failed(true);
		return 1.0;
	}
}

bool LuaVM::getBool(const char* var)
{
	lua_getglobal(L(), var);
	if (lua_isboolean(L(), -1))
	{
		return lua_toboolean(L(), -1);
	}
	else
	{
		std::cout << "Value " << var << " is not an boolean." << std::endl;
		failed(true);
		return false;
	}
}

int LuaVM::getInt(const char* var)
{
	lua_getglobal(L(), var);
	if (lua_isnumber(L(), -1))
	{
		return (int)lua_tonumber(L(), -1);
	}
	else
	{
		std::cout << "Value " << var << " is not an int." << std::endl;
		failed(true);
		return 1;
	}
}

std::string LuaVM::getString(const char* var)
{
	lua_getglobal(L(), var);
	if (lua_isstring(L(), -1))
	{
		return lua_tostring(L(), -1);
	}
	else
	{
		std::cout << "Value " << var << " is not a string." << std::endl;
		failed(true);
		return "";
	}
}
