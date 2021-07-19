#pragma once
#include "Solve.h"
#include <vector>

#define INVALID_INDEX -1

class FieldHandler
{
public:
	FieldHandler();
	~FieldHandler();
	int clearFields();
	const Solve* getSolveAtIndex( int index ) const;
	int addField( const Field& field, const MainConfig& mainConfig, const SolverConfig& solverConfig, const std::vector<double>& permativityVector );

	static inline FieldHandler* instance();
private:
	static FieldHandler* global_instance;
	std::vector<Solve> m_fields;
};

FieldHandler* FieldHandler::instance()
{
	return global_instance;
}

