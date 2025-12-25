#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include "iGameFileIO.h"
#include "iGameDataObject.h"
#include "iGameSmartPointer.h"
#include "iGameFlatArray.h"
#include "iGameType.h"
#include "DataProcessing/iGameMeshSimplificationFilterPro.h"
#include "iGameFilter.h"

namespace py = pybind11;
using namespace iGame;

// 声明 SmartPointer 为 pybind11 的 holder type
PYBIND11_DECLARE_HOLDER_TYPE(T, iGame::SmartPointer<T>);

int add(int i, int j) {
    return i + j;
}

// 辅助函数：将 ArrayObject 转换为 Python 字典
py::dict ConvertArrayToDict(ArrayObject* array) {
    py::dict info;
    if (!array) return info;

    info["name"] = array->GetName();
    info["dimension"] = array->GetDimension();
    
    // 获取数组类型和数据
    // 注意：这里我们创建了数据的副本，因为直接共享内存需要管理生命周期，较为复杂
    // 如果性能是瓶颈，可以考虑使用 capsule 或 memoryview
    
    IGenum type = array->GetArrayType();
    
    // 根据 iGameType.h 定义的类型进行转换
    // 注意：GetArrayType 返回的是具体的数组类型枚举，如 IG_FloatArray
    
    switch(type) {
        case IG_FloatArray: {
            info["type"] = "Float32";
            auto* fArray = static_cast<FloatArray*>(array);
            info["data"] = py::array_t<float>(
                { (py::ssize_t)fArray->GetNumberOfElements(), (py::ssize_t)fArray->GetDimension() },
                { sizeof(float) * fArray->GetDimension(), sizeof(float) },
                fArray->RawPointer()
            );
            break;
        }
        case IG_DoubleArray: {
            info["type"] = "Float64";
            auto* dArray = static_cast<DoubleArray*>(array);
            info["data"] = py::array_t<double>(
                { (py::ssize_t)dArray->GetNumberOfElements(), (py::ssize_t)dArray->GetDimension() },
                { sizeof(double) * dArray->GetDimension(), sizeof(double) },
                dArray->RawPointer()
            );
            break;
        }
        case IG_IntArray: 
        case IG_INTARRAY: {
            info["type"] = "Int32";
            auto* iArray = static_cast<IntArray*>(array);
            info["data"] = py::array_t<int>(
                { (py::ssize_t)iArray->GetNumberOfElements(), (py::ssize_t)iArray->GetDimension() },
                { sizeof(int) * iArray->GetDimension(), sizeof(int) },
                iArray->RawPointer()
            );
            break;
        }
        case IG_UnsignedIntArray: {
            info["type"] = "UInt32";
            auto* uArray = static_cast<UnsignedIntArray*>(array);
            info["data"] = py::array_t<unsigned int>(
                { (py::ssize_t)uArray->GetNumberOfElements(), (py::ssize_t)uArray->GetDimension() },
                { sizeof(unsigned int) * uArray->GetDimension(), sizeof(unsigned int) },
                uArray->RawPointer()
            );
            break;
        }
        case IG_CharArray: {
            info["type"] = "Int8";
            auto* cArray = static_cast<CharArray*>(array);
            info["data"] = py::array_t<char>(
                { (py::ssize_t)cArray->GetNumberOfElements(), (py::ssize_t)cArray->GetDimension() },
                { sizeof(char) * cArray->GetDimension(), sizeof(char) },
                cArray->RawPointer()
            );
            break;
        }
        case IG_UnsignedCharArray: {
            info["type"] = "UInt8";
            auto* ucArray = static_cast<UnsignedCharArray*>(array);
            info["data"] = py::array_t<unsigned char>(
                { (py::ssize_t)ucArray->GetNumberOfElements(), (py::ssize_t)ucArray->GetDimension() },
                { sizeof(unsigned char) * ucArray->GetDimension(), sizeof(unsigned char) },
                ucArray->RawPointer()
            );
            break;
        }
         case IG_ShortArray: {
            info["type"] = "Int16";
            auto* sArray = static_cast<ShortArray*>(array);
            info["data"] = py::array_t<short>(
                { (py::ssize_t)sArray->GetNumberOfElements(), (py::ssize_t)sArray->GetDimension() },
                { sizeof(short) * sArray->GetDimension(), sizeof(short) },
                sArray->RawPointer()
            );
            break;
        }
        case IG_UnsignedShortArray: {
            info["type"] = "UInt16";
            auto* usArray = static_cast<UnsignedShortArray*>(array);
            info["data"] = py::array_t<unsigned short>(
                { (py::ssize_t)usArray->GetNumberOfElements(), (py::ssize_t)usArray->GetDimension() },
                { sizeof(unsigned short) * usArray->GetDimension(), sizeof(unsigned short) },
                usArray->RawPointer()
            );
            break;
        }
        case IG_LongLongArray: {
            info["type"] = "Int64";
            auto* lArray = static_cast<LongLongArray*>(array);
            info["data"] = py::array_t<long long>(
                { (py::ssize_t)lArray->GetNumberOfElements(), (py::ssize_t)lArray->GetDimension() },
                { sizeof(long long) * lArray->GetDimension(), sizeof(long long) },
                lArray->RawPointer()
            );
            break;
        }
        case IG_UnsignedLongLongArray: {
            info["type"] = "UInt64";
            auto* ulArray = static_cast<UnsignedLongLongArray*>(array);
            info["data"] = py::array_t<unsigned long long>(
                { (py::ssize_t)ulArray->GetNumberOfElements(), (py::ssize_t)ulArray->GetDimension() },
                { sizeof(unsigned long long) * ulArray->GetDimension(), sizeof(unsigned long long) },
                ulArray->RawPointer()
            );
            break;
        }
        default:
            info["type"] = "Unknown";
            // Unknown type or not a FlatArray
            break;
    }
    
    return info;
}

// 辅助函数：将 AttributeType 转换为字符串
std::string GetAttributeTypeAsString(IGenum type) {
    switch(type) {
        case IG_SCALAR: return "SCALAR";
        case IG_VECTOR: return "VECTOR";
        case IG_NORMAL: return "NORMAL";
        case IG_TCOORD: return "TCOORD";
        case IG_TENSOR: return "TENSOR";
        case IG_RGB:    return "RGB";
        default:        return "UNKNOWN";
    }
}

std::string GetAttachmentTypeAsString(IGenum type) {
    switch(type) {
        case IG_POINT: return "POINT";
        case IG_CELL:  return "CELL";
        default:       return "UNKNOWN";
    }
}

// Helpers for String -> Enum
IGenum GetAttributeTypeFromString(const std::string& type) {
    if (type == "SCALAR") return IG_SCALAR;
    if (type == "VECTOR") return IG_VECTOR;
    if (type == "NORMAL") return IG_NORMAL;
    if (type == "TCOORD") return IG_TCOORD;
    if (type == "TENSOR") return IG_TENSOR;
    if (type == "RGB")    return IG_RGB;
    return IG_SCALAR; // Default
}

IGenum GetAttachmentTypeFromString(const std::string& type) {
    if (type == "POINT") return IG_POINT;
    if (type == "CELL")  return IG_CELL;
    return IG_POINT; // Default
}

// Helper to create array from numpy
ArrayObject::Pointer CreateArrayFromNumpy(const py::array& b, const std::string& name) {
    py::buffer_info info = b.request();
    
    int num_elements = info.shape[0];
    int dimension = (info.ndim > 1) ? info.shape[1] : 1;
    
    ArrayObject::Pointer array = nullptr;
    
    if (py::isinstance<py::array_t<float>>(b)) {
        auto arr = FloatArray::New();
        arr->SetDimension(dimension);
        arr->Resize(num_elements);
        std::memcpy(arr->RawPointer(), info.ptr, sizeof(float) * num_elements * dimension);
        array = arr;
    } else if (py::isinstance<py::array_t<double>>(b)) {
        auto arr = DoubleArray::New();
        arr->SetDimension(dimension);
        arr->Resize(num_elements);
        std::memcpy(arr->RawPointer(), info.ptr, sizeof(double) * num_elements * dimension);
        array = arr;
    } else if (py::isinstance<py::array_t<int>>(b)) {
        auto arr = IntArray::New();
        arr->SetDimension(dimension);
        arr->Resize(num_elements);
        std::memcpy(arr->RawPointer(), info.ptr, sizeof(int) * num_elements * dimension);
        array = arr;
    } else if (py::isinstance<py::array_t<unsigned int>>(b)) {
        auto arr = UnsignedIntArray::New();
        arr->SetDimension(dimension);
        arr->Resize(num_elements);
        std::memcpy(arr->RawPointer(), info.ptr, sizeof(unsigned int) * num_elements * dimension);
        array = arr;
    } else if (py::isinstance<py::array_t<char>>(b)) { // int8
        auto arr = CharArray::New();
        arr->SetDimension(dimension);
        arr->Resize(num_elements);
        std::memcpy(arr->RawPointer(), info.ptr, sizeof(char) * num_elements * dimension);
        array = arr;
    } else if (py::isinstance<py::array_t<unsigned char>>(b)) { // uint8
        auto arr = UnsignedCharArray::New();
        arr->SetDimension(dimension);
        arr->Resize(num_elements);
        std::memcpy(arr->RawPointer(), info.ptr, sizeof(unsigned char) * num_elements * dimension);
        array = arr;
    } else if (py::isinstance<py::array_t<short>>(b)) { // int16
        auto arr = ShortArray::New();
        arr->SetDimension(dimension);
        arr->Resize(num_elements);
        std::memcpy(arr->RawPointer(), info.ptr, sizeof(short) * num_elements * dimension);
        array = arr;
    } else if (py::isinstance<py::array_t<unsigned short>>(b)) { // uint16
        auto arr = UnsignedShortArray::New();
        arr->SetDimension(dimension);
        arr->Resize(num_elements);
        std::memcpy(arr->RawPointer(), info.ptr, sizeof(unsigned short) * num_elements * dimension);
        array = arr;
    } else if (py::isinstance<py::array_t<long long>>(b)) { // int64
        auto arr = LongLongArray::New();
        arr->SetDimension(dimension);
        arr->Resize(num_elements);
        std::memcpy(arr->RawPointer(), info.ptr, sizeof(long long) * num_elements * dimension);
        array = arr;
    } else if (py::isinstance<py::array_t<unsigned long long>>(b)) { // uint64
        auto arr = UnsignedLongLongArray::New();
        arr->SetDimension(dimension);
        arr->Resize(num_elements);
        std::memcpy(arr->RawPointer(), info.ptr, sizeof(unsigned long long) * num_elements * dimension);
        array = arr;
    }
    
    if (array) {
        array->SetName(name);
    }
    return array;
}

// DataObject 的扩展方法
py::list GetDataObjectAttributes(DataObject& self) {
    py::list res;
    AttributeSet* attrSet = self.GetAttributeSet();
    if (!attrSet) return res;

    for (int i = 0; i < attrSet->GetNumberOfAttributes(); ++i) {
        auto& attr = attrSet->GetAttribute(i);
        ArrayObject* array = attr.pointer;
        if (!array) continue;

        py::dict info = ConvertArrayToDict(array);
        // 添加 Attribute Type 信息
        info["attribute_type"] = GetAttributeTypeAsString(attr.GetType());
        info["attachment_type"] = GetAttachmentTypeAsString(attr.GetAttachmentType());
        
        res.append(info);
    }
    return res;
}

void SetDataObjectAttributes(DataObject& self, py::list attributes) {
    AttributeSet::Pointer attrSet = AttributeSet::New();
    self.SetAttributeSet(attrSet);
    
    for (auto item : attributes) {
        if (!py::isinstance<py::dict>(item)) continue;
        
        py::dict dict = item.cast<py::dict>();
        if (!dict.contains("name") || !dict.contains("data")) continue;
        
        std::string name = dict["name"].cast<std::string>();
        py::array data = dict["data"].cast<py::array>();
        
        std::string attr_type_str = "SCALAR";
        if (dict.contains("attribute_type")) {
            attr_type_str = dict["attribute_type"].cast<std::string>();
        }
        
        std::string attach_type_str = "POINT";
        if (dict.contains("attachment_type")) {
            attach_type_str = dict["attachment_type"].cast<std::string>();
        }
        
        IGenum attrType = GetAttributeTypeFromString(attr_type_str);
        IGenum attachType = GetAttachmentTypeFromString(attach_type_str);
        
        ArrayObject::Pointer array = CreateArrayFromNumpy(data, name);
        if (array) {
            attrSet->AddAttribute(attrType, attachType, array);
        }
    }
}

AttributeSet::Pointer MakeAttributeSet(py::list attributes) {
    AttributeSet::Pointer attrSet = AttributeSet::New();

    for (auto item: attributes) {
        if (!py::isinstance<py::dict>(item)) continue;

        py::dict dict = item.cast<py::dict>();
        if (!dict.contains("name") || !dict.contains("data")) continue;

        std::string name = dict["name"].cast<std::string>();
        py::array data = dict["data"].cast<py::array>();

        std::string attr_type_str = "SCALAR";
        if (dict.contains("attribute_type")) { attr_type_str = dict["attribute_type"].cast<std::string>(); }

        std::string attach_type_str = "POINT";
        if (dict.contains("attachment_type")) { attach_type_str = dict["attachment_type"].cast<std::string>(); }

        IGenum attrType = GetAttributeTypeFromString(attr_type_str);
        IGenum attachType = GetAttachmentTypeFromString(attach_type_str);

        ArrayObject::Pointer array = CreateArrayFromNumpy(data, name);
        if (array) { attrSet->AddAttribute(attrType, attachType, array); }
    }
    return attrSet;
}

bool ExecuteWithLatentFeatures(MeshSimplificationFilterPro& self, py::list attributes) {
    AttributeSet::Pointer attrSet = MakeAttributeSet(attributes);
    return self.ExecuteWithLatentFeatures(attrSet);
}

PYBIND11_MODULE(test_pybind11, m) {
    m.doc() = "iGameCore Python Bindings Example";

    m.def("add", &add, "A function that adds two numbers");
    m.def("MakeAttributeSet", &MakeAttributeSet, "Make attributes from a list of dictionaries");

    // 绑定 DataObject
    py::class_<DataObject, iGame::SmartPointer<DataObject>>(m, "DataObject")
        .def_static("New", &DataObject::New)
        .def("GetDataObjectId", &DataObject::GetDataObjectId)
        .def("GetAttributes", &GetDataObjectAttributes, "Get all attributes as a list of dictionaries")
        .def("SetAttributes", &SetDataObjectAttributes, "Set attributes from a list of dictionaries");

    // 绑定 FileIO
    py::class_<FileIO, iGame::SmartPointer<FileIO>>(m, "FileIO")
        .def_static("ReadFile", &FileIO::ReadFile)
        .def_static("WriteFile", &FileIO::WriteFile)
        .def_static("GetFileType", &FileIO::GetFileType)
        .def_static("GetFileTypeAsString", &FileIO::GetFileTypeAsString);

    // 绑定 Filter 基类
    py::class_<Filter, iGame::SmartPointer<Filter>>(m, "Filter")
        .def("Execute", &Filter::Execute)
        .def("SetInput", py::overload_cast<int, DataObject::Pointer>(&Filter::SetInput))
        .def("SetInput", py::overload_cast<DataObject::Pointer>(&Filter::SetInput))
        .def("GetOutput", py::overload_cast<int>(&Filter::GetOutput))
        .def("GetOutput", py::overload_cast<>(&Filter::GetOutput));

    // 绑定 MeshSimplificationFilterPro
    py::class_<MeshSimplificationFilterPro, Filter, iGame::SmartPointer<MeshSimplificationFilterPro>>(m, "MeshSimplificationFilterPro")
        .def_static("New", &MeshSimplificationFilterPro::New)
        .def("Execute", &MeshSimplificationFilterPro::Execute)
        .def("SetTargetReduction", &MeshSimplificationFilterPro::SetTargetReduction, py::arg("target") = 0.5f)
        .def("SetTargetFaceCount", &MeshSimplificationFilterPro::SetTargetFaceCount, py::arg("target") = 0)
        .def("SetPreserveBoundary", &MeshSimplificationFilterPro::SetPreserveBoundary, py::arg("flag") = true)
        .def("SetFreeze", &MeshSimplificationFilterPro::SetFreeze, py::arg("flag") = false)
        .def("SetTransformToCellData", &MeshSimplificationFilterPro::SetTransformToCellData, py::arg("flag") = false)
        .def("ExecuteWithLatentFeatures", &ExecuteWithLatentFeatures);

    // 绑定 FileType 枚举
    py::enum_<FileIO::FileType>(m, "FileType")
        .value("VTK", FileIO::FileType::VTK)
        .value("IGC", FileIO::FileType::IGC)
        .value("OBJ", FileIO::FileType::OBJ)
        .value("OFF", FileIO::FileType::OFF)
        .value("MESH", FileIO::FileType::MESH)
        .value("STL", FileIO::FileType::STL)
        .value("PLY", FileIO::FileType::PLY)
        .value("STEP", FileIO::FileType::STEP)
        .value("IGES", FileIO::FileType::IGES)
        .value("PVD", FileIO::FileType::PVD)
        .value("VTU", FileIO::FileType::VTU)
        .value("VTM", FileIO::FileType::VTM)
        .value("VTS", FileIO::FileType::VTS)
        .value("EX2", FileIO::FileType::EX2)
        .value("CGNS", FileIO::FileType::CGNS)
        .value("INP", FileIO::FileType::INP)
        .value("ODB", FileIO::FileType::ODB)
        .value("CAS", FileIO::FileType::CAS)
        .value("BDF", FileIO::FileType::BDF)
        .export_values();
}
