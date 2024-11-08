//
// Created by m_ky on 2024/9/18.
//

/**
 * @class   iGameODBReader
 * @brief   iGameODBReader's brief
 */
#if defined(AbqSDK_ENABLE)
#include "iGameODBReader.h"


#include <odb_API.h>
#include <VTK/iGameVTKAbstractReader.h>

IGAME_NAMESPACE_BEGIN

/* Define Internal Adaptor Class. */
class AttributeParserHelper{
public:
    std::string stepName;
    int frameIdx{0};
    AttributeSet::Pointer m_AttributeSet{nullptr};

    AttributeSet::Pointer GetResult(){ return m_AttributeSet;};
    AttributeParserHelper(const std::string& _stepName, int _frameIdx) : stepName(_stepName), frameIdx(_frameIdx){
        m_AttributeSet = AttributeSet::New();
    }
    ~AttributeParserHelper() = default;
};

/* Public API: */
AttributeSet::Pointer ODBReader::ReadOdbFieldData(const std::string &filePath, const std::string& stepName, int frame_idx) {
    SetFilePath(filePath);
    if(ExecuteWithFieldData(stepName, frame_idx)) return m_Attribute_helper->GetResult();
    return nullptr;
}
DataObject::Pointer ODBReader::ReadOdbMesh(const std::string &filePath) {
    SetFilePath(filePath);
    Execute();
    return this->GetOutput();
}
/* Protected API: */

bool ODBReader::ExecuteWithFieldData(const std::string& stepName, int frameIdx) {
    m_Attribute_helper = new AttributeParserHelper(stepName, frameIdx);

    odb_initializeAPI();
    try {
        OpenODB();
        std::cout <<"Open end\n";
        ExtractHeader();
        std::cout <<"ExtractHeader end\n";
        ConstructMap();
        std::cout <<"ConstructMap end\n";
        ReadAttributes();
        std::cout <<"ReadAttributes end\n";
    }
    catch (odb_BaseException& exc) {
        odb_finalizeAPI();
        std::cout << "Abaqus error message: " << exc.UserReport().CStr() << std::endl;
        return false;
    }catch (...) {
        odb_finalizeAPI();
        std::cout << "Unknown Exception.\n";
        return false;
    }
    odb_finalizeAPI();
    return true;
}

bool iGame::ODBReader::Execute() {
    std::cout << "Init Odb start\n";
    odb_initializeAPI();
    std::cout << "Init Odb end\n";
    try {
        std::cout << "Open Odb start\n";
        OpenODB();
        std::cout <<"Open end\n";
        ExtractHeader();
        std::cout <<"ExtractHeader end\n";
        ConstructMap();
        std::cout <<"ConstructMap end\n";
        ReadCoordinates();

        CreateDataObject();
        std::cout <<"ReadCoordinates end\n";
    }
    catch (odb_BaseException& exc) {
        std::cout << "Abaqus error message: " << exc.UserReport().CStr() << std::endl;
    }catch (...) {
        std::cout << "Unknown Exception.\n";
    }
    odb_finalizeAPI();


    int size = m_Output->GetAttributeSet()->GetAllAttributes()->GetNumberOfElements();
    if (size > 0) {
        StringArray::Pointer attrbNameArray = StringArray::New();
        for (int i = 0; i < size; i++) {
            auto& data = m_Output->GetAttributeSet()->GetAttribute(i);
            attrbNameArray->AddElement(data.pointer->GetName());
        }
        m_Output->GetMetadata()->AddStringArray(ATTRIBUTE_NAME_ARRAY, attrbNameArray);
    }
    m_Output->SetName(m_FileName);
    SetOutput(0, m_Output);
    return true;
}

void iGame::ODBReader::SetFilePath(const std::string &filePath) {
    this->m_FilePath = filePath;
    this->m_FileName =
            filePath.substr(filePath.find_last_of('/') + 1, filePath.size());
    this->m_FileDir = filePath.substr(0, filePath.find_last_of('/') + 1);
}

bool ODBReader::OpenODB() {
    odb_String odbFile = odb_String(m_FilePath.c_str());
    return m_ODB = &openOdb(odbFile);
}

bool ODBReader::ExtractHeader() {
    if (m_ODB != nullptr)
    {
        auto filename = m_FileDir + "header.txt";
        {
            // write instances
            odb_InstanceRepositoryIT instIter(m_ODB->rootAssembly().instances());
            for (instIter.first(); !instIter.isDone(); instIter.next())
            {
                m_Instance_names.push_back(instIter.currentKey().CStr());
            }
            // write steps and frames
            odb_StepRepositoryIT stepIter(m_ODB->steps());
            for (stepIter.first(); !stepIter.isDone(); stepIter.next())
            {
                std::string stepName = std::string(stepIter.currentKey().CStr());
                m_StepFrameMap[stepName] = stepIter.currentValue().frames().size();
            }
        }
    }

    return true;
}

bool ODBReader::ConstructMap() {
    // we need to map the local label from abaqus to global index for paraview.
    // global index used in paraview for node and cell.
    int nodeIndex = 0;
    int cellIndex = 0;
    for (const auto& inst_name : m_Instance_names)
    {
        m_NodesMap[inst_name] = std::map<int, int>();
        m_CellsMap[inst_name] = std::map<int, int>();
        auto rootAssy = m_ODB->rootAssembly();
        auto inst = rootAssy.instances()[inst_name];
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
        const auto& inst = rootAssy.instances()[inst_name];
        // write node coordinates
        for (int i = 0; i < inst.nodes().size(); i ++)
        {
            dataSetPoints->AddPoint(inst.nodes(i).coordinates());
        }
        // write cell connectivity, offset, and type
        for (int i = 0; i < inst.elements().size(); i++)
        {
//            auto t1 = std::chrono::steady_clock::now();
            const auto& cell = inst.elements(i);
//            auto t2 = std::chrono::steady_clock::now();
            // connectivity
            const int* conn = cell.connectivity(nodesNumCell);
//            auto t3 = std::chrono::steady_clock::now();
            for (int j = 0; j < nodesNumCell; j++)
            {
                cellConnectivity->AddValue(m_NodesMap[inst_name][conn[j]]);
            }
//            auto t4 = std::chrono::steady_clock::now();
            // offset
            offset += nodesNumCell;
            cellOffsets->AddValue(offset);
            // CellType
            cellTypes->AddValue(ABAQUS_VTK_CELL_MAP(cell.type().cStr()));
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
//            DynamicCast<UnstructuredMesh>(m_Output)->SetAttributeSet(m_Data.Data);
                return true;
            }
            case IG_SURFACE_MESH:{
                DynamicCast<SurfaceMesh>(m_Output)->SetPoints(m_Data.GetPoints());
//            DynamicCast<SurfaceMesh>(m_Output)->SetAttributeSet(m_Data.Data);
                return true;
            }
        }
        return false;
}


bool ODBReader::ReadAttributes() {

    return false;
}


ODBReader::ODBReader() {
    SetNumberOfInputs(0);
    SetNumberOfOutputs(1);
}

ODBReader::~ODBReader() {
    if(m_ODB != nullptr)m_ODB->close();
    if(m_Attribute_helper != nullptr) delete m_Attribute_helper;
    std::cout << "Close ODB Reader\n";
}

uint8_t ODBReader::ABAQUS_VTK_CELL_MAP(const char *abqElementType) {
    if (strcmp(abqElementType, "C3D4") == 0)
    {
        return 10;
    }
    else if (strcmp(abqElementType, "C3D6") == 0)
    {
        return 13;
    }
    else if (strcmp(abqElementType, "C3D8") == 0)
    {
        return 12;
    }
    else if (strcmp(abqElementType, "C3D10") == 0)
    {
        return 24;
    }
    else if (strcmp(abqElementType, "C3D15") == 0)
    {
        return 26;
    }
    else if (strcmp(abqElementType, "C3D20") == 0)
    {
        return 25;
    }
    else if (strcmp(abqElementType, "S3") == 0)
    {
        return 5;
    }
    else if (strcmp(abqElementType, "S4") == 0)
    {
        return 9;
    }
    else if (strcmp(abqElementType, "S8") == 0)
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

    std::cerr << abqElementType << " not supported by the converter." << std::endl;
    return -1;
}




IGAME_NAMESPACE_END
#endif