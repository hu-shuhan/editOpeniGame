#ifndef iGameTensorRepresentation_h
#define iGameTensorRepresentation_h
/**
* @file
* @brief  This class is used to represent tensors.
* This class calculates the eigenvalues ​​and eigenvectors of the tensor,
* and calculates and returns the corresponding glyph drawing information.
* The current drawing glyph is set to an ellipse, and other glyphs will be added later.
* Note the tensor is an array which size is 9, meaning a 3*3 tensor matrix.
* @version  v1.0.0
* @date     6/1/2024
* @author   xs
* @par Copyright(c): Hangzhou Dianzi University iGame Laboratory
* @par History:
*    v1.0.0: xs, 6/1/2024, first create this file.
*/
#include "iGameObject.h"
#include "iGamePoints.h"
#include "iGameFilter.h"
#include "iGameArrayObject.h"

IGAME_NAMESPACE_BEGIN
class iGameTensorRepresentation :public Object {
public:
	I_OBJECT(iGameTensorRepresentation);
	static iGameTensorRepresentation* New() { return new iGameTensorRepresentation; }
	~iGameTensorRepresentation();

	/**
	* @brief Get the tensor data. It is an array which size is 9,
	* meaning a 3*3 tensor matrix.
	*/
	double* GetTensor() { return this->Tensor; };
	/**
	* @brief Set the tensor data (a 3x3 symmetric tensor). The eigenvalues ​​and eigenvectors of
	* the tensor matrix are automatically calculated, and the drawing data of the glyph is updated.
	*/
	void SetTensor(double t[9]);
	void SetTensor(float t[9]) {
		double tt[9] = { .0 };
		for (int i = 0; i < 9; i++) {
			tt[i] = (double)t[i];
		}
		SetTensor(tt);
	}
	/**
	* @brief Initialize the eigen date of tensor,
	* including eigenvalues ​​and eigenvectors.
	* The tensor data is setted in class before.
	*/
	void InitTensorEigenData() { InitTensorEigenData(this->Tensor); }
	/**
	* @brief Initialize the eigen date of tensor,
	* including eigenvalues ​​and eigenvectors.
	* @param t[9] The tensor data, is an 3*3 matrix.
	*/
	void InitTensorEigenData(double t[9]);
	void InitTensorEigenData(float t[9]) {
		double tt[9] = { .0 };
		for (int i = 0; i < 9; i++) {
			tt[i] = (double)t[i];
		}
		InitTensorEigenData(tt);
	}
	/**
	* @brief Compute the eigenvalues ​​and eigenvectors of a matrix.
	* @param m[3][3] An 3*3 matrix to compute.
	* @param values[3] The array of eigenvalues.
	* @param v[3][3] The matrix of eigenvectors.
	* Note values[i] is the eigenvalue corresponding to the eigenvector v[i]
	*/
	void InitTensorCharacteristicData(double m[3][3], double values[3], double v[3][3]);
	/*
	* @brief Update the tensor information. The tensor data is sorted
	* from largest to smallest eigenvalues.
	*/
	void SortTensorCharacteristicData(double tensor[3][3]);
	/*
	* @brief Update the drawing information of the glyph.
	* Currently only ellipsoid glyph are supported.
	*/
	void UpdateDrawDataFromTensor();

	/**
	* @brief Set a position for the location of the tensor.
	* This position is used for the purpose of placing glyph.
	*/
	void SetPosition(double pos[3]) {
		std::copy(pos, pos + 3, this->TensorPosition);
	}
	void SetPosition(Vector3f pos) {
		this->TensorPosition[0] = pos[0];
		this->TensorPosition[1] = pos[1];
		this->TensorPosition[2] = pos[2];
	}
	void SetPosition(Vector3d pos) {
		this->TensorPosition[0] = pos[0];
		this->TensorPosition[1] = pos[1];
		this->TensorPosition[2] = pos[2];
	}

	/**
	* @brief Get a position for the location of the tensor.
	* This position is used for the purpose of placing glyph.
	*/
	void GetPosition(double pos[3]) {
		std::copy(this->TensorPosition, this->TensorPosition + 3, pos);
	}
	/**
	* @brief These are methods used to get the eigenvalues. The eigenvalues and
	* associated eigenvectors are sorted in decreasing order.
	*/
	void GetEigenvalues(double evals[3]) {
		std::copy(this->Eigenvalues, this->Eigenvalues + 3, evals);
	}
	/**
	* @brief These are methods used to get the ith eigenvector. The eigenvalues and
	* associated eigenvectors are sorted in decreasing order.
	*/
	void GetEigenVector(int n, double ev[3]) {
		n = (n < 0 ? 0 : (n > 2 ? 2 : n));
		std::copy(this->Eigenvectors[n], this->Eigenvectors[n] + 3, ev);
	}

	/**
	* @Note The following function is used to draw the ellipsoid, which may be placed in a dedicated class later.
	*/

	/**
	* @brief Set the scaling ratio of the glyph.
	* @param s The scaling ratio.
	*/
	void SetScale(double s) { this->scale = s; }
	/**
	* @brief Get the number of draw points, which is used to draw glyph.
	*/
	igIndex GetNumberOfDrawPoints() {
		switch (m_DrawType)
		{
		case iGame::iGameTensorRepresentation::ELLIPSOID:
			return 2 * m_Slices * m_Slices;
		case iGame::iGameTensorRepresentation::CUBOID:
			return 8;
		default:
			break;
		}
		return 0;
	}
	/**
	* @brief Get the draw points of glyph, which is used to draw glyph.
	*/
	FloatArray::Pointer GetDrawPoints() { return this->DrawPoints; }
	/**
	* @brief Get the face sequence of the glyph drawing.
	*/
	IntArray::Pointer GetDrawPointIndexOrders() { return this->DrawPointIndexOrders; }
	/**
	* @brief Get the ellipsoid coordinate point at the current longitude and latitude.
	* @param rx,ry,yz The lengths of the three axes of the ellipsoid.
	* @param a The current longitude.
	* @param b The current latitude.
	* @param p The coordinate point.
	*/
	void GetPointOval(double& rx, double& ry, double& rz, double& a, double& b, iGame::Point& p);
	/**
	* @brief Initialize the draw points using the data set in the class.
	* Enter the three axis lengths of the glyph.
	*/
	void InitDrawPoints() {
		InitDrawPoints(Eigenvalues[0], Eigenvalues[1], Eigenvalues[2]);
	}
	/**
	* @brief Initialize the draw points.
	* Enter the three axis lengths of the glyph.
	*/
	void InitDrawPoints(double eval[3]) {
		InitDrawPoints(eval[0], eval[1], eval[2]);
	}
	/**
	* @brief Initialize the draw points.
	* Enter the three axis lengths of theglyph.
	*/
	void InitDrawPoints(double& rx, double& ry, double& rz);
	/**
	* @brief Set the number of slices. It will automatically initialize basic drawing information,
	* such as the order of drawing points of the slice and basic spherical coordinates.
	*/
	void SetSliceNum(int s);
	/**
	* @brief Update the order of drawing points of the slice with ellipsoid glyph.
	*/
	void UpdateEllipsoidDrawPointIndexOrders();
	/**
	* @brief Update the basic spherical coordinates with ellipsoid glyph.
	*/
	void UpdateEllipsoidDrawPointBasedData();

	/**
	 * @brief Update the order of drawing points of the slice with cuboid glyph.
	*/
	void UpdateCuboidDrawPointIndexOrders();
	/**
	* @brief Update the basic spherical coordinates with cuboid glyph.
	*/
	void UpdateCuboidDrawPointBasedData();
	/**
	* @brief Update the order of drawing points of the slice.
	*/
	void UpdateDrawPointBasedData();
	/**
	* @brief Update the basic spherical coordinates.
	*/
	void UpdateDrawPointIndexOrders();

	enum DrawType {
		INVALID = -1,
		ELLIPSOID,
		CUBOID,
	};


	void SetDrawType(DrawType drawType) {
		this->m_DrawType = drawType;
		UpdateDrawPointBasedData();
		UpdateDrawPointIndexOrders();
	}
protected:
	iGameTensorRepresentation();


private:
	double Tensor[9]; // stored as 3x3 symmetric matrix
	double Eigenvalues[3]; // the eigenvalues of the matrix
	double Eigenvectors[3][3];// the eigenvectors of the matrix
	double TensorPosition[3];// the positon to place glyph
	double scale = 1.0;//  the scaling raito of the glyph

	DrawType m_DrawType = DrawType::ELLIPSOID;

	/**
	* The basic spherical coordinates to draw.
	* Scale, rotate, and translate the coordinates to get the actual point coordinates of the glyph
	*/
	Points::Pointer DrawPointsBaseData;
	FloatArray::Pointer DrawPoints; //the actual point coordinates of the ellipsoid
	IntArray::Pointer DrawPointIndexOrders;// the order of drawing points of the slice
	int m_Slices = 0; // the number of slices

};
IGAME_NAMESPACE_END

#endif
