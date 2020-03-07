#include "FieldTools.h"
#include "Field.h"
#include "Config.h"

static inline int index(const int i, const int j, const MainConfig& config)
{
	return i + j * config.m_width;
}

double overlap(const Field& field1, const int mode1, const Field& field2, const int mode2, const MainConfig& config)
{
	double xSum12 = 0.0;
	double xSum21 = 0.0;
	double xSum11 = 0.0;
	double xSum22 = 0.0;
	double sum12 = 0.0;
	double sum21 = 0.0;
	double sum11 = 0.0;
	double sum22 = 0.0;

	for (int i = 0; i < config.m_width; i += 1)
	{
		for (int j = 0; j < config.m_height; j += 1)
		{

			int pos = index(i, j, config);

			double Ex1 = field1.Ex.col(mode1)[pos];
			double Ey1 = field1.Ey.col(mode1)[pos];
			double Hx1 = field1.Hx.col(mode1)[pos];
			double Hy1 = field1.Hy.col(mode1)[pos];

			double Ex2 = field2.Ex.col(mode2)[pos];
			double Ey2 = field2.Ey.col(mode2)[pos];
			double Hx2 = field2.Hx.col(mode2)[pos];
			double Hy2 = field2.Hy.col(mode2)[pos];

			xSum12 += (Ex1 * Hy2 - Hx2 * Ey1);
			xSum21 += (Ex2 * Hy1 - Hx1 * Ey2);
			xSum11 += (Ex1 * Hy1 - Hx1 * Ey1);
			xSum22 += (Ex2 * Hy2 - Hx2 * Ey2);
		}
		sum12 += xSum12;
		sum21 += xSum21;
		sum11 += xSum11;
		sum22 += xSum22;
	}
	return std::abs(sum12 * sum21) / std::abs(sum22 * sum11);
}