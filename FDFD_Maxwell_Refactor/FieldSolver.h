#pragma once
#include <Eigen/Sparse>
#include <Spectra/GenEigsRealShiftSolver.h>
#include <Spectra/MatOp/SparseGenMatProd.h>
#include <Spectra/MatOp/SparseGenRealShiftSolve.h>
#include "Vector.h"
#define PI 3.14159
#include "Config.h"
#include "Color.h"
#include "Field.h"
class FieldSolver
{
public:
	FieldSolver(const MainConfig& mainConfig, const SolverConfig& config, const std::vector<Color>& geometry);
 
	void setup(const SolverConfig& config); //happens before every solve
	void buildGeometry();
	bool solve(Field& field); //solve obviously

	void setWavelength(const double wavelength);
	const std::vector<double>& getPermativityVector() const;

private:
	typedef Eigen::Triplet<double> Triplet;
	typedef Eigen::SparseMatrix<double> SparseMatrix;

	//Tools
	int index(const int i, const int j) const;
	void insertCoeff(std::vector<Triplet>& coeffs, const int superI, const int superJ, const double value) const;
	Vector3 getPerm(const int i, const int j) const;
	double getRefractiveIndex(const Color& color, double rMax, double gMax, double bMax) const;
	double getRefractiveIndex(const Color& color) const;
	void clearCoeffs();

	void buildBoundaries();
	void buildMatrices();
	void tensorContraction(const SparseMatrix& pxx, const SparseMatrix& pxy, const SparseMatrix& pyx, const SparseMatrix& pyy);
	void contractionThread(const SparseMatrix& matrix, std::vector<Triplet>& vectorTriplets, const int lowI, const int lowJ) const;
	bool solveForEigenVectors();
	bool solveForEigenVectors(double sigma);
	bool getStatus(int status);
	void constructField(Field& field);

	//geometry that gets the m_permativity
	const std::vector<Color> m_geometry;
	//permativities for solving
	std::vector<double> m_permativity;

	//Temp -> zeroed between passes
	std::vector<Triplet> m_coeffsUx;
	std::vector<Triplet> m_coeffsUy;
	std::vector<Triplet> m_coeffsPermX;
	std::vector<Triplet> m_coeffsPermY;
	std::vector<Triplet> m_coeffsPermZInverse;

	//Just before Solve, also temp
	SparseMatrix m_p, Ux, Uy;
	//Results
	Eigen::VectorXd eigenValues;
	Eigen::MatrixXd eigenVectors;
	//size in x, y and x*y
	int m_nx, m_ny, m_m, m_convergance, m_numberEigenValues;
	double m_k, m_dx, m_dy; //constant for class
	SolverConfig m_config;
	MainConfig m_mainConfig;
};

inline int FieldSolver::index
(
	const int i, //(IN) i'th (x) grid point
	const int j //(IN) j'th (y) grid point
) const
{
	return i + j * m_nx; //(OUT) index into the array
}

inline void FieldSolver::setWavelength(const double wavelength) //(IN) wavelength to set wave vector
{
	m_k = 2.0 * PI / wavelength;
}

inline void FieldSolver::insertCoeff
(
	std::vector<Triplet>& coeffs, //(OUT) matrix being set
	const int superI, //(IN) column to be set
	const int superJ, //(IN) row to be set
	const double value //(IN) value to be set in matrix
) const
{
	if (superI < m_m && superJ < m_m && superI >= 0 && superJ >= 0)
	{
		coeffs.push_back(Triplet(superI, superJ, value));
	}
}

inline Vector3 FieldSolver::getPerm
(
	const int i, //(IN) i'th (x) grid point
	const int j //(IN) j'th (y) grid point
) const
{
	int superI = index(i, j);
	return Vector3(m_permativity[superI]);
}

inline double FieldSolver::getRefractiveIndex
(
	const Color& color, //(IN) Color in
	double rMax, //(IN) Max refractive index red
	double gMax, //(IN) Max refractive index green
	double bMax //(IN) Max refractive index blue
) const
{
	if (rMax < 0)
	{
		rMax = m_config.m_maxIndexRed;
		gMax = m_config.m_maxIndexGreen;
		bMax = m_config.m_maxIndexBlue;
	}

	double refR = (rMax - 1.0) * (double)color.r / 255.0 + 1.0;
	double refG = (gMax - 1.0) * (double)color.g / 255.0 + 1.0;
	double refB = (bMax - 1.0) * (double)color.b / 255.0 + 1.0;

	return std::max(std::max(refG, refB), refR); //(OUT) refractive index for this color and settings
}

inline double FieldSolver::getRefractiveIndex
(
	const Color& color //(IN) Color in
) const
{
	return getRefractiveIndex(color, -1.0, -1.0, -1.0);
}

inline void FieldSolver::clearCoeffs()
{
	m_coeffsPermX.clear();
	m_coeffsPermY.clear();
	m_coeffsPermZInverse.clear();
	m_coeffsUx.clear();
	m_coeffsUy.clear();
}

inline bool FieldSolver::solveForEigenVectors()
{
	double maxPerm = std::max(std::max(m_config.m_maxIndexRed, m_config.m_maxIndexGreen), m_config.m_maxIndexBlue);
	return solveForEigenVectors(m_k * m_k * maxPerm * maxPerm);
}

inline const std::vector<double>& FieldSolver::getPermativityVector() const
{
	return m_permativity;
}