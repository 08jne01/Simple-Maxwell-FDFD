#include "FieldHandler.h"
#include <iostream>
FieldHandler* FieldHandler::global_instance = NULL;

FieldHandler::FieldHandler()
{
	if ( ! global_instance )
	{
		global_instance = this;
	}
	else
	{
		std::cout << "Multiple instances of FieldHandler!" << std::endl;
	}
}

FieldHandler::~FieldHandler()
{
	if ( FieldHandler::instance() == this )
		global_instance = NULL;

	m_fields.clear();
}

void FieldHandler::clearFields()
{
	m_fields.clear();
}

const Solve* FieldHandler::getSolveAtIndex( int index ) const
{
	if ( index <= INVALID_INDEX )
		return NULL;

	if ( index >= m_fields.size() )
		return NULL;

	return &m_fields[index];
}

int FieldHandler::addField( const Field& field, const MainConfig& mainConfig, const SolverConfig& solverConfig, const std::vector<double>& permativityVector )
{
	int fieldID = m_fields.size();

	Solve solve;
	solve.field = field;
	solve.mainConfig = mainConfig;
	solve.solverConfig = solverConfig;
	solve.permativityVector = permativityVector;

	m_fields.push_back( solve );

	return fieldID;
}