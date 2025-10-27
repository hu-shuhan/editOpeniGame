/**
 * @class   iGameNastranReader
 * @brief   Nastran BDF/OP2文件读取器，通过Python pyNastran库解析
 *
 * 该类使用pybind11调用Python的pyNastran库来解析Nastran文件
 * 支持BDF几何文件和OP2结果文件的读取
 */

#pragma once
#include "iGameFileReader.h"
#include "iGameUnstructuredMesh.h"
#include <memory>

//// 前置声明Python相关的命名空间
//namespace pybind11 {
//    class scoped_interpreter;
//    class module_;
//    class dict;
//    class object;
//}

IGAME_NAMESPACE_BEGIN

class NastranReader : public Filter{
public:
    I_OBJECT(NastranReader);
    static Pointer New() { return new NastranReader; }

    // 设置BDF文件路径（几何数据）
    void SetBDFFileName(const std::string& filename);

    // 设置OP2文件路径（结果数据，可选）Optional, the op2 file path can be set to read physical field data
    void SetOP2FileName(const std::string& filename);

    // 获取OP2文件路径
    std::string GetOP2FileName() const { return m_OP2FilePath; }

    void SetFilePath(const std::string& filePath);
    DataObject::Pointer GetOutput() override;

    // 获取UnstructuredMesh输出
//    UnstructuredMesh::Pointer GetUnstructuredMeshOutput();

protected:
    NastranReader();
    ~NastranReader() override;

public:
    bool Execute() override;

protected:

    bool Parsing() ;
    bool CreateDataObject() ;
private:
//    // 初始化Python解释器和环境
//    bool InitializePythonEnvironment();
//
//    // 从Python字典解析几何数据
//    bool ParseGeometryData(pybind11::dict& py_data);
//
//    // 从Python字典解析结果数据
//    bool ParseResultsData(pybind11::dict& py_data);
//
//    // 将numpy数组转换为Points
//    bool ConvertNumpyToPoints(pybind11::object& numpy_array);
//
//    // 将numpy数组转换为单元连接性
//    bool ConvertNumpyToCells(pybind11::object& connectivity,
//                             pybind11::object& cell_types,
//                             pybind11::object& cell_offsets);
//
//    // 将结果数据添加到网格
//    bool AddResultArrays(pybind11::dict& results_dict);

private:
    std::string m_BDFFilePath;          // BDF文件路径
    std::string m_OP2FilePath;          // OP2文件路径（可选）
    DataObject::Pointer m_Output;

    std::string m_FilePath;
    std::string m_FileName;

//    UnstructuredMesh::Pointer m_Mesh;   // 输出的非结构网格

//    // Python解释器（单例，整个应用程序生命周期保持）
//    static std::shared_ptr<pybind11::scoped_interpreter> s_PythonInterpreter;
//    static bool s_PythonInitialized;
};

IGAME_NAMESPACE_END
