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
IGAME_NAMESPACE_BEGIN
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
        cur_Progress_val /= blkDataBlock.size();
        for (int i = 0; i < blkDataBlock.size(); i++)
        {
            auto blk = blkDataBlock[i];
            int numValues = blk.length();
            int numComp = blk.width();
            int nElems = blk.numberOfElements();
            if(nElems == 0) continue;
            int numIP = numValues / nElems;
            float* data = blk.data();
            for (int j = 0; j < nElems; j++)
            {
                int cellLabelAbq = blk.elementLabels()[j];
                INT64 cellLabelVtk = dataMap[cellLabelAbq];
                for (int ip = 0; ip < numIP; ip++)
                {
                    for (int comp = 0; comp < numComp; comp++)
                    {
                        int idx = cellLabelVtk * numComp * numIP + (INT64)ip * (INT64)numComp + comp;
                        float val = data[j * numComp * numIP + ip * numComp + comp];
                        resDataArray->SetValue(idx, val);
//                        dataArray[cellLabelVtk * numComp * numIP + (INT64)ip * (INT64)numComp + comp] =
//                                data[j * numComp * numIP + ip * numComp + comp];
                    }
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
        cur_Progress_val /= blkDataBlock.size();
        for (int i = 0; i < blkDataBlock.size(); i++)
        {
            auto blk = blkDataBlock[i];
            int numNodes = blk.length();
            int numComp = blk.width();
            float* data = blk.data();
            for (int j = 0; j < numNodes; j++)
            {
                int nodeLabelAbq = blk.nodeLabels()[j];
                INT64 nodeLabelVtk = dataMap[nodeLabelAbq];
                for (int comp = 0; comp < numComp; comp++)
                {
                    int idx = nodeLabelVtk * numComp + comp;
                    float val = data[j * numComp + comp];
                    resDataArray->SetValue(idx, val);
//                    o_data[nodeLabelVtk * numComp + comp] = data[j * numComp + comp];
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
    m_NeedRequestMap = m_NeedRequestInstance = true;
    m_NeedRequestStep = false;
    SetFilePath(filePath);
    Execute();
    UpdateProgress(1.f);
    return this->GetOutput();
}
DataObject::Pointer ODBReader::ReadOdbFirstFrameMesh(const std::string &filePath) {
    m_NeedRequestMap = m_NeedRequestInstance = m_NeedRequestStep = true;
    SetFilePath(filePath);
    SetProgressCoefficient(0.5f);
    Execute();
    m_NeedRequestMap = m_NeedRequestInstance = m_NeedRequestStep = false;
    auto outputObj = this->GetOutput();
    int frameIdx = 0;
    if(ExecuteWithFieldData(frameIdx)) {
        auto attributeSet = m_Attribute_helper->GetResult();
        outputObj->SetAttributeSet(attributeSet);
    }
    UpdateProgress(1.f);
    return outputObj;
}
DataObject::Pointer ODBReader::ReadOdbFirstFrameMesh(const std::string &filePath, const std::string &stepName) {
    m_NeedRequestMap = m_NeedRequestInstance = true;
    m_NeedRequestStep = true;
    SetFilePath(filePath);
    SetProgressCoefficient(0.5f);
    Execute();
    m_NeedRequestMap = m_NeedRequestInstance = false;

    auto outputObj = this->GetOutput();
    int frameIdx = 0;
    if(ExecuteWithFieldData(stepName, frameIdx)) {
        auto attributeSet = m_Attribute_helper->GetResult();
        outputObj->SetAttributeSet(attributeSet);
    }
    UpdateProgress(1.f);
    return outputObj;
}

AttributeSet::Pointer ODBReader::ReadOdbFieldData(const std::string &filePath, int frame_idx) {
    SetFilePath(filePath);
    m_NeedRequestInstance = m_NeedRequestStep = true;
    m_NeedRequestMap = true;
    if(ExecuteWithFieldData(frame_idx)) return m_Attribute_helper->GetResult();
    UpdateProgress(1.f);
    return nullptr;
}
AttributeSet::Pointer ODBReader::ReadOdbFieldData(const std::string &filePath, const std::string& stepName, int frame_idx) {
    SetFilePath(filePath);
    m_NeedRequestInstance = true;
    m_NeedRequestStep = false;
    m_NeedRequestMap = true;
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
            UpdateProgress(AddODBProgress(0.05f));
            IGAME_CORE_DEBUG("Open end");
            ExtractHeader();
            UpdateProgress(AddODBProgress(0.05f));
            IGAME_CORE_DEBUG("ExtractHeader end");
            ConstructMap();
            UpdateProgress(AddODBProgress(0.1f));
            m_Attribute_helper = new AttributeParserHelper(m_StepFrameMap.begin()->first, frameIdx);
            IGAME_CORE_DEBUG("ConstructMap end");
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
    m_Attribute_helper = new AttributeParserHelper(stepName, frameIdx);

    odb_initializeAPI();
    try {
        if(!OpenODB()){
            IGAME_CORE_ERROR("Fail to Open ODB dataBase");
            return false;
        }
        IGAME_CORE_DEBUG("Open end");
        ExtractAllInstance();
        IGAME_CORE_DEBUG("ExtractHeader end");
        ConstructMap();
        IGAME_CORE_DEBUG("ConstructMap end");
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

//    int size = m_Output->GetAttributeSet()->GetAllAttributes()->GetNumberOfElements();
//    if (size > 0) {
//        StringArray::Pointer attrbNameArray = StringArray::New();
//        for (int i = 0; i < size; i++) {
//            auto& data = m_Output->GetAttributeSet()->GetAttribute(i);
//            attrbNameArray->AddElement(data.pointer->GetName());
//        }
//        m_Output->GetMetadata()->AddStringArray(ATTRIBUTE_NAME_ARRAY, attrbNameArray);
//    }
//    m_Output->SetName(m_FileName);
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
bool ODBReader::ExtractAllInstance() {
    if(!m_NeedRequestInstance) return true;
    odb_InstanceRepositoryIT instIter(m_ODB->rootAssembly().instances());
    m_Instance_names.clear();
    for (instIter.first(); !instIter.isDone(); instIter.next())
    {
//        igDebug("Instance Name : {}", instIter.currentKey().CStr());
        m_Instance_names.push_back(instIter.currentKey().CStr());
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
        auto& current_nodeMap = m_NodesMap[inst_name];
        const auto& inst = rootAssy.instances()[inst_name];
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

        // TODO: Current only support first step file's timeStep
        auto firstStep = m_StepFrameMap.begin();
        for(int i = 0; i < firstStep->second; i ++){
            double time_val = m_ODB->steps().constGet(firstStep->first.c_str()).frames()[i].frameValue();
            StringArray::Pointer array = StringArray::New();
            array->AddElement(m_FilePath);
            m_Output->GetTimeFrames()->AddTimeStep(time_val, array, StreamingType::SingleFieldAttributes);
        }
        return true;
}


bool ODBReader::ReadAttributes() {
    const char* stepName = m_Attribute_helper->stepName.c_str();
    int frameIdx = m_Attribute_helper->frameIdx;
    const odb_Frame& frame = m_ODB->steps().constGet(stepName).frames()[frameIdx];
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
    float left_attr_val = left_progress / fldOutputs.size();
    for (fieldIter.first(); !fieldIter.isDone(); fieldIter.next())
    {
//        const auto& fldOutput = fldOutputs.constGet(names.constGet(field_idx));
        const auto& fldOutput = fieldIter.currentValue();
//        std::string vtk_type = AttributeParserHelper::ABAQUS_VTK_FIELD_OUTPUTS_MAP(fldOutput);

        // assuming location only has size of 1
        // TODO: may need to consider cases with locations more than 1
        auto abqPos = fldOutput.locations().constGet(0).position();
        // if field output contains sectionPoint data, we need to generate separate dataset

        // purpose of this loop is to iterate over field ouput from all the instance
        // and check to see if we can find any section points
        std::vector<odb_SectionPoint> sectionPoints;
        int maxNumIntegrationPoints = 1;
        for (const auto& inst_name : m_Instance_names)
        {
            auto& inst = m_ODB->rootAssembly().instances().constGet(inst_name);
            //get data block of the instance
            //filter by position
            //note that subset.bulkDataBlocks may have more than one
            //because of different element type or sectionPoint in the same instance
            auto subset = fldOutput.getSubset(inst).getSubset(abqPos);
            for (int i = 0; i < subset.bulkDataBlocks().size(); i++)
            {
                auto blk = subset.bulkDataBlocks()[i];
                const auto& sectionPoint = blk.sectionPoint();
                if (sectionPoint.number() != -1)
                {
                    sectionPoints.push_back(sectionPoint);
                }
                // number of integration points is calculated by dividing bulkdata length by number of elements within the bulk
                // the purpose of this is that we need to track the maximum number of integration point among all instances
                // and use max to fill in the VTK data entry
                if (blk.numberOfElements() != 0)
                {
                    int num_ip = blk.length() / blk.numberOfElements();
                    if (num_ip > maxNumIntegrationPoints)
                    {
                        maxNumIntegrationPoints = num_ip;
                    }
                }
            }
        }

        if (abqPos == odb_Enum::odb_ResultPositionEnum::NODAL)
        {
            ReadDataArrayWithSectionPoints(sectionPoints,
                                           fldOutput,
                                           abqPos,
                                           fldOutput.name(),
                                           maxNumIntegrationPoints,
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
    }

    /* Read LocalCS */
    auto fldOutput = frame.fieldOutputs().constGet("S");
    FloatArray::Pointer localCSArray = FloatArray::New();
    localCSArray->SetName("Material_Orientation");
    localCSArray->SetDimension(3);
    localCSArray->Resize(m_cellsNum);
    for (const auto& inst_name : m_Instance_names)
    {
        auto inst = m_ODB->rootAssembly().instances()[inst_name];
        auto& curCellMap = m_CellsMap[inst_name];
        auto instStress = fldOutput
                .getSubset(inst)
                .getSubset(odb_Enum::odb_ResultPositionEnum::CENTROID);
        for (int i = 0; i < instStress.bulkDataBlocks().size(); i++)
        {
            auto block = instStress.bulkDataBlocks()[i];
            // local coordinate system is a quaternion.
            // size is 4 x numElements
            float* localCS = block.localCoordSystem();
            if (localCS == nullptr)
            {
                for (int elem = 0; elem < block.numberOfElements(); elem++)
                {
                    int abqIndex = block.elementLabels()[elem];
                    int vtkIndex = curCellMap[abqIndex];
                    localCSArray->SetValue(vtkIndex * 3 + 0, 1);// default orientation if localCS is empty
                    localCSArray->SetValue(vtkIndex * 3 + 1, 0);
                    localCSArray->SetValue(vtkIndex * 3 + 2, 0);
                }
            }
            else
            {
                for (int elem = 0; elem < block.numberOfElements(); elem++)
                {
                    int abqIndex = block.elementLabels()[elem];
                    int vtkIndex = curCellMap[abqIndex];
                    float q1 = localCS[elem * 4];
                    float q2 = localCS[elem * 4 + 1];
                    float q3 = localCS[elem * 4 + 2];
                    float q4 = localCS[elem * 4 + 3]; // scalar term
                    localCSArray->SetValue(vtkIndex * 3 + 0, q4 * q4 + q1 * q1 - q2 * q2 - q3 * q3);
                    localCSArray->SetValue(vtkIndex * 3 + 1, 2 * (q1 * q2 - q3 * q4));
                    localCSArray->SetValue(vtkIndex * 3 + 2, 2 * (q1 * q3 + q2 * q4));
                }
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
            auto selectedInstance =  m_ODB->rootAssembly().instances().constGet(instanceName);
            auto subset = fldOutput.getSubset(selectedInstance).getSubset(pos);
            auto subsetname = subset.name().cStr();
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
            auto selectedInstance =  m_ODB->rootAssembly().instances().constGet(instanceName);
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
    odb_initializeAPI();
    auto odb_ptr =&openOdb(odb_String(filePath.c_str()));
    odb_StepRepositoryIT stepIt(odb_ptr->steps());
    for(stepIt.first(); !stepIt.isDone(); stepIt.next())
    {
        res.emplace_back(stepIt.currentKey().cStr());
    }
    odb_finalizeAPI();
    return res;
}


IGAME_NAMESPACE_END
#endif