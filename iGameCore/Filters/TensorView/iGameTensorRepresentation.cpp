#include "iGameTensorRepresentation.h" 
#include "iGameHexahedron.h" 
#include <Eigen/Dense>
using namespace Eigen;
#define DEFAULT_SLICE_NUM 8
const float pi = IGM_PI;
IGAME_NAMESPACE_BEGIN
iGameTensorRepresentation::iGameTensorRepresentation()
{
	this->DrawPoints = FloatArray::New();
	this->DrawPoints->SetDimension(3);
	this->DrawPointsBaseData = Points::New();
	this->DrawPointIndexOrders = IntArray::New();
	this->DrawPointIndexOrders->SetDimension(3);
	this->scale = 0.01;
	this->m_DrawType = ELLIPSOID;
	this->SetSliceNum(DEFAULT_SLICE_NUM);
}
iGameTensorRepresentation::~iGameTensorRepresentation()
{
}



/**
* @brief Set the tensor data (a 3x3 symmetric tensor). The eigenvalues ​​and eigenvectors of
* the tensor matrix are automatically calculated, and the drawing data of the glyph is updated.
*/
void iGameTensorRepresentation::SetTensor(double tensor[9])
{
	std::copy(tensor, tensor + 9, this->Tensor);

	this->InitTensorEigenData();
	// Now update the widget/representation from the tensor
	this->UpdateDrawDataFromTensor();
}
/**
* @brief Initialize the eigen date of tensor,
* including eigenvalues ​​and eigenvectors.
* @param t[9] The tensor data, is an 3*3 matrix.
*/
void iGameTensorRepresentation::InitTensorEigenData(double tensor[9]) {
	// Evaluate eigenfunctions: set up working matrices
	double m[3][3];
	double v[3][3];
	//Prevent the input matrix from being asymmetric
	for (auto j = 0; j < 3; j++) {
		for (auto i = 0; i < 3; i++) {
			m[i][j] = 0.5 * (tensor[i + 3 * j] + tensor[j + 3 * i]);
		}
	}
	this->InitTensorCharacteristicData(m, this->Eigenvalues, v);
	this->SortTensorCharacteristicData(v);
}
/**
* @brief Compute the eigenvalues ​​and eigenvectors of a matrix.
* @param m[3][3] An 3*3 matrix to compute.
* @param values[3] The array of eigenvalues.
* @param v[3][3] The matrix of eigenvectors.
* Note values[i] is the eigenvalue corresponding to the eigenvector v[i]
*/
void iGameTensorRepresentation::InitTensorCharacteristicData(double m[3][3], double values[3], double v[3][3]) {
	Matrix3d matrix;
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			matrix(i, j) = m[i][j];
		}
	}
	SelfAdjointEigenSolver<Matrix3d> eigensolver(matrix);
	if (eigensolver.info() != Success) {
		std::cerr << "Eigenvalue computation failed!" << std::endl;
		return;
	}
	for (int i = 0; i < 3; ++i) {
		values[i] = eigensolver.eigenvalues()[i];
	}
	// Store the feature vectors into the v array
	// Note the difference between row vectors and column vectors
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			v[i][j] = eigensolver.eigenvectors()(j, i);
		}
	}
}
/*
* @brief Update the tensor information. The tensor data is sorted
* from largest to smallest eigenvalues.
*/
void iGameTensorRepresentation::SortTensorCharacteristicData(double vec[3][3])
{
	double n[3]; // eigenvector norms
	int order[3] = { -1, -1, -1 };
	n[0] = std::abs(this->Eigenvalues[0]);
	n[1] = std::abs(this->Eigenvalues[1]);
	n[2] = std::abs(this->Eigenvalues[2]);
	order[0] = (n[0] >= n[1] ? (n[0] >= n[2] ? 0 : 2) : (n[1] >= n[2] ? 1 : 2)); // max
	order[2] = (n[0] < n[1] ? (n[0] < n[2] ? 0 : 2) : (n[1] < n[2] ? 1 : 2));    // min
	order[1] = 3 - order[0] - order[2];                                          // second 

	this->Eigenvalues[0] = n[order[0]];
	this->Eigenvalues[1] = n[order[1]];
	this->Eigenvalues[2] = n[order[2]];

	std::copy(vec[order[0]], vec[order[0]] + 3, this->Eigenvectors[0]);
	std::copy(vec[order[1]], vec[order[1]] + 3, this->Eigenvectors[1]);
	std::copy(vec[order[2]], vec[order[2]] + 3, this->Eigenvectors[2]);
}
/*
* @brief Update the drawing information of the glyph.
* Currently only ellipsoid glyph are supported.
*/
void iGameTensorRepresentation::UpdateDrawDataFromTensor()
{
	//Different graphics will be added later
	InitDrawPoints();
}




/**
* @brief Get the ellipsoid coordinate point at the current longitude and latitude.
* @param rx,ry,yz The lengths of the three axes of the ellipsoid.
* @param a The current longitude.
* @param b The current latitude.
* @param p The coordinate point.
*/
void iGameTensorRepresentation::GetPointOval(double& rx, double& ry, double& rz, double& a, double& b, iGame::Point& p) {
	p[0] = rx * sin(a * pi / 180.0) * cos(b * pi / 180.0);
	p[1] = ry * sin(a * pi / 180.0) * sin(b * pi / 180.0);
	p[2] = rz * cos(a * pi / 180.0);
	p = p * this->scale;
	return;
}
/**
* @brief Initialize the draw points.
* Enter the three axis lengths of the glyph.
*/
void iGameTensorRepresentation::InitDrawPoints(double& rx, double& ry, double& rz) {
	this->DrawPoints->Reset();
	this->DrawPoints->Reserve(DrawPointsBaseData->GetNumberOfPoints());
	Point tmpP;
	Point radius(rx, ry, rz);
	radius = radius * this->scale;
	Point translation(TensorPosition[0], TensorPosition[1], TensorPosition[2]);
	Point rotate[3];
	rotate[0] = Point(Eigenvectors[0][0], Eigenvectors[1][0], Eigenvectors[2][0]);
	rotate[1] = Point(Eigenvectors[0][1], Eigenvectors[1][1], Eigenvectors[2][1]);
	rotate[2] = Point(Eigenvectors[0][2], Eigenvectors[1][2], Eigenvectors[2][2]);
	Point p;

	for (igIndex i = 0; i < this->DrawPointsBaseData->GetNumberOfPoints(); i++) {
		tmpP = this->DrawPointsBaseData->GetPoint(i).scale(radius);
		p[0] = tmpP.dot(rotate[0]);
		p[1] = tmpP.dot(rotate[1]);
		p[2] = tmpP.dot(rotate[2]);
		p += translation;
		this->DrawPoints->AddElement3(p[0], p[1], p[2]);
	}
}
/**
* @brief Set the number of m_Slices. It will automatically initialize basic drawing information,
* such as the order of drawing points of the slice and basic spherical coordinates.
*/
void iGameTensorRepresentation::SetSliceNum(int s) {
	this->m_Slices = s;
	UpdateDrawPointBasedData();
	UpdateDrawPointIndexOrders();
}

/**
* @brief Update the order of drawing points of the slice.
*/
void iGameTensorRepresentation::UpdateDrawPointBasedData()
{
	switch (m_DrawType)
	{
	case ELLIPSOID:
		UpdateEllipsoidDrawPointBasedData();
		break;
	case CUBOID:
		UpdateCuboidDrawPointBasedData();
		break;
	default:
		UpdateEllipsoidDrawPointBasedData();
		break;
	}
}
/**
* @brief Update the basic spherical coordinates.
*/
void iGameTensorRepresentation::UpdateDrawPointIndexOrders()
{
	switch (m_DrawType)
	{
	case ELLIPSOID:
		UpdateEllipsoidDrawPointIndexOrders();
		break;
	case CUBOID:
		UpdateCuboidDrawPointIndexOrders();
		break;
	default:
		UpdateEllipsoidDrawPointIndexOrders();
		break;
	}
}

/**
* @brief Update the basic spherical coordinates.
*/
void iGameTensorRepresentation::UpdateEllipsoidDrawPointBasedData()
{
	int i, j, w = 2 * m_Slices, h = m_Slices;
	double a = 0.0, b = 0.0;
	double hStep = 180.0 / (h - 1);
	double wStep = 360.0 / w;
	int length = w * h;
	this->DrawPointsBaseData->Reset();
	this->DrawPointsBaseData->Reserve(length);
	double degreeScale = pi / 180.0;
	Point p;
	for (a = 0.0, i = 0; i < h; i++, a += hStep) {
		for (b = 0.0, j = 0; j < w; j++, b += wStep) {
			p[0] = sin(a * degreeScale) * cos(b * degreeScale);
			p[1] = sin(a * degreeScale) * sin(b * degreeScale);
			p[2] = cos(a * pi / 180.0);
			this->DrawPointsBaseData->AddPoint(p);
		}
	}
}
/**
* @brief Update the order of drawing points of the slice.
*/
void iGameTensorRepresentation::UpdateEllipsoidDrawPointIndexOrders() {
	int i = 0, j = 0, w = 2 * this->m_Slices, h = this->m_Slices;
	DrawPointIndexOrders->Reset();
	DrawPointIndexOrders->Reserve(w * h);
	for (; i < h - 1; i++) {
		for (j = 0; j < w - 1; j++) {
			DrawPointIndexOrders->AddElement3(i * w + j, i * w + j + 1, (i + 1) * w + j + 1);
			DrawPointIndexOrders->AddElement3(i * w + j, (i + 1) * w + j + 1, (i + 1) * w + j);
		}
		DrawPointIndexOrders->AddElement3(i * w + j, i * w, (i + 1) * w);
		DrawPointIndexOrders->AddElement3(i * w + j, (i + 1) * w, (i + 1) * w + j);
	}
}

/**
 * @brief Update the order of drawing points of the slice with cuboid glyph.
*/
void iGameTensorRepresentation::UpdateCuboidDrawPointIndexOrders()
{
	DrawPointIndexOrders->Reset();
	DrawPointIndexOrders->Reserve(12);
	auto hexahedron = Hexahedron::New();
	for (int i = 0; i < 6; i++) {
		auto quad = Hexahedron::faces[i];
		DrawPointIndexOrders->AddElement3(quad[0], quad[1], quad[2]);
		DrawPointIndexOrders->AddElement3(quad[0], quad[2], quad[3]);
	}
}
/**
* @brief Update the basic spherical coordinates with cuboid glyph.
*/
void iGameTensorRepresentation::UpdateCuboidDrawPointBasedData()
{
	this->DrawPointsBaseData->Resize(8);
	Point p[8] = { {-1,-1,-1},{-1,1,-1},{1,1,-1},{1,-1,-1},
	{-1,-1,1},{-1,1,1},{1,1,1},{1,-1,1} };
	for (int i = 0; i < 8; i++) {
		this->DrawPointsBaseData->SetPoint(i, p[i]);
	}
}

IGAME_NAMESPACE_END
