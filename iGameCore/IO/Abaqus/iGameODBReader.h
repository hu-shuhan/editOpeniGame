#if defined(AbqSDK_ENABLE)
#pragma once


#include <iGameFilter.h>
#include "iGameDataCollection.h"
class odb_Odb;
IGAME_NAMESPACE_BEGIN
class ODBReader : public Filter{
public:
    I_OBJECT(ODBReader)

    static Pointer New(){return new ODBReader;}

    DataObject::Pointer ReadFile(const std::string& filePath);

protected:

    bool Execute() override;

    bool CreateDataObject();

    void SetFilePath(const std::string& filePath);

    void OpenODB();

    void ExtractHeader();

    void ConstructMap();

    void ReadCoordinates();

    void ReadAttributes();


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


protected:
    ODBReader();
    ~ODBReader() ;

};

IGAME_NAMESPACE_END
#endif
