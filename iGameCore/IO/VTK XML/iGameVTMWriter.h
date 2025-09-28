#pragma once

#include "iGameDataObject.h"
#include "iGameFileWriter.h"
#include <tinyxml2.h>

IGAME_NAMESPACE_BEGIN

class VTMWriter : public FileWriter {
public:
    using Pointer = std::shared_ptr<VTMWriter>;
    static Pointer New() { return std::make_shared<VTMWriter>(); }

    VTMWriter() = default;
    ~VTMWriter() override = default;

protected:
    /// 生成 VTM 文件的缓冲区内容（由父类 WriteToFile() 调用）
    bool GenerateBuffers() override;

private:
    /// 写子块信息（递归或二层展开）
    void WriteSubBlocks(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* parentElem, DataObject::Pointer obj,
                        const std::string& fileDir);
};

IGAME_NAMESPACE_END
