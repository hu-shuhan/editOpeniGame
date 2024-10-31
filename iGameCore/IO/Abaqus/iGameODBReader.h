#if defined(AbqSDK_ENABLE)
#pragma once


#include <iGameFilter.h>
#include <iGameAttributeSet.h>

#include "iGameDataCollection.h"
class odb_Odb;
IGAME_NAMESPACE_BEGIN

class AttributeParserHelper;
class ODBReader : public Filter{
public:
    I_OBJECT(ODBReader)

    static Pointer New(){return new ODBReader;}

    /* Read Odb file's raw Mesh without SPECIFIC frame's field data. */
    DataObject::Pointer ReadOdbMesh(const std::string& filePath);

    AttributeSet::Pointer ReadOdbFieldData(const std::string& filePath, const std::string& stepName, int frame_idx);
protected:


    void SetFilePath(const std::string& filePath);

    bool Execute() override;

    bool ExecuteWithFieldData(const std::string& stepName, int frameIdx);

    bool CreateDataObject();

    bool OpenODB();

    bool ExtractHeader();

    bool ConstructMap();

    bool ReadCoordinates();

    bool ReadAttributes();


    static uint8_t ABAQUS_VTK_CELL_MAP(const char* abqElementType);
private:
    DataObject::Pointer m_Output;
    DataCollection m_Data;

    std::string m_FilePath;
    std::string m_FileName;
    std::string m_FileDir;
    /*Odb stuff.*/
    odb_Odb* m_ODB;
    std::vector<const char*> m_Instance_names;
    std::map<std::string, int> m_StepFrameMap;

    std::map<const char*, std::map<int, int>> m_NodesMap;
    std::map<const char*, std::map<int, int>> m_CellsMap;
    size_t  m_nodesNum{0}, m_cellsNum{0};

protected:
    AttributeParserHelper* m_Attribute_helper{nullptr};

protected:
    ODBReader();
    ~ODBReader();

};

IGAME_NAMESPACE_END
#endif
