#pragma once
#include <string>
struct MainConfig;
struct SolverConfig;
struct SweepConfig;

extern bool loadMainConfig(const std::string& filename, MainConfig& config);
extern bool loadSolverConfig(const std::string& filename, SolverConfig& config);
extern bool loadSweepConfig(const std::string& filename, SweepConfig& config);
extern double evaluateSweepFunction(double wavelength, int color);
extern void closeSweepConfig();

extern bool runScript(const std::string& filename);

struct lua_State;

class LuaVM
{
public:
	LuaVM(const std::string& filename);
	~LuaVM();
	lua_State*& L();
	bool failed();
	void failed(bool f);
	double getDouble(const char* var);
	bool getBool(const char* var);
	int getInt(const char* var);
	std::string getString(const char* var);
private:
	lua_State* m_L;
	bool m_luaFailed = false;
};

inline lua_State*& LuaVM::L()
{
	return m_L;
}

inline bool LuaVM::failed()
{
	return m_luaFailed;
}

inline void LuaVM::failed(bool f)
{
	m_luaFailed = f;
}

class LuaMainConfig : public LuaVM
{
public:
	LuaMainConfig(const std::string& filename, MainConfig& config);
	bool loadConfig();
private:
	MainConfig& m_config;
};

class LuaSolverConfig : public LuaVM
{
public:
	LuaSolverConfig(const std::string& filename, SolverConfig& config);
	bool loadConfig();
private:
	SolverConfig& m_config;
};

class LuaSweepConfig : public LuaVM
{
public:
	enum class Color
	{
		RED,
		GREEN,
		BLUE
	};

	LuaSweepConfig(const std::string& filename, SweepConfig& config);
	bool loadConfig();
	double evaluateIndex(double wavelength, Color color);
private:
	SweepConfig& m_config;
};

class LuaScript : public LuaVM
{
public:
	LuaScript(const std::string& filename);
	bool executeScript();
private:

};