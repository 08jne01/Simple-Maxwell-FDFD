#include <iostream>
#include <SFML/Graphics.hpp>
#include "Program.h"
static bool isParam(char* phrase, const char* longer, const char* shorter);

bool isParam(char* phrase, const char* longer, const char* shorter)
{
	return (strcmp(phrase, longer) == 0 || strcmp(phrase, shorter) == 0);
}

int main(int argc, char** argv)
{
	sf::err().rdbuf(NULL);

	bool sweep = false;
	bool lua = false;
	char* script = NULL;
	for (int i = 0; i < argc; i++)
	{
		if (isParam(argv[i], "--sweep", "-s"))
		{
			sweep = true;
		}
		else if (isParam(argv[i], "--lua", "-l"))
		{
			lua = true;
			if (i + 1 < argc)
			{
				script = argv[i + 1];
			}
		}
		else if (isParam(argv[i], "--single", "-z"))
		{

		}
		else if (isParam(argv[i], "--help", "-h"))
		{
			std::cout << "[] indicates optional parameter" << std::endl;
			std::cout << "<> indicates mandatory parameter" << std::endl;
			std::cout << "--help or -h for help" << std::endl;
			std::cout << "--lua <script> or -l <script> to run lua script" << std::endl;
			std::cout << "--sweep [config] or -s [config] to run sweep" << std::endl;
			std::cout << "--single [config] or -z [config] to run one solve" << std::endl;
		}
	}

	MainConfig config;
	LuaMainConfig configLua("main.lua", config);

	//if (!loadMainConfig("main.lua", config))
	//{
		//return EXIT_FAILURE;
	//}

	if (!configLua.loadConfig())
	{
		return EXIT_FAILURE;
	}

	Program::ProgramType type;
	if (lua)
	{
		type = Program::ProgramType::SCRIPT;
		if ( script )
			config.m_scriptPath = script;
	}
	else if (sweep)
	{
		type = Program::ProgramType::SWEEP;
	}
	else
	{
		type = Program::ProgramType::SINGLE;
	}

	Program program(config, type);
	return program.returnWithErrors();
}