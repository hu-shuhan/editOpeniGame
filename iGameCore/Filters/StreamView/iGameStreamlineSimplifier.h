#pragma once
#include <iGameAttributeSet.h>
#include <iGameUnstructuredMesh.h>
#include <vector>

namespace iGame
{

class StreamlineSimplifier : public Object {
public:
    I_OBJECT(StreamlineSimplifier);
    static Pointer New() { return new StreamlineSimplifier; }

    void SetInput(UnstructuredMesh::Pointer mesh) { m_Input = mesh; }

    void SetCurvBins(int b) { m_CurvBins = b; }
    void SetNumClusters(int k) { m_NumClusters = k; }
    void SetPerCluster(int m) { m_PerCluster = m; }
    void SetCutThreshold(float t) { m_CutThreshold = t; }
    void SetTotalTarget(int t) { m_TotalTarget = t; }
    bool Execute();
    UnstructuredMesh::Pointer GetOutput() const { return m_Output; }

private:
    struct StreamlineData {
        std::vector<Vector3f> points;
        std::vector<Vector3f> velocities;
        std::vector<float> histCurv;
        std::vector<float> cdfCurv;
        int clusterId = -1;
    };

    bool ExtractStreamlines();
    void ComputeHistograms();
    void BuildDistanceMatrix();
    void ClusterAverage();
    void SampleRepresentatives();
    void BuildOutputMesh();

    UnstructuredMesh::Pointer m_Input;
    UnstructuredMesh::Pointer m_Output;
    std::vector<StreamlineData> m_Streamlines;
    std::vector<float> m_D; // ÉÏÈý½Ç
    std::vector<int> m_SelectedIds;

    int m_CurvBins = 40;
    int m_NumClusters = 20;
    int m_PerCluster = 3;
    float m_CurvMax = 1e-6f;
    float m_CutThreshold = 0.5f; 
    int m_TotalTarget = 50;

protected:
    StreamlineSimplifier() = default;
    ~StreamlineSimplifier() override = default;
};

} // namespace iGame