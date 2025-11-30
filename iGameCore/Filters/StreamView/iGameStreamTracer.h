/**
* @file        iGameStreamTracer.h
* @brief       Flow field data calculation component
* @author      RYA
* @date        2024/07/09
* @version     1.0
*/
#pragma once
#include <iGamePointFinder.h>
#include <iGameScene.h>
#include <iGameStructuredMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVector.h>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <cmath>
#include <thread>
using namespace iGame;

/**
 * @class   iGameStreamTracer
 * @brief   iGameStreamTracer's brief
 */
class iGameStreamTracer : public Filter {
public:
    /**
	* @brief Constructor for iGameStreamTracer.
	*/
    iGameStreamTracer() {};
    std::vector<Vector3f> subdataSeedGenerate(int numOfseed);
    std::vector<Vector3f> seedLineGenerate(int numOfseed);

    Vector3f SampleVector(const Vector3f& coord, bool& inside, igIndex& VolumeId,
                          const std::string& vectorName, float terminalSpeed);
    /**
	* @brief Generate point seed with admin's parameter.
	* @param[in] model  Input model data
	* @param[in] control Control seedface move in which plane
	* @param[in] proportion Control seedface move proportion
	* @param[in] numOfseed Input num of seed
	*/
    std::vector<Vector3f> seedDataGenerate(int control, float proportion, int numOfseed, igIndex pId1, igIndex pId2);
    std::vector<Vector3f> seedGenerate(int control, float proportion, int numOfseed);
    std::vector<Vector3f> seedPidGenerate(int numOfseed, igIndex pId1, igIndex pId2);
    std::vector<Vector3f> seedPCoordGenerate(int numOfseed, Vector3f p1, Vector3f p2);
    /**
	* @brief Generate point seed with admin's parameter.
	* @param[in] model  Input model data
	* @param[in] control Control seedline move in which plane
	* @param[in] proportion Control seedline move proportion
	* @param[in] numOfseed Input num of seed
	*/
    std::vector<Vector3f> streamSeedGenerate(int control, float proportion, int numOfseed);
    std::vector<Vector3f> streamBoundSeedGenerate(int numOfseed);
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
    void initStreamTracer(DataObject::Pointer obj);
    void initSubmodelLinks();
    std::vector<Vector3f> computeSubBlockCenters(const Vector3f& minCorner,
    const Vector3f& maxCorner,
    int splitCount
);
    std::vector<Vector3f> getAllSubBlockCenters(
    const Vector3f& boxMax,      // 包围盒最大值
    const Vector3f& boxMin,      // 包围盒最小值
    const Vector3f& focusMax,    // 重点观察区域最大值
    const Vector3f& focusMin,    // 重点观察区域最小值
    int boxSplitCount,          // 包围盒分割数量（e×e×e）
    int focusSplitCount         // 重点观察区域分割数量（f×f×f）
);
    std::vector<Vector3f> getModelSelect();
    std::vector<Vector3f> getModelSelectMax(std::string VectorName);
    std::vector<std::vector<float>> showStreamLineCellData(std::vector<Vector3f> seed, std::string vectorName,
                                                           std::vector<std::vector<float>>& streamColor,
                                                           float lengthOfStreamLine, float lengthOfStep,
                                                           float terminalSpeed, int maxSteps);
    bool CellData2PointData(std::string vectorName);
    void SetMesh(VolumeMesh::Pointer _mesh) { this->mesh = _mesh; };
    VolumeMesh::Pointer GetMesh() { return this->mesh; };
    void SetSubFlag(bool Subflag) { this->isSubModel = Subflag; };
    void AddPtFinder(PointFinder::Pointer ptf) { this->ptFinder.emplace_back(ptf); };
    void ClearPtFinder() { this->ptFinder.clear(); }
    bool PtFinderEmpty() { return this->ptFinder.empty(); }
    float AccuracyCul(std::vector<std::vector<float>> streamline, float threshold, int Nth);
    /**
     * @brief 设置流线计算的所有参数
     * @param seeds 种子点数组
     * @param vectorName 向量场名称
     * @param lengthOfStreamLine 流线最大长度
     * @param lengthOfStep 积分步长
     * @param terminalSpeed 终止速度
     * @param maxSteps 最大步数
     */
    void SetInput(std::vector<Vector3f>& seeds, std::string& vectorName,
                  float lengthOfStreamLine, float lengthOfStep,
                  float terminalSpeed, int maxSteps) {
        m_SeedPoints = seeds;
        m_VectorName = vectorName;
        m_LengthOfStreamLine = lengthOfStreamLine;
        m_LengthOfStep = lengthOfStep;
        m_TerminalSpeed = terminalSpeed;
        m_MaxSteps = maxSteps;
    }

    /**
     * @brief 执行流线计算，结果存储在内部成员变量中
     */
    bool Execute();

    /**
     * @brief 获取计算结果
     * @return UnstructuredMesh::Pointer 包含流线的非结构化网格
     */
    UnstructuredMesh::Pointer GetOutput() const { return m_ResultMesh; }
    std::vector<std::vector<float>> showStreamLineMix(std::vector<Vector3f> seed, std::string vectorName,
                                                      std::vector<std::vector<float>>& streamColor,
                                                      float lengthOfStreamLine, float lengthOfStep, float terminalSpeed,
                                                      int maxSteps);
    std::vector<std::vector<float>> showStreamLineHex(std::vector<Vector3f> seed, std::string vectorName,
                                                      std::vector<std::vector<float>>& streamColor,
                                                      float lengthOfStreamLine, float lengthOfStep, float terminalSpeed,
                                                      int maxSteps);
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
    std::vector<std::vector<std::vector<float>>>
    showStreamFace(std::vector<Vector3f> seed, std::string vectorName,
                   std::vector<std::vector<std::vector<float>>>& streamColor, float lengthOfStreamLine,
                   float lengthOfStep, float terminalSpeed, int maxSteps);
    void InitAdjacent(iGame::CellArray::Pointer cellData, int vetexNum);

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
    Vector3f interpolationVector(const Vector3f& coord, bool& inside, igIndex& VolumeId, std::string vectorName,
                                 float terminalSpeed);
    Vector3f interpolationVectorTri(const Vector3f& coord, bool& inside, igIndex& VolumeId, std::string vectorName,
                                    float terminal);
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
    Vector3f interpolationVectorMixWithMeanV(const Vector3f& coord, bool& inside, igIndex& VolumeId,
                                             std::string vectorName, float terminal);
    Vector3f interpolationVectorHexWithNatural(const Vector3f& coord, bool& inside, igIndex& VolumeId,
                                               std::string vectorName, float terminal);
    /**
	* @brief Determine if the point is in the tetrahedron
	* @param[in] coord  Input coord data
	* @param[in] v0  Input a vertex that makes up the tetrahedron
	* @param[in] v1  Input a vertex that makes up the tetrahedron
	* @param[in] v2  Input a vertex that makes up the tetrahedron
	* @param[in] v3  Input a vertex that makes up the tetrahedron
	* @param[out] dis Output the weight calculated based on the distance from the point to the surface
	*/
    void ComputeWeightsForPolygonMesh(igIndex* PointIds, const Vector3f& coord, igIndex* FaceIds,
                                       int MaxPolygonSize, int psize, int fsize, float* weights);
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
    bool checkContact(Vector3f coord, Vector3f v0, Vector3f v1, Vector3f v2);
    
private:
    struct adjacent {
        std::vector<long long> offset;
        std::vector<int> data;
    };
    adjacent vetex_link;
    adjacent cell_link;

    // Performance optimization functions
    inline double fastSin(double x);
    inline double fastTan(double x);
    inline double fastAsin(double x);
    inline double fastSqrt(double x);
    void precomputeTrigValues();

    std::unordered_map<int, float> cellBoundLength{};
    VolumeMesh::Pointer mesh{};
    Model::Pointer model{};
    DataObjectId meshId = -1;
    bool isSubModel = false;
    bool isChange = false;
    std::vector<PointFinder::Pointer> ptFinder;
    std::vector<int> subIndex;
    enum StreamMode { Diagonal, PointId, Line };
    StreamMode streamMode = PointId;
    std::shared_mutex rwMutex;
    int processCount;
    int totalProcess;
    std::shared_mutex ProMutex;

    // 存储流线计算参数
    std::vector<Vector3f> m_SeedPoints;
    std::string m_VectorName = "";
    float m_LengthOfStreamLine = 1000.0f;
    float m_LengthOfStep = 0.01f;
    float m_TerminalSpeed = 0.001f;
    int m_MaxSteps = 10000;

    // 存储计算结果
    UnstructuredMesh::Pointer m_ResultMesh;

    // Performance optimization members
    struct TrigCache {
        std::unordered_map<int, double> sinCache;
        std::unordered_map<int, double> tanCache;
        std::unordered_map<int, double> asinCache;
    };
    static thread_local TrigCache trigCache;

    // Memory pool for weights
    static thread_local std::vector<float> reusableWeights;
    static thread_local std::vector<Vector3f> reusableVectors;
    static thread_local std::vector<double> reusableDoubles;
};