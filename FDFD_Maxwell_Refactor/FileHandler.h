#pragma once
#include <string>
#include <vector>
#include "Color.h"
#include "Config.h"

class FileHandler //this is going to handle only files now and not try to build the geometry like before
{
public:

	FileHandler(MainConfig& config);
	bool loadGeometry(const std::string& path);
	bool readCSV(const std::string& path, std::vector<std::vector<double>>& dataVector, const int columns, int ignoreLine = 0) const;
	const std::vector<Color>& getGeometry() const;
	static inline FileHandler* instance();
private:
	static FileHandler* global_instance;
	MainConfig& m_config;
	std::vector<Color> m_geometry;
};

FileHandler* FileHandler::instance()
{
	return global_instance;
}

inline const std::vector<Color>& FileHandler::getGeometry() const
{
	return m_geometry;
}