#ifndef iGameMeshCodecEncodedDataObject_h
#define iGameMeshCodecEncodedDataObject_h

#include "iGameDataObject.h"
#include <fstream>
#include <memory>

IGAME_NAMESPACE_BEGIN

/**
 * @class MeshEncodedDataObject
 * @brief 存储编码后的网格数据，包含文件路径和按需创建的流
 * 
 * 用于在编码器和解码器之间传递编码后的二进制数据
 * - 编码器将其作为输出，写入编码数据
 * - 解码器将其作为输入，读取编码数据
 */
class MeshEncodedDataObject : public DataObject {
public:
    I_OBJECT(MeshEncodedDataObject);
    static Pointer New() { return new MeshEncodedDataObject; }

    IGenum GetDataObjectType() const override { return IG_MESH_ENCODED_DATA; }

    /**
     * @brief 设置文件路径
     * @param filePath 编码数据文件的路径
     */
    void SetFilePath(const std::string& filePath) { 
        m_FilePath = filePath; 
        // 路径改变时清理缓存的流
        CloseStreams();
    }

    /**
     * @brief 获取文件路径
     */
    const std::string& GetFilePath() const { 
        return m_FilePath; 
    }

    /**
     * @brief 获取输入流（用于解码器读取）
     * @return 输入流指针，如果文件打开失败返回nullptr
     */
    std::istream* GetInputStream() {
        if (!m_InputStream && !m_FilePath.empty()) {
            m_InputFileStream = std::make_unique<std::ifstream>(m_FilePath, std::ios::binary);
            if (m_InputFileStream->is_open()) {
                m_InputStream = m_InputFileStream.get();
            }
        }
        return m_InputStream;
    }

    /**
     * @brief 获取输出流（用于编码器写入）
     * @return 输出流指针，如果文件打开失败返回nullptr
     */
    std::ostream* GetOutputStream() {
        if (!m_OutputStream && !m_FilePath.empty()) {
            m_OutputFileStream = std::make_unique<std::ofstream>(m_FilePath, std::ios::binary);
            if (m_OutputFileStream->is_open()) {
                m_OutputStream = m_OutputFileStream.get();
            }
        }
        return m_OutputStream;
    }

    /**
     * @brief 检查输入流是否可用
     */
    bool IsInputAvailable() const {
        return !m_FilePath.empty() && std::ifstream(m_FilePath, std::ios::binary).is_open();
    }

    /**
     * @brief 检查是否可以创建输出流
     */
    bool IsOutputAvailable() const {
        return !m_FilePath.empty();
    }

    /**
     * @brief 关闭所有流
     */
    void CloseStreams() {
        if (m_InputFileStream) {
            m_InputFileStream->close();
            m_InputFileStream.reset();
            m_InputStream = nullptr;
        }
        if (m_OutputFileStream) {
            m_OutputFileStream->close();
            m_OutputFileStream.reset();
            m_OutputStream = nullptr;
        }
    }

protected:
    MeshEncodedDataObject() = default;
    ~MeshEncodedDataObject() override {
        CloseStreams();
    }

private:
    std::string m_FilePath;
    
    // 缓存的流
    std::unique_ptr<std::ifstream> m_InputFileStream;
    std::unique_ptr<std::ofstream> m_OutputFileStream;
    std::istream* m_InputStream = nullptr;
    std::ostream* m_OutputStream = nullptr;
};

IGAME_NAMESPACE_END
#endif