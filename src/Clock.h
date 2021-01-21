#pragma once
#include <ctime>

class Clock

{
public:
	Clock();
	inline void reset();
	inline double elapsed() const; //Returns elapsed time in miliseconds
private:
	double m_start;
};

Clock::Clock()
{
	m_start = clock();
}

void Clock::reset()
{
	m_start = clock();
}

double Clock::elapsed() const
{
	return 1000 * (clock() - m_start) / CLOCKS_PER_SEC;
}
