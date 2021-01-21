#pragma once
#include <Eigen/SparseCore>
#include <string>
class Field
{
public:
	enum Type
	{
		FIELD_EX,
		FIELD_EY,
		FIELD_EZ,
		FIELD_HX,
		FIELD_HY,
		FIELD_HZ,
		FIELD_TE,
		FIELD_TM
	};

	typedef Eigen::MatrixXd mat;
	typedef Eigen::VectorXd vec;

	mat Ex, Ey, Ez;
	mat Hx, Hy, Hz;
	mat TE, TM;

	vec eigenValues;
	double k, dx, dy;

	const char* m_fieldNames[8] = {"Ex", "Ey", "Ez", "Hx", "Hy", "Hz", "Tranverse Electric", "Transverse Magnetic"};

	Field(){}
	Field(mat ex, mat ey, mat ez, mat hx, mat hy, mat hz) :
		Ex(ex), Ey(ey), Ez(ez),
		Hx(hx), Hy(hy), Hz(hz)
	{

	}

	inline double neff( int mode ) const
	{
		return sqrt( std::abs( (double)eigenValues[mode] ) ) / (double)k;
	}

	void makeTEandTM()

	{
		TE = Ex;
		TM = Ex;
		for (int i = 0; i < Ex.outerSize(); i++)
		{
			for (int j = 0; j < Ex.innerSize(); j++)
			{
				TE.coeffRef(j, i) = sqrt(Ex.col(i)[j] * Ex.col(i)[j] + Ey.col(i)[j] * Ey.col(i)[j]);
				TM.coeffRef(j, i) = sqrt(Hx.col(i)[j] * Hx.col(i)[j] + Hy.col(i)[j] * Hy.col(i)[j]);
			}
		}
	}
	const char* getFieldName(const int field) const;
	const Field::mat& getField(const int field) const
	{
		switch (field)
		{
		case FIELD_EX:
			return Ex;
		case FIELD_EY:
			return Ey;
		case FIELD_EZ:
			return Ez;
		case FIELD_HX:
			return Hx;
		case FIELD_HY:
			return Hy;
		case FIELD_HZ:
			return Hz;
		case FIELD_TE:
			return TE;
		case FIELD_TM:
			return TM;
		default:
			return TE;
		}
	}

	

private:
};

inline const char* Field::getFieldName(const int field) const
{
	return m_fieldNames[field];
}

