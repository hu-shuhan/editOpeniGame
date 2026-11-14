//
// Created by m_ky on 2024/9/18.
//

/**
 * @class   iGameODBReader
 * @brief   iGameODBReader's brief
 */
#if defined(AbqSDK_ENABLE)
#include "iGameODBReader.h"
#include "Log/iGameLogger.h"

#include <VTK/iGameVTKAbstractReader.h>

#include <utility>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <set>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

IGAME_NAMESPACE_BEGIN

namespace {
#if defined(_WIN32)
std::mutex s_abaqusRuntimeMutex;
bool s_abaqusRuntimeLoaded = false;
std::string s_abaqusRuntimeError;
std::vector<HMODULE> s_abaqusRuntimeModules;

std::filesystem::path EnvironmentPath(const char* name) {
    char* value = nullptr;
    size_t valueLength = 0;
    if (_dupenv_s(&value, &valueLength, name) != 0 || value == nullptr) {
        return {};
    }
    std::filesystem::path result = std::filesystem::u8path(value);
    free(value);
    return result;
}

void AddRuntimeCandidate(std::vector<std::filesystem::path>& candidates,
                         const std::filesystem::path& path) {
    if (path.empty()) return;
    candidates.push_back(path);
    candidates.push_back(path / "win_b64" / "code" / "bin");
    candidates.push_back(path / "code" / "bin");
    candidates.push_back(path / "bin");
}

std::vector<std::filesystem::path> AbaqusRuntimeCandidates() {
    std::vector<std::filesystem::path> candidates;

    wchar_t executablePath[MAX_PATH] = {};
    const DWORD pathLength = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
    if (pathLength > 0 && pathLength < MAX_PATH) {
        candidates.emplace_back(std::filesystem::path(executablePath).parent_path());
    }

    AddRuntimeCandidate(candidates, EnvironmentPath("IGAME_ABAQUS_RUNTIME_DIR"));
    AddRuntimeCandidate(candidates, EnvironmentPath("ABAQUS_HOME"));
    AddRuntimeCandidate(candidates, EnvironmentPath("ABAQUS_DIR"));
    AddRuntimeCandidate(candidates, EnvironmentPath("SIMULIA_HOME"));

    // The ODB C++ ABI is version-specific. Only auto-discover the version
    // against which this executable was built; explicit environment variables
    // above remain available for non-standard installation layouts.
    for (const auto* drive : {L"C:", L"D:", L"E:"}) {
        candidates.push_back(std::filesystem::path(drive) / "SIMULIA" /
                "EstProducts" / IGAME_ABAQUS_VERSION /
                "win_b64" / "code" / "bin");
    }

    std::set<std::filesystem::path> uniqueCandidates;
    std::vector<std::filesystem::path> result;
    for (auto& candidate : candidates) {
        candidate = candidate.lexically_normal();
        if (uniqueCandidates.insert(candidate).second) result.push_back(candidate);
    }
    return result;
}

bool LoadAbaqusRuntimeFrom(const std::filesystem::path& runtimeDirectory) {
    const auto odbApiPath = runtimeDirectory / "ABQSMAOdbApi.dll";
    std::error_code error;
    if (!std::filesystem::is_regular_file(odbApiPath, error)) return false;

    // LOAD_WITH_ALTERED_SEARCH_PATH makes dependencies resolve beside the
    // user's ABQSMAOdbApi.dll without permanently modifying process PATH.
    HMODULE module = LoadLibraryExW(odbApiPath.c_str(), nullptr,
                                    LOAD_WITH_ALTERED_SEARCH_PATH);
    if (module == nullptr) {
        s_abaqusRuntimeError = "Found Abaqus at " + runtimeDirectory.string() +
                ", but its ODB runtime or one of its dependencies could not be loaded " +
                "(Windows error " + std::to_string(GetLastError()) + ").";
        return false;
    }
    s_abaqusRuntimeModules.push_back(module);
    return true;
}
#endif
} // namespace

bool ODBReader::IsRuntimeAvailable(std::string* errorMessage) {
#if defined(_WIN32)
    std::lock_guard<std::mutex> lock(s_abaqusRuntimeMutex);
    if (s_abaqusRuntimeLoaded) return true;

    // First respect the normal Windows loader search path (application folder
    // and PATH). This supports users who launch from an Abaqus command shell.
    if (HMODULE module = LoadLibraryW(L"ABQSMAOdbApi.dll")) {
        s_abaqusRuntimeModules.push_back(module);
        s_abaqusRuntimeLoaded = true;
        return true;
    }

    for (const auto& candidate : AbaqusRuntimeCandidates()) {
        if (LoadAbaqusRuntimeFrom(candidate)) {
            s_abaqusRuntimeLoaded = true;
            s_abaqusRuntimeError.clear();
            return true;
        }
    }

    if (s_abaqusRuntimeError.empty()) {
        s_abaqusRuntimeError =
                "A compatible Abaqus ODB runtime was not found. Install the Abaqus "
                "version used by this build, launch iGameVis from an Abaqus command "
                "shell, or set IGAME_ABAQUS_RUNTIME_DIR to win_b64/code/bin.";
    }
    if (errorMessage != nullptr) *errorMessage = s_abaqusRuntimeError;
    return false;
#else
    (void)errorMessage;
    return true;
#endif
}

/* Used for Update Progress : [0, 1]*/
float s_current_progress = 0.f;
float s_progress_coefficient = 1.f;
float AddODBProgress(float add_value = 0.f){
    s_current_progress += s_progress_coefficient * add_value;
    if(s_current_progress > 1.f) s_current_progress = 1.0f;
    return s_current_progress;
}
void SetProgressCoefficient(float _C){
    s_progress_coefficient = _C;
}
/* The progress accumulator is a translation-unit global, so it has to be
 * cleared at the start of every top-level read.  Without this, a second read
 * (e.g. loading another model or reading a time frame) starts at 100% and the
 * bar never moves again. */
void ResetODBProgress(float coefficient = 1.f){
    s_current_progress = 0.f;
    s_progress_coefficient = coefficient;
}

/* Define Internal Adaptor Class. */
class AttributeParserHelper{
public:
    std::string stepName;
    int frameIdx{0};
    AttributeSet::Pointer m_AttributeSet{nullptr};
    AttributeSet::Pointer GetResult() { return m_AttributeSet;};

public:

    static std::string ABAQUS_VTK_FIELD_OUTPUTS_MAP(const odb_FieldOutput& fieldOutput){
        // map abaqus data type to vtk type Scalars, Vectors, Tensors
        if (fieldOutput.type() == odb_Enum::odb_DataTypeEnum::SCALAR)
        {
            return "Scalars";
        }
        else if (fieldOutput.type() == odb_Enum::odb_DataTypeEnum::VECTOR)
        {
            return "Vectors";
        }
        else if (fieldOutput.type() == odb_Enum::odb_DataTypeEnum::TENSOR_3D_FULL ||
                 fieldOutput.type() == odb_Enum::odb_DataTypeEnum::TENSOR_3D_SURFACE ||
                 fieldOutput.type() == odb_Enum::odb_DataTypeEnum::TENSOR_3D_PLANAR ||
                 fieldOutput.type() == odb_Enum::odb_DataTypeEnum::TENSOR_2D_SURFACE ||
                 fieldOutput.type() == odb_Enum::odb_DataTypeEnum::TENSOR_2D_PLANAR)
        {
            return "Tensors";
        }

        IGAME_CORE_WARN("VTK field data type not understood.");
        return "";
    }


    static void ReadSortedCellData(const odb_SequenceFieldBulkData& blkDataBlock,
                                      std::map<int, int>& dataMap,
                                   FloatArray::Pointer resDataArray,
                                   float cur_Progress_val,
                                   ODBReader* reader
                                   )
    {
        if (blkDataBlock.size() == 0) return;
        cur_Progress_val /= blkDataBlock.size();
        // Element stride inside the target array: components * integration points.
        const int targetDimension = static_cast<int>(resDataArray->GetDimension());
        for (int i = 0; i < blkDataBlock.size(); i++)
        {
            const auto& blk = blkDataBlock[i];
            const int numValues = blk.length();
            const int numComp = blk.width();
            const int nElems = blk.numberOfElements();
            if(numComp <= 0 || numValues <= 0) continue;
            // Number of integration point values stored per element.
            const int numIP = nElems > 0 ? numValues / nElems : 1;
            if(numIP <= 0) continue;
            const float* data = blk.data();
            if(data == nullptr) continue;
            const int* elementLabels = blk.elementLabels();
            if(elementLabels == nullptr) continue;
            // elementLabels() has one entry per *value row*, i.e. the element
            // label is repeated once per integration point (verified against
            // ABQSMAOdbApi: a C3D10 block reports length=4*nElems and labels
            // 1,1,1,1,2,2,...).  The previous loop treated it as one label per
            // element, so every element received another element's values.
            for (int row = 0; row < numValues; row++)
            {
                auto mapIt = dataMap.find(elementLabels[row]);
                // Field blocks may reference labels that the mesh mapping does
                // not know (e.g. element sets of another instance).  Skipping is
                // the only safe option -- looking them up with operator[] would
                // silently pile every one of those values onto element 0.
                if (mapIt == dataMap.end()) continue;
                const INT64 cellLabelVtk = mapIt->second;
                const int ip = numIP > 0 ? row % numIP : 0;
                for (int comp = 0; comp < numComp; comp++)
                {
                    const int localIndex = ip * numComp + comp;
                    if (localIndex >= targetDimension) continue;
                    const INT64 idx = cellLabelVtk * targetDimension + localIndex;
                    const float val = data[static_cast<INT64>(row) * numComp + comp];
                    resDataArray->SetValue(idx, val);
                }
            }
            reader->UpdateProgress(cur_Progress_val);
        }
    }

    static void ReadSortedPointData(const odb_SequenceFieldBulkData& blkDataBlock,
                             std::map<int, int>& dataMap,
                                FloatArray::Pointer resDataArray,
                                float cur_Progress_val,
                                ODBReader* reader
                                ){
        if (blkDataBlock.size() == 0) return;
        cur_Progress_val /= blkDataBlock.size();
        const int targetDimension = static_cast<int>(resDataArray->GetDimension());
        for (int i = 0; i < blkDataBlock.size(); i++)
        {
            const auto& blk = blkDataBlock[i];
            const int numNodes = blk.length();
            const int numComp = blk.width();
            if(numNodes <= 0 || numComp <= 0) continue;
            const float* data = blk.data();
            if(data == nullptr) continue;
            const int* nodeLabels = blk.nodeLabels();
            if(nodeLabels == nullptr) continue;
            for (int j = 0; j < numNodes; j++)
            {
                auto mapIt = dataMap.find(nodeLabels[j]);
                if (mapIt == dataMap.end()) continue;
                const INT64 nodeLabelVtk = mapIt->second;
                for (int comp = 0; comp < numComp; comp++)
                {
                    if (comp >= targetDimension) continue;
                    const INT64 idx = nodeLabelVtk * targetDimension + comp;
                    const float val = data[j * numComp + comp];
                    resDataArray->SetValue(idx, val);
                }
            }
            reader->UpdateProgress(cur_Progress_val);
        }
    }

public:
    AttributeParserHelper(std::string  _stepName, int _frameIdx) : stepName(std::move(_stepName)), frameIdx(_frameIdx){
        m_AttributeSet = AttributeSet::New();
    }
    ~AttributeParserHelper() = default;
};

/* Public API: */
DataObject::Pointer ODBReader::ReadOdbRawMesh(const std::string &filePath) {
    if (!IsRuntimeAvailable()) return nullptr;
    m_NeedRequestMap = m_NeedRequestInstance = true;
    m_NeedRequestStep = false;
    SetFilePath(filePath);
    ResetODBProgress();
    Execute();
    UpdateProgress(1.f);
    return this->GetOutput();
}
DataObject::Pointer ODBReader::ReadOdbFirstFrameMesh(const std::string &filePath) {
    return ReadOdbMesh(filePath, 0);
}
DataObject::Pointer ODBReader::ReadOdbFirstFrameMesh(const std::string &filePath, const std::string &stepName) {
    return ReadOdbMesh(filePath, stepName, 0);
}

DataObject::Pointer ODBReader::ReadOdbMesh(const std::string &filePath, int frameIdx) {
    if (!IsRuntimeAvailable()) return nullptr;

    m_NeedRequestMap = m_NeedRequestInstance = m_NeedRequestStep = true;
    SetFilePath(filePath);
    ResetODBProgress(0.5f);
    Execute();
    m_NeedRequestMap = m_NeedRequestInstance = false;

    auto outputObj = this->GetOutput();
    if (outputObj == nullptr) {
        IGAME_CORE_ERROR("Fail to read ODB mesh: {}", filePath);
        return nullptr;
    }
    if (m_StepFrameMap.empty()) {
        // No step/frame to attach; the raw mesh is still valid.
        UpdateProgress(1.f);
        return outputObj;
    }
    const std::string stepName = m_StepFrameMap.begin()->first;
    const int resolvedFrame = ResolveFrameIndex(stepName, frameIdx);
    if (ExecuteWithFieldData(stepName, resolvedFrame)) {
        if (m_Attribute_helper != nullptr) {
            outputObj->SetAttributeSet(m_Attribute_helper->GetResult());
        }
        if (auto timeFrames = outputObj->GetTimeFrames()) {
            timeFrames->SetInitialFrameIndex(resolvedFrame);
        }
    }
    UpdateProgress(1.f);
    return outputObj;
}

DataObject::Pointer ODBReader::ReadOdbMesh(const std::string &filePath, const std::string &stepName, int frameIdx) {
    if (!IsRuntimeAvailable()) return nullptr;

    m_NeedRequestMap = m_NeedRequestInstance = true;
    m_NeedRequestStep = true;
    SetFilePath(filePath);
    ResetODBProgress(0.5f);
    Execute();
    m_NeedRequestMap = m_NeedRequestInstance = false;

    auto outputObj = this->GetOutput();
    if (outputObj == nullptr) {
        IGAME_CORE_ERROR("Fail to read ODB mesh: {}", filePath);
        return nullptr;
    }
    std::string resolvedStep = stepName;
    if (m_StepFrameMap.find(resolvedStep) == m_StepFrameMap.end() &&
        !m_StepFrameMap.empty()) {
        IGAME_CORE_WARN("Step '{}' not found in the ODB, using '{}' instead.",
                        stepName, m_StepFrameMap.begin()->first);
        resolvedStep = m_StepFrameMap.begin()->first;
    }
    const int resolvedFrame = ResolveFrameIndex(resolvedStep, frameIdx);
    if (ExecuteWithFieldData(resolvedStep, resolvedFrame)) {
        if (m_Attribute_helper != nullptr) {
            outputObj->SetAttributeSet(m_Attribute_helper->GetResult());
        }
        if (auto timeFrames = outputObj->GetTimeFrames()) {
            timeFrames->SetInitialFrameIndex(resolvedFrame);
        }
    }
    UpdateProgress(1.f);
    return outputObj;
}

int ODBReader::ResolveFrameIndex(const std::string &stepName, int frameIdx) const {
    auto it = m_StepFrameMap.find(stepName);
    if (it == m_StepFrameMap.end() || it->second <= 0) return 0;
    const int frameCount = it->second;
    if (frameIdx < 0 || frameIdx >= frameCount) return frameCount - 1;
    return frameIdx;
}

AttributeSet::Pointer ODBReader::ReadOdbFieldData(const std::string &filePath, int frame_idx) {
    if (!IsRuntimeAvailable()) return nullptr;
    SetFilePath(filePath);
    m_NeedRequestInstance = m_NeedRequestStep = true;
    m_NeedRequestMap = true;
    ResetODBProgress();
    if(ExecuteWithFieldData(frame_idx)) return m_Attribute_helper->GetResult();
    UpdateProgress(1.f);
    return nullptr;
}
AttributeSet::Pointer ODBReader::ReadOdbFieldData(const std::string &filePath, const std::string& stepName, int frame_idx) {
    if (!IsRuntimeAvailable()) return nullptr;
    SetFilePath(filePath);
    m_NeedRequestInstance = true;
    m_NeedRequestStep = false;
    m_NeedRequestMap = true;
    ResetODBProgress();
    if(ExecuteWithFieldData(stepName, frame_idx)) return m_Attribute_helper->GetResult();
    UpdateProgress(1.f);
    return nullptr;
}

/* Protected API: */
bool ODBReader::ExecuteWithFieldData(int frameIdx) {
    odb_initializeAPI();
    try {
        if(!OpenODB()){
            IGAME_CORE_ERROR("Fail to Open ODB dataBase");
            return false;
        }
        ExtractHeader();
        ConstructMap();
        if (m_StepFrameMap.empty()) {
            IGAME_CORE_ERROR("ODB contains no analysis step, so no field data can be read.");
            odb_finalizeAPI();
            return false;
        }
        const std::string stepName = m_StepFrameMap.begin()->first;
        delete m_Attribute_helper;
        m_Attribute_helper = new AttributeParserHelper(stepName, frameIdx);
        ReadAttributes();
        IGAME_CORE_DEBUG("ReadAttributes end");
    }
    catch (odb_BaseException& exc) {
        odb_finalizeAPI();
        IGAME_CORE_ERROR("Abaqus error message: {}", exc.UserReport().CStr());
        return false;
    }catch (...) {
        odb_finalizeAPI();
        IGAME_CORE_ERROR("Unknown Exception.");
        return false;
    }
    UpdateProgress(1.f);
    odb_finalizeAPI();
    return true;
}

bool ODBReader::ExecuteWithFieldData(const std::string& stepName, int frameIdx) {
    odb_initializeAPI();
    try {
        if(!OpenODB()){
            IGAME_CORE_ERROR("Fail to Open ODB dataBase");
            return false;
        }
        IGAME_CORE_DEBUG("Open end");
        ExtractAllInstance();
        ExtractAllStep();
        IGAME_CORE_DEBUG("ExtractHeader end");
        ConstructMap();
        IGAME_CORE_DEBUG("ConstructMap end");
        delete m_Attribute_helper;
        m_Attribute_helper = new AttributeParserHelper(stepName, frameIdx);
        ReadAttributes();
        IGAME_CORE_DEBUG("ReadAttributes end");

    }
    catch (odb_BaseException& exc) {
        odb_finalizeAPI();
        IGAME_CORE_ERROR("Abaqus error message: {}", exc.UserReport().CStr());
        return false;
    }catch (...) {
        odb_finalizeAPI();
        IGAME_CORE_ERROR("Unknown Exception.");
        return false;
    }
    UpdateProgress(1.f);
    odb_finalizeAPI();
    return true;
}

bool iGame::ODBReader::Execute() {
    IGAME_CORE_DEBUG("Init Odb start");
    odb_initializeAPI();
    UpdateProgress(AddODBProgress(0.05f));
    IGAME_CORE_DEBUG("Init Odb end");
    try {
        if(!OpenODB()){
            IGAME_CORE_ERROR("Fail to Open ODB dataBase");
            return false;
        }
        ExtractHeader();
        UpdateProgress(AddODBProgress(0.05f));
        IGAME_CORE_DEBUG("ExtractHeader end");
        ConstructMap();
        UpdateProgress(AddODBProgress(0.05f));
        IGAME_CORE_DEBUG("ConstructMap end");
        ReadCoordinates();
        IGAME_CORE_DEBUG("ReadCoordinates end");
        CreateDataObject();
    }
    catch (odb_BaseException& exc) {
        IGAME_CORE_ERROR("Abaqus error message: {}", exc.UserReport().CStr());
    }catch (...) {
        IGAME_CORE_ERROR("Unknown Exception.");
    }
    UpdateProgress(1.f);
    odb_finalizeAPI();

    SetOutput(0, m_Output);
    return m_Output != nullptr;
}

void iGame::ODBReader::SetFilePath(const std::string &filePath) {
    this->m_FilePath = filePath;
    this->m_FileName =
            filePath.substr(filePath.find_last_of("/\\") + 1, filePath.size());
    this->m_FileDir = filePath.substr(0, filePath.find_last_of("/\\") + 1);
}

bool ODBReader::OpenODB() {
    odb_String odbFile = odb_String(m_FilePath.c_str());
    return m_ODB = &openOdb(odbFile);
}
bool ODBReader::ExtractAllInstance() {
    if(!m_NeedRequestInstance) return true;
    odb_InstanceRepositoryIT instIter(m_ODB->rootAssembly().instances());
    m_Instance_names.clear();
    for (instIter.first(); !instIter.isDone(); instIter.next())
    {
//        igDebug("Instance Name : {}", instIter.currentKey().CStr());
        // Copy the name: currentKey() refers to storage owned by the ODB, and
        // the previous const char* cache made the maps depend on that pointer
        // staying alive (and compared instance names by address, not content).
        m_Instance_names.emplace_back(instIter.currentKey().CStr());
    }
    igDebug("Instance Num : {}", m_Instance_names.size());
    return true;
}

bool ODBReader::ExtractAllStep() {
    if(!m_NeedRequestStep) return true;
    odb_StepRepositoryIT stepIter(m_ODB->steps());
    m_StepFrameMap.clear();
    for (stepIter.first(); !stepIter.isDone(); stepIter.next())
    {
        std::string stepName = std::string(stepIter.currentKey().CStr());
        IGAME_CORE_DEBUG("Step Name : {}", stepName.c_str());
        m_StepFrameMap[stepName] = stepIter.currentValue().frames().size();
    }
    return true;
}

bool ODBReader::ExtractHeader() {
    if(m_ODB == nullptr) return false;
    // write instances
    ExtractAllInstance();
    // write steps and frames
    ExtractAllStep();
    return true;
}

bool ODBReader::ConstructMap() {
    if(!m_NeedRequestMap) return true;
    // we need to map the local label from abaqus to global index for paraview.
    // global index used in paraview for node and cell.
    int nodeIndex = 0;
    int cellIndex = 0;
    m_nodesNum = m_cellsNum = 0;
    m_NodesMap.clear();
    m_CellsMap.clear();
    for (const auto& inst_name : m_Instance_names)
    {
        m_NodesMap[inst_name] = std::map<int, int>();
        m_CellsMap[inst_name] = std::map<int, int>();
        auto rootAssy = m_ODB->rootAssembly();
        auto inst = rootAssy.instances()[inst_name.c_str()];
        const auto& node_list = inst.nodes();
        const auto& cell_list = inst.elements();
        m_nodesNum += node_list.size();
        m_cellsNum += cell_list.size();
        for (int i = 0; i < node_list.size(); i++)
        {
            m_NodesMap[inst_name][inst.nodes(i).label()] = nodeIndex;
            nodeIndex++;
        }
        for (int i = 0; i < cell_list.size(); i++)
        {
            m_CellsMap[inst_name][inst.elements(i).label()] = cellIndex;
            cellIndex++;
        }
    }

    return true;
}

bool ODBReader::ReadCoordinates() {
    auto rootAssy = m_ODB->rootAssembly();
    Points::Pointer dataSetPoints = m_Data.GetPoints();
    int nodesNumCell = 0;
    int offset = 0;
    IntArray::Pointer cellConnectivity = IntArray::New();
    IntArray::Pointer cellOffsets = IntArray::New();
    //  Note that it need to add a zero index.
    cellOffsets->AddValue(offset);
    IntArray::Pointer cellTypes = IntArray::New();


    for (const auto& inst_name : m_Instance_names)
    {
        auto& current_nodeMap = m_NodesMap[inst_name];
        const auto& inst = rootAssy.instances()[inst_name.c_str()];
        // write node coordinates
        dataSetPoints->Reserve(dataSetPoints->GetNumberOfPoints() + inst.nodes().size());
        for (int i = 0; i < inst.nodes().size(); i ++)
        {
            dataSetPoints->AddPoint(inst.nodes(i).coordinates());
        }

        UpdateProgress(AddODBProgress(0.3f));

        // write cell connectivity, offset, and type
        int elem_size = inst.elements().size();
        cellConnectivity->Reserve(cellConnectivity->GetArrayTypedSize() + elem_size);
        cellOffsets->Reserve(cellOffsets->GetArrayTypedSize() + elem_size);
        cellTypes->Reserve(cellTypes->GetArrayTypedSize() + elem_size);

        float left_progress = 1.f - s_current_progress / s_progress_coefficient;
        float left_elem_val = left_progress / elem_size;
        for (int i = 0; i < elem_size; i++)
        {
//            auto t1 = std::chrono::steady_clock::now();
            const auto& cell = inst.elements(i);
//            auto t2 = std::chrono::steady_clock::now();
            // connectivity
            const int* conn = cell.connectivity(nodesNumCell);
//            auto t3 = std::chrono::steady_clock::now();
            for (int j = 0; j < nodesNumCell; j++)
            {
                cellConnectivity->AddValue(current_nodeMap[conn[j]]);
            }
//            auto t4 = std::chrono::steady_clock::now();
            // offset
            offset += nodesNumCell;
            cellOffsets->AddValue(offset);
            // CellType
            cellTypes->AddValue(ABAQUS_VTK_CELL_MAP(cell.type().cStr()));
            UpdateProgress(AddODBProgress(left_elem_val));
//            auto t5 = std::chrono::steady_clock::now();
//            std::cout << "===================\n";
//            std::cout << "cost 1 : " << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() << '\n';
//            std::cout << "cost 2 : " << std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count() << '\n';
//            std::cout << "cost 3 : " << std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count() << '\n';
//            std::cout << "cost 4 : " << std::chrono::duration_cast<std::chrono::microseconds>(t5 - t4).count() << '\n';
        }
    }
    if (cellTypes)
    {
        VTKAbstractReader::TransferVtkCellToiGameCell(m_Output, cellOffsets, cellConnectivity, cellTypes);
        m_Output->GetBoundingBox();
    }

    return true;
}

bool ODBReader::CreateDataObject() {
        switch (m_Output->GetDataObjectType())
        {
            case IG_UNSTRUCTURED_MESH:
            {
                DynamicCast<UnstructuredMesh>(m_Output)->SetPoints(m_Data.GetPoints());
                break;
                //            DynamicCast<UnstructuredMesh>(m_Output)->SetAttributeSet(m_Data.Data);
            }
            case IG_SURFACE_MESH:{
                DynamicCast<SurfaceMesh>(m_Output)->SetPoints(m_Data.GetPoints());
                break;
//            DynamicCast<SurfaceMesh>(m_Output)->SetAttributeSet(m_Data.Data);
            }
        }

        // Raw-mesh reads intentionally skip step extraction. Only attach time
        // frames when a step was requested and the map is populated.
        if (!m_StepFrameMap.empty()) {
            auto firstStep = m_StepFrameMap.begin();
            for(int i = 0; i < firstStep->second; i ++){
                double time_val = m_ODB->steps().constGet(firstStep->first.c_str()).frames()[i].frameValue();
                StringArray::Pointer array = StringArray::New();
                array->AddElement(m_FilePath);
                m_Output->GetTimeFrames()->AddTimeStep(time_val, array, StreamingType::SingleFieldAttributes);
            }
        }
        return true;
}


bool ODBReader::ReadAttributes() {
    const char* stepName = m_Attribute_helper->stepName.c_str();
    int frameIdx = m_Attribute_helper->frameIdx;
    const auto& step = m_ODB->steps().constGet(stepName);
    const int frameCount = step.frames().size();
    if (frameCount <= 0) {
        IGAME_CORE_ERROR("Step '{}' of the ODB has no frame.", stepName);
        return false;
    }
    // Clamp rather than indexing out of range when the caller asks for a frame
    // that this step does not have (e.g. a time frame index from another step).
    if (frameIdx < 0) frameIdx = frameCount - 1;
    if (frameIdx >= frameCount) frameIdx = frameCount - 1;
    m_Attribute_helper->frameIdx = frameIdx;
    IGAME_CORE_DEBUG("Reading ODB step '{}' frame {}/{}", stepName, frameIdx, frameCount);
    const odb_Frame& frame = step.frames()[frameIdx];
    const auto& fldOutputs = frame.fieldOutputs();

//    for(int i = 0; i < names.size(); i ++){
//        std::cout << names.constGet(i).cStr()  <<'\t';
//        auto fldout = fldOutputs.constGet(names.constGet(i));
//        std::string vtk_type = AttributeParserHelper::ABAQUS_VTK_FIELD_OUTPUTS_MAP(fldout);
//        std::cout << vtk_type<< '\n';
//    }
//    std::cout << fldOutputs.size() << ' ' << fldOutputs.
//    const auto& names = fldOutputs.getFieldOutputNames();
//    for(int field_idx = 0; field_idx < names.size(); ++ field_idx)

    /* Read Field data */
    odb_FieldOutputRepositoryIT fieldIter(fldOutputs);
    UpdateProgress(AddODBProgress(0.05f));
    float left_progress = 1.f - s_current_progress / s_progress_coefficient;
    float left_attr_val = fldOutputs.size() > 0 ? left_progress / fldOutputs.size() : 0.f;
    for (fieldIter.first(); !fieldIter.isDone(); fieldIter.next())
    {
//        const auto& fldOutput = fldOutputs.constGet(names.constGet(field_idx));
        const auto& fldOutput = fieldIter.currentValue();
//        std::string vtk_type = AttributeParserHelper::ABAQUS_VTK_FIELD_OUTPUTS_MAP(fldOutput);

        // assuming location only has size of 1
        // TODO: may need to consider cases with locations more than 1
        if (fldOutput.locations().size() == 0) continue;
        auto abqPos = fldOutput.locations().constGet(0).position();
        // if field output contains sectionPoint data, we need to generate separate dataset

        // purpose of this loop is to iterate over field ouput from all the instance
        // and check to see if we can find any section points
        std::vector<odb_SectionPoint> sectionPoints;
        int maxNumIntegrationPoints = 1;
        for (const auto& inst_name : m_Instance_names)
        {
            auto& inst = m_ODB->rootAssembly().instances().constGet(inst_name.c_str());
            //get data block of the instance
            //filter by position
            //note that subset.bulkDataBlocks may have more than one
            //because of different element type or sectionPoint in the same instance
            auto subset = fldOutput.getSubset(inst).getSubset(abqPos);
            for (int i = 0; i < subset.bulkDataBlocks().size(); i++)
            {
                const auto& blk = subset.bulkDataBlocks()[i];
                const auto& sectionPoint = blk.sectionPoint();
                if (sectionPoint.number() != -1)
                {
                    sectionPoints.push_back(sectionPoint);
                }
                // number of integration points is calculated by dividing bulkdata length by number of elements within the bulk
                // the purpose of this is that we need to track the maximum number of integration point among all instances
                // and use max to fill in the VTK data entry
                if (blk.numberOfElements() != 0 && blk.width() != 0)
                {
                    int num_ip = blk.length() / blk.numberOfElements();
                    if (num_ip > maxNumIntegrationPoints)
                    {
                        maxNumIntegrationPoints = num_ip;
                    }
                }
            }
        }

        if (abqPos == odb_Enum::odb_ResultPositionEnum::NODAL ||
            abqPos == odb_Enum::odb_ResultPositionEnum::ELEMENT_NODAL)
        {
            // ELEMENT_NODAL shares the node-labelled bulk blocks of NODAL; the
            // values are accumulated per node, which is what a point attribute
            // needs.  Ignoring this position (as before) silently produced an
            // empty attribute set for odb files written with "*EL FILE, POSITION=
            // ELEMENT NODAL".
            // Point attributes always hold one value per node, so the number of
            // integration points is 1 regardless of the element blocks.
            ReadDataArrayWithSectionPoints(sectionPoints,
                                           fldOutput,
                                           abqPos,
                                           fldOutput.name(),
                                           1,
                                           PointData,
                                           left_attr_val
                                           );
        }
        else if(abqPos == odb_Enum::odb_ResultPositionEnum::INTEGRATION_POINT){
            ReadDataArrayWithSectionPoints(sectionPoints,
                                           fldOutput,
                                           odb_Enum::odb_ResultPositionEnum::CENTROID,
                                           fldOutput.name() + "_Centroid",
                                           1,
                                           CellData,
                                           left_attr_val * 0.5
                                           );
            ReadDataArrayWithSectionPoints(sectionPoints,
                                           fldOutput,
                                           abqPos,
                                           fldOutput.name() + "_IntegrationPoints",
                                           maxNumIntegrationPoints,
                                           CellData,
                                           left_attr_val * 0.5
                                           );
        }
        else if(abqPos == odb_Enum::odb_ResultPositionEnum::CENTROID ||
                abqPos == odb_Enum::odb_ResultPositionEnum::WHOLE_ELEMENT){
            // Already one value per element (e.g. "*EL FILE, POSITION=CENTROID"
            // or element volume): keep it as a cell attribute instead of
            // dropping the field.
            ReadDataArrayWithSectionPoints(sectionPoints,
                                           fldOutput,
                                           abqPos,
                                           fldOutput.name(),
                                           1,
                                           CellData,
                                           left_attr_val
                                           );
        }
    }

    /* Read LocalCS. Not every ODB stores stress, and a field without an
     * orientation still needs the material orientation attribute. */
    if (m_cellsNum == 0 || !fldOutputs.isMember(odb_String("S"))) {
        return true;
    }
    const auto& fldOutput = fldOutputs.constGet(odb_String("S"));
    FloatArray::Pointer localCSArray = FloatArray::New();
    localCSArray->SetName("Material_Orientation");
    localCSArray->SetDimension(3);
    localCSArray->Resize(m_cellsNum);
    for (const auto& inst_name : m_Instance_names)
    {
        auto inst = m_ODB->rootAssembly().instances()[inst_name.c_str()];
        auto& curCellMap = m_CellsMap[inst_name];
        auto instStress = fldOutput
                .getSubset(inst)
                .getSubset(odb_Enum::odb_ResultPositionEnum::CENTROID);
        for (int i = 0; i < instStress.bulkDataBlocks().size(); i++)
        {
            const auto& block = instStress.bulkDataBlocks()[i];
            const int* elementLabels = block.elementLabels();
            if (elementLabels == nullptr) continue;
            // local coordinate system is a quaternion.
            // size is 4 x numElements
            float* localCS = block.localCoordSystem();
            const bool hasQuaternion = localCS != nullptr && block.orientationWidth() >= 4;
            for (int elem = 0; elem < block.numberOfElements(); elem++)
            {
                auto mapIt = curCellMap.find(elementLabels[elem]);
                if (mapIt == curCellMap.end()) continue;
                const int vtkIndex = mapIt->second;
                if (!hasQuaternion)
                {
                    localCSArray->SetValue(vtkIndex * 3 + 0, 1);// default orientation if localCS is empty
                    localCSArray->SetValue(vtkIndex * 3 + 1, 0);
                    localCSArray->SetValue(vtkIndex * 3 + 2, 0);
                    continue;
                }
                const float q1 = localCS[elem * 4];
                const float q2 = localCS[elem * 4 + 1];
                const float q3 = localCS[elem * 4 + 2];
                const float q4 = localCS[elem * 4 + 3]; // scalar term
                localCSArray->SetValue(vtkIndex * 3 + 0, q4 * q4 + q1 * q1 - q2 * q2 - q3 * q3);
                localCSArray->SetValue(vtkIndex * 3 + 1, 2 * (q1 * q2 - q3 * q4));
                localCSArray->SetValue(vtkIndex * 3 + 2, 2 * (q1 * q3 + q2 * q4));
            }
        }

        UpdateProgress(AddODBProgress(left_attr_val));
    }
    m_Attribute_helper->m_AttributeSet->AddAttribute(IG_VECTOR, IG_CELL, localCSArray);

    return true;
}
void ODBReader::ReadDataArrayWithSectionPoints(const std::vector<odb_SectionPoint>& sectionPoints,
                                                    const odb_FieldOutput& fldOutput,
                                                    const odb_Enum::odb_ResultPositionEnum& pos,
                                                    const odb_String& fieldName,
                                                    int maxNumOfIntergrationPoints,
                                                    const DataArrayType& dataArrayType,
                                                    float current_progress_val /*For update Progress*/
                                                    ){
    if (sectionPoints.empty())
    {
        ReadDataArray(fldOutput, pos, maxNumOfIntergrationPoints, fieldName, dataArrayType, current_progress_val);
        return;
    }
    current_progress_val /= sectionPoints.size();
    for (const auto& sp : sectionPoints)
    {
        auto subset = fldOutput.getSubset(sp);
        ReadDataArray(subset, pos, maxNumOfIntergrationPoints, fieldName + sp.description(), dataArrayType, current_progress_val);
    }
}
void ODBReader::ReadDataArray(const odb_FieldOutput &fldOutput, const odb_Enum::odb_ResultPositionEnum &pos,
                              int maxNumOfIntergrationPoints, const odb_String &arrayName,
                              const ODBReader::DataArrayType &dataArrayType,
                              float current_progress_val /*For update Progress*/
                              ) {
    FloatArray::Pointer array = FloatArray::New();

    /* Set attribute's Dimension. */
    auto abqComponentLabels = fldOutput.componentLabels();
    int componentsSize = abqComponentLabels.Length() == 0 ? 1 : abqComponentLabels.Length();
    array->SetName(arrayName.cStr());
    array->SetDimension(componentsSize * maxNumOfIntergrationPoints);
    /* Get All Dimension's Name, Currently is not used. */
//                for (int j = 0; j < maxNumIntegrationPoints; j++)
//                {
//                    for (int i = 0; i < componentsSize; i++)
//                    {
//                        const char* dimensionName = abqComponentLabels.Length() == 0 ? "Value" : abqComponentLabels[i].cStr();
//                    }
//                }
    if(dataArrayType == PointData){
        array->Resize(m_nodesNum);
        for (const auto& instanceName : m_Instance_names)
        {
            auto selectedInstance =  m_ODB->rootAssembly().instances().constGet(instanceName.c_str());
            auto subset = fldOutput.getSubset(selectedInstance).getSubset(pos);
            AttributeParserHelper::ReadSortedPointData(subset.bulkDataBlocks(), m_NodesMap[instanceName], array, current_progress_val, this);
        }
        auto attributeType = AttributeParserHelper::ABAQUS_VTK_FIELD_OUTPUTS_MAP(fldOutput);
        if(attributeType == "Scalars"){
            m_Attribute_helper->m_AttributeSet->AddAttribute(IG_SCALAR, IG_POINT, array);
        } else if(attributeType == "Vectors"){
            m_Attribute_helper->m_AttributeSet->AddAttribute(IG_VECTOR, IG_POINT, array);
        } else if(attributeType == "Tensors"){
            m_Attribute_helper->m_AttributeSet->AddAttribute(IG_TENSOR, IG_POINT, array);
        }
    } else if(dataArrayType == CellData){
        array->Resize(m_cellsNum);
        for (const auto& instanceName : m_Instance_names)
        {
            auto selectedInstance =  m_ODB->rootAssembly().instances().constGet(instanceName.c_str());
            auto subset = fldOutput.getSubset(selectedInstance).getSubset(pos);
            AttributeParserHelper::ReadSortedCellData(subset.bulkDataBlocks(), m_CellsMap[instanceName], array, current_progress_val, this);
        }
        auto attributeType = AttributeParserHelper::ABAQUS_VTK_FIELD_OUTPUTS_MAP(fldOutput);
        if(attributeType == "Scalars"){
            m_Attribute_helper->m_AttributeSet->AddAttribute(IG_SCALAR, IG_CELL, array);
        } else if(attributeType == "Vectors"){
            m_Attribute_helper->m_AttributeSet->AddAttribute(IG_VECTOR, IG_CELL, array);
        } else if(attributeType == "Tensors"){
            m_Attribute_helper->m_AttributeSet->AddAttribute(IG_TENSOR, IG_CELL, array);
        }
    }

}

ODBReader::ODBReader() {
    SetNumberOfInputs(0);
    SetNumberOfOutputs(1);
}

ODBReader::~ODBReader() {
    if(m_ODB != nullptr)m_ODB->close();
    delete m_Attribute_helper;
    IGAME_CORE_DEBUG("Close ODB Reader");
}

uint8_t ODBReader::ABAQUS_VTK_CELL_MAP(const char *abqElementType) {
    if (strncmp(abqElementType, "C3D4", 4) == 0)
    {
        return 10;
    }
    else if (strncmp(abqElementType, "C3D8", 4) == 0)
    {
        return 12;
    }
    else if (strncmp(abqElementType, "C3D6", 4) == 0)
    {
        return 13;
    }
    else if (strncmp(abqElementType, "C3D10", 5) == 0)
    {
        return 24;
    }
    else if (strncmp(abqElementType, "C3D15", 5) == 0)
    {
        return 26;
    }
    else if (strncmp(abqElementType, "C3D20", 5) == 0)
    {
        return 25;
    }
    else if (strncmp(abqElementType, "SC8R", 4) == 0)
    {
        return 12;
    }
    else if (strncmp(abqElementType, "SC6R", 4) == 0)
    {
        return 13;
    }
    else if (strncmp(abqElementType, "S3", 2) == 0)
    {
        return 5;
    }
    else if (strncmp(abqElementType, "S4", 2) == 0)
    {
        return 9;
    }
    else if (strncmp(abqElementType, "S8", 2) == 0)
    {
        return 23;
    }
    else if (strcmp(abqElementType, "S9") == 0)
    {
        return 28;
    }
    else if (strcmp(abqElementType, "B31") == 0)
    {
        return 3;
    }
    else if (strcmp(abqElementType, "R3D3") == 0)
    {
        return 5;
    }
    else if (strcmp(abqElementType, "R3D4") == 0)
    {
        return 9;
    }
    else if (strcmp(abqElementType, "IDCOUP3D") == 0)
    {
        return 10;
    }
    else if (strcmp(abqElementType, "T3D2") == 0)
    {
        return 3;
    }
    else if (strcmp(abqElementType, "B31") == 0)
    {
        return 3;
    }
    /* Mass elements are 0D point elements defined by a single node.
       VTK_VERTEX = 1 */
    else if (strncmp(abqElementType, "MASS", 4) == 0)
    {
        return 1;
    }
    /*brick_sph_indenter_new.odb*/
    else if (strcmp(abqElementType, "COH3D8") == 0)
    {
        return 12;
    }
    /*If you want to represent a geometric topology, you can use VTK_TRIANGLE (ID: 5) because SFM3D4R is essentially a triangular face unit.
        VTK_TETRA (ID: 10) can be used to represent its relationship to the volume element, but this is only to match the geometry and does not include contact characteristics.*/
    else if (strcmp(abqElementType, "SFM3D4R") == 0)
    {
        return 10;
    }
    /*Support M3D4R(Membrane Element, Reduced Integration) && M3D4(Membrane Element, Full Integration)*/
    else if (strncmp(abqElementType, "M3D4", 4) == 0)
    {
        return 9;
    }

    IGAME_CORE_ERROR("{} not supported by the converter.", abqElementType);
    return -1;
}

std::vector<std::string> ODBReader::ReadOdbAllStep(const std::string &filePath) {
    std::vector<std::string> res;
    if (!IsRuntimeAvailable()) return res;
    odb_initializeAPI();
    try {
        auto odb_ptr = &openOdb(odb_String(filePath.c_str()));
        if (odb_ptr == nullptr) {
            odb_finalizeAPI();
            return res;
        }
        odb_StepRepositoryIT stepIt(odb_ptr->steps());
        for(stepIt.first(); !stepIt.isDone(); stepIt.next())
        {
            res.emplace_back(stepIt.currentKey().cStr());
        }
        // The ODB was opened only to list its steps; release it before
        // finalizing the API instead of leaking it for the whole session.
        odb_ptr->close();
    }
    catch (odb_BaseException& exc) {
        IGAME_CORE_ERROR("Abaqus error message: {}", exc.UserReport().CStr());
    }
    catch (...) {
        IGAME_CORE_ERROR("Unknown exception while reading the ODB step list.");
    }
    odb_finalizeAPI();
    return res;
}


IGAME_NAMESPACE_END
#endif
