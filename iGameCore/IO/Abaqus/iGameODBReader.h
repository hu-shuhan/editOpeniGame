#if defined(AbqSDK_ENABLE)
#pragma once


#include <iGameFilter.h>
#include <iGameAttributeSet.h>

#include "iGameDataCollection.h"
#include <odb_API.h>

class odb_Odb;
IGAME_NAMESPACE_BEGIN

class AttributeParserHelper;
class ODBReader : public Filter{
public:
    I_OBJECT(ODBReader)

    static Pointer New(){return new ODBReader;}

    // Abaqus is optional at runtime. On Windows this locates and preloads the
    // user's local ODB runtime before any delayed Abaqus import is called.
    static bool IsRuntimeAvailable(std::string* errorMessage = nullptr);

    /* Read Odb file's Mesh WITH first keyframe's field data without specific stepName.
    * Therefore, the first one step is readed by default */
    DataObject::Pointer ReadOdbFirstFrameMesh(const std::string& filePath);
    /* Read Odb file's Mesh WITH first keyframe's field data with specific stepName. */
    DataObject::Pointer ReadOdbFirstFrameMesh(const std::string& filePath, const std::string& stepName);

    /* Read raw Mesh without specific frame's field data. */
    DataObject::Pointer ReadOdbRawMesh(const std::string& filePath);

    /* Read specific frame's field data with specific stepName*/
    AttributeSet::Pointer ReadOdbFieldData(const std::string& filePath, const std::string& stepName, int frame_idx);
    /* Read specific frame's field data without specific stepName.
     * Therefore, the first one step is readed by default */
    AttributeSet::Pointer ReadOdbFieldData(const std::string& filePath, int frame_idx);
    /* Read All step names from odb file*/
    static std::vector<std::string> ReadOdbAllStep(const std::string& filePath);
protected:
    enum DataArrayType{
        PointData,
        CellData
    } ;

protected:


    void SetFilePath(const std::string& filePath);

    bool Execute() override;

    /* Get Target Step's target frame Field data. */
    bool ExecuteWithFieldData(const std::string& stepName, int frameIdx);

    /* Get First Step's target frame Field data. */
    bool ExecuteWithFieldData(int frameIdx);

    bool CreateDataObject();

    bool OpenODB();

    bool ExtractHeader();
    bool ExtractAllInstance();
    bool ExtractAllStep();

    bool ConstructMap();

    bool ReadCoordinates();

    bool ReadAttributes();
    void ReadDataArrayWithSectionPoints(const std::vector<odb_SectionPoint>& sectionPoints,
                                   const odb_FieldOutput& fldOutput,
                                   const odb_Enum::odb_ResultPositionEnum& pos,
                                   const odb_String& fieldName,
                                   int maxNumOfIntergrationPoints,
                                   const DataArrayType& dataArrayType,
                                   float current_progress_val /*For update Progress*/

                                   )
                                   ;

    void ReadDataArray(const odb_FieldOutput& fldOutput,
                       const odb_Enum::odb_ResultPositionEnum& pos,
                       int maxNumOfIntergrationPoints,
                       const odb_String& arrayName,
                       const ODBReader::DataArrayType& dataArrayType,
                       float current_progress_val /*For update Progress*/
                       );

    static uint8_t ABAQUS_VTK_CELL_MAP(const char* abqElementType);


private:
    DataObject::Pointer m_Output;
    DataCollection m_Data;

    std::string m_FilePath;
    std::string m_FileName;
    std::string m_FileDir;
    /*Odb stuff.*/
    odb_Odb* m_ODB{nullptr};//防止未加载AbqSDK时读取ODB文件导致崩溃
    std::vector<const char*> m_Instance_names;
    std::map<std::string, int> m_StepFrameMap;

    std::map<const char*, std::map<int, int>> m_NodesMap;
    std::map<const char*, std::map<int, int>> m_CellsMap;
    size_t  m_nodesNum{0}, m_cellsNum{0};

    bool m_NeedRequestMap {true};
    bool m_NeedRequestStep {true};
    bool m_NeedRequestInstance {true};
protected:
    friend class AttributeParserHelper;
    AttributeParserHelper* m_Attribute_helper{nullptr};

protected:
    ODBReader();
    ~ODBReader();

};

IGAME_NAMESPACE_END
#endif
