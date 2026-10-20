#ifndef iGameMultiBlockGeometryFilter_h
#define iGameMultiBlockGeometryFilter_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameModelGeometryFilter.h"

IGAME_NAMESPACE_BEGIN


struct BlockErrorInfo {
    std::string path;   // 哪个构件（如 "/Assembly[0]/partA"）
    std::string reason; // 为什么失败（如 "非实体网格类型"、或者抓到的 e.what()）
};

class MultiBlockGeometryFilter : public Filter{
public:
    I_OBJECT(MultiBlockGeometryFilter);
    static MultiBlockGeometryFilter::Pointer New() {return new MultiBlockGeometryFilter;}
    bool Execute() override;
    bool Execute(DataObject::Pointer);
    bool Execute(DataObject::Pointer, DataObject::Pointer&);
    const std::string& GetMessage() const { return m_Message; }
    const std::vector<BlockErrorInfo>& GetFailedBlocks() const { return m_FailedBlocks; }

protected:
    DataObject::Pointer input;
    DataObject::Pointer output;

    MultiBlockGeometryFilter();
    ~MultiBlockGeometryFilter() override =default;

    bool ExtractRecursively(DataObject::Pointer, DataObject::Pointer&, const std::string& currentPath = "");
    // 根模型错误
    std::string m_Message;
    // 子模型错误
    std::vector<BlockErrorInfo> m_FailedBlocks;
};



IGAME_NAMESPACE_END
#endif