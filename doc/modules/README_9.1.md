# 指标 9.1：大规模 CAE 仿真数据的自适应压缩

## 指标构成

面向大规模 CAE 仿真网格、拓扑和结果属性数据，提供 IGC 编解码与压缩集成能力。

| 项目 | 考核要求 |
|------|----------|
| 精度 | 可视化关键特征区域的变量误差不超过 5% |
| 压缩 | CAE 仿真数据压缩率不低于 20% |
| 考核方式 | 有资质第三方机构评测 |
| 交付物 | 开放链接库 |

> 本文档记录当前 IGC 编解码实现和测试入口。关键特征区域误差 <= 5% 及压缩率 >= 20% 需要固定数据集、区域标注和第三方测试报告验证，不能仅由编解码代码推断为已达标。

---

## 子功能 1：IGC 网格与属性编码

### 功能说明

编码器将几何、拓扑、属性和参数组织为独立载荷，并使用 Zstandard 对载荷压缩后写入 `.igc` 文件。控制参数由 `CodecControlParams` 管理，可由输入数据生成默认参数，也可在调用前调整。

### 源码路径

| 路径 | 类 / 文件 | 说明 |
|------|-----------|------|
| `iGameCore/Filters/MeshCodec/iGameMeshEncoderFilter.h` | `MeshEncoderFilter` | 几何、拓扑、属性编码和压缩调度 |
| `iGameCore/Filters/MeshCodec/EncodeAdapter/` | 编码适配器 | 从 `DataObject` 提取编码数据 |
| `iGameCore/Filters/MeshCodec/SubCodec/iGameMeshCodecZSTD.h` | `MeshCodecZSTD` | Zstandard 压缩与解压封装 |
| `iGameCore/IO/IGC/iGameIGCWriter.*` | `IGCWriter` | `.igc` 文件写出 |
| `Examples/Filter/Compression/TestEncoder.cpp` | `testEncoder` | 编码示例 |

### 调用方式

对应示例 `Examples/Filter/Compression/TestEncoder.cpp`：
```cpp
auto source = iGame::FileIO::ReadFile("./Models/Quad_Plane_Tensor.vtk");

auto writer = iGame::IGCWriter::New();
writer->SetCodecControlParams(
    iGame::MeshEncoderFilter<iGame::EncodeOutputBinaryArray>::GenerateDefaultCodecParams(source));
writer->WriteToFile(source, "./Models/comp.igc");
```

### GUI

| 入口                                 | 说明                   |
| ------------------------------------ | ---------------------- |
| 菜单「文件」→ 压缩/`action_compress` | 打开「压缩」面板       |
| `igQtMeshCodecDialog`                | 调整编码参数并执行压缩 |

![image-20260729185004497](../../Resources/Images/image-20260729185004497.png)

### 效果图

![](../../Resources/Images/image-20260729192510189.png)

|                            压缩前                            |                            压缩后                            |
| :----------------------------------------------------------: | :----------------------------------------------------------: |
| ![image-20260729191415859](../../Resources/Images/image-20260729191415859.png) | ![image-20260729192536836](../../Resources/Images/image-20260729192536836.png) |



### 测试用例

| Target | 源文件 | 默认输入 | 输出 |
|--------|--------|----------|------|
| `testEncoder` | `Examples/Filter/Compression/TestEncoder.cpp` | `./Models/Quad_Plane_Tensor.vtk` | `./Models/comp.igc` |
| `testLosslessEncode` | `Examples/Filter/Compression/TestLosslessEncode.cpp` | 命令行传入数据文件 | 编码/解码一致性输出 |

---

## 子功能 2：IGC 解码与可视化恢复

### 功能说明

解码器读取 `.igc` 载荷，完成 Zstandard 解压并重建网格和属性数据对象，随后可进入现有渲染流程显示。

### 源码路径

| 路径 | 类 / 文件 | 说明 |
|------|-----------|------|
| `iGameCore/Filters/MeshCodec/iGameMeshDecoderFilter.h` | `MeshDecoderFilter` | 载荷解压和数据重建 |
| `iGameCore/Filters/MeshCodec/DecodeInput/` | 解码输入 | 文件、内存等输入适配 |
| `iGameCore/Filters/MeshCodec/DecodeAdapter/` | 解码适配器 | 重建 `DataObject`、网格和属性 |
| `iGameCore/IO/IGC/iGameIGCReader.*` | `IGCReader` | `.igc` 文件读取 |
| `Examples/Filter/Compression/TestDecoder.cpp` | `testDecoder` | 解码与显示示例 |

### 调用方式

对应示例 `Examples/Filter/Compression/TestDecoder.cpp`：

```cpp
auto object = iGame::FileIO::ReadFile("./Models/comp.igc");
if (object != nullptr) {
    scene->AddModel(object);
}
```

### 效果图
![image-20260729192721858](../../Resources/Images/image-20260729192721858.png)
### 测试用例

| Target | 源文件 | 前置条件 | 说明 |
|--------|--------|----------|------|
| `testDecoder` | `Examples/Filter/Compression/TestDecoder.cpp` | 先运行 `testEncoder` 生成 `comp.igc` | 解码后显示 |

---

## 第三方评测建议

压缩率按下式计算：

```text
compressionRatio = (originalBytes - compressedBytes) / originalBytes
```

关键特征区域变量误差应在评测前固定变量、区域掩码和误差定义。建议同时报告最大相对误差、平均相对误差和零值处理规则。

| 验收项 | 建议验证内容 |
|--------|--------------|
| 数据集 | 固定网格、结果属性、关键特征区域掩码和文件校验值 |
| 压缩率 | 记录原始与 `.igc` 文件字节数，验证压缩率 >= 20% |
| 变量误差 | 对关键区域逐变量比对解码结果，验证误差 <= 5% |
| 完整性 | 对比点数、单元数、单元类型、属性名称、维度、挂载位置和数值范围 |
| 可复现性 | 固定发布库版本、编译器、依赖版本、命令行和原始日志 |

开放链接库交付包应至少包含公开头文件、库文件、运行时依赖、CMake 配置、编码/解码示例和版本说明。当前仓库提供 IGC 编解码基础；自适应策略、特征区域误差控制和指标达标结果应以发布版集成测试与第三方评测为准。
