#include "FileHandler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <SFML/Graphics.hpp>
FileHandler* FileHandler::global_instance = nullptr;

FileHandler::FileHandler(MainConfig& config):
	m_config(config)
{
	if (!global_instance)
	{
		global_instance = this;
	}
	else
	{
		std::cout << "Multiple instances of the filehandler!" << std::endl;
	}
}

bool FileHandler::loadGeometry(const std::string& path)
{
	m_geometry.clear();
	m_currentGeometry = path;
	sf::Image geometry;
	bool success = geometry.loadFromFile(path);
	m_config.m_width = geometry.getSize().x;
	m_config.m_height = geometry.getSize().y;
	if (success)
	{
		Color color;
		sf::Color sfColor;
		for (int j = m_config.m_height - 1; j >= 0; j--)
		{
			for (int i = 0; i < m_config.m_width; i++)
			{
				
				sfColor = geometry.getPixel(i, j);
				color.r = sfColor.r;
				color.g = sfColor.g;
				color.b = sfColor.b;
				m_geometry.push_back(color);
			}
		}
		return true;
	}
	return false;
}

bool FileHandler::readCSV
(
	const std::string& path, //(IN) File path
	std::vector<std::vector<double>>& dataVector, //(OUT) 2d vector containing columns and data
	const int columns, //(IN) number of columns
	int ignoreLine //(IN) number of lines to ignore
) const
{
	std::ifstream in(path);
	std::string line;
	std::string data;

	std::vector<std::vector<double>> tempVec;

	if (!in.is_open())
	{
		std::cout << "Failed to open data!" << std::endl;
		//char arr[20];
		char* arr = strerror(errno);
		std::cout << "ERROR: " << arr << std::endl;
		return false;
	}
	while (in.good())
	{
		std::vector<double> buffer;

		for (int i = 0; i < columns; i++)
		{
			if (i < columns - 1)
			{
				getline(in, data, ',');
			}
			else
			{
				getline(in, data, '\n');
			}
			std::stringstream os(data);
			double temp;
			os >> temp;
			buffer.push_back(temp);
		}

		if (ignoreLine != 1) tempVec.push_back(buffer);
		ignoreLine = 0;
	}

	tempVec.pop_back();

	dataVector = tempVec;
	return true;
}