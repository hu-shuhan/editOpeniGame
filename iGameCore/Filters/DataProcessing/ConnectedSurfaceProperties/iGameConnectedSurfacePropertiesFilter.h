#pragma once

#include <iGameFilter.h>
#include <iGameFlatArray.h>
#include <iGameSurfaceMesh.h>

#include <string>

IGAME_NAMESPACE_BEGIN

class ConnectedSurfacePropertiesFilter : public Filter {
public:
    I_OBJECT(ConnectedSurfacePropertiesFilter);
    static Pointer New() { return new ConnectedSurfacePropertiesFilter; }

    bool Execute() override;

    void SetSkipObjectIdentification(bool value) { m_SkipObjectIdentification = value; }
    bool GetSkipObjectIdentification() const { return m_SkipObjectIdentification; }

    void SetObjectIdsArrayName(const std::string& name) {
        m_ObjectIdsArrayName = name.empty() ? "ObjectIds" : name;
    }
    const std::string& GetObjectIdsArrayName() const { return m_ObjectIdsArrayName; }
    bool GetUsedSuppliedObjectIds() const { return m_UsedSuppliedObjectIds; }

    int GetNumberOfObjects() const { return m_NumberOfObjects; }
    bool GetAllValid() const { return m_AllValid; }
    double GetTotalArea() const { return m_TotalArea; }
    double GetTotalVolume() const { return m_TotalVolume; }
    const std::string& GetLastError() const { return m_LastError; }

    LongLongArray::Pointer GetObjectIds() const { return m_ObjectIds; }
    DoubleArray::Pointer GetAreas() const { return m_Areas; }
    DoubleArray::Pointer GetVolumes() const { return m_Volumes; }
    IntArray::Pointer GetObjectValidity() const { return m_ObjectValidity; }
    DoubleArray::Pointer GetObjectAreas() const { return m_ObjectAreas; }
    DoubleArray::Pointer GetObjectVolumes() const { return m_ObjectVolumes; }
    DoubleArray::Pointer GetObjectCentroids() const { return m_ObjectCentroids; }

    static const char* ObjectIdsArrayName() { return "ObjectIds"; }
    static const char* AreasArrayName() { return "Areas"; }
    static const char* VolumesArrayName() { return "Volumes"; }
    static const char* ObjectValidityArrayName() { return "ObjectValidity"; }
    static const char* ObjectAreasArrayName() { return "ObjectAreas"; }
    static const char* ObjectVolumesArrayName() { return "ObjectVolumes"; }
    static const char* ObjectCentroidsArrayName() { return "ObjectCentroids"; }

protected:
    ConnectedSurfacePropertiesFilter();
    ~ConnectedSurfacePropertiesFilter() override = default;

private:
    bool m_SkipObjectIdentification{false};
    std::string m_ObjectIdsArrayName{"ObjectIds"};
    bool m_UsedSuppliedObjectIds{false};
    int m_NumberOfObjects{0};
    bool m_AllValid{false};
    double m_TotalArea{0.0};
    double m_TotalVolume{0.0};
    std::string m_LastError{};

    LongLongArray::Pointer m_ObjectIds{};
    DoubleArray::Pointer m_Areas{};
    DoubleArray::Pointer m_Volumes{};
    IntArray::Pointer m_ObjectValidity{};
    DoubleArray::Pointer m_ObjectAreas{};
    DoubleArray::Pointer m_ObjectVolumes{};
    DoubleArray::Pointer m_ObjectCentroids{};
};

IGAME_NAMESPACE_END
