#pragma once
#include <SFML/Graphics.hpp>
#include "Color.h"
#include "Field.h"
#include "Config.h"

class FieldViewer

{
public:
	FieldViewer(const MainConfig& config, const SolverConfig& solverConfig, const Field& field, const std::vector<double>& geometry);
	FieldViewer(const MainConfig& config, const SolverConfig& solverConfig, const Field& field, const std::vector<double>& geometry, const std::vector<double>& overlaps);
	void init();
	bool mainLoop();
	void draw();
	void setMode(int mode);
	void writeFields();
	void normalise(Eigen::VectorXd& vec, std::vector<double>& normalisedVals);
	void keyCallBack(sf::Event events);
	void makeGeometryPoints(const std::vector<double>& drawGeometry, const SolverConfig& solverConfig);
	void getColorScheme();
	double interpolate(double d1, double d2, double w) const;
	double getValue(const std::vector<double>& gridPoints, const int sideLengthX, const int sideLengthY, const int x, const int y, const int w, const int h) const;



private:
	//int w, h, modeSet, displayField, gOn, mode, eigs, overlapOn, textOn;

	int m_width, m_height, m_displayField, m_mode, m_numModes;
	bool m_modeSet = false;
	bool m_geomOn = false;
	bool m_overlapOn = false;
	bool m_textOn = true;

	bool failed = false;

	std::vector<double> m_overlaps;
	std::vector<Color> m_colorMap;
	Field m_field;
	sf::RenderWindow window;
	sf::VertexArray m_drawGeometry, m_points;
	//sf::VertexArray points, geometry;
	std::string windowText;
	const MainConfig m_config;
};

inline double FieldViewer::interpolate
(
	double d1, //(IN) point 1
	double d2, //(IN) point 2
	double w //(IN) weight alone line 0.0 -> 1.0
) const

{
	return d1 + w * (d2 - d1);
}