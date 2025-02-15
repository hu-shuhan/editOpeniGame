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

    QuantMode m_PointQuantMode = QuantMode::Float;
    int m_PointQuantizedBits = 16;
    QuantMode m_AttrbQuantMode = QuantMode::Float;
    int m_AttrbQuantizedBits = 16;

	ErrorStaMode m_errorStaMode = ErrorStaMode::None;
	std::vector<std::string>* m_errorStaResult = nullptr;
	CompactnessMode m_cpStaMode = CompactnessMode::None;
	std::vector<std::string>* m_cpStaResult = nullptr;

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
        inputParams.PointQuantMode = this->m_PointQuantMode;
        inputParams.PointQuantizedBits = this->m_PointQuantizedBits;
        inputParams.AttrbQuantMode = this->m_AttrbQuantMode;
        inputParams.AttrbQuantizedBits = this->m_AttrbQuantizedBits;
		inputParams.errorStaMode = this->m_errorStaMode;
		inputParams.errorStaResult = this->m_errorStaResult;
		inputParams.cpStaMode = this->m_cpStaMode;
		inputParams.cpStaResult = this->m_cpStaResult;
        // 编码
        MeshOptParameters params;
        MeshOptEncoder encoder(this->m_BytestreamFile, this->m_DataObj, params, inputParams);
        //encoder.SetUpdateProgress(&MeshEncoder::UpdateProgress, this);

        encoder.Execute();

        // ---------------------------------------------------------
        this->closeStream();
        return true;
    }

    void SetSaveFilePath(const std::string& path) { m_SaveFilePath = path; }

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