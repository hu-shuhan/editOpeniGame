#include "iGameIGCReader.h"

IGAME_NAMESPACE_BEGIN

bool IGCReader::Execute() {
    clock_t start, end;
    start = clock();

    clock_t time1 = clock();
    if (!Open()) {
        std::cerr << "Open failure\n";
        return false;
    }
    clock_t time2 = clock();
    std::cout << "Read file to buffer Cost " << time2 - time1 << "ms\n";
    if (!Parsing()) {
        std::cerr << "Parsing failure\n";
        return false;
    }
    if (!Close()) {
        std::cerr << "Close failure\n";
        return false;
    }
    clock_t time3 = clock();
    std::cout << "Generate DataObject Cost " << time3 - time2 << "ms\n";
    this->SetOutput(0, m_Output);
    end = clock();
    std::cout << "Read file success! The time cost: " << end - start << "ms" << std::endl;
    return true;
}

bool IGCReader::Parsing()
{
    // 创建 MeshEncodedDataObject 来传递文件路径
    auto encodedData = MeshEncodedDataObject::New();
    
    // 只传递文件路径
    encodedData->SetFilePath(m_FilePath);

    // 创建 MeshLoomDecoder 实例
    auto decoder = MeshLoomDecoder::New();
    
    // 设置解码器的输入
    decoder->SetInput(encodedData);
    
    // 执行解码
    if (!decoder->Execute()) {
        return false;
    }
    
    // 获取解码器的输出并从中提取原始网格数据
    auto decodedData = DynamicCast<MeshDecodedDataObject>(decoder->GetOutput());
    if (!decodedData) {
        return false;
    }
    m_Output = decodedData->GetMeshData();
    
    return true;
}

IGAME_NAMESPACE_END
