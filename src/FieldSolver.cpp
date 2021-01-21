#include "FieldSolver.h"
#include "Clock.h"
#include <thread>
#include <iostream>
FieldSolver::FieldSolver
(
	const MainConfig& mainConfig,
	const SolverConfig& config,
	const std::vector<Color>& geometry
):
	m_geometry(geometry),
	m_nx(mainConfig.m_width),
	m_ny(mainConfig.m_height),
	m_m(mainConfig.m_width*mainConfig.m_height),
	m_mainConfig(mainConfig)
{
	m_dx = config.m_size / (double)config.m_points;
	m_dy = m_dx;

	setup(config);
}

void FieldSolver::setup(const SolverConfig& config) //happens before every solve
{
	m_config = config;

	setWavelength(config.m_wavelength);
	buildGeometry();
	//setup perm geometry
}

void FieldSolver::buildGeometry()
{
	m_permativity.clear();
	for (int i = 0; i < m_geometry.size(); i++)
	{
		double n = getRefractiveIndex(m_geometry[i]);
		m_permativity.push_back(n * n);
	}
}

void FieldSolver::buildBoundaries()
{
	Clock c;
	if (m_config.m_timers)
	{
		std::cout << "Building derivatives..." << std::endl;
	}
	
	//clear prev vectors
	clearCoeffs();

	int superI;
	Vector3 p;
	for (int j = 0; j < m_ny; j++)
	{
		for (int i = 0; i < m_nx; i++)
		{
			superI = index(i, j);
			p = getPerm(i, j);

			//Build perm x, y and inverse z coeffs
			m_coeffsPermX.push_back(Triplet(superI, superI, p.x));
			m_coeffsPermY.push_back(Triplet(superI, superI, p.y));
			m_coeffsPermZInverse.push_back(Triplet(superI, superI, 1.0 / p.z));

			//Build Ux and Uy coeffs
			insertCoeff(m_coeffsUx, superI, superI, -1.0);
			if (i != 0 && i != (m_nx - 1))
			{
				insertCoeff(m_coeffsUx, superI, superI + 1, 1.0);
			}
			insertCoeff(m_coeffsUy, superI, superI, -1.0);
			insertCoeff(m_coeffsUy, superI, superI + m_nx, 1.0);
		}
	}
	if (m_config.m_timers)
	{
		std::cout << "Done in " << c.elapsed() << " ms" << std::endl;
	}
}

void FieldSolver::buildMatrices()
{
	Clock c;
	if (m_config.m_timers)
	{
		std::cout << "Building matricies..." << std::endl;
	}
	//resize member functions
	Ux.resize(m_m, m_m);
	Uy.resize(m_m, m_m);

	//Setup matrices of correct size
	SparseMatrix Pxx(m_m, m_m), Pxy(m_m, m_m), Pyx(m_m, m_m), Pyy(m_m, m_m);
	SparseMatrix Vx(m_m, m_m), Vy(m_m, m_m);
	SparseMatrix erx(m_m, m_m), ery(m_m, m_m), erzI(m_m, m_m);
	SparseMatrix I(m_m, m_m);

	Ux.setFromTriplets(m_coeffsUx.begin(), m_coeffsUx.end());
	Uy.setFromTriplets(m_coeffsUy.begin(), m_coeffsUy.end());

	m_coeffsUx.clear();
	m_coeffsUy.clear();

	SparseMatrix Ux_temp = Ux / m_dx;
	SparseMatrix Uy_temp = Uy / m_dy;

	Ux = Ux_temp;
	Uy = Uy_temp;

	Vx = -Ux_temp.transpose();
	Vy = -Uy_temp.transpose();

	erx.setFromTriplets(m_coeffsPermX.begin(), m_coeffsPermX.end());
	ery.setFromTriplets(m_coeffsPermY.begin(), m_coeffsPermY.end());
	erzI.setFromTriplets(m_coeffsPermZInverse.begin(), m_coeffsPermZInverse.end());
	I.setIdentity();
	double kSqr = m_k * m_k;

	Pxx = -(Ux * erzI * Vy * Vx * Uy) / kSqr + (kSqr * I + Ux * erzI * Vx) * (erx + (Vy * Uy) / kSqr);
	Pyy = -(Uy * erzI * Vx * Vy * Ux) / kSqr + (kSqr * I + Uy * erzI * Vy) * (ery + (Vx * Ux) / kSqr);
	Pxy = (Ux * erzI * Vy) * (ery + (Vx * Ux) / kSqr) - ((kSqr * I + Ux * erzI * Vx) * (Vy * Ux)) / kSqr;
	Pyx = (Uy * erzI * Vx) * (erx + (Vy * Uy) / kSqr) - ((kSqr * I + Uy * erzI * Vy) * (Vx * Uy)) / kSqr;

	Pxx.prune(1.0 / m_k);
	Pyy.prune(1.0 / m_k);
	Pxy.prune(1.0 / m_k);
	Pyx.prune(1.0 / m_k);

	if (m_config.m_timers)
	{
		std::cout << "Done in " << c.elapsed() << " ms" << std::endl;
	}

	tensorContraction(Pxx, Pxy, Pyx, Pyy);
}

void FieldSolver::tensorContraction(const SparseMatrix& pxx, const SparseMatrix& pxy, const SparseMatrix& pyx, const SparseMatrix& pyy)
{
	Clock c;
	if (m_config.m_timers)
	{
		std::cout << "Tensor contraction..." << std::endl;
	}

	std::vector<Triplet> v, v1, v2, v3, v4;
	Eigen::initParallel();

	std::thread tr1([&]() {contractionThread(pxx, v1, 0, 0); });
	std::thread tr2([&]() {contractionThread(pxy, v2, 0, m_m); });
	std::thread tr3([&]() {contractionThread(pyx, v3, m_m, 0); });
	std::thread tr4([&]() {contractionThread(pyy, v4, m_m, m_m); });

	tr1.join();
	tr2.join();
	tr3.join();
	tr4.join();

	v.reserve(v1.size() + v2.size() + v3.size() + v4.size());

	v.insert(v.end(), v1.begin(), v1.end());
	v.insert(v.end(), v2.begin(), v2.end());
	v.insert(v.end(), v3.begin(), v3.end());
	v.insert(v.end(), v4.begin(), v4.end());

	m_p.resize(2 * m_m, 2 * m_m);
	m_p.setFromTriplets(v.begin(), v.end());
	if (m_config.m_timers)
	{
		std::cout << "Done in " << c.elapsed() << " ms" << std::endl;
	}
}

void FieldSolver::contractionThread(const SparseMatrix& matrix, std::vector<Triplet>& vectorTriplets, const int lowI, const int lowJ) const
{
	for (int l = 0; l < matrix.outerSize(); l++)
	{
		for (Eigen::SparseMatrix<double>::InnerIterator it(matrix, l); it; ++it)
		{
			const int i = it.col() + lowJ;
			const int j = it.row() + lowI;
			double value = it.value();
			if (i < 2 * m_m && i >= 0 && j < 2 * m_m && j >= 0)
			{
				vectorTriplets.push_back(Triplet(j, i, value));
			}
		}
	}
}

bool FieldSolver::solveForEigenVectors(double sigma)
{
	Clock c;
	
	std::cout << "Solving for eigen vectors/values in " << m_config.m_fileName << std::endl;

	Spectra::SparseGenRealShiftSolve<double> op(m_p);
	Spectra::GenEigsRealShiftSolver<double, Spectra::LARGEST_MAGN, Spectra::SparseGenRealShiftSolve<double>> eigs(&op, m_config.m_modes, m_config.m_modes * 2 + 1, sigma);

	eigs.init();
	int nconv = eigs.compute();
	bool success = getStatus(eigs.info());

	if (success)
	{
		eigenValues = eigs.eigenvalues().real();
		eigenVectors = eigs.eigenvectors().real();
	}

	if (m_config.m_timers)
	{
		std::cout << "Done in " << c.elapsed() << " ms" << std::endl;
	}

	return success;
}

bool FieldSolver::getStatus(int status)
{
	switch (status)
	{
	case Spectra::SUCCESSFUL:
		std::cout << "Successful!" << std::endl;
		return true;
	case Spectra::NOT_CONVERGING:
		std::cout << "ERROR: Sparse Shift Invert Solver: not converging." << std::endl;
		return false;
	case Spectra::NUMERICAL_ISSUE:
		std::cout << "ERROR: Sparse Shift Invert Solver: numerical issue." << std::endl;
		return false;
	default:
		std::cout << "ERROR: Sparse Shift Invert Solver: unknown!" << std::endl;
		return false;
	}
}

bool FieldSolver::solve //(OUT) Whether the process was successful or not
(
	Field& field //(OUT) Solved for field
)
{
	buildBoundaries();
	buildMatrices();
	bool status = solveForEigenVectors();
	if (status)
	{
		constructField(field);
	}
	return status;
}

void FieldSolver::constructField(Field& field)
{
	Clock c;
	if (m_config.m_timers)
	{
		std::cout << "Constructing remaining field components..." << std::endl;
	}

	int inner, outer;
	inner = eigenVectors.innerSize() / 2;
	outer = eigenVectors.outerSize();

	//Set eigen values and variables
	field.eigenValues = eigenValues;
	field.k = m_k;
	field.dx = m_dx;
	field.dy = m_dy;

	//Resize fields
	field.Ex.resize(inner, outer);
	field.Ey.resize(inner, outer);
	field.Ez.resize(inner, outer);
	field.Hx.resize(inner, outer);
	field.Hy.resize(inner, outer);
	field.Hz.resize(inner, outer);

	//Split output into Ex field and Ey field
	for (int i = 0; i < eigenVectors.outerSize(); i++)
	{
		field.Ex.col(i) = eigenVectors.col(i).head(inner);
		field.Ey.col(i) = eigenVectors.col(i).tail(inner);
	}

	//Remake Matrices for operations
	SparseMatrix erzI(m_m, m_m), erx(m_m, m_m), ery(m_m, m_m);
	SparseMatrix Vx(m_m, m_m), Vy(m_m, m_m);
	erzI.setFromTriplets(m_coeffsPermZInverse.begin(), m_coeffsPermZInverse.end());
	erx.setFromTriplets(m_coeffsPermX.begin(), m_coeffsPermX.end());
	ery.setFromTriplets(m_coeffsPermY.begin(), m_coeffsPermY.end());
	Vx = -Ux.transpose();
	Vy = -Uy.transpose();

	clearCoeffs();

	double k0 = m_k;
	//Construct field components
	for (int i = 0; i < outer; i++)
	{
		field.Hz.col(i) = (-1 / k0) * (-Uy * (field.Ex.col(i)) + Ux * (field.Ey.col(i)));
		field.Hx.col(i) = (1 / sqrt(std::abs(eigenValues[i]))) * (Vx * field.Hz.col(i) - k0 * ery * field.Ey.col(i));
		field.Hy.col(i) = (1 / sqrt(std::abs(eigenValues[i]))) * (k0 * erx * field.Ex.col(i) + Vy * field.Hz.col(i));
		field.Ez.col(i) = (1 / k0) * erzI * (-Vy * field.Hx.col(i) + Vx * field.Hy.col(i));
	}

	//Clear boundary matrices used for last time
	Ux.resize(0, 0);
	Uy.resize(0, 0);

	field.makeTEandTM();

	if (m_config.m_timers)
	{
		std::cout << "Done in " << c.elapsed() << " ms" << std::endl;
	}
}