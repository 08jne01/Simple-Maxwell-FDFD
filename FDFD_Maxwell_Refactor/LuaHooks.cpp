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
#include "FieldHandler.h"
#include "FieldTools.h"

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
static int luaDisplay( lua_State* L );
static int luaClosestOverlap( lua_State* L );
static int luaOutputField( lua_State* L );
static int luaEffectiveIndex( lua_State* L );
static int luaGetAllEffectiveIndex( lua_State* L );
static int luaClearSolves( lua_State* L );

//helper
static void pushTable( lua_State* L, char* key, char* value );
static void pushTable( lua_State* L, int key, int value );
static void pushTable( lua_State* L, int key, double value );

static int getLineNumber( lua_State* L );

static bool loadSolverConfigFromTable( lua_State* L, SolverConfig& config );

//get global lua vars
static int getInt(lua_State* L, const char* var);
static double getDouble(lua_State* L, const char* var);
static std::string getString(lua_State* L, const char* var);
static bool getBool(lua_State* L, const char* var);

static bool getTableValue( lua_State* L, const char* var, bool optional = false );
static int getTableInt( lua_State* L, const char* var, bool& success, bool optional = false );
static double getTableDouble( lua_State* L, const char* var, bool& success, bool optional = false );
static std::string getTableString( lua_State* L, const char* var, bool& success, bool optional = false );
static bool getTableBool( lua_State* L, const char* var, bool& success, bool optional = false );

static void addModule(lua_State* L, const luaL_Reg* funcs, const char* name);
static bool luaCallFunction(lua_State* L, const char* functionName, int numParams, int numReturns);
static double callWavelength(double wavelength, const char* name);

static const luaL_Reg solverFunctions[] = {
	{"loadGeometry", luaLoadGeometry},					// ( string filename ), returns bool success
	{"solve", luaSolve},								// ( table config ) or ( string confg_filename ), returns solve_index or nil
	{"display", luaDisplay},							// ( number solve_index ), returns success, selected_mode
	{"closestOverlap", luaClosestOverlap},				// ( number mode1, number solve_index1, number solve_index2 ), returns overlap, mode2
	{"outputField", luaOutputField},					// ( number mode, number solve_index, string filename ), returns success
	{"getEffectiveIndex", luaEffectiveIndex},			// ( number mode, number solve_index ), returns neff for given mode
	{"getAllEffectiveIndex", luaGetAllEffectiveIndex},  // ( solve_index1 ), returns table of effective indices
	{"clearSolves", luaClearSolves},					// returns number of solves cleared
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

int getLineNumber( lua_State* L )
{
	lua_Debug ar;
	lua_getstack( L, 1, &ar );
	lua_getinfo( L, "l", &ar );
	return ar.currentline;
}

void closeSweepConfig()
{
	closeLua(LFunctions);
}

void pushTable( lua_State* L, int key, double value )
{
	lua_pushnumber( L, key );
	lua_pushnumber( L, value );
	lua_settable( L, -3 );
}

bool loadSolverConfigFromTable( lua_State* L, SolverConfig& config )
{
	bool success = true;

	bool result;
	config.m_points = getTableInt( L, "n_points", result );
	success &= result;
	config.m_size = getTableDouble( L, "size_of_structure", result );
	success &= result;
	config.m_maxIndexRed = getTableDouble( L, "max_index_red", result );
	success &= result;
	config.m_maxIndexGreen = getTableDouble( L, "max_index_green", result );
	success &= result;
	config.m_maxIndexBlue = getTableDouble( L, "max_index_blue", result );
	success &= result;
	config.m_neffGuess = getTableDouble( L, "max_neff_guess", result, true );
	if ( ! result )
	{
		config.m_neffGuess = -1.0;
	}
	
	config.m_wavelength = getTableDouble( L, "wavelength", result );
	success &= result;
	config.m_fileName = getTableString( L, "geometry_filename", result );
	success &= result;
	config.m_timers = getTableBool( L, "timers", result );
	success &= result;
	config.m_modes = getTableInt( L, "num_modes", result );
	success &= result;

	return success;
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

bool getTableValue( lua_State* L, const char* var, bool optional )
{
	if ( lua_istable( L, -1 ) )
	{
		lua_pushstring( L, var );
		lua_gettable( L, -2 );
		return true;
	}
	else
	{
		if ( ! optional )
			printf( "(%d) Error table not supplied.\n", getLineNumber(L) );
		return false;
	}
}

int getTableInt( lua_State* L, const char* var, bool& success, bool optional )
{
	return (int)getTableDouble( L, var, success );
}

double getTableDouble( lua_State* L, const char* var, bool& success, bool optional )
{
	if ( getTableValue( L, var, optional ) )
	{
		if ( lua_isnumber( L, -1 ) )
		{
			double number = lua_tonumber( L, -1 );
			lua_pop( L, 1 );
			success = true;
			return number;
		}
		else
		{
			if ( ! optional )
				printf( "(%d) Error value is not a number.\n", getLineNumber(L) );

			lua_pop( L, 1 );
			success = false;
			return 0.0;
		}
	}
	return 0.0;
}

std::string getTableString( lua_State* L, const char* var, bool& success, bool optional )
{
	if ( getTableValue( L, var, optional ) )
	{
		if ( lua_isstring( L, -1 ) )
		{
			
			std::string value = lua_tostring( L, -1 );
			lua_pop( L, 1 );
			success = true;
			return value;
		}
		else
		{
			if ( ! optional )
				printf( "(%d) Error value is not a string.\n", getLineNumber(L) );

			lua_pop( L, 1 );
			success = false;
			return "";
		}
	}

	success = false;
	return "";
}

bool getTableBool( lua_State* L, const char* var, bool& success, bool optional )
{
	if ( getTableValue( L, var, optional ) )
	{
		if ( lua_isboolean( L, -1 ) )
		{
			bool value = lua_toboolean( L, -1 );
			success = true;
			lua_pop( L, 1 );
			return value;
		}
		else
		{
			if ( ! optional )
				printf( "(%d) Error value is not a number.\n", getLineNumber(L) );

			lua_pop( L, 1 );
			success = false;
			return false;
		}
	}

	success = false;
	return false;
}

bool luaCallFunction(lua_State* L, const char* functionName, int numParams, int numReturns)
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

	printf( "Function \"%s\" could not be found.\n", functionName );
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
	SolverConfig config;
	if ( lua_istable( L, -1 ) )
	{
		if ( ! loadSolverConfigFromTable( L, config ) )
		{
			lua_pushnil( L );
			return 1;
		}
	}
	else if ( lua_isstring(L, -1) )
	{
		const char* str = lua_tostring( L, -1 );
		LuaSolverConfig luaConfig(str, config);
		if ( ! luaConfig.loadConfig() )
		{
			printf( "(%d) Error loading solver config.\n", getLineNumber(L) );
			lua_pushnil( L );
			return 1;
		}
	}
	else
	{
		printf( "(%d) Error unknown argument.\n", getLineNumber(L) );
		//lua_error( L );
		lua_pushnil( L );
		return 1;
	}

	if ( FileHandler::instance()->getGeometryName() != config.m_fileName )
	{
		FileHandler::instance()->loadGeometry( config.m_fileName );
	}

	FieldSolver solver( Program::instance()->getMainConfig(), config, FileHandler::instance()->getGeometry() );
	Field field;
	if ( solver.solve( field ) )
	{
		int index = FieldHandler::instance()->addField( field, Program::instance()->getMainConfig(), config, solver.getPermativityVector() );
		lua_pushnumber( L, index );
		return 1;
	}
	else
	{
		lua_pushnil( L );
		return 1;
	}


	printf( "(%d) Error incorrect params for solve.\n", getLineNumber( L ) );
	lua_pushnil( L );
	return 1;
}

int luaDisplay( lua_State* L )
{
	if ( lua_isnumber( L, -1 ) )
	{
		int index = (int)lua_tonumber( L, -1 );
		const Solve* solve = FieldHandler::instance()->getSolveAtIndex(index);

		if ( solve )
		{
			FieldViewer viewer( solve->mainConfig, solve->solverConfig, solve->field, solve->permativityVector );
			bool quit = viewer.mainLoop();
			lua_pushboolean( L, quit );
			lua_pushnumber( L, (lua_Number)viewer.getCurrentMode() );
			return 2;
		}
		else
		{
			printf( "(%d) Invalid Solve Index.\n", getLineNumber(L) );
			lua_pushnil( L );
			lua_pushnil( L );
			return 2;
		}
	}
	else
	{
		printf( "(%d) Error no solve index supplied.\n", getLineNumber( L ) );
		lua_pushnil( L );
		lua_pushnil( L );
		return 2;
	}
}

int luaOutputField( lua_State* L )
{
	if ( lua_isnumber( L, -3 ) && lua_isnumber(L, -2) && lua_isstring(L, -1) )
	{
		int solveIndex = (int)lua_tonumber( L, -2 );
		int mode = (int)lua_tonumber( L, -3 );
		const char* filename = lua_tostring( L, -1 );

		const Solve* solve = FieldHandler::instance()->getSolveAtIndex( solveIndex );

		if ( ! solve )
		{
			printf( "(%d) Error unable to find solve.\n", getLineNumber( L ) );
			lua_pushboolean( L, false );
			return 1;
		}
		
		bool success = outputFields( solve->field, mode, solve->solverConfig.m_width, solve->solverConfig.m_height, filename );
		lua_pushboolean( L, success );
		return 1;
	}

	printf( "(%d) Error unknown parameters expected (number, string)\n", getLineNumber( L ) );
	lua_pushboolean( L, false );
	return 1;
}

int luaClosestOverlap( lua_State* L )
{
	if ( lua_isnumber(L, -3) && lua_isnumber( L, -2 ) && lua_isnumber( L, -1 ) )
	{
		int mode1 = (int)lua_tonumber( L, -3 );
		int fieldIndex1 = (int)lua_tonumber( L, -2 );
		int fieldIndex2 = (int)lua_tonumber( L, -1 );
		

		const Solve* solve1 = FieldHandler::instance()->getSolveAtIndex( fieldIndex1 );
		const Solve* solve2 = FieldHandler::instance()->getSolveAtIndex( fieldIndex2 );

		if ( solve1 && solve2 )
		{
			if ( solve1->mainConfig.m_width == solve2->mainConfig.m_width &&
				solve1->mainConfig.m_height == solve2->mainConfig.m_height)
			{
				if ( mode1 >= 0 && mode1 < solve1->field.Ex.outerSize() )
				{
					double bestOverlap = 0.0;
					int closestMode = -1;
					for ( int i = 0; i < solve2->field.Ex.outerSize(); i++ )
					{
						double curOverlap = overlap( solve1->field, mode1, solve2->field, i, solve2->mainConfig );

						if ( curOverlap > bestOverlap )
						{
							bestOverlap = curOverlap;
							closestMode = i;
						}
					}

					lua_pushnumber( L, (lua_Number)bestOverlap );
					lua_pushnumber( L, (lua_Number)closestMode );
					return 2;
				}
				else
				{
					printf( "(%d) Error mode not in Solve 1.\n", getLineNumber( L ) );
					lua_pushnil( L );
					lua_pushnil( L );
					return 2;
				}
			}
			else
			{
				printf( "(%d) Error fields are not the same dimensions.\n", getLineNumber( L ) );
				lua_pushnil( L );
				lua_pushnil( L );
				return 2;
			}
		}
		else
		{
			if ( !solve1 )
				printf( "(%d) Error Solve1 could not be found.\n", getLineNumber( L ) );

			if ( !solve2 )
				printf( "(%d) Error Solve2 could not be found.\n", getLineNumber( L ) );

			lua_pushnil( L );
			lua_pushnil( L );
			return 2;
		}
	}
	else
	{
		printf( "(%d) Error please supply two field indices.\n", getLineNumber( L ) );
		lua_pushnil( L );
		lua_pushnil( L );
		return 2;
	}
}

int luaEffectiveIndex( lua_State* L )
{
	if ( lua_isnumber( L, -1 ) && lua_isnumber( L, -2 ) )
	{
		int solveIndex = lua_tonumber( L, -1 );
		int mode = lua_tonumber( L, -2 );
		const Solve* solve = FieldHandler::instance()->getSolveAtIndex( solveIndex );

		if ( solve )
		{
			lua_pushnumber( L, solve->field.neff( mode ) );
			return 1;
		}
		else
		{
			printf( "(%d) Error Solve could not be found.\n", getLineNumber( L ) );
			lua_pushnil( L );
			return 1;
		}
	}
	
	printf( "(%d) Error unknown parameters expected (number, number).\n", getLineNumber( L ) );
	lua_pushnil( L );
	return 1;
}

int luaGetAllEffectiveIndex( lua_State* L )
{
	if ( lua_isnumber( L, -1 ) )
	{
		int solveIndex = lua_tonumber( L, -1 );
		const Solve* solve = FieldHandler::instance()->getSolveAtIndex( solveIndex );

		if ( ! solve )
		{
			printf( "(%d) Error Solve could not be found.\n", getLineNumber( L ) );
			lua_pushnil( L );
			return 1;
		}

		lua_createtable( L, solve->field.Ex.outerSize(), 0 );
		for ( int i = 0; i < solve->field.Ex.outerSize(); i++ )
		{
			pushTable( L, i, solve->field.neff( i ) );
		}

		return 1;
	}

	printf( "(%d) Error unknown parameters expected (number).\n", getLineNumber( L ) );
	lua_pushnil( L );
	return 1;
}

int luaClearSolves( lua_State* L )
{
	int solves = FieldHandler::instance()->clearFields();
	lua_pushboolean( L, solves );
	return 1;
}

int luaSweep(lua_State* L)
{

	return 0;
}

LuaVM::LuaVM( const std::string& filename, bool func)
{
	L() = luaL_newstate();
	luaL_openlibs( L() );
	if ( func )
		addModule( L(), solverFunctions, "solver" );

	m_luaFailed = !luaOkay( L(), luaL_dofile( L(), filename.c_str() ) );
}

LuaVM::LuaVM(const std::string& filename)
{
	L() = luaL_newstate();
	luaL_openlibs(L());
	addModule( L(), solverFunctions, "solver" );
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
		m_config.m_screenWidth = getInt("screen_width");
		m_config.m_colorMapPath = getString("color_map_path");
		m_config.m_scriptPath = getString( "script_path" );
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
				std::cout << "(" << getLineNumber( L() ) << ") Index function must return a float!" << std::endl;
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
		std::cout << "(" << getLineNumber( L() ) << ") Value " << var << " is not an float." << std::endl;
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
		std::cout << "(" << getLineNumber( L() ) << ") Value " << var << " is not an boolean." << std::endl;
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
		std::cout << "(" << getLineNumber( L() ) << ") Value " << var << " is not an int." << std::endl;
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
		std::cout << "(" << getLineNumber( L() ) << ") Value " << var << " is not a string." << std::endl;
		failed(true);
		return "";
	}
}

LuaScript::LuaScript( const std::string& filename ):
	LuaVM(filename, true)
{
	
}

bool LuaScript::executeScript()
{
	luaCallFunction( L(), "main", 0, 1 );
	
	if ( lua_isboolean( L(), -1 ) )
	{
		return lua_toboolean( L(), -1 );
	}
	
	return false;
}
