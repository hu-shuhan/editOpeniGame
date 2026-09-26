#pragma once

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN

class MaskPointsFilter : public Filter {
public:
    I_OBJECT(MaskPointsFilter);
    static Pointer New() { return new MaskPointsFilter; }

    enum RandomModeType {
        RANDOMIZED_ID_STRIDES = 0,
        RANDOM_SAMPLING = 1,
        SPATIALLY_STRATIFIED = 2,
        UNIFORM_SPATIAL_BOUNDS = 3,
        UNIFORM_SPATIAL_SURFACE = 4,
        UNIFORM_SPATIAL_VOLUME = 5
    };

    bool Execute() override;

    void SetOnRatio(int ratio);
    int GetOnRatio() const;

    void SetMaximumNumberOfPoints(IGsize maxPoints);
    IGsize GetMaximumNumberOfPoints() const;

    void SetProportionalMaximumNumberOfPoints(bool enabled);
    bool GetProportionalMaximumNumberOfPoints() const;

    void SetOffset(IGsize offset);
    IGsize GetOffset() const;

    void SetRandomMode(bool enabled);
    bool GetRandomMode() const;

    void SetRandomModeType(int mode);
    int GetRandomModeType() const;

    void SetRandomSeed(unsigned int seed);
    unsigned int GetRandomSeed() const;

    void SetGenerateVertices(bool enabled);
    bool GetGenerateVertices() const;

    void SetSingleVertexPerCell(bool enabled);
    bool GetSingleVertexPerCell() const;

protected:
    MaskPointsFilter();
    ~MaskPointsFilter() override = default;

private:
    int m_OnRatio{2};
    IGsize m_MaximumNumberOfPoints{0};
    bool m_ProportionalMaximumNumberOfPoints{false};
    IGsize m_Offset{0};
    bool m_RandomMode{false};
    int m_RandomModeType{RANDOMIZED_ID_STRIDES};
    unsigned int m_RandomSeed{1};
    bool m_GenerateVertices{false};
    bool m_SingleVertexPerCell{false};
};

IGAME_NAMESPACE_END