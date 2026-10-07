#pragma once
#include "iGameFileReader.h"
#include "iGameSpectralData.h"

IGAME_NAMESPACE_BEGIN
class SpectralReaderCPU : public FileReader {
public:
    I_OBJECT(SpectralReaderCPU);
    static Pointer New() { return new SpectralReaderCPU; }
    // No mode selection: DAT and Nektar XML/FLD are selected by file format.
    bool Execute() override;
    // Zero means automatic; otherwise the number of subdivisions per direction.
    void SetSamplingSubdivisions(int subdivisions) { m_Subdivisions=subdivisions; }
    int GetSamplingSubdivisions() const { return m_Subdivisions; }
    static bool IsNektarFile(const std::string& path) { return Spectral::IsNektarFile(path); }
protected:
    SpectralReaderCPU() = default;
    ~SpectralReaderCPU() override = default;
    bool Parsing() override;
    bool CreateDataObject() override;
    Spectral::Data m_SpectralData;
    int m_Subdivisions=0;
};
IGAME_NAMESPACE_END
