#include "FieldViewer.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include "FileHandler.h"
#include <fstream>
#include "FieldTools.h"
FieldViewer::FieldViewer
(
	const MainConfig& config,
	const SolverConfig& solverConfig,
	const Field& field,
	const std::vector<double>& geometry
) :
	m_field(field),
	m_config(config)
{
	init();
	makeGeometryPoints(geometry, solverConfig);
}

FieldViewer::FieldViewer
(
	const MainConfig& config,
	const SolverConfig& solverConfig,
	const Field& field,
	const std::vector<double>& geometry,
	const std::vector<double>& overlaps
) :
	m_field(field),
	m_config(config),
	m_overlaps(overlaps)
{
	init();
	makeGeometryPoints(geometry, solverConfig);
}

void FieldViewer::init()
{
	double ratio = (double)m_config.m_height / (double)m_config.m_width;
	m_width = m_config.m_screenWidth;
	m_height = m_width * ratio;
	m_numModes = m_field.eigenValues.size();
	getColorScheme();
}

void FieldViewer::getColorScheme()
{
	std::vector<std::vector<double>> buffer;
	failed = !FileHandler::instance()->readCSV(m_config.m_colorMapPath, buffer, 3);
	for (int i = 0; i < buffer.size(); i++)

	{
		m_colorMap.push_back(Color(buffer[i][0] * 255, buffer[i][1] * 255, buffer[i][2] * 255));
	}
}

void FieldViewer::makeGeometryPoints(const std::vector<double>& drawGeometry, const SolverConfig& solverConfig)
{
	double maxn = std::max(solverConfig.m_maxIndexRed, std::max(solverConfig.m_maxIndexGreen, solverConfig.m_maxIndexBlue));
	for (int i = 0; i < m_width; i++)
	{
		for (int j = 0; j < m_height; j++)
		{
			double value = getValue(drawGeometry, m_config.m_width, m_config.m_height, i, j, m_width, m_height);
			value = 255.0*(value-1) / (maxn * maxn - 1.0);
			if (value > 10)
			{
				m_drawGeometry.append(sf::Vertex(sf::Vector2f(i, (-j + m_height) % m_height), sf::Color(value, value, value, 100)));
			}
		}
	}
}

bool FieldViewer::mainLoop()
{
	if (failed)
	{
		return false;
	}


	sf::Event events;
	sf::Clock sfClock;
	sf::Text text;
	sf::Font font;
	if (!font.loadFromFile("Resources/arial.ttf"))
	{
		failed = true;
	}
	text.setFont(font);
	text.setCharacterSize(15);
	text.setPosition(sf::Vector2f(0, 0));
	text.setFillColor(sf::Color::Red);
	windowText = "";

	window.create(sf::VideoMode(m_width, m_height), "FDFD Field Solver - Field Viewer");

	if (!window.isOpen())
	{
		return false;
	}

	while (window.isOpen())
	{
		while (window.pollEvent(events))
		{
			if (events.type == sf::Event::EventType::Closed)
			{
				window.close();
			}
			//Callback for user commands
			keyCallBack(events);
		}

		if (!m_modeSet)
		{
			setMode(m_mode);
			text.setString(windowText);
			m_modeSet = true;
		}

		window.clear(sf::Color::Black);
		//Draw Here
		draw();
		if (m_textOn) window.draw(text);
		window.display();
	}

	if (m_overlapOn)
	{
		return m_mode;
	}
	
	return true;

}

void FieldViewer::draw()
{

	window.draw(m_points);
	if (m_geomOn)
	{
		window.draw(m_drawGeometry);
	}
}

void FieldViewer::setMode(int mode)
{
	//Sets the mode
	std::stringstream os;
	os << "Displaying " << m_field.getFieldName(m_displayField) << " Component" << std::endl;

	if (m_overlapOn)
	{
		os << "Overlap: " << m_overlaps[mode] << std::endl;
	}

	os << std::setprecision(10) << "neff: " << m_field.neff( mode ) << std::endl
		<< "Current Mode: " << (mode) << std::endl;

	windowText = os.str();
	m_points.clear();
	std::vector<double> normal;
	Eigen::VectorXd vec = m_field.getField(m_displayField).col(mode);
	normalise(vec, normal);
	//Sets color scheme
	for (int i = 0; i < m_width; i++)
	{
		for (int j = 0; j < m_height; j++)
		{
			double val = getValue(normal, m_config.m_width, m_config.m_height, i, j, m_width, m_height);

			double greyScale = (255.0 / 2.0) * val + (255.0 / 2.0);
			int greyInt = (int)greyScale;
			Color c;
			if (greyInt > m_colorMap.size()) c = m_colorMap.back();
			else if (greyInt < 0) c = m_colorMap.front();
			else c = m_colorMap[greyInt];
			m_points.append(sf::Vertex(sf::Vector2f(i, (-j + m_height) % m_height), sf::Color(c.r, c.g, c.b)));
		}
	}
}

double FieldViewer::getValue
(
	const std::vector<double>& gridPoints,
	const int sideLengthX,
	const int sideLengthY,
	const int x,
	const int y,
	const int w,
	const int h
) const
{
	//Interpolation algorithm (Similar to the last half of a perlin noise algorithm)

	double xVal = ((double)x / (double)w) * (double)sideLengthX;
	double yVal = ((double)y / (double)h) * (double)sideLengthY;

	int xVal0 = (int)xVal;
	int xVal1 = xVal0 + 1;
	int yVal0 = (int)yVal;
	int yVal1 = yVal0 + 1;

	if (xVal1 + yVal1 * sideLengthX < sideLengthX * sideLengthY)
	{
		double sX = xVal - double(xVal0);
		double sY = yVal - double(yVal0);
		double ix0, ix1, n0, n1;
		n0 = gridPoints[xVal0 + yVal0 * sideLengthX];
		n1 = gridPoints[xVal1 + yVal0 * sideLengthX];

		ix0 = interpolate(n0, n1, sX);

		n0 = gridPoints[xVal0 + yVal1 * sideLengthX];
		n1 = gridPoints[xVal1 + yVal1 * sideLengthX];

		ix1 = interpolate(n0, n1, sX);
		return interpolate(ix0, ix1, sY);
	}

	else
	{
		return 0;
	}
}

void FieldViewer::normalise(Eigen::VectorXd& vec, std::vector<double>& normalisedVals)

{
	//Normalising algorithm for rendering
	double max = -10.0;

	for (int i = 0; i < vec.size(); i++)
	{
		if (std::abs(vec[i]) > max)
		{
			max = std::abs(vec[i]);
		}
	}

	for (int i = 0; i < vec.size(); i++)
	{
		double val = 1 * vec[i] / max;

		if (val > 1.0)
		{
			val = 1.0;
		}

		else if (val < -1.0)
		{
			val = -1.0;
		}
		normalisedVals.push_back(val);
	}
}

void FieldViewer::writeFields()
{
	//Output fields to a file
	std::ofstream file;
	std::stringstream os;
	std::string s = m_config.m_outputDataPath;
	s += "/Field_Components_Mode_";
	std::string end = ".dat";
	os << s << m_mode << end;
	file.open(os.str());

	outputFields( m_field, m_mode, m_config.m_width, m_config.m_height, os.str().c_str() );
}

void FieldViewer::keyCallBack(sf::Event events)
{	//Callbacks for commands
	if (events.type == sf::Event::EventType::KeyPressed)
	{
		switch (events.key.code)
		{
		case sf::Keyboard::Left:
			m_displayField--;
			m_displayField = m_displayField < 0 ? 7 : m_displayField;
			//if (displayField < 0) displayField = 7;
			m_modeSet = false;
			break;

		case sf::Keyboard::Right:
			m_displayField++;
			m_displayField = m_displayField > 7 ? 0 : m_displayField;
			//if (displayField > 7) displayField = 0;
			m_modeSet = false;
			break;

		case sf::Keyboard::Up:
			m_mode++;
			m_mode = m_mode >= m_numModes ? m_mode = 0 : m_mode;
			//if (m_mode > eigs - 1) mode = 0;
			m_modeSet = false;
			break;

		case sf::Keyboard::Down:
			m_mode--;
			m_mode = m_mode < 0 ? m_numModes - 1 : m_mode;
			//if (mode < 0) mode = eigs - 1;
			m_modeSet = false;
			break;

		case sf::Keyboard::Space:
			m_geomOn = m_geomOn ? false : true;
			//if (gOn == 1) gOn = 0;
			//else gOn = 1;
			break;

		case sf::Keyboard::Return:
			std::cout << "Outputing Current Fields..." << std::endl;
			writeFields();
			break;

		case sf::Keyboard::Escape:
			window.close();
			break;

		case sf::Keyboard::BackSpace:
			window.close();
			m_mode = -1;
			break;

		case sf::Keyboard::T:
			//if (m_textOn == 1) m_textOn = 0;
			//else m_textOn = 1;
			m_textOn = m_textOn ? false : true;
			break;
		}
	}
}