#ifndef IGAME_P3SAM_SEGMENTER_H
#define IGAME_P3SAM_SEGMENTER_H

#include "iGameObject.h"
#include "iGameDataObject.h"
#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * P3SAM网格分割器
 * 封装完整的3D网格部件分割流程：
 * 1. 网格三角化
 * 2. 快速简化（降低网络传输和服务器负载）
 * 3. 发送到P3SAM服务器进行分割
 * 4. 接收VTK分割结果
 * 5. 映射回原始高分辨率网格
 */
class P3SAMSegmenter : public Object {
public:
    I_OBJECT(P3SAMSegmenter);
    static Pointer New() { return new P3SAMSegmenter; }

    /**
     * 构造函数
     * @param serverHost P3SAM服务器地址
     * @param serverPort P3SAM服务器端口
     */
    P3SAMSegmenter(const std::string& serverHost = "127.0.0.1", int serverPort = 8765);
    ~P3SAMSegmenter() override;

    /**
     * 设置输入网格
     */
    void SetInput(DataObject::Pointer input);

    /**
     * 获取输入网格
     */
    DataObject::Pointer GetInput() const;

    /**
     * 执行分割
     * @return 是否成功
     */
    bool Execute();

    /**
     * 获取输出网格（带part_id属性）
     */
    DataObject::Pointer GetOutput();

    // === 参数设置 ===

    /**
     * 设置简化比例（简化后保留的面片比例）
     * @param ratio 0.0~1.0，默认0.1（保留10%）
     */
    void SetSimplificationRatio(float ratio);

    /**
     * 设置P3SAM点数
     * @param pointNum 默认10000
     */
    void SetPointNum(int pointNum);

    /**
     * 设置P3SAM提示点数
     * @param promptNum 默认100
     */
    void SetPromptNum(int promptNum);

    /**
     * 设置随机种子
     * @param seed 默认42
     */
    void SetSeed(int seed);

    /**
     * 设置是否后处理
     * @param postProcess 默认false
     */
    void SetPostProcess(bool postProcess);

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
     * 获取错误信息
     */
    std::string GetErrorMessage() const;

    /**
     * 获取上次分割结果的部件数量
     */
    int GetPartCount() const;

protected:
    std::string m_serverHost;
    int m_serverPort;

    DataObject::Pointer m_input;
    DataObject::Pointer m_output;

    // P3SAM参数
    float m_simplificationRatio;
    int m_pointNum;
    int m_promptNum;
    int m_seed;
    bool m_postProcess;
    int m_timeoutMs;
    bool m_preserveBoundary;

    // 状态
    std::string m_errorMessage;
    int m_partCount;

    // 内部方法
    bool triangulateInput(DataObject::Pointer input, DataObject::Pointer& triangulated);
    bool simplifyMesh(DataObject::Pointer input, DataObject::Pointer& simplified);
    bool exportToOBJ(DataObject::Pointer mesh, std::vector<uint8_t>& objData);
    bool sendToServer(const std::vector<uint8_t>& objData, std::vector<uint8_t>& vtkData);
    bool parseVTKResult(const std::vector<uint8_t>& vtkData, DataObject::Pointer& segmented);
    bool mapBackToOriginal(DataObject::Pointer original, DataObject::Pointer segmented);
};

IGAME_NAMESPACE_END

#endif // IGAME_P3SAM_SEGMENTER_H
