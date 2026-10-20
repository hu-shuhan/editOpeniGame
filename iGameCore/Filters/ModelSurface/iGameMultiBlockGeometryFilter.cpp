#include "iGameMultiBlockGeometryFilter.h"
#include "Log/iGameLogger.h"
IGAME_NAMESPACE_BEGIN

MultiBlockGeometryFilter::MultiBlockGeometryFilter() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
}

bool MultiBlockGeometryFilter::Execute() {
    input = this->GetInput(0);return Execute(this->input);
}

bool MultiBlockGeometryFilter::Execute(DataObject::Pointer input) {
    this->output = nullptr;
    return Execute(input, this->output);
}

bool MultiBlockGeometryFilter::Execute(DataObject::Pointer input, DataObject::Pointer& output) {
    m_Message.clear();
    m_FailedBlocks.clear();
    if (!input) { 
        m_Message = "输入模型为空";
        igError("[MultiBlockGeometryFilter] Input data object is null."); // ← 日志英文
        return false; 
    }
    if (!input->HasSubDataObject() || input->GetNumberOfSubDataObjects() == 0) {
        m_Message = "当前模型不是多块复合装配体，请使用普通单网格表面提取";
        igError("[MultiBlockGeometryFilter] Input is not a MultiBlock dataset (no sub-data objects).");
        return false;
    }
    bool success = ExtractRecursively(input, output);
    if (success && output) {
        this->SetOutput(0, output);
        igDebug("[MultiBlockGeometryFilter] Done. failedBlocks={}", m_FailedBlocks.size());
        return true;
    }

    if (m_Message.empty()) { 
        m_Message = "失败原因待排查";
        igError("[MultiBlockGeometryFilter] Unknow reason fail");
    }
    return false;
}

bool MultiBlockGeometryFilter::ExtractRecursively(DataObject::Pointer input, DataObject::Pointer& output,
                                                  const std::string& currentPath) {
    // Defensive check
    if (!input) {
        if (currentPath == "" || currentPath == "/") {
            output = nullptr;
            return false;
        }
        else
        {
            BlockErrorInfo blockErrorInfo;
            blockErrorInfo.path = currentPath + "/(null)";
            blockErrorInfo.reason = "子模型为空";
            IGAME_CORE_WARN("[MultiBlockGeometryFilter] Block failed: path='{}', reason='null sub-object'",
                            blockErrorInfo.path);
            m_FailedBlocks.push_back(blockErrorInfo);
            output = nullptr;
            return false;
        }
    }
    std::string name = input->GetName();
    if (name.empty()) { name = "null_name"; };
    std::string thisPath = currentPath + "/" + name;

    // Branch node: recursive traversal
    if (input->HasSubDataObject()) {
        // 这里必须用DrawObject，因为dataObject传入scene会被强转成DrawObject
        DrawObject::Pointer outContainer = DrawObject::New();
        outContainer->SetName(input->GetName()); // Preserve name hierarchy

        for (auto it = input->SubDataObjectIteratorBegin(); it != input->SubDataObjectIteratorEnd(); ++it) {
            DataObject::Pointer subOutput;
            if (ExtractRecursively(it->second, subOutput, thisPath)) {
                if (subOutput) { outContainer->AddSubDataObject(subOutput); }
            } 
            else {
                // 提取失败也不静默丢弃
                if (it->second) { outContainer->AddSubDataObject(it->second); }
            }
        }

        output = outContainer;
        if (!output) { return false; }
        return true;
    }
    // Leaf node: extract surface using ModelGeometryFilter
    else {
        auto modelFilter = ModelGeometryFilter::New();

        auto type = input->GetDataObjectType();
        if (type != IG_VOLUME_MESH && type != IG_UNSTRUCTURED_MESH && type != IG_SURFACE_MESH &&
            type != IG_STRUCTURED_MESH) {
            m_FailedBlocks.push_back({thisPath, "非实体网格类型（数据对象不支持抽面）"});
            IGAME_CORE_WARN("[MultiBlockGeometryFilter] Block failed: path='{}', reason='unsupported data type {}'",
                            thisPath, static_cast<int>(type));
            return false; // 返回 false，上层父节点会自动将其原样保留
        }
        modelFilter->SetInput(input);

        try {
            if (modelFilter->Execute()) {
                output = modelFilter->GetOutput();
                auto surfaceMesh = DynamicCast<SurfaceMesh>(output);

                // 检查：转换成功，并且面片数大于 0
                if (surfaceMesh && surfaceMesh->GetNumberOfFaces() > 0) {
                    output = surfaceMesh;
                    output->SetName(input->GetName());
                    return true;
                }
            }
        } 
        catch (const std::exception& e) {
            m_FailedBlocks.push_back({thisPath, std::string("底层算法异常: ") + e.what()});
            igError("[MultiBlockGeometryFilter] ModelGeometryFilter threw: '{}'", e.what());
            return false;
        }
        catch (...) {
            m_FailedBlocks.push_back({thisPath, std::string("底层算法未知异常")});
            igError("[MultiBlockGeometryFilter] Block failed: path='{}', reason='unknown exception'", thisPath);
            return false;
        }

    }
    // 抽取失败记录

    m_FailedBlocks.push_back({thisPath, "表面抽取失败或提取面数为0"});
    IGAME_CORE_WARN("[MultiBlockGeometryFilter] Block failed: path='{}', reason='no surface extracted (0 faces)'",
                    thisPath);
    return false;
}

IGAME_NAMESPACE_END