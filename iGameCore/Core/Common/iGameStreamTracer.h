/**
* @file        iGameStreamTracer.h
* @brief       Flow field data calculation component
* @author      RYA
* @date        2024/07/09
* @version     1.0
*/
#pragma once
#include<iGameScene.h>
#include<iGameVector.h>
#include<set>
#include<iGamePointFinder.h>
#include <iGameUnstructuredMesh.h>
#include<iGameStructuredMesh.h>
#include<unordered_map>
#include <shared_mutex>
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
    std::vector<Vector3d> subdataSeedGenerate(int numOfseed);
    std::vector<Vector3d> seedLineGenerate(int numOfseed);
	/**
	* @brief Generate point seed with admin's parameter.
	* @param[in] model  Input model data
	* @param[in] control Control seedface move in which plane
	* @param[in] proportion Control seedface move proportion
	* @param[in] numOfseed Input num of seed
	*/
    std::vector<Vector3d> seedDataGenerate(int control, float proportion, int numOfseed, igIndex pId1, igIndex pId2);
    std::vector<Vector3d> seedGenerate(int control, float proportion, int numOfseed);
    std::vector<Vector3d> seedPidGenerate(int numOfseed, igIndex pId1, igIndex pId2);
    std::vector<Vector3d> seedPCoordGenerate(int numOfseed, Vector3d p1, Vector3d p2);
	/**
	* @brief Generate point seed with admin's parameter.
	* @param[in] model  Input model data
	* @param[in] control Control seedline move in which plane
	* @param[in] proportion Control seedline move proportion
	* @param[in] numOfseed Input num of seed
	*/
    std::vector<Vector3d> streamSeedGenerate(int control, float proportion, int numOfseed);
    std::vector<Vector3d> streamBoundSeedGenerate(int numOfseed);
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
    void initStreamTracer(Model::Pointer _model);
    void initSubmodelLinks();
	std::vector<std::vector<float>> showStreamLineCellData(std::vector<Vector3d>seed,  std::string vectorName, std::vector<std::vector<float>>& streamColor, float lengthOfStreamLine, float lengthOfStep, float terminalSpeed, int maxSteps);
	bool CellData2PointData( std::string vectorName);
    void SetMesh(VolumeMesh::Pointer _mesh) { this->mesh = _mesh; };
    VolumeMesh::Pointer GetMesh() { return this->mesh; };
    void SetSubFlag(bool Subflag) { this->isSubModel = Subflag; };
	void AddPtFinder(PointFinder::Pointer ptf) {
		this->ptFinder.emplace_back(ptf);
	};
    void ClearPtFinder(){ 
		this->ptFinder.clear();
	
	}
    bool PtFinderEmpty() { return this->ptFinder.empty();
	}
    float AccuracyCul(std::vector<std::vector<float>> streamline, float threshold,int Nth);
	std::vector<std::vector<float>> showStreamLineMix(std::vector<Vector3d>seed, std::string vectorName, std::vector<std::vector<float>>& streamColor, float lengthOfStreamLine, float lengthOfStep, float terminalSpeed, int maxSteps);
	std::vector<std::vector<float>> showStreamLineHex(std::vector<Vector3d>seed, std::string vectorName, std::vector<std::vector<float>>& streamColor, float lengthOfStreamLine, float lengthOfStep, float terminalSpeed, int maxSteps);
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
	std::vector<std::vector<std::vector<float>>> showStreamFace(std::vector<Vector3d>seed, std::string vectorName, std::vector<std::vector<std::vector<float>>>& streamColor, float lengthOfStreamLine, float lengthOfStep, float terminalSpeed, int maxSteps);
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
    Vector3d interpolationVector(Vector3d coord, bool& inside, igIndex& VolumeId,
                                                    std::string vectorName, std::vector<std::vector<Vector3d>> _vector,
                                                    float terminalSpeed);
	Vector3d interpolationVectorTri(Vector3d coord, bool& inside, igIndex& VolumeId,  std::string vectorName, std::vector<Vector3d>_vector, float terminal);
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
	Vector3d interpolationVectorMixWithMeanV(Vector3d coord, bool& inside, igIndex& VolumeId, std::string vectorName, std::vector<Vector3d>_vector, float terminal);
	Vector3d interpolationVectorHexWithNatural(Vector3d coord, bool& inside, igIndex& VolumeId, std::string vectorName, std::vector<Vector3d>_vector, float terminal);
	/**
	* @brief Determine if the point is in the tetrahedron
	* @param[in] coord  Input coord data
	* @param[in] v0  Input a vertex that makes up the tetrahedron
	* @param[in] v1  Input a vertex that makes up the tetrahedron
	* @param[in] v2  Input a vertex that makes up the tetrahedron
	* @param[in] v3  Input a vertex that makes up the tetrahedron
	* @param[out] dis Output the weight calculated based on the distance from the point to the surface
	*/
	std::vector<float> ComputeWeightsForPolygonMesh( igIndex* PointIds,Vector3d coord,igIndex* FaceIds,int MaxPolygonSize,int size,int fsize);
	bool isInside(Vector3d coord, Vector3d v0, Vector3d v1, Vector3d v2, Vector3d v3, std::vector<float>& dis);
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
	float distance2Line(Vector3d point, Vector3d lineP1, Vector3d lineP2);
	/**
* @brief Calculate the distance from the point to the face
* @param[in] coord  Input coord data
* @param[in] v0  Input a vertex that makes up the face
* @param[in] v1  Input a vertex that makes up the face
* @param[in] v2  Input a vertex that makes up the face
*/
    float pointToFaceDis(Vector3f coord, Vector3f v0, Vector3f v1, Vector3f v2);
    double pointToFaceDis(Vector3d coord, Vector3d v0, Vector3d v1, Vector3d v2);
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
	bool checkContact(Vector3d coord, Vector3d v0, Vector3d v1, Vector3d v2);
    std::unordered_map<int, float> cellBoundLength{};
	VolumeMesh::Pointer mesh{};
    Model::Pointer model{};
    bool isSubModel = false;
    bool isChange = false;
	std::vector<PointFinder::Pointer> ptFinder;
    std::vector<std::vector<Vector3d>> _vector;
    std::vector<int> subIndex;
    enum StreamMode { Diagonal, PointId, Line };
    StreamMode streamMode = PointId;
    std::shared_mutex rwMutex;
};