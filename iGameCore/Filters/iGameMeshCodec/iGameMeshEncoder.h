#ifndef MeshEncoder_h
#define MeshEncoder_h

#include "iGameFilter.h"
#include "iGameMacro.h"
#include "iGameSurfaceMesh.h"

#include "iGameMeshCodecParamSet.h"
#include "iGameMeshOptEncoder.h"
#include "iGameThreadPool.h"
#include "meshoptimizer.h"
#include <numeric>
#include <string>

IGAME_NAMESPACE_BEGIN
class MeshEncoder : public Filter {
public:
    I_OBJECT(MeshEncoder);
    static Pointer New() { return new MeshEncoder; }

    MeshEncoder() { SetNumberOfInputs(1); };

    iGame::QuantMode PointQuantMode = iGame::QuantMode::FP16;
    int PointQuantizedBits = 16;
    iGame::QuantMode AttrbQuantMode = iGame::QuantMode::Float;
    int AttrbQuantizedBits = 16;

    bool Execute() override {
        if (this->GetNumberOfInputs() == 0) { return false; }
        this->m_DataObj = this->GetInput(0);
        if (!m_SaveFilePath.empty() && !this->OpenStream(m_SaveFilePath)) { return false; }

        //StructuredMesh::Pointer mesh = DynamicCast<StructuredMesh>(this->m_DataObj);
        //auto fs = mesh->GetFaces();
        //auto vs = mesh->GetVolumes();
        //auto cs = mesh->GetCells();
        //
        //auto buffer = vs->GetCellIdArray();
        //auto offset = vs->GetOffset();

        //return true;
        // ---------------------------------------------------------

        // TODO: 临时添加的，怕影响MeshOptParameters，定义在MeshOptParameters下面
        ParamInformation inputParams;
        inputParams.PointQuantMode = this->PointQuantMode;
        inputParams.PointQuantizedBits = this->PointQuantizedBits;
        inputParams.AttrbQuantMode = this->AttrbQuantMode;
        inputParams.AttrbQuantizedBits = this->AttrbQuantizedBits;
        // 编码
        MeshOptParameters params;
        MeshOptEncoder encoder(this->m_BytestreamFile, this->m_DataObj, params, inputParams, m_IsDebugMode);
        encoder.SetUpdateProgress(&MeshEncoder::UpdateProgress, this);

        encoder.Execute();

        // ---------------------------------------------------------
        this->closeStream();
        return true;
    }

    void SetSaveFilePath(const std::string& path) { m_SaveFilePath = path; }

    void SetDebugModeOn() { m_IsDebugMode = true; }

private:
    std::ofstream m_BytestreamFile;
    DataObject::Pointer m_DataObj;
    std::string m_SaveFilePath;
    bool m_IsDebugMode = false;

    bool OpenStream(std::string path) {
        this->m_BytestreamFile.open(path, std::ios::binary);
        if (!this->m_BytestreamFile.is_open()) { return false; }
        return true;
    }

    void closeStream() {
        //std::cout << "Total bitstream size " << m_BytestreamFile.tellp() << " B\n";
        m_BytestreamFile.close();
    }
};

IGAME_NAMESPACE_END
#endif