#ifndef MeshEncoder_h
#define MeshEncoder_h

#include "iGameEncodedMeshData.h"
#include "iGameFilter.h"
#include "iGameFlatArray.h"
#include "iGameMacro.h"
#include "iGameMeshCodec.h"
#include "iGameMeshCodecAdjacency.h"
#include "iGameMeshCodecParamSet.h"
#include "iGameMeshCodecZSTD.h"
#include "iGameMeshEncoderAdapter.h"
#include "iGameMeshFloatCodec.h"
#include <filesystem>
#include <memory>
#include "iGameThreadPool.h"

IGAME_NAMESPACE_BEGIN

class MeshEncoder : public MeshCodec {
public:
    I_OBJECT(MeshEncoder);
    static Pointer New() { return new MeshEncoder; }

    MeshEncoder() {
        this->SetNumberOfInputs(1);
        this->SetNumberOfOutputs(1);
    }

    void SetUIControlParams(const UIControlParams& params) {
        m_uiControlParams = params;
        m_hasUIControlParams = true;
    }

    bool Execute() override {
        if (!InitializeEncoder()) { return false; }

        std::vector<unsigned int> pointIdRemap;
        std::vector<unsigned int> topCellIdsRemap, bottomCellIdRemap;

        PayloadBuffer geomPayload(PayloadType::kGeometryBrick);
        PayloadBuffer topoPayload(PayloadType::kTopologyBrick);
        PayloadBuffer attrPayload(PayloadType::kAttributeBrick);
        PayloadBuffer paramPayload(PayloadType::kParameterSet);

        EncodePayloads(geomPayload, topoPayload, attrPayload, paramPayload, pointIdRemap, topCellIdsRemap,
                       bottomCellIdRemap);

        CompressAndWritePayloads(geomPayload, topoPayload, attrPayload, paramPayload);

        FinalizeEncoding();

        return true;
    }

    std::vector<std::pair<std::string, std::string>> GetReport() { return m_report; }

    static UIControlParams GenUiControlParams(DataObject::Pointer dataObj) {
        UIControlParams params;
        params.showReport = false;
        for (int i = 0; i < dataObj->GetAttributeSet()->GetNumberOfAttributes() + 2; i++) {
            iGame::FloatErrorControlParameters p;

            if (i == 0) {
                p.dataName = "全体数据";
                p.isKeyElement = std::vector<bool>();
            } else if (i == 1) {
                p.dataName = "顶点坐标";
                p.isKeyElement =
                        std::vector<bool>(iGame::DynamicCast<iGame::PointSet>(dataObj)->GetNumberOfPoints(), false);

            } else {
                auto attr = dataObj->GetAttributeSet()->GetAttribute(i - 2);
                p.dataName = attr.pointer->GetName();
                p.isKeyElement = std::vector<bool>(attr.pointer->GetNumberOfElements(), false);
            }

            p.lossyMode = iGame::LossyMode::MantissaTruncation;
            p.errorMode = iGame::ErrorMode::None;
            p.globalQuantizeLevel = 0;
            p.criticalQuantizeLevel = 0;
            p.normalQuantizeLevel = 0;
            params.errorBoundSetting.push_back(p);
        }
        return params;
    }

private:
    // I/O
    DataObject::Pointer m_DataObj;
    std::unique_ptr<MeshEncoderAdapter> m_EncoderAdapter;
    EncodedMeshData::Pointer m_encodedData;

    // params
    FloatErrorControlParameters m_geomErrorControl;
    std::vector<FloatErrorControlParameters> m_attrErrorControl;
    UIControlParams m_uiControlParams;
    bool m_hasUIControlParams = false;

    // reports
    bool m_visualError;
    bool m_showReport;
    std::vector<std::pair<std::string, std::string>> m_report;


    // region caller
    bool InitializeEncoder() {
        m_DataObj = GetInput(0);
        if (!m_DataObj) { return false; }

        m_encodedData = EncodedMeshData::New();
        m_EncoderAdapter = std::make_unique<MeshEncoderAdapter>(m_DataObj);

        InitParams();

        if (m_hasUIControlParams) { LoadUIControlParams(m_uiControlParams); }

        InitAdjacencyScore();
        return true;
    }

    void EncodePayloads(PayloadBuffer& geomPayload, PayloadBuffer& topoPayload, PayloadBuffer& attrPayload,
                        PayloadBuffer& paramPayload, std::vector<unsigned int>& pointIdRemap,
                        std::vector<unsigned int>& topCellIdsRemap, std::vector<unsigned int>& bottomCellIdRemap) {

        this->GeomEncoder(geomPayload, pointIdRemap);
        this->TopoEncoder(topoPayload, pointIdRemap, topCellIdsRemap, bottomCellIdRemap);
        this->AttrEncoder(attrPayload, pointIdRemap, topCellIdsRemap, bottomCellIdRemap);
        this->ParamsEncoder(paramPayload);
    }

    void CompressAndWritePayloads(PayloadBuffer& geomPayload, PayloadBuffer& topoPayload, PayloadBuffer& attrPayload,
                                  PayloadBuffer& paramPayload) {

        int compressLevel = 1;
        int numThreads = ThreadPool::GetDefaultThreadCount();

        PayloadBuffer geomCompressed(PayloadType::kGeometryBrick);
        PayloadBuffer topoCompressed(PayloadType::kTopologyBrick);
        PayloadBuffer attrCompressed(PayloadType::kAttributeBrick);
        PayloadBuffer paramCompressed(PayloadType::kParameterSet);

        std::vector<std::future<void>> result;
        ThreadPool* tp = ThreadPool::Instance();
        std::atomic<float> progress(0.8);

        // LZMA compression (old implementation)
        // result.push_back(tp->Commit(
        //         [&]() -> void { MeshCodecLZMA::Compress(geomCompressed, geomPayload, compressLevel, numThreads); }));
        // result.push_back(tp->Commit(
        //         [&]() -> void { MeshCodecLZMA::Compress(topoCompressed, topoPayload, compressLevel, numThreads); }));
        // result.push_back(tp->Commit(
        //         [&]() -> void { MeshCodecLZMA::Compress(attrCompressed, attrPayload, compressLevel, numThreads); }));
        // result.push_back(tp->Commit(
        //         [&]() -> void { MeshCodecLZMA::Compress(paramCompressed, paramPayload, compressLevel, numThreads); }));

        // ZSTD compression (new implementation)
        result.push_back(tp->Commit(
                [&]() -> void { MeshCodecZSTD::Compress(geomCompressed, geomPayload, compressLevel, numThreads); }));
        result.push_back(tp->Commit(
                [&]() -> void { MeshCodecZSTD::Compress(topoCompressed, topoPayload, compressLevel, numThreads); }));
        result.push_back(tp->Commit(
                [&]() -> void { MeshCodecZSTD::Compress(attrCompressed, attrPayload, compressLevel, numThreads); }));
        result.push_back(tp->Commit(
                [&]() -> void { MeshCodecZSTD::Compress(paramCompressed, paramPayload, compressLevel, numThreads); }));

        for (int i = 0; i < result.size(); i++) {
            result[i].wait();
            progress += 0.04;
            UpdateProgress(progress);
        }

        WriteBuf(paramCompressed);
        WriteBuf(geomCompressed);
        WriteBuf(topoCompressed);
        WriteBuf(attrCompressed);
    }

    void FinalizeEncoding() {
        UpdateProgress(1.0);
        SetOutput(m_encodedData);

        auto calCR = [this]() -> void {
            long long sourceSize = -1;

            // 1) 优先使用读取阶段写入的 FileSize 属性
            if (auto fileSizeProperty = this->m_DataObj->GetPropertys()->GetProperty("FileSize")) {
                sourceSize = fileSizeProperty->Get<long long>();
            }

            // 2) 若缺失或无效，尝试通过 FilePath 读取磁盘文件大小（适配 CGNS 等路径）
            if (sourceSize <= 0) {
                if (auto filePathProperty = this->m_DataObj->GetPropertys()->GetProperty("FilePath")) {
                    const std::string filePath = filePathProperty->Get<std::string>();
                    std::error_code ec;
                    auto sz = std::filesystem::file_size(std::filesystem::path(filePath), ec);
                    if (!ec) sourceSize = static_cast<long long>(sz);
                }
            }

            // 3) 兜底：用对象的实际内存占用估算（保证报告不空）
            if (sourceSize <= 0) {
                sourceSize = static_cast<long long>(this->m_DataObj->GetRealMemorySize());
            }

            if (sourceSize > 0) {
                long long compressSize = static_cast<long long>(m_encodedData->m_Buffers.size());
                double cr = compressSize * 1.0 / sourceSize;
                std::cout << "compress rate: " << cr << std::endl;

                m_report.push_back(std::make_pair("压缩率", std::format("{:.2f}%", cr * 100.0)));
            } else {
                // 源大小不可用时，明确给出不可用提示，避免用户误解
                m_report.push_back(std::make_pair("压缩率", std::string("N/A")));
            }
        };

        if (m_showReport) calCR();
    }
    // endregion

    // region I/O
    void WriteBuf(const PayloadBuffer& buf) {
        uint32_t length = uint32_t(buf.size());

        // 计算总需要的字节数: 1字节类型 + 4字节长度 + payload数据
        IGsize totalSize = 1 + 4 + length;

        // 获取当前 m_Buffers 的大小，作为新数据的起始位置
        IGsize currentSize = m_encodedData->m_Buffers.size();

        // 扩展 m_Buffers 以容纳新数据
        m_encodedData->m_Buffers.resize(currentSize + totalSize);

        // 获取数据指针
        unsigned char* data = m_encodedData->m_Buffers.data() + currentSize;
        IGsize offset = 0;

        // 写入 payload 类型 (1 字节)
        data[offset++] = static_cast<unsigned char>(buf.type);

        // 写入长度 (4 字节，大端序)
        data[offset++] = static_cast<unsigned char>(length >> 24);
        data[offset++] = static_cast<unsigned char>(length >> 16);
        data[offset++] = static_cast<unsigned char>(length >> 8);
        data[offset++] = static_cast<unsigned char>(length >> 0);

        // 写入 payload 数据
        if (length > 0) { std::memcpy(data + offset, buf.data(), length); }
    }

    template<typename T>
    void AddErrorReport(const std::vector<T>& source, const std::vector<T>& quantized,
                        const FloatParameters& floatParams, const FloatErrorControlParameters& errorParams,
                        std::string dataName) {
        if (m_showReport) {
            float keyError, nonKeyError;
            FloatCodecError::TotalError(source, quantized, floatParams, errorParams, keyError, nonKeyError);

			/*
			if (floatParams.errorMode == ErrorMode::KeyArea) {
				m_report.push_back(
						std::make_pair(dataName + " 相对误差",
									   std::format("▲{:.10f}% ■{:.10f}%", keyError * 100.0, nonKeyError * 100.0)));
			} else {
				m_report.push_back(std::make_pair(dataName + " 相对误差", std::format("{:.10f}%", keyError)));
			}
			*/

			// 新逻辑：无论是否 KeyArea，统一使用整体平均相对误差，输出单一百分比
			const float errPercent = keyError * 100.0f;
			m_report.push_back(
					std::make_pair(dataName + " 相对误差", std::format("{:.10f}%", errPercent)));
        }
    }

    // endregion

    // region params control
    void LoadUIControlParams(const UIControlParams& uiConParams) {
        m_showReport = uiConParams.showReport;
        m_codecParams.geomParams.lossyMode = uiConParams.errorBoundSetting[1].lossyMode;
        m_codecParams.geomParams.errorMode = uiConParams.errorBoundSetting[1].errorMode;
        m_codecParams.geomParams.globalQuantizeLevel = uiConParams.errorBoundSetting[1].globalQuantizeLevel;
        m_codecParams.geomParams.criticalQuantizeLevel = uiConParams.errorBoundSetting[1].criticalQuantizeLevel;
        m_codecParams.geomParams.normalQuantizeLevel = uiConParams.errorBoundSetting[1].normalQuantizeLevel;
        m_geomErrorControl = uiConParams.errorBoundSetting[1];

        m_attrErrorControl.resize(m_codecParams.attrCount);
        for (int i = 2; i < uiConParams.errorBoundSetting.size(); i++) {
            auto& attrParam = m_codecParams.attrParams[i - 2];
            attrParam.lossyMode = uiConParams.errorBoundSetting[i].lossyMode;
            attrParam.errorMode = uiConParams.errorBoundSetting[i].errorMode;
            attrParam.globalQuantizeLevel = uiConParams.errorBoundSetting[i].globalQuantizeLevel;
            attrParam.criticalQuantizeLevel = uiConParams.errorBoundSetting[i].criticalQuantizeLevel;
            attrParam.normalQuantizeLevel = uiConParams.errorBoundSetting[i].normalQuantizeLevel;

            m_attrErrorControl[i - 2] = uiConParams.errorBoundSetting[i];
        }
    }

    void InitParams() {
        // 网格类型
        this->m_codecParams.meshType = this->m_EncoderAdapter->GetMeshType();
        // multiblock网格类型暂不支持
        assert(this->m_codecParams.meshType == IG_SURFACE_MESH || this->m_codecParams.meshType == IG_VOLUME_MESH ||
               this->m_codecParams.meshType == IG_STRUCTURED_MESH ||
               this->m_codecParams.meshType == IG_UNSTRUCTURED_MESH || this->m_codecParams.meshType == IG_POINT_SET);

        // 点云特殊处理
        if (this->m_codecParams.meshType == IG_POINT_SET) {
            this->m_codecParams.topoParams.isSecondaryIndex = false;
            this->m_codecParams.topoParams.fixedCellSize = -1;
        } else {
            this->m_codecParams.topoParams.isSecondaryIndex = this->m_EncoderAdapter->IsSecondaryIndexPolyhedronMesh();
            this->m_codecParams.topoParams.fixedCellSize = this->m_EncoderAdapter->GetFixedCellSize();
        }

        if (this->m_codecParams.meshType == IG_STRUCTURED_MESH) {
            std::memcpy(&this->m_codecParams.structuredMeshParams.axisSize,
                        DynamicCast<StructuredMesh>(this->m_DataObj)->GetDimensionSize(), 3 * sizeof(int));
        }

        // 顶点坐标
        this->m_codecParams.geomParams.valueSize = sizeof(float); // float
        this->m_codecParams.geomParams.elementCount = this->m_EncoderAdapter->GetNumberOfPoints();
        this->m_codecParams.geomParams.dimension = 3;

        // 属性参数
        auto allAttrs = this->m_DataObj->GetAttributeSet()->GetAllAttributes();
        for (int dataIndex = 0; dataIndex < allAttrs->GetNumberOfElements(); dataIndex++) {
            AttributeSet::Attribute attr = allAttrs->GetElement(dataIndex);
            AttrParameters attrParams;

            std::memset(attrParams.name, '\0', sizeof(attrParams.name));
            attr.pointer->GetName().copy(attrParams.name, sizeof(attrParams.name) - 1); // 保留一位给\0

            attrParams.dimension = attr.pointer->GetDimension();
            attrParams.type = attr.type;
            attrParams.attachmentType = attr.attachmentType;
            attrParams.valueSize = attr.pointer->GetArrayTypedSize();
            attrParams.elementCount = attr.pointer->GetNumberOfElements();

            this->m_codecParams.attrParams.push_back(attrParams);
        }
        this->m_codecParams.attrCount = this->m_codecParams.attrParams.size();
    }

    // endregion

    // region main encoders
    void ParamsEncoder(PayloadBuffer& payload) {
        ParametersWoAttr paramsWoAttr = static_cast<ParametersWoAttr>(this->m_codecParams);
        IGsize staticSize = sizeof(ParametersWoAttr);
        IGsize dynamicSize = this->m_codecParams.attrParams.size() * sizeof(AttrParameters);

        payload.resize(staticSize + dynamicSize);

        // 写入静态数据
        std::memcpy(payload.data(), &paramsWoAttr, staticSize);
        UpdateProgress(0.7);
        // 写入动态数据
        if (!this->m_codecParams.attrParams.empty()) {
            std::memcpy(payload.data() + staticSize, this->m_codecParams.attrParams.data(), dynamicSize);
        }
        UpdateProgress(0.8);
    }

    void GeomEncoder(PayloadBuffer& payload, std::vector<unsigned int>& pointIdRemap) {
        Points::Pointer points = this->m_EncoderAdapter->GetPoints();
        IGsize pointCount = this->m_codecParams.geomParams.elementCount;
        IGsize pointBufferSize = pointCount * this->m_codecParams.geomParams.dimension; // 一个顶点3个维度

        std::vector<float> remappedPointBuffer(pointBufferSize);

        // 结构化网格不做顶点重排序，保持原始顺序
        if (this->m_codecParams.meshType == IG_STRUCTURED_MESH) {
            // pointIdRemap保持为空，表示不需要重映射
            pointIdRemap.clear();

            // 直接复制原始顶点坐标
            std::memcpy(remappedPointBuffer.data(), points->RawPointer(), pointBufferSize * sizeof(float));

            UpdateProgress(0.1);
        } else {
            // 非结构化网格：进行顶点重排序优化
            // 重映射
            pointIdRemap.resize(pointCount);
            meshopt_spatialSortRemap(pointIdRemap.data(), points->RawPointer(), pointCount, sizeof(Vector3f));

            UpdateProgress(0.1);

            // 部署重映射
            meshopt_remapVertexBuffer(remappedPointBuffer.data(), points->RawPointer(), pointCount, sizeof(Vector3f),
                                      pointIdRemap.data());
        }

        // key重映射 (仅非结构化网格需要)
        if (!m_geomErrorControl.isKeyElement.empty() && !pointIdRemap.empty()) {
            std::vector<bool> remappedIsKey(pointCount, false);

            // 将原始关键元素标记转移到重映射后的位置
            ThreadPool::parallelFor(0, pointCount, [&](int start, int end) -> void {
                for (int j = start; j < end; j++) {
                    // 如果原始元素是关键元素，则标记 remap 后的对应元素也为关键元素
                    if (m_geomErrorControl.isKeyElement[j]) { remappedIsKey[pointIdRemap[j]] = true; }
                }
            });

            // 用重映射后的isKey替换原始的isKey
            m_geomErrorControl.isKeyElement = std::move(remappedIsKey);
        }

        std::vector<unsigned char> encodedFloat;

        std::vector<float> preserve = remappedPointBuffer;
        MeshOptFloatCodec::FloatEncoder(encodedFloat, remappedPointBuffer, m_codecParams.geomParams,
                                        m_geomErrorControl);

        AddErrorReport(preserve, remappedPointBuffer, m_codecParams.geomParams, m_geomErrorControl, "顶点坐标");

        UpdateProgress(0.2);

        payload.resize(encodedFloat.size());
        std::memcpy(payload.data(), encodedFloat.data(), encodedFloat.size());
    }

    void AttrEncoder(PayloadBuffer& payload, const std::vector<unsigned int>& pointRemap,
                     const std::vector<unsigned int>& topCellRemap, const std::vector<unsigned int>& bottomCellRemap) {
        ElementArray<AttributeSet::Attribute>::Pointer attrs = this->m_DataObj->GetAttributeSet()->GetAllAttributes();
        std::vector<std::vector<unsigned char>> outFloats(this->m_codecParams.attrParams.size());

        auto remapAttributeValues = [&](auto attrArray, auto& remappedBuffer, AttrParameters& params, int attrIndex) {
            if (this->m_codecParams.meshType == IG_STRUCTURED_MESH) {
                // 结构化网格：不需要重映射，直接复制原始数据
                size_t valueCount = params.dimension * params.elementCount;
                remappedBuffer.resize(valueCount);

                ThreadPool::parallelFor(0, params.elementCount, [&](int start, int end) -> void {
                    for (int j = start; j < end; j++) {
                        for (int k = 0; k < params.dimension; k++) {
                            remappedBuffer[j * params.dimension + k] = attrArray->GetValue(j * params.dimension + k);
                        }
                    }
                });
                // isKeyElement保持不变，不需要重映射
            } else {
                // 非结构化网格：需要重映射
                const auto& remapArray = params.attachmentType == IG_POINT ? pointRemap : topCellRemap;
                size_t valueCount = params.dimension * remapArray.size();
                remappedBuffer.resize(valueCount);

                size_t remappedElementCount = remapArray.size();
                std::vector<bool> remappedIskey(remappedElementCount, false);

                ThreadPool::parallelFor(0, params.elementCount, [&](int start, int end) -> void {
                    for (int j = start; j < end; j++) {
                        igIndex remapIndex = remapArray[j];

                        // 如果原始元素是关键元素，则标记 remap 后的对应元素也为关键元素
                        if (m_attrErrorControl[attrIndex].isKeyElement[j]) { remappedIskey[remapIndex] = true; }

                        for (int k = 0; k < params.dimension; k++) {
                            remappedBuffer[remapIndex * params.dimension + k] =
                                    attrArray->GetValue(j * params.dimension + k);
                        }
                    }
                });

                // 用 remap 后的 iskey 替换原始的 iskey
                m_attrErrorControl[attrIndex].isKeyElement = std::move(remappedIskey);
            }
        };

        std::mutex reportMutex;
        ProgressParallelFor(0, this->m_codecParams.attrParams.size(), 0.4, 0.6, [&](int start, int end) -> void {
            for (int i = start; i < end; i++) {
                AttributeSet::Attribute& attr = attrs->GetElement(i);
                auto& attrParams = this->m_codecParams.attrParams[i];
                auto& errorParams = this->m_attrErrorControl[i];

                // 部署remap
                std::vector<float> remappedFloatAttrBuffer;
                std::vector<double> remappedDoubleAttrBuffer;

                if (attrParams.valueSize == sizeof(float)) {
                    // 直接使用attr.pointer，因为remapAttributeValues使用的是GetValue()虚函数
                    remapAttributeValues(attr.pointer, remappedFloatAttrBuffer, attrParams, i);
                } else {
                    // 直接使用attr.pointer，因为remapAttributeValues使用的是GetValue()虚函数
                    remapAttributeValues(attr.pointer, remappedDoubleAttrBuffer, attrParams, i);
                }

                // 编码
                std::vector<unsigned char> encoded;

                if (attrParams.valueSize == sizeof(float)) {
                    std::vector<float> preserve = remappedFloatAttrBuffer;
                    MeshOptFloatCodec::FloatEncoder(encoded, remappedFloatAttrBuffer, attrParams, errorParams);

                    {
                        std::lock_guard<std::mutex> lock(reportMutex);
                        AddErrorReport(preserve, remappedFloatAttrBuffer, attrParams, errorParams,
                                       attr.pointer->GetName());
                    }
                } else if (attrParams.valueSize == sizeof(double)) {
                    std::vector<double> preserve = remappedDoubleAttrBuffer;
                    MeshOptFloatCodec::FloatEncoder(encoded, remappedDoubleAttrBuffer, attrParams, errorParams);

                    {
                        std::lock_guard<std::mutex> lock(reportMutex);
                        AddErrorReport(preserve, remappedDoubleAttrBuffer, attrParams, errorParams,
                                       attr.pointer->GetName());
                    }
                }

                // 只靠count就足以读取
                attrParams.binaryCount = encoded.size() * sizeof(uint8_t);
                outFloats[i] = encoded;
            }
        });

        UpdateProgress(0.6);

        size_t currentPayloadCursor = 0;
        for (int i = 0; i < this->m_codecParams.attrParams.size(); i++) {
            payload.resize(currentPayloadCursor + outFloats[i].size());
            std::memcpy(payload.data() + currentPayloadCursor, outFloats[i].data(), outFloats[i].size());
            currentPayloadCursor += outFloats[i].size();
        }
    }

    void TopoEncoder(PayloadBuffer& payload, const std::vector<unsigned int> pointIdRemap,
                     std::vector<unsigned int>& topCellRemap,   // 最高级cell remap
                     std::vector<unsigned int>& bottomCellRemap // 最低级cell remap
    ) {
        if (this->m_codecParams.meshType == IG_POINT_SET) {
            payload.resize(0);
            UpdateProgress(0.4);
            return;
        }

        // 结构化网格：不需要编码cell连接关系，只需axisSize即可自动生成
        if (this->m_codecParams.meshType == IG_STRUCTURED_MESH) {
            // topCellRemap和bottomCellRemap保持为空
            topCellRemap.clear();
            bottomCellRemap.clear();

            // 不编码cell buffer和offset，payload为空
            payload.resize(0);
            this->m_codecParams.topoParams.topCellBufferSize = 0;
            this->m_codecParams.topoParams.topCellBufferBinaryCount = 0;
            this->m_codecParams.topoParams.topCellSizeBinaryCount = 0;
            this->m_codecParams.topoParams.cellTypeBinaryCount = 0;

            UpdateProgress(0.4);
            return;
        }

        // 输出
        std::vector<unsigned char> outputTopo;

        // 判断是否需要启用二级索引
        if (this->m_EncoderAdapter->IsSecondaryIndexPolyhedronMesh()) {
            // 顺序 bottomCellBuffer bottomCellOffset topCellBuffer topCellOffset

            // 必须先执行面 -> 点 随后是 体 -> 面

            // 获取拆解索引列
            std::vector<unsigned int> volume2facesIndex;
            std::vector<unsigned int> volume2facesOffset;
            std::vector<unsigned int> face2pointsIndex;
            std::vector<unsigned int> face2pointsOffset;
            this->m_EncoderAdapter->SplitSecondaryIndex(volume2facesIndex, volume2facesOffset, face2pointsIndex,
                                                        face2pointsOffset);

            // 重排序顶点
            std::vector<unsigned int> remappedf2p;
            remappedf2p.resize(face2pointsIndex.size());

            for (size_t i = 0; i < face2pointsIndex.size(); ++i) { remappedf2p[i] = pointIdRemap[face2pointsIndex[i]]; }

            UpdateProgress(0.25);

            // 编码 面 -> 点
            IndexNOffsetEncoder(outputTopo, bottomCellRemap, this->m_EncoderAdapter->GetNumberOfFaces(),
                                this->m_EncoderAdapter->GetNumberOfPoints(), remappedf2p,
                                this->m_codecParams.topoParams.bottomCellBufferBinaryCount,
                                this->m_codecParams.topoParams.bottomCellBufferPadding, -1,
                                this->m_codecParams.topoParams.bottomCellSizeBinaryCount, face2pointsOffset);

            UpdateProgress(0.3);

            this->m_codecParams.topoParams.bottomCellBufferSize = remappedf2p.size();

            // 重排序面
            std::vector<unsigned int> remappedv2f;
            remappedv2f.resize(volume2facesIndex.size());

            for (size_t i = 0; i < volume2facesIndex.size(); ++i) {
                remappedv2f[i] = bottomCellRemap[volume2facesIndex[i]];
            }

            // 编码 体 -> 面
            IndexNOffsetEncoder(outputTopo, topCellRemap, this->m_EncoderAdapter->GetNumberOfCells(),
                                this->m_EncoderAdapter->GetNumberOfFaces(), remappedv2f,
                                this->m_codecParams.topoParams.topCellBufferBinaryCount,
                                this->m_codecParams.topoParams.topCellBufferPadding, -1,
                                this->m_codecParams.topoParams.topCellSizeBinaryCount, volume2facesOffset);

            UpdateProgress(0.4);

            this->m_codecParams.topoParams.topCellBufferSize = remappedv2f.size();
        } else {
            // 顺序 CellBuffer CellOffset CellTypes
            this->m_codecParams.topoParams.topCellBufferSize = this->m_EncoderAdapter->GetCellIdBufferSize();
            IGsize pointCount = this->m_EncoderAdapter->GetNumberOfPoints();
            IGsize cellCount = this->m_EncoderAdapter->GetNumberOfCells();
            IGsize cellBufferSize = this->m_EncoderAdapter->GetCellIdBufferSize();
            IGsize cellOffsetSize = this->m_EncoderAdapter->GetCellIdOffsetSize();
            UnsignedIntArray::Pointer cellIdOffset = this->m_EncoderAdapter->GetCellIdOffset();

            // 应用顶点重映射
            IdArray::Pointer cellIdBuffer = this->m_EncoderAdapter->GetCellIdBuffer();
            std::vector<unsigned int> remappedc2v;
            remappedc2v.resize(cellBufferSize);

            for (size_t i = 0; i < cellBufferSize; ++i) { remappedc2v[i] = pointIdRemap[cellIdBuffer->GetId(i)]; }

            if (this->m_EncoderAdapter->IsFixedCellSize()) {
                IndexNOffsetEncoder(outputTopo, topCellRemap, cellCount, pointCount, remappedc2v,
                                    this->m_codecParams.topoParams.topCellBufferBinaryCount,
                                    this->m_codecParams.topoParams.topCellBufferPadding,
                                    this->m_EncoderAdapter->GetFixedCellSize(),
                                    this->m_codecParams.topoParams.topCellSizeBinaryCount);
            } else {
                std::vector<unsigned int> volume2pointsOffset;
                volume2pointsOffset.resize(cellOffsetSize);
                memcpy(volume2pointsOffset.data(), cellIdOffset->RawPointer(),
                       cellIdOffset->GetNumberOfValues() * sizeof(unsigned int));

                IndexNOffsetEncoder(outputTopo, topCellRemap, cellCount, pointCount, remappedc2v,
                                    this->m_codecParams.topoParams.topCellBufferBinaryCount,
                                    this->m_codecParams.topoParams.topCellBufferPadding, -1,
                                    this->m_codecParams.topoParams.topCellSizeBinaryCount, volume2pointsOffset);
            }

            UpdateProgress(0.3);

            // 如果非结构化 要写入type 非结构化与是否存在fixed cell size没有关系
            this->m_codecParams.topoParams.cellTypeBinaryCount = 0;
            if (this->m_codecParams.meshType == IG_UNSTRUCTURED_MESH) {
                std::vector<unsigned char> encodeBuffer;
                UnsignedIntArray::Pointer cellTypes = this->m_EncoderAdapter->GetCellTypes();
                // 应用map做对cell类型做重排序
                std::vector<uint32_t> remappedCellTypes(cellCount);
                for (int i = 0; i < cellCount; i++) { remappedCellTypes[topCellRemap[i]] = cellTypes->GetValue(i); }

                encodeBuffer.clear();
                this->CellTypeEncoder(encodeBuffer, remappedCellTypes);
                outputTopo.insert(outputTopo.end(), encodeBuffer.begin(), encodeBuffer.end());
                this->m_codecParams.topoParams.cellTypeBinaryCount = encodeBuffer.size();
            }

            UpdateProgress(0.4);
        }

        payload.resize(outputTopo.size());
        std::memcpy(payload.data(), outputTopo.data(), outputTopo.size());
    }
    // endregion

    // region sub-encoders
    // 和常规的delta encoder有点区别 产生的结果是每个cell的点的数量
    void DeltaEncoder(std::vector<uint32_t>& dest, std::vector<uint32_t> source) {
        IGsize destSize = source.size() - 1;
        dest.resize(destSize);
        ThreadPool::parallelFor(0, destSize, [&](int start, int end) -> void {
            int prev = source[start];
            for (int i = start; i < end; i++) {
                const uint32_t& cur = source[i + 1];
                dest[i] = cur - prev;
                prev = cur;
            }
        });
    }

    // import from meshoptimizer
    void encodeVByte(unsigned char*& data, unsigned int v) {
        // encode 32-bit value in up to 5 7-bit groups
        do {
            *data++ = (v & 127) | (v > 127 ? 128 : 0);
            v >>= 7;
        } while (v);
    }

    void RunLengthEncoder(std::vector<unsigned char>& dest, std::vector<uint32_t> source) {
        // rle编码按理说不一定可以压缩 有时反而会导致数据变长 这里先不考虑
        // 由于采用了push_back 所以不用resize dest
        dest.resize(source.size() * sizeof(uint32_t) + 1024);
        unsigned char* startPointer = dest.data();
        unsigned char* destPointer = dest.data();


        int sizeCount = source.size();
        int maxThreadSize = ThreadPool::GetDefaultThreadCount(); // 线程数量

        // 对于 AAAABBBBCCCCCC
        // 输出形如 0 4 8 14
        std::vector<std::vector<int>> threadRusult(
                maxThreadSize); // 线程结果 按顺序存储了每个线程检测到的多个数据变换点的位置

        ThreadPool::parallelFor(0, sizeCount, maxThreadSize, [&](int start, int end, int threadIndex) -> void {
            std::vector<int>& curResult = threadRusult[threadIndex];

            // 找上一个block中最后一个值 判断是否本block的第一个值就是变换点
            bool isFirstValue = (start == 0);
            uint32_t prev;

            if (!isFirstValue) { prev = source[start - 1]; }

            for (int i = start; i < end; i++) {
                // 写入threadResult
                if (!isFirstValue && source[i] != prev) {
                    curResult.push_back(i);
                    prev = source[i];
                }
                if (isFirstValue) {
                    curResult.push_back(0);
                    prev = source[i];

                    isFirstValue = false;
                }
            }
        });

        std::vector<int> offset; // 方便遍历
        for (int i = 0; i < threadRusult.size(); i++) {
            std::vector<int>& curResult = threadRusult[i];
            offset.insert(offset.end(), curResult.begin(), curResult.end());
        }
        offset.push_back(sizeCount);
        threadRusult.clear();

        // 编码
        // 先写入转义符号 用于声明转义
        //dest.push_back(minByte);
        int outputSize = 0;
        for (int i = 0; i < offset.size() - 1; i++) {
            const uint32_t curInt = source[offset[i]];
            unsigned int length = offset[i + 1] - offset[i];

            std::memcpy(destPointer, &curInt, sizeof(uint32_t)); // 写入cellsize
            destPointer += sizeof(uint32_t);
            this->encodeVByte(destPointer, length); // 写入长度 末尾block的高位为0
        }

        dest.resize(destPointer - startPointer);
    }

    // 对于types 直接执行rle
    void CellTypeEncoder(std::vector<unsigned char>& dest, std::vector<uint32_t> source) {
        this->RunLengthEncoder(dest, source);
    }

    // 对于offset 执行 delta + rle
    void CellSizeEncoder(std::vector<unsigned char>& dest, std::vector<uint32_t> source) {
        std::vector<uint32_t> delta;
        this->DeltaEncoder(delta, source);
        this->RunLengthEncoder(dest, delta);
    }

    void CellBufferEncoder(std::vector<unsigned char>& dest, std::vector<uint32_t>& source, int& bufferPadding) {
        //std::vector<unsigned char> ibuf(meshopt_encodeIndexBufferBound(bufferSize, pointCount));
        //ibuf.resize(meshopt_encodeIndexBuffer(&ibuf[0], ibuf.size(), uintidbuffer.data(), bufferSize));

        //std::vector<char> binary(ibuf.size());
        //std::memcpy(binary.data(), ibuf.data(), ibuf.size());
        //this->m_BytestreamFile.write(binary.data(), binary.size());

        //FastPForLib::IntegerCODEC& codec = *FastPForLib::CODECFactory::getFromName("simdfastpfor256");
        //size_t compressedsize = dest.size();
        //codec.encodeArray(source.data(), source.size(), dest.data(),
        //    compressedsize);
        //dest.resize(compressedsize);
        //dest.shrink_to_fit();

        // 经过实验 meshopt的index buffer encoder在小规模mesh上略低于fastpfor(一种通用整数编码器)
        // 但在大规模mesh上显著超越fastpfor
        // meshopt的encoder以三角形为单元处理 其算法具有一定的对称性(指一次处理一个边和一个点)
        // 所以选择用padding来解决index buffer size不匹配问题 而不是大规模改造算法

        int pointCount = this->m_codecParams.geomParams.elementCount;

        // padding
        int bufferSize = source.size();
        int padding = (3 - bufferSize % 3) % 3;
        bufferPadding = padding;
        for (int i = 0; i < padding; i++) { source.push_back(i); }
        bufferSize += padding;
        // opt操作已经在外部完成

        dest.resize(IndexBufferCodec::encodeIndexBufferBound(bufferSize, pointCount));
        dest.resize(IndexBufferCodec::encodeIndexBuffer(&dest[0], dest.size(), source.data(), bufferSize));

        source.resize(bufferSize - padding);
    }

    void IndexNOffsetEncoder(std::vector<unsigned char>& outputTopo, std::vector<unsigned int>& cellRemap,

                             int cellCount, int childCount,

                             std::vector<unsigned int> indices, IGsize& indicesBinarySize, int& bufferPadding,

                             int fixedCellSize, IGsize& cellSizeBinarySize, std::vector<unsigned int> offsets = {}) {
        // !! 注意 !! 由于Adjacency只支持单级的point/cell邻接关系构造
        // 所以对于cgns这种多级映射数据结构 必需单独构造adj 也因此无法在构造编码器时就产生adj
        // 在此输入的cell buffer, offset应在调用时就确定了是point/cell, point/face, face/cell关系

        // 重排序以及写入
        std::vector<unsigned int> optCellBuffer(indices.size());
        std::vector<unsigned int> optCellOffset(offsets.size());
        cellRemap.resize(cellCount);

        if (fixedCellSize != -1) {
            auto mca = std::make_unique<MeshCodecAdjacency>(indices.data(), indices.size(), childCount, fixedCellSize);
            this->OptimizeCellVertexCache(optCellBuffer.data(), cellRemap.data(), indices.data(), indices.size(),
                                          childCount, fixedCellSize, mca->GetAdjacencyData());
        } else {
            auto mca = std::make_unique<MeshCodecAdjacency>(indices.data(), offsets.data(), indices.size(),
                                                            offsets.size(), childCount, cellCount);
            this->OptimizeHybirdCellVertexCache(optCellBuffer.data(), optCellOffset.data(), cellRemap.data(),
                                                indices.data(), offsets.data(), indices.size(), offsets.size(),
                                                childCount, cellCount, mca->GetAdjacencyData());
        }

        std::vector<unsigned char> encodeBuffer;

        encodeBuffer.clear();
        this->CellBufferEncoder(encodeBuffer, optCellBuffer, bufferPadding);
        outputTopo.insert(outputTopo.end(), encodeBuffer.begin(), encodeBuffer.end());
        //this->m_Params.topoParams.cellBufferBinaryCount = encodeBuffer.size();
        indicesBinarySize = encodeBuffer.size();

        cellSizeBinarySize = 0;
        if (fixedCellSize == -1) {
            // 写入size 非结构化网格也有可能是定长size
            // 所以这里直接写就行了 如果是定长size 这里就会被自动跳过
            encodeBuffer.clear();
            // 写入offset
            this->CellSizeEncoder(encodeBuffer, optCellOffset); // 在解码的时候处理完rle就结束了
            outputTopo.insert(outputTopo.end(), encodeBuffer.begin(), encodeBuffer.end());

            cellSizeBinarySize = encodeBuffer.size();
            //this->m_Params.topoParams.cellSizeBinaryCount = encodeBuffer.size();
        }
    }
    // endregion

    // region cache optimization
    // 下列与cache优化相关的代码改造自 meshoptimizer(license in thirdparty\meshoptimizer)
    static const size_t kCacheSizeMax = 16;
    static const size_t kValenceMax = 8;
    struct VertexScoreTable {
        float cache[1 + kCacheSizeMax];
        float live[1 + kValenceMax];
    };
    VertexScoreTable kVertexScoreTableStrip;

    void InitAdjacencyScore() {
        this->kVertexScoreTableStrip = {
                {0.f, 1.000f, 1.000f, 1.000f, 0.453f, 0.561f, 0.490f, 0.459f, 0.179f, 0.526f, 0.000f, 0.227f, 0.184f,
                 0.490f, 0.112f, 0.050f, 0.131f},
                {0.f, 0.956f, 0.786f, 0.577f, 0.558f, 0.618f, 0.549f, 0.499f, 0.489f},
        };
    }

    float VertexScore(int cachePosition, unsigned int liveCellCount) {
        assert(cachePosition >= -1 && cachePosition < int(kCacheSizeMax));
        unsigned int liveCellClamped = liveCellCount < this->kValenceMax ? liveCellCount : this->kValenceMax;
        return this->kVertexScoreTableStrip.cache[1 + cachePosition] +
               this->kVertexScoreTableStrip.live[liveCellClamped];
    }

    unsigned int GetNextCellDeadEnd(unsigned int& inputCursor, const unsigned char* emittedFlags, size_t cellCount) {
        // input order
        while (inputCursor < cellCount) {
            if (!emittedFlags[inputCursor]) return inputCursor;

            ++inputCursor;
        }

        return ~0u;
    }

    // 不定长offset
    void OptimizeHybirdCellVertexCache(unsigned int* destCellBuffer, unsigned int* destCellOffset,
                                       unsigned int* destCellRemap, const unsigned int* sourceCellBuffer,
                                       const unsigned int* sourceCellOffsets, size_t bufferSize, size_t offsetCount,
                                       size_t pointCount, size_t cellCount,
                                       MeshCodecAdjacency::CellAdjacency adjacency) {
        meshopt_Allocator allocator;

        // guard for empty meshes
        if (bufferSize == 0 || pointCount == 0) return;

        // support in-place optimization
        if (destCellBuffer == sourceCellBuffer) {
            unsigned int* temp_copy = allocator.allocate<unsigned int>(bufferSize);
            memcpy(temp_copy, sourceCellBuffer, bufferSize * sizeof(unsigned int));
            sourceCellBuffer = temp_copy;
        }
        if (destCellOffset == sourceCellOffsets) {
            unsigned int* temp_copy = allocator.allocate<unsigned int>(offsetCount);
            memcpy(temp_copy, sourceCellOffsets, offsetCount * sizeof(unsigned int));
            sourceCellOffsets = temp_copy;
        }

        unsigned int cache_size = 16;
        assert(cache_size <= kCacheSizeMax);

        // live triangle counts; note, we alias adjacency.counts as we remove triangles after emitting them so the counts always match
        unsigned int* liveCellCount = adjacency.counts;

        // 已访问cell标记
        unsigned char* emittedFlags = allocator.allocate<unsigned char>(cellCount);
        memset(emittedFlags, 0, cellCount);

        // compute initial vertex scores
        float* vertexScores = allocator.allocate<float>(pointCount);

        for (size_t i = 0; i < pointCount; ++i) vertexScores[i] = this->VertexScore(-1, liveCellCount[i]);

        // compute cells scores
        float* cellScores = allocator.allocate<float>(cellCount);
        unsigned int maxIdPerCell = 0; // 用于稍后开双缓存
        for (size_t i = 0; i < cellCount; ++i) {
            cellScores[i] = 0;
            for (size_t j = sourceCellOffsets[i]; j < sourceCellOffsets[i + 1]; j++) {
                cellScores[i] += vertexScores[sourceCellBuffer[j]];
                maxIdPerCell = std::max(maxIdPerCell, sourceCellOffsets[i + 1] - sourceCellOffsets[i]);
            }
        }

        std::vector<unsigned int> cacheHolder(2 * (kCacheSizeMax + maxIdPerCell + 1));
        unsigned int* cache = cacheHolder.data();
        unsigned int* cacheNew = cacheHolder.data() + kCacheSizeMax + maxIdPerCell + 1;
        size_t cacheCount = 0;

        unsigned int currentCell = 0;
        unsigned int inputCursor = 1;

        unsigned int outputCell = 0;
        destCellOffset[0] = 0;
        while (currentCell != ~0u) {
            assert(outputCell < cellCount);
            size_t cacheWriteCursor = 0;
            for (size_t j = sourceCellOffsets[currentCell]; j < sourceCellOffsets[currentCell + 1]; j++) {
                destCellBuffer[destCellOffset[outputCell] + cacheWriteCursor] = sourceCellBuffer[j];
                cacheNew[cacheWriteCursor] = sourceCellBuffer[j];
                cacheWriteCursor++;
            }
            size_t currentCellVertexCount = sourceCellOffsets[currentCell + 1] - sourceCellOffsets[currentCell];
            destCellOffset[outputCell + 1] = destCellOffset[outputCell] + currentCellVertexCount;
            destCellRemap[currentCell] = outputCell;

            outputCell++;

            // update emitted flags
            emittedFlags[currentCell] = true;
            cellScores[currentCell] = 0;

            // 移动老cache的cell到cacheNew 但是当前cell的顶点不额外再次放入
            for (size_t i = 0; i < cacheCount; ++i) {
                unsigned int index = cache[i];
                cacheNew[cacheWriteCursor] = index;

                int indepIndex = 1;
                for (size_t j = sourceCellOffsets[currentCell]; j < sourceCellOffsets[currentCell + 1]; j++) {
                    if (index == sourceCellBuffer[j]) {
                        indepIndex = 0;
                        break;
                    }
                }
                cacheWriteCursor += indepIndex;
            }

            unsigned int* cacheTemp = cache;
            cache = cacheNew, cacheNew = cacheTemp;
            cacheCount = cacheWriteCursor > cache_size ? cache_size : cacheWriteCursor;

            // 从data中移除当前cell的vertex对当前cell的记录
            for (size_t k = 0; k < currentCellVertexCount; ++k) {
                unsigned int index = sourceCellBuffer[sourceCellOffsets[currentCell] + k];
                unsigned int* neighbors = &adjacency.data[0] + adjacency.offsets[index];
                size_t neighborsSize = adjacency.counts[index];
                for (size_t i = 0; i < neighborsSize; ++i) {
                    unsigned int cell = neighbors[i];
                    if (cell == currentCell) {
                        neighbors[i] = neighbors[neighborsSize - 1];
                        adjacency.counts[index]--;
                        break;
                    }
                }
            }

            unsigned int bestCell = ~0u;
            float bestCellScore = 0;

            // 更新被写入cache的顶点
            for (size_t i = 0; i < cacheWriteCursor; ++i) {
                unsigned int index = cache[i];

                // 如果当前顶点没有活动的邻接cell就跳过
                if (adjacency.counts[index] == 0) continue;

                // 已经写入cache的顶点数量cache_write可能多于cache size
                // 如果超出cache 其位置就记为-1
                int cachePosition = i >= cache_size ? -1 : int(i);

                // 通过计算顶点分数的方式 逐步更新cell分数
                float score = this->VertexScore(cachePosition, liveCellCount[index]);
                float scoreDiff = score - vertexScores[index];

                vertexScores[index] = score;

                // update scores of vertex triangles
                const unsigned int* neighborsBegin = &adjacency.data[0] + adjacency.offsets[index];
                const unsigned int* neighborsEnd = neighborsBegin + adjacency.counts[index];

                for (const unsigned int* it = neighborsBegin; it != neighborsEnd; ++it) {
                    unsigned int cell = *it;
                    assert(!emittedFlags[cell]);

                    float cellScore = cellScores[cell] + scoreDiff;
                    assert(cellScore > 0);

                    bestCell = bestCellScore < cellScore ? cell : bestCell;
                    bestCellScore = bestCellScore < cellScore ? cellScore : bestCellScore;

                    cellScores[cell] = cellScore;
                }
            }

            // step through input triangles in order if we hit a dead-end
            currentCell = bestCell;

            if (currentCell == ~0u) {
                currentCell = this->GetNextCellDeadEnd(inputCursor, &emittedFlags[0], cellCount);
            }
        }

        assert(inputCursor == cellCount);
        assert(outputCell == cellCount);
    }

    // 固定offset
    void OptimizeCellVertexCache(unsigned int* destCellBuffer, unsigned int* destCellRemap,
                                 const unsigned int* sourceCellBuffer, size_t bufferSize, size_t pointCount,
                                 size_t fixedCellSize, MeshCodecAdjacency::CellAdjacency adjacency) {
        assert(bufferSize % fixedCellSize == 0);

        meshopt_Allocator allocator;

        // guard for empty meshes
        if (bufferSize == 0 || pointCount == 0) return;

        // support in-place optimization
        if (destCellBuffer == sourceCellBuffer) {
            unsigned int* indicesCopy = allocator.allocate<unsigned int>(bufferSize);
            memcpy(indicesCopy, sourceCellBuffer, bufferSize * sizeof(unsigned int));
            sourceCellBuffer = indicesCopy;
        }

        unsigned int cacheSize = 16;
        assert(cacheSize <= kCacheSizeMax);

        size_t cellCount = bufferSize / fixedCellSize;

        // build adjacency information

        // live cell counts; note, we alias adjacency.counts as we remove cells after emitting them so the counts always match
        unsigned int* live_cells = adjacency.counts;

        // emitted flags
        unsigned char* emittedFlags = allocator.allocate<unsigned char>(cellCount);
        memset(emittedFlags, 0, cellCount);

        // compute initial vertex scores
        float* vertexScores = allocator.allocate<float>(pointCount);

        for (size_t i = 0; i < pointCount; ++i) vertexScores[i] = this->VertexScore(-1, live_cells[i]);

        // compute triangle scores
        float* cellScores = allocator.allocate<float>(cellCount);

        for (size_t i = 0; i < cellCount; ++i) {
            cellScores[i] = 0;
            for (size_t j = 0; j < fixedCellSize; j++) {
                cellScores[i] += vertexScores[sourceCellBuffer[i * fixedCellSize + j]];
            }
        }

        std::vector<unsigned int> cacheHolder(2 * (kCacheSizeMax + fixedCellSize + 1));
        unsigned int* cache = cacheHolder.data();
        unsigned int* cacheNew = cacheHolder.data() + kCacheSizeMax + fixedCellSize + 1;
        size_t cacheCount = 0;

        unsigned int currentCell = 0;
        unsigned int inputCursor = 1;

        unsigned int outputCell = 0;

        while (currentCell != ~0u) {
            assert(outputCell < cellCount);

            size_t cacheWrite = 0;
            for (size_t j = 0; j < fixedCellSize; j++) {
                destCellBuffer[outputCell * fixedCellSize + j] = sourceCellBuffer[currentCell * fixedCellSize + j];
                cacheNew[cacheWrite++] = sourceCellBuffer[currentCell * fixedCellSize + j];
            }
            destCellRemap[currentCell] = outputCell;
            outputCell++;

            // update emitted flags
            emittedFlags[currentCell] = true;
            cellScores[currentCell] = 0;

            // old cells
            for (size_t i = 0; i < cacheCount; ++i) {
                unsigned int index = cache[i];

                cacheNew[cacheWrite] = index;

                int indep_index = 1;
                for (size_t j = 0; j < fixedCellSize; j++) {
                    if (index == sourceCellBuffer[currentCell * fixedCellSize + j]) {
                        indep_index = 0;
                        break;
                    }
                }
                cacheWrite += indep_index;
            }

            unsigned int* cache_temp = cache;
            cache = cacheNew, cacheNew = cache_temp;
            cacheCount = cacheWrite > cacheSize ? cacheSize : cacheWrite;

            for (size_t k = 0; k < fixedCellSize; ++k) {
                unsigned int index = sourceCellBuffer[currentCell * fixedCellSize + k];

                unsigned int* neighbors = &adjacency.data[0] + adjacency.offsets[index];
                size_t neighborsSize = adjacency.counts[index];

                for (size_t i = 0; i < neighborsSize; ++i) {
                    unsigned int cell = neighbors[i];

                    if (cell == currentCell) {
                        neighbors[i] = neighbors[neighborsSize - 1];
                        adjacency.counts[index]--;
                        break;
                    }
                }
            }

            unsigned int bestCell = ~0u;
            float bestScore = 0;

            // update cache remappedPointBuffer, vertex scores and triangle scores, and find next best triangle
            for (size_t i = 0; i < cacheWrite; ++i) {
                unsigned int index = cache[i];

                // no need to update scores if we are never going to use this vertex
                if (adjacency.counts[index] == 0) continue;

                int cachePosition = i >= cacheSize ? -1 : int(i);

                // update vertex score
                float score = this->VertexScore(cachePosition, live_cells[index]);
                float scoreDiff = score - vertexScores[index];

                vertexScores[index] = score;

                // update scores of vertex triangles
                const unsigned int* neighborsBegin = &adjacency.data[0] + adjacency.offsets[index];
                const unsigned int* neighborsEnd = neighborsBegin + adjacency.counts[index];

                for (const unsigned int* it = neighborsBegin; it != neighborsEnd; ++it) {
                    unsigned int cell = *it;
                    assert(!emittedFlags[cell]);

                    float cellScore = cellScores[cell] + scoreDiff;
                    assert(cellScore > 0);

                    bestCell = bestScore < cellScore ? cell : bestCell;
                    bestScore = bestScore < cellScore ? cellScore : bestScore;

                    cellScores[cell] = cellScore;
                }
            }

            // step through input cells in order if we hit a dead-end
            currentCell = bestCell;

            if (currentCell == ~0u) {
                currentCell = this->GetNextCellDeadEnd(inputCursor, &emittedFlags[0], cellCount);
            }
        }

        assert(inputCursor == cellCount);
        assert(outputCell == cellCount);
    }

    // endregion

    // region deprecated
    // MeshLoomEncoder(std::string saveFilePath, DataObject::Pointer dataObj, UIControlParams uiConParams)
    //     : m_DataObj(dataObj), m_SaveFilePath(saveFilePath), m_EncoderAdapter(new MeshEncoderAdapter(dataObj)) {
    //     InitAdjacencyScore();
    //     InitParams();
    //     LoadUIControlParams(uiConParams);
    // }

    //     bool Execute() {
    //         // 尝试打开流
    //         if (!m_SaveFilePath.empty() && !this->OpenStream(m_SaveFilePath)) { return false; }
    //
    //         std::vector<unsigned int> pointIdRemap;
    //         std::vector<unsigned int> topCellIdsRemap, bottpmCellIdRemap;
    //
    //         PayloadBuffer geomPayload(PayloadType::kGeometryBrick);
    //         this->GeomEncoder(geomPayload, pointIdRemap);
    //
    //         PayloadBuffer topoPayload(PayloadType::kTopologyBrick);
    //         this->TopoEncoder(topoPayload, pointIdRemap, topCellIdsRemap, bottpmCellIdRemap);
    //
    //         PayloadBuffer attrPayload(PayloadType::kAttributeBrick);
    //         this->AttrEncoder(attrPayload, pointIdRemap, topCellIdsRemap, bottpmCellIdRemap);
    //
    //         PayloadBuffer paramPayload(PayloadType::kParameterSet);
    //         this->ParamsEncoder(paramPayload);
    //
    //         // lzma
    //         int compressLevel = 1;
    //         int numThreads = ThreadPool::GetDefaultThreadCount();
    //
    //         PayloadBuffer geomCompressed(PayloadType::kGeometryBrick);
    //         PayloadBuffer topoCompressed(PayloadType::kTopologyBrick);
    //         PayloadBuffer attrCompressed(PayloadType::kAttributeBrick);
    //         PayloadBuffer paramCompressed(PayloadType::kParameterSet);
    //
    //         std::vector<std::future<void>> result;
    //         ThreadPool* tp = ThreadPool::Instance();
    //
    //         std::atomic<float> progress(0.8);
    //
    //         result.push_back(tp->Commit(
    //                 [&]() -> void { MeshCodecLZMA::Compress(geomCompressed, geomPayload, compressLevel, numThreads); }));
    //         result.push_back(tp->Commit(
    //                 [&]() -> void { MeshCodecLZMA::Compress(topoCompressed, topoPayload, compressLevel, numThreads); }));
    //         result.push_back(tp->Commit(
    //                 [&]() -> void { MeshCodecLZMA::Compress(attrCompressed, attrPayload, compressLevel, numThreads); }));
    //         result.push_back(tp->Commit(
    //                 [&]() -> void { MeshCodecLZMA::Compress(paramCompressed, paramPayload, compressLevel, numThreads); }));
    //
    //         for (int i = 0; i < result.size(); i++) {
    //             result[i].wait();
    //             progress += 0.04;
    //             UpdateProgress(progress); // 似乎不支持多线程
    //         }
    //
    //         //WriteBuf(paramPayload, this->m_BytestreamFile); // 必须先写参数信息
    //         //WriteBuf(geomPayload, this->m_BytestreamFile);
    //         //WriteBuf(topoPayload, this->m_BytestreamFile);
    //         //WriteBuf(attrPayload, this->m_BytestreamFile);
    //
    //         WriteBuf(paramCompressed, this->m_BytestreamFile); // 必须先写参数信息
    //         WriteBuf(geomCompressed, this->m_BytestreamFile);
    //         WriteBuf(topoCompressed, this->m_BytestreamFile);
    //         WriteBuf(attrCompressed, this->m_BytestreamFile);
    //
    //         UpdateProgress(1.0);
    //         closeStream();
    //
    //         //std::vector<char> input(geomPayload.begin(), geomPayload.end());
    //         //std::vector<char> result;
    //         //MeshCodecLZMA::Compress(result, input, 10, 12);
    //
    //         //PayloadBuffer output;
    //
    //         //int ret = MeshCodecLZMA::Decompress(output, result);
    //
    //         //        auto calBpv = [=]() -> void {
    //         //            int geomSize = geomCompressed.size();
    //         //            int topoSize = topoCompressed.size();
    //         //            int attrSize = attrCompressed.size();
    //         //            int pointCount = this->m_EncoderAdapter->GetNumberOfPoints();
    //         //
    //         //            float geomBpv = geomSize * 8.0 / pointCount;
    //         //            float topoBpv = topoSize * 8.0 / pointCount;
    //         //            float attrBpv = attrSize * 8.0 / pointCount;
    //         //            float totalBpv = geomBpv + topoBpv + attrBpv;
    //         //
    //         //            std::cout << "Geom BPV: " << geomBpv << std::endl;
    //         //            std::cout << "Topo BPV: " << topoBpv << std::endl;
    //         //            std::cout << "Attr BPV: " << attrBpv << std::endl;
    //         //            std::cout << "Total BPV: " << totalBpv << std::endl;
    //         //
    //         //            this->m_UIconParams.cpStaResult->push_back("顶点坐标BPV: " + std::to_string(geomBpv));
    //         //            this->m_UIconParams.cpStaResult->push_back("拓扑BPV: " + std::to_string(topoBpv));
    //         //            this->m_UIconParams.cpStaResult->push_back("属性BPV: " + std::to_string(attrBpv));
    //         //            this->m_UIconParams.cpStaResult->push_back("综合BPV: " + std::to_string(totalBpv));
    //         //        };
    //         //
    //         auto calCR = [=]() -> void {
    //             long long sourceSize = -1;
    //
    // #ifdef _WIN32
    //             WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    //             auto filePath = this->m_DataObj->GetPropertys()->GetProperty("FilePath")->Get<std::string>();
    //             if (GetFileAttributesEx(filePath.c_str(), GetFileExInfoStandard, &fileInfo)) {
    //                 LARGE_INTEGER size;
    //                 size.HighPart = fileInfo.nFileSizeHigh;
    //                 size.LowPart = fileInfo.nFileSizeLow;
    //                 sourceSize = size.QuadPart;
    //             } else {
    //                 sourceSize = -1;
    //             }
    // #else
    //             struct stat stat_buf;
    //             int rc = stat(filePath.c_str(), &stat_buf);
    //             sourceSize = (rc == 0 ? stat_buf.st_size : -1);
    // #endif
    //
    //             if (sourceSize != -1) {
    //                 long long compressSize =
    //                         geomCompressed.size() + topoCompressed.size() + attrCompressed.size() + paramCompressed.size();
    //                 double cr = compressSize * 1.0 / sourceSize;
    //                 std::cout << "compress rate: " << cr << std::endl;
    //
    //                 m_report.push_back(std::make_pair("压缩率", std::format("{:.2f}%", cr * 100.0)));
    //             }
    //         };
    //
    //         if(m_showReport)
    //             calCR();
    //         return true;
    //     }


    /*
    void AttrEncoder(
        PayloadBuffer& payload,
        const std::vector<unsigned int>& pointRemap,
        const std::vector<unsigned int>& topCellRemap,
        const std::vector<unsigned int>& bottomCellRemap
    ){
        ElementArray<AttributeSet::Attribute>::Pointer attrs = this->m_DataObj->GetAttributeSet()->GetAllAttributes();
        std::vector<std::vector<unsigned char>> outFloats(this->m_codecParams.attrParams.size());

		float progress = 0.4;

        auto remapAttributeValues = [=](auto attrArray, auto& remappedBuffer, AttrParameters& params, int attrIndex) {
            size_t valueCount = params.dimension *
                (params.attachmentType == IG_POINT ? pointRemap.size() : topCellRemap.size());
            remappedBuffer.resize(valueCount);

            size_t remappedElementCount = params.attachmentType == IG_POINT ? pointRemap.size() : topCellRemap.size();
            std::vector<bool> remappedIskey(remappedElementCount, false);

            ThreadPool::parallelFor(0, params.elementCount, [&](int start, int end) -> void {
                for (int j = start; j < end; j++) {
                    igIndex remapIndex = params.attachmentType == IG_POINT ? pointRemap[j] : topCellRemap[j];

                    // 如果原始元素是关键元素，则标记 remap 后的对应元素也为关键元素
                    if (m_attrErorContrl[attrIndex].isKeyElement[j]) {
                        remappedIskey[remapIndex] = true;
                    }

                    for (int k = 0; k < params.dimension; k++) {
                        remappedBuffer[remapIndex * params.dimension + k] =
                            attrArray->GetValue(j * params.dimension + k);
                    }
                }
                });

            // 用 remap 后的 iskey 替换原始的 iskey
            m_attrErorContrl[attrIndex].isKeyElement = std::move(remappedIskey);
            };

        for (int i = 0; i < this->m_codecParams.attrParams.size(); i++)
        {
            AttributeSet::Attribute& attr = attrs->GetElement(i);
            auto& attrParams = this->m_codecParams.attrParams[i];
            auto& errorParams = this->m_attrErorContrl[i];

            // 部署remap
            std::vector<float> remappedFloatAttrBuffer;
            std::vector<double> remappedDoubleAttrBuffer;

            if (attrParams.valueSize == sizeof(float)) {
                auto floatAttrArray = DynamicCast<FlatArray<float>>(attr.pointer);
                remapAttributeValues(floatAttrArray, remappedFloatAttrBuffer, attrParams, i);
            }
            else {
                auto doubleAttrArray = DynamicCast<FlatArray<double>>(attr.pointer);
                remapAttributeValues(doubleAttrArray, remappedDoubleAttrBuffer, attrParams, i);
            }

            progress += 0.2 * (i * 0.1 / this->m_codecParams.attrParams.size());
            UpdateProgress(progress);

            // 编码
            std::vector<unsigned char> encoded;

            if (attrParams.valueSize == sizeof(float)) {
                std::vector<float> preserve = remappedFloatAttrBuffer;
                MeshOptFloatCodec::FloatEncoder(encoded, remappedFloatAttrBuffer, attrParams, errorParams);
                AddErrorReport(preserve, remappedFloatAttrBuffer, attrParams, errorParams, attr.pointer->GetName());
            }
            else if (attrParams.valueSize == sizeof(double)) {
                std::vector<double> preserve = remappedDoubleAttrBuffer;
                MeshOptFloatCodec::FloatEncoder(encoded, remappedDoubleAttrBuffer, attrParams, errorParams);
                AddErrorReport(preserve, remappedDoubleAttrBuffer, attrParams, errorParams, attr.pointer->GetName());
            }

            // 只靠count就足以读取
            attrParams.binaryCount = encoded.size() * sizeof(uint8_t);
            outFloats[i] = encoded;
        }

        UpdateProgress(0.6);

        size_t currentPayloadCursor = 0;
        for (int i = 0; i < this->m_codecParams.attrParams.size(); i++) {
            payload.resize(currentPayloadCursor + outFloats[i].size());
            std::memcpy(payload.data() + currentPayloadCursor, outFloats[i].data(), outFloats[i].size());
            currentPayloadCursor += outFloats[i].size();
        }
    }
    */

// endregion

};

IGAME_NAMESPACE_END
#endif