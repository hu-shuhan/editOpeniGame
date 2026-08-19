#ifndef IGAME_MESH_REPORT_GENERATOR_H
#define IGAME_MESH_REPORT_GENERATOR_H

#include "iGameObject.h"
#include "iGameDataObject.h"
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * 网格分析报告生成器
 * 封装完整的网格分析报告生成流程：
 * 1. 网格三角化
 * 2. 简化
 * 3. 导出为VTK临时文件并读取为二进制内容
 * 4. 发送到报告生成服务器
 * 5. 接收报告文件（如Word文档）并保存到指定路径
 */
class MeshReportGenerator : public Object {
public:
    I_OBJECT(MeshReportGenerator);
    static Pointer New(const std::string& reportSavePath,
                        const std::string& serverHost = "127.0.0.1",
                        int serverPort = 8766) {
        return new MeshReportGenerator(reportSavePath, serverHost, serverPort);
    }

    /**
     * 构造函数
     * @param reportSavePath 报告文件保存路径（含扩展名，如 xxx.docx）
     * @param serverHost 报告生成服务器地址
     * @param serverPort 报告生成服务器端口
     */
    MeshReportGenerator(const std::string& reportSavePath,
                         const std::string& serverHost = "127.0.0.1",
                         int serverPort = 8766);
    ~MeshReportGenerator() override;

    /**
     * 设置输入网格
     */
    void SetInput(DataObject::Pointer input);

    /**
     * 获取输入网格
     */
    DataObject::Pointer GetInput() const;

    /**
     * 执行报告生成流程
     * @return 是否成功
     */
    bool Execute();

    // === 参数设置 ===

    /**
     * 设置简化比例（简化后保留的面片比例）
     * @param ratio 0.0~1.0，默认0.1（保留10%）
     */
    void SetSimplificationRatio(float ratio);

    void SetServerHost(const std::string& host) { m_serverHost = host; }
    void SetServerPort(int port) { m_serverPort = port; }

    /**
     * 设置超时时间（毫秒）
     * @param timeoutMs 默认300000（5分钟）
     */
    void SetTimeout(int timeoutMs);

    /**
     * 设置是否保留边界
     * @param preserve 默认false
     */
    void SetPreserveBoundary(bool preserve);

    /**
     * 设置指定要分析的属性场名称（与网格上通过 SetName 命名的属性一致）。
     * 传输的 VTK 只会保留 part_id 与这里列出的属性；
     * @param fields 属性场名称列表，为空表示不过滤、保留全部属性（默认）
     */
    void SetSpecifiedFields(const std::vector<std::string>& fields);

    /**
     * 获取错误信息
     */
    std::string GetErrorMessage() const;

    /**
     * 获取报告保存路径
     */
    std::string GetReportSavePath() const;

protected:
    std::string m_reportSavePath;
    std::string m_serverHost;
    int m_serverPort;

    DataObject::Pointer m_input;

    // 简化参数
    float m_simplificationRatio;
    int m_timeoutMs;
    bool m_preserveBoundary;

    // 指定要分析的属性场名称
    std::vector<std::string> m_specifiedFields;

    // 状态
    std::string m_errorMessage;

    // 内部方法
    bool triangulateInput(DataObject::Pointer input, DataObject::Pointer& triangulated);
    bool simplifyMesh(DataObject::Pointer input, DataObject::Pointer& simplified);
    bool filterAttributes(DataObject::Pointer mesh);
    bool exportToVTKFile(DataObject::Pointer mesh, std::string& tempVtkPath, std::vector<uint8_t>& vtkData);
    bool sendToServer(const std::vector<uint8_t>& vtkData, std::vector<uint8_t>& reportData);
    bool saveReport(const std::vector<uint8_t>& reportData);
};

IGAME_NAMESPACE_END

#endif // IGAME_MESH_REPORT_GENERATOR_H
