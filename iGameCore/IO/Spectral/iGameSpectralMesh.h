#pragma once
#include "iGameSpectralData.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN
// Retains the original expansions as well as the sampled display mesh. The
// usual UnstructuredMesh rendering, scalar colouring and clipping paths apply.
class SpectralMesh : public UnstructuredMesh {
public:
    I_OBJECT(SpectralMesh);
    static Pointer New() { return new SpectralMesh; }
    const Spectral::Data& GetSpectralData() const { return m_SpectralData; }
    void SetSpectralData(Spectral::Data data) { m_SpectralData=std::move(data); }
protected:
    SpectralMesh() = default;
    ~SpectralMesh() override = default;
    Spectral::Data m_SpectralData;
};
IGAME_NAMESPACE_END
