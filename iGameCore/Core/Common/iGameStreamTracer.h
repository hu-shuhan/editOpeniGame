/**
* @file        iGameStreamTracer.h
* @brief       Flow field data calculation component
* @author      RYA
* @date        2024/07/09
* @version     1.0
*/
#pragma once
#include<iGameScene.h>
#include<iGameVolumeMesh.h>
#include<iGameVector.h>
#include<set>
#include<iGamePointFinder.h>
using namespace iGame;

/**
 * @class   iGameStreamTracer
 * @brief   iGameStreamTracer's brief
 */
class iGameStreamTracer {
public:
	/**
	* @brief Constructor for iGameStreamTracer.
	*/
	iGameStreamTracer() {};
    std::vector<Vector3f> seedLineGenerate(int numOfseed);
	/**
	* @brief Generate point seed with admin's parameter.
	* @param[in] model  Input model data
	* @param[in] control Control seedface move in which plane
	* @param[in] proportion Control seedface move proportion
	* @param[in] numOfseed Input num of seed
	*/
	std::vector<Vector3f> seedGenerate( int control, float proportion, int numOfseed);
	/**
	* @brief Generate point seed with admin's parameter.
	* @param[in] model  Input model data
	* @param[in] control Control seedline move in which plane
	* @param[in] proportion Control seedline move proportion
	* @param[in] numOfseed Input num of seed
	*/
	std::vector<Vector3f> streamSeedGenerate( int control, float proportion, int numOfseed);
	/**
	* @brief Calculate the flow  line with admin's parameter.
	* @param[in] seed  Input seed data
	* @param[in] model  Input model data
	* @param[in] vectorName Calculate the flow line with which vectorName
	* @param[in] lengthOfStreamLine Input max line length
	* @param[in] lengthOfStep Input length of a step
	* @param[in] terminalSpeed Stop calculating when the speed falls below how much
	* @param[in] maxSteps Input max num of step
	* @param[out] streamColor Output streamline color data
	*/
	std::vector<std::vector<float>> showStreamLineCellData(std::vector<Vector3f>seed,  std::string vectorName, std::vector<std::vector<float>>& streamColor, float lengthOfStreamLine, float lengthOfStep, float terminalSpeed, int maxSteps);
	bool CellData2PointData( std::string vectorName);
	void SetMesh(VolumeMesh::Pointer mesh) {
		this->mesh = mesh;
	};
	void SetPtFinder(PointFinder::Pointer ptf) {
		this->ptFinder = ptf;
	};
	std::vector<std::vector<float>> showStreamLineMix(std::vector<Vector3f>seed, std::string vectorName, std::vector<std::vector<float>>& streamColor, float lengthOfStreamLine, float lengthOfStep, float terminalSpeed, int maxSteps);
	std::vector<std::vector<float>> showStreamLineHex(std::vector<Vector3f>seed, std::string vectorName, std::vector<std::vector<float>>& streamColor, float lengthOfStreamLine, float lengthOfStep, float terminalSpeed, int maxSteps);
	/**
	* @brief Calculate the flow  face with admin's parameter.
	* @param[in] seed  Input seed data
	* @param[in] model  Input model data
	* @param[in] vectorName Calculate the flow line with which vectorName
	* @param[in] lengthOfStreamLine Input max line length
	* @param[in] lengthOfStep Input length of a step
	* @param[in] terminalSpeed Stop calculating when the speed falls below how much
	* @param[in] maxSteps Input max num of step
	* @param[out] streamColor Output streamline color data
	*/
	std::vector<std::vector<std::vector<float>>> showStreamFace(std::vector<Vector3f>seed, std::string vectorName, std::vector<std::vector<std::vector<float>>>& streamColor, float lengthOfStreamLine, float lengthOfStep, float terminalSpeed, int maxSteps);
private:
	/**
	* @brief Calculate vector values with Newton interpolation method
	* @param[in] coord  Input coord data
	* @param[in] model  Input model data
	* @param[in] vectorName Calculate the flow line with which vectorName
	* @param[in] _vector Input pre-extracted vector data
	* @param[in] terminal Stop calculating when the speed falls below how much
	* @param[out] VolumeId Output In which cell was the point in the last calculation
	* @param[out] inside Output whether this point is in the model
	*/
	Vector3f interpolationVectorTri(Vector3f coord, bool& inside, igIndex& VolumeId,  std::string vectorName, std::vector<Vector3f>_vector, float terminal);
	/**
	* @brief Calculate vector values with Newton interpolation method
	* @param[in] coord  Input coord data
	* @param[in] model  Input model data
	* @param[in] vectorName Calculate the flow line with which vectorName
	* @param[in] _vector Input pre-extracted vector data
	* @param[in] terminal Stop calculating when the speed falls below how much
	* @param[out] VolumeId Output In which cell was the point in the last calculation
	* @param[out] inside Output whether this point is in the model
	*/
	Vector3f interpolationVectorMixWithMeanV(Vector3f coord, bool& inside, igIndex& VolumeId, std::string vectorName, std::vector<Vector3f>_vector, float terminal);
	Vector3f interpolationVectorHexWithNatural(Vector3f coord, bool& inside, igIndex& VolumeId, std::string vectorName, std::vector<Vector3f>_vector, float terminal);
	/**
	* @brief Determine if the point is in the tetrahedron
	* @param[in] coord  Input coord data
	* @param[in] v0  Input a vertex that makes up the tetrahedron
	* @param[in] v1  Input a vertex that makes up the tetrahedron
	* @param[in] v2  Input a vertex that makes up the tetrahedron
	* @param[in] v3  Input a vertex that makes up the tetrahedron
	* @param[out] dis Output the weight calculated based on the distance from the point to the surface
	*/
	std::vector<float> ComputeWeightsForPolygonMesh( igIndex* PointIds,Vector3f coord,igIndex* FaceIds,int MaxPolygonSize,int size,int fsize);
	bool isInside(Vector3f coord, Vector3f v0, Vector3f v1, Vector3f v2, Vector3f v3, std::vector<float>& dis);
	/**
* @brief Computes the values of 8 interpolation functions at given local coordinates.
* @param[in] pcoords  Local coordinates within a cell
* @param[out] sf The value of the interpolation function
*/
	void InterpolationFunctions(const double pcoords[3], double sf[8]);
	/**
* @brief Compute the derivatives of these eight interpolating functions in the r, s, and t directions.
* @param[in] pcoords  Local coordinates within a cell
* @param[out] derivs The derivative of the interpolation function at each node in the r, s and t directions, a total of 8 nodes, 3 derivatives in each direction
*/
	void InterpolationDerivs(const double pcoords[3], double derivs[24]);
	/**
* @brief Calculate the distance from the point to the line
* @param[in] point  Input coord data
* @param[in] lineP1  Input a vertex that makes up the line
* @param[in] lineP2  Input a vertex that makes up the line
*/
	float distance2Line(Vector3f point, Vector3f lineP1, Vector3f lineP2);
	/**
* @brief Calculate the distance from the point to the face
* @param[in] coord  Input coord data
* @param[in] v0  Input a vertex that makes up the face
* @param[in] v1  Input a vertex that makes up the face
* @param[in] v2  Input a vertex that makes up the face
*/
	float pointToFaceDis(Vector3f coord, Vector3f v0, Vector3f v1, Vector3f v2);
	/**
* @brief Calculates the value of the 3x3 determinant
* @param[in] coord  Input coord data
* @param[in] c1  Input A column that forms a determinant
* @param[in] c2  Input A column that forms a determinant
* @param[in] c3  Input A column that forms a determinant
*/
	double Determinant3x3(const double c1[3], const double c2[3], const double c3[3]);
	/**
* @brief Determine if the point is in the tetrahedron
* @param[in] coord  Input coord data
* @param[in] v0  Input a vertex that makes up the face
* @param[in] v1  Input a vertex that makes up the face
* @param[in] v2  Input a vertex that makes up the face
*/
	bool checkContact(Vector3f coord, Vector3f v0, Vector3f v1, Vector3f v2);
	VolumeMesh::Pointer mesh{};
	PointFinder::Pointer ptFinder{};
	FloatArray::Pointer tranform{};
};