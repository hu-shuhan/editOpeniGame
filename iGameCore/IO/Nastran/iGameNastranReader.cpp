////
//// Created by m_ky on 2025/10/20.
////
//
///**
// * @class   iGameNastranReader
// * @brief   使用Python pyNastran库读取Nastran BDF/OP2文件
// */
//
//#include "iGameNastranReader.h"
//#include "iGamePoints.h"
//#include "iGameFlatArray.h"
//
//#include <pybind11/pybind11.h>
//#include <pybind11/embed.h>
//#include <pybind11/numpy.h>
//#include <pybind11/stl.h>
//
//#include <iostream>
//#include <stdexcept>
//
//namespace py = pybind11;
//
//IGAME_NAMESPACE_BEGIN
//
//// 静态成员初始化
//std::shared_ptr<py::scoped_interpreter> NastranReader::s_PythonInterpreter = nullptr;
//bool NastranReader::s_PythonInitialized = false;
//
//NastranReader::NastranReader() {
//    m_Mesh = UnstructuredMesh::New();
//}
//
//NastranReader::~NastranReader() {
//    // 注意: 不要在这里释放Python解释器，应该在程序结束时统一释放
//}
//
//void NastranReader::SetBDFFileName(const std::string& filename) {
//    m_BDFFilePath = filename;
//    SetFilePath(filename);  // 同时设置基类的文件路径
//}
//
//void NastranReader::SetOP2FileName(const std::string& filename) {
//    m_OP2FilePath = filename;
//}
//
//bool NastranReader::InitializePythonEnvironment() {
//    // 使用单例模式初始化Python解释器（整个应用生命周期只初始化一次）
//    if (!s_PythonInitialized) {
//        try {
//            s_PythonInterpreter = std::make_shared<py::scoped_interpreter>();
//            s_PythonInitialized = true;
//
//            // 添加Python脚本路径到sys.path
//            py::module_ sys = py::module_::import("sys");
//            py::object path = sys.attr("path");
//
//            // 这里需要添加nastran_reader_wrapper.py所在的目录
//            // 在CMake中会通过环境变量或编译定义传递这个路径
//            #ifdef PYNASTRAN_SCRIPT_PATH
//                path.attr("insert")(0, PYNASTRAN_SCRIPT_PATH);
//            #endif
//
//            std::cout << "[NastranReader] Python环境初始化成功" << std::endl;
//        } catch (const std::exception& e) {
//            std::cerr << "[NastranReader] Python初始化失败: " << e.what() << std::endl;
//            return false;
//        }
//    }
//    return true;
//}
//
//bool NastranReader::Parsing() {
//    if (m_BDFFilePath.empty()) {
//        std::cerr << "[NastranReader] 错误: 未设置BDF文件路径" << std::endl;
//        return false;
//    }
//
//    // 初始化Python环境
//    if (!InitializePythonEnvironment()) {
//        return false;
//    }
//
//    try {
//        // 导入Python包装器模块
//        py::module_ nastran_wrapper = py::module_::import("nastran_reader_wrapper");
//
//        // 调用read_nastran_full函数
//        py::object read_func = nastran_wrapper.attr("read_nastran_full");
//
//        py::object py_result;
//        if (!m_OP2FilePath.empty()) {
//            py_result = read_func(m_BDFFilePath, m_OP2FilePath);
//        } else {
//            py_result = read_func(m_BDFFilePath, py::none());
//        }
//
//        // 转换为字典
//        py::dict result_dict = py_result.cast<py::dict>();
//
//        // 检查是否成功
//        if (!result_dict.contains("success") || !result_dict["success"].cast<bool>()) {
//            std::string error_msg = "Unknown error";
//            if (result_dict.contains("error")) {
//                error_msg = result_dict["error"].cast<std::string>();
//            }
//            std::cerr << "[NastranReader] Python解析失败: " << error_msg << std::endl;
//
//            if (result_dict.contains("traceback")) {
//                std::cerr << result_dict["traceback"].cast<std::string>() << std::endl;
//            }
//            return false;
//        }
//
//        // 解析几何数据
//        if (result_dict.contains("geometry")) {
//            py::dict geom_dict = result_dict["geometry"].cast<py::dict>();
//            if (!ParseGeometryData(geom_dict)) {
//                std::cerr << "[NastranReader] 几何数据解析失败" << std::endl;
//                return false;
//            }
//        }
//
//        // 解析结果数据（如果有）
//        if (result_dict.contains("results") && !result_dict["results"].is_none()) {
//            py::dict results_dict = result_dict["results"].cast<py::dict>();
//            if (results_dict.contains("success") && results_dict["success"].cast<bool>()) {
//                ParseResultsData(results_dict);
//            }
//        }
//
//        std::cout << "[NastranReader] 解析完成" << std::endl;
//        return true;
//
//    } catch (const py::error_already_set& e) {
//        std::cerr << "[NastranReader] Python异常: " << e.what() << std::endl;
//        return false;
//    } catch (const std::exception& e) {
//        std::cerr << "[NastranReader] 异常: " << e.what() << std::endl;
//        return false;
//    }
//}
//
//bool NastranReader::ParseGeometryData(py::dict& geom_dict) {
//    try {
//        int num_nodes = geom_dict["num_nodes"].cast<int>();
//        int num_elements = geom_dict["num_elements"].cast<int>();
//
//        std::cout << "[NastranReader] 节点数: " << num_nodes
//                  << ", 单元数: " << num_elements << std::endl;
//
//        // 提取节点坐标
//        py::object node_coords_obj = geom_dict["node_coords"];
//        if (!ConvertNumpyToPoints(node_coords_obj)) {
//            return false;
//        }
//
//        // 提取单元连接性
//        py::object connectivity_obj = geom_dict["connectivity"];
//        py::object cell_types_obj = geom_dict["cell_types"];
//        py::object cell_offsets_obj = geom_dict["cell_offsets"];
//
//        if (!ConvertNumpyToCells(connectivity_obj, cell_types_obj, cell_offsets_obj)) {
//            return false;
//        }
//
//        // 存储节点ID和单元ID作为属性数组
//        py::array_t<int32_t> node_ids = geom_dict["node_ids"].cast<py::array_t<int32_t>>();
//        py::array_t<int32_t> element_ids = geom_dict["element_ids"].cast<py::array_t<int32_t>>();
//
//        auto node_ids_buf = node_ids.request();
//        auto element_ids_buf = element_ids.request();
//
//        // 创建节点ID数组
//        IntArray::Pointer nodeIdArray = IntArray::New();
//        nodeIdArray->Resize(node_ids_buf.shape[0]);
////        nodeIdArray->SetNumberOfValues(node_ids_buf.shape[0]);
//        std::memcpy(nodeIdArray->RawPointer(), node_ids_buf.ptr,
//                    node_ids_buf.shape[0] * sizeof(int32_t));
//        nodeIdArray->SetName("NodeIds");
////        m_Mesh->GetPointData()->AddArray(nodeIdArray);
//
//        // 创建单元ID数组
//        IntArray::Pointer elemIdArray = IntArray::New();
//        elemIdArray->SetNumberOfValues(element_ids_buf.shape[0]);
//        std::memcpy(elemIdArray->RawPointer(), element_ids_buf.ptr,
//                    element_ids_buf.shape[0] * sizeof(int32_t));
//        elemIdArray->SetName("ElementIds");
//        m_Mesh->GetCellData()->AddArray(elemIdArray);
//
//        return true;
//
//    } catch (const std::exception& e) {
//        std::cerr << "[NastranReader] 几何数据转换异常: " << e.what() << std::endl;
//        return false;
//    }
//}
//
//bool NastranReader::ConvertNumpyToPoints(py::object& numpy_array) {
//    try {
//        // 转换为numpy array
//        py::array_t<float> coords = numpy_array.cast<py::array_t<float>>();
//        auto buf = coords.request();
//
//        if (buf.ndim != 2 || buf.shape[1] != 3) {
//            std::cerr << "[NastranReader] 节点坐标数组维度错误" << std::endl;
//            return false;
//        }
//
//        igIndex num_points = buf.shape[0];
//
//        // 创建Points对象
//        Points::Pointer points = Points::New();
//        points->Resize(num_points);
//
//        float* data = static_cast<float*>(buf.ptr);
//
//        for (igIndex i = 0; i < num_points; ++i) {
//            points->SetPoint(i, data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
//        }
//
//        m_Mesh->SetPoints(points);
//
//        std::cout << "[NastranReader] 成功加载 " << num_points << " 个节点" << std::endl;
//        return true;
//
//    } catch (const std::exception& e) {
//        std::cerr << "[NastranReader] 节点转换异常: " << e.what() << std::endl;
//        return false;
//    }
//}
//
//bool NastranReader::ConvertNumpyToCells(py::object& connectivity,
//                                        py::object& cell_types,
//                                        py::object& cell_offsets) {
//    try {
//        // 转换numpy数组
//        py::array_t<int32_t> conn_array = connectivity.cast<py::array_t<int32_t>>();
//        py::array_t<uint8_t> types_array = cell_types.cast<py::array_t<uint8_t>>();
//        py::array_t<int32_t> offsets_array = cell_offsets.cast<py::array_t<int32_t>>();
//
//        auto conn_buf = conn_array.request();
//        auto types_buf = types_array.request();
//        auto offsets_buf = offsets_array.request();
//
//        int32_t* conn_ptr = static_cast<int32_t*>(conn_buf.ptr);
//        uint8_t* types_ptr = static_cast<uint8_t*>(types_buf.ptr);
//        int32_t* offsets_ptr = static_cast<int32_t*>(offsets_buf.ptr);
//
//        igIndex num_cells = types_buf.shape[0];
//
//        // 创建CellArray
//        CellArray::Pointer cells = CellArray::New();
//
//        for (igIndex i = 0; i < num_cells; ++i) {
//            int32_t start = offsets_ptr[i];
//            int32_t end = offsets_ptr[i + 1];
//            int32_t num_pts = end - start;
//
//            IGenum cell_type = static_cast<IGenum>(types_ptr[i]);
//
//            // 创建单元
//            igIndex* pts = new igIndex[num_pts];
//            for (int32_t j = 0; j < num_pts; ++j) {
//                pts[j] = static_cast<igIndex>(conn_ptr[start + j]);
//            }
//
//            cells->AddCellIds(cell_type, num_pts, pts);
//            delete[] pts;
//        }
//
//        m_Mesh->SetCells(cells);
//
//        std::cout << "[NastranReader] 成功加载 " << num_cells << " 个单元" << std::endl;
//        return true;
//
//    } catch (const std::exception& e) {
//        std::cerr << "[NastranReader] 单元转换异常: " << e.what() << std::endl;
//        return false;
//    }
//}
//
//bool NastranReader::ParseResultsData(py::dict& results_dict) {
//    try {
//        // 处理位移结果
//        if (results_dict.contains("displacements")) {
//            py::dict disp_dict = results_dict["displacements"].cast<py::dict>();
//
//            for (auto item : disp_dict) {
//                int subcase_id = item.first.cast<int>();
//                py::dict disp_data = item.second.cast<py::dict>();
//
//                py::array_t<float> data_array = disp_data["data"].cast<py::array_t<float>>();
//                auto buf = data_array.request();
//
//                // data shape: (ntimes, num_nodes, 6)
//                if (buf.ndim == 3 && buf.shape[2] == 6) {
//                    int ntimes = buf.shape[0];
//                    int num_nodes = buf.shape[1];
//
//                    // 如果只有一个时间步，添加位移向量
//                    if (ntimes == 1) {
//                        float* data_ptr = static_cast<float*>(buf.ptr);
//
//                        // 创建位移数组 (只取tx, ty, tz)
//                        FloatArray::Pointer dispArray = FloatArray::New();
//                        dispArray->SetNumberOfComponents(3);
//                        dispArray->Resize(num_nodes);
//
//                        for (int i = 0; i < num_nodes; ++i) {
//                            float* disp = &data_ptr[i * 6];
//                            dispArray->SetTuple3(i, disp[0], disp[1], disp[2]);
//                        }
//
//                        dispArray->SetName("Displacement");
//                        m_Mesh->GetPointData()->AddArray(dispArray);
//
//                        std::cout << "[NastranReader] 添加位移结果 (子工况 "
//                                  << subcase_id << ")" << std::endl;
//                    }
//                }
//            }
//        }
//
//        // 处理应力结果
//        if (results_dict.contains("stresses")) {
//            py::dict stress_dict = results_dict["stresses"].cast<py::dict>();
//
//            for (auto item : stress_dict) {
//                std::string stress_name = item.first.cast<std::string>();
//                py::dict stress_data = item.second.cast<py::dict>();
//
//                // 这里可以根据需要添加应力数据到单元数据
//                // 暂时跳过，因为应力数据可能有多个分量和层
//
//                std::cout << "[NastranReader] 检测到应力结果: " << stress_name << std::endl;
//            }
//        }
//
//        return true;
//
//    } catch (const std::exception& e) {
//        std::cerr << "[NastranReader] 结果数据解析异常: " << e.what() << std::endl;
//    return false;
//    }
//}
//
//bool NastranReader::CreateDataObject() {
//    m_Output = m_Mesh;
//    return m_Output != nullptr;
//}
//
//DataObject::Pointer NastranReader::GetOutput() {
//    return m_Output;
//}
//
//UnstructuredMesh::Pointer NastranReader::GetUnstructuredMeshOutput() {
//    return m_Mesh;
//}
//
//IGAME_NAMESPACE_END
