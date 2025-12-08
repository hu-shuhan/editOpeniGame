#include "iGameFileIO.h"

#include "Abaqus/iGameODBReader.h"
#include "CGNS/iGameCGNSReader.h"
#include "FFMPEG/iGameFFMPEGVideoWriter.h"
#include "Fluent/iGameCASReader.h"
#include "IGC/iGameIGCReader.h"
#include "INP/iGameINPReader.h"
#include "MESH/iGameMESHReader.h"
#include "MESH/iGameMESHWriter.h"
#include "OBJ/iGameOBJReader.h"
#include "OBJ/iGameOBJWriter.h"
#include "OFF/iGameOFFReader.h"
#include "OFF/iGameOFFWriter.h"
#include "PLY/iGamePLYReader.h"
#include "PLY/iGamePLYWriter.h"
#include "STL/iGameSTLReader.h"
#include "STL/iGameSTLWriter.h"
#include "VTK XML/iGamePVDReader.h"
#include "VTK XML/iGameVTMReader.h"
#include "VTK XML/iGameVTSReader.h"
#include "VTK XML/iGameVTUReader.h"
#include "VTK/iGameVTKReader.h"
#include "VTK/iGameVTKWriter.h"
#include "iGameFileReader.h"
#include "iGameFileWriter.h"
#include <Nastran/iGameNastranReader.h>
#include <VTK XML/iGameVTMWriter.h>
#include <filesystem>

IGAME_NAMESPACE_BEGIN
IGenum FileIO::GetFileType(const std::string& file_name) {
    const char* pos = strrchr(file_name.data(), '.');
    const char* fileEnd = file_name.data() + file_name.size();
    std::string FileSuffix;
    if (pos != nullptr) { FileSuffix = std::string(pos + 1, fileEnd); }
    if (FileSuffix == "vtk") {
        return VTK;
    } else if (FileSuffix == "igc") {
        return IGC;
    } else if (FileSuffix == "obj") {
        return OBJ;
    } else if (FileSuffix == "mesh" || FileSuffix == "MESH") {
        return MESH;
    } else if (FileSuffix == "off") {
        return OFF;
    } else if (FileSuffix == "stl") {
        return STL;
    } else if (FileSuffix == "ply") {
        return PLY;
    } else if (FileSuffix == "step") {
        return STEP;
    } else if (FileSuffix == "igs" || FileSuffix == "iges" || FileSuffix == "IGS" || FileSuffix == "IGES") {
        return IGES;
    } else if (FileSuffix == "pvd") {
        return PVD;
    } else if (FileSuffix == "vts") {
        return VTS;
    } else if (FileSuffix == "vtu") {
        return VTU;
    } else if (FileSuffix == "vtm") {
        return VTM;
    } else if (FileSuffix == "e" || FileSuffix == "ex2" || FileSuffix == "EX2") {
        return EX2;
    } else if (FileSuffix == "cgns") {
        return CGNS;
    } else if (FileSuffix == "inp") {
        return INP;
    } else if (FileSuffix == "odb") {
        return ODB;
    } else if (FileSuffix == "cas") {
        return CAS;
    } else if (FileSuffix == "bdf") {
        return BDF;
    }
    return NONE;
}

std::string FileIO::GetFileTypeAsString(IGenum type) {
    switch (type) {
        case NONE:
            return "NONE";
        case VTK:
            return "VTK";
        case IGC:
            return "IGC";
        case OBJ:
            return "OBJ";
        case OFF:
            return "OFF";
        case MESH:
            return "MESH";
        case STL:
            return "STL";
        case PLY:
            return "PLY";
        case STEP:
            return "STEP";
        case IGES:
            return "IGES";
        case PVD:
            return "PVD";
        case VTS:
            return "VTS";
        case VTM:
            return "VTM";
        case VTU:
            return "VTU";
        case CGNS:
            return "CGNS";
        case INP:
            return "INP";
        case ODB:
            return "ODB";
        case BDF:
            return "BDF";
        case CAS:
            return "CAS";
        default:
            return "NONE";
    }
}

static AttributeSet::Pointer TransformScalars2VectorArray(AttributeSet* Attrs) {
    AttributeSet::Pointer newAttrs = AttributeSet::New();
    int size = Attrs->GetNumberOfAttributes();
    for (int i = 0; i < size; i++) {
        auto& attr = Attrs->GetAttribute(i);
        auto name = attr.pointer->GetName();
        if (attr.type != IG_SCALAR) {
            newAttrs->AddAttribute(attr.GetType(), attr.attachmentType, attr.pointer, attr.dataRange);
            continue;
        }
        if (attr.pointer->GetDimension() == 3) {
            newAttrs->AddAttribute(IG_VECTOR, attr.attachmentType, attr.pointer, attr.dataRange);
            continue;
        }
        // std::cout << "attachmentType:" << attachmentType << std::endl;
        bool isvector = false;
        if (name[name.length() - 1] == 'X') {
            isvector = true;
            int j = 1;
            for (j = 1; j < 3; j++) {
                if (i + j >= size) {
                    isvector = false;
                    break;
                }
                auto tmpName = Attrs->GetAttribute(i + j).pointer->GetName();
                if (tmpName[tmpName.length() - 1] != 'X' + j ||
                    Attrs->GetAttribute(i + j).GetAttachmentType() != attr.GetAttachmentType() ||
                    Attrs->GetAttribute(i + j).GetType() != IG_SCALAR) {
                    isvector = false;
                }
            }
        } else if (name[name.length() - 1] == '0') {
            isvector = true;
            int j = 1;
            for (j = 1; j < 3; j++) {
                if (i + j >= size) {
                    isvector = false;
                    break;
                }
                auto tmpName = Attrs->GetAttribute(i + j).pointer->GetName();
                if (tmpName[tmpName.length() - 1] != 'X' + j ||
                    Attrs->GetAttribute(i + j).GetAttachmentType() != attr.GetAttachmentType() ||
                    Attrs->GetAttribute(i + j).GetType() != IG_SCALAR) {
                    isvector = false;
                }
            }
        } else if (name[name.length() - 1] == '1') {
            isvector = true;
            int j = 1;
            for (j = 1; j < 3; j++) {
                if (i + j >= size) {
                    isvector = false;
                    break;
                }
                auto tmpName = Attrs->GetAttribute(i + j).pointer->GetName();
                if (tmpName[tmpName.length() - 1] != 'X' + j ||
                    Attrs->GetAttribute(i + j).GetAttachmentType() != attr.GetAttachmentType() ||
                    Attrs->GetAttribute(i + j).GetType() != IG_SCALAR) {
                    isvector = false;
                }
            }
        }

        if (!isvector) {
            newAttrs->AddAttribute(attr.type, attr.attachmentType, attr.pointer, attr.dataRange);
        } else {
            FloatArray::Pointer Vector = FloatArray::New();
            if (name[name.length() - 2] == '_') {
                Vector->SetName(name.substr(0, name.length() - 2));
            } else
                Vector->SetName(name.substr(0, name.length() - 1));
            Vector->SetName(name.substr(0, name.length() - 1));
            Vector->SetDimension(3);
            Vector->Resize(attr.pointer->GetNumberOfElements());
            float* vector = Vector->RawPointer();
            igIndex index = 0;
            int j = 0;
            for (j = 0; j < 3; j++) {
                auto scalarData = Attrs->GetAttribute(i + j).pointer;
                index = j;
                int k = 0;
                for (k = 0; k < attr.pointer->GetNumberOfElements(); k++) {
                    vector[index] = scalarData->GetValue(k);
                    index += 3;
                }
            }
            DoubleArray::Pointer newDataRange = DoubleArray::New();
            newDataRange->SetDimension(2);
            newDataRange->Resize(6 + 2);
            /* 将输入的x、y、z的维度标量范围直接更新成Vector的x、y、z维度的范围 */
            for(int j = 0; j < 3; j ++){
                auto scalarData = Attrs->GetAttribute(i + j).dataRange;
                newDataRange->SetElement(j + 1, scalarData->RawPointer());
            }
            /* 计算Vector 维度的Magnitude*/
            double maxMagnitude = DBL_MIN, minMagnitude = DBL_MAX;
            for (int k = 0; k < Vector->GetNumberOfValues(); k += 3) {
                double curMagnitude = std::sqrt(vector[k + 0] * vector[k + 0] + vector[k + 1] * vector[k + 1]  + vector[k + 2] * vector[k + 2]);
               maxMagnitude = std::max(maxMagnitude, curMagnitude);
               minMagnitude = std::min(minMagnitude, curMagnitude);
            }
            newDataRange->SetValue(0, minMagnitude);
            newDataRange->SetValue(1, maxMagnitude);

            newAttrs->AddAttribute(IG_VECTOR, attr.attachmentType, Vector, newDataRange);
            i += 2;
        }
    }
    return newAttrs;
}

DataObject::Pointer FileIO::ReadFile(const std::string& file_name) {
    IGenum fileType = GetFileType(file_name);
    std::string out;
    out.append("Read file [type: ");
    out.append(GetFileTypeAsString(fileType));
    clock_t start, end;
    DataObject::Pointer resObj = nullptr;

    start = clock();
    switch (fileType) {
        case NONE: {
            break;
        }
        case VTK: {
            VTKReader::Pointer reader = VTKReader::New();
            resObj = reader->ReadFile(file_name);
            break;
        }
        case IGC: {
            // MeshLoomDecoder* reader = new MeshLoomDecoder(file_name);
            // resObj = reader->Execute();

            IGCReader::Pointer reader = IGCReader::New();
            reader->SetFilePath(file_name);
            reader->Execute();
            resObj = reader->GetOutput();
            break;
        }
        case OBJ: {
            OBJReader::Pointer reader = OBJReader::New();
            reader->SetFilePath(file_name);
            reader->Execute();
            resObj = reader->GetOutput();
            break;
        }
        case iGame::FileIO::OFF: {
            OFFReader::Pointer reader = OFFReader::New();
            reader->SetFilePath(file_name);
            reader->Execute();
            resObj = reader->GetOutput();
            break;
        }
        case iGame::FileIO::MESH: {
            MESHReader::Pointer reader = MESHReader::New();
            reader->SetFilePath(file_name);
            reader->Execute();
            resObj = reader->GetOutput();
            break;
        }
        case iGame::FileIO::STL: {
            STLReader::Pointer reader = STLReader::New();
            reader->SetFilePath(file_name);
            reader->Execute();
            resObj = reader->GetOutput();
            break;
        }
        case iGame::FileIO::PLY: {
            PLYReader::Pointer reader = PLYReader::New();
            resObj = reader->ReadFile(file_name);
            break;
        }
#if defined(CGNS_ENABLE)
        case iGame::FileIO::CGNS: {
            iGameCGNSReader::Pointer reader = iGameCGNSReader::New();
            resObj = reader->ReadFile(file_name);

            break;
        }
#endif

        case iGame::FileIO::INP: {
            INPReader::Pointer reader = INPReader::New();
            resObj = reader->ReadFile(file_name);
            break;
        }
#if defined(AbqSDK_ENABLE)
        case iGame::FileIO::ODB: {
            ODBReader::Pointer reader = ODBReader::New();
            resObj = reader->ReadOdbFirstFrameMesh(file_name);
            //        resObj = reader->ReadOdbRawMesh(file_name);

            //		ODBReader::Pointer reader = ODBReader::New();
            //		resObj = reader->ReadOdbMesh(file_name);
            //        AttributeSet::Pointer attributeSet = reader->ReadOdbFieldData(file_name, "Step-1", 2);
            ////
            ////        for(int i = 0; i < attributeSet->GetAllAttributes()->GetNumberOfElements(); i ++){
            ////            auto attribute = attributeSet->GetAllAttributes()->GetElement(i);
            ////            attribute.updateAllDataRange();
            ////            auto range = attribute.GetDataRange();
            ////            std::cout << "===================\n";
            ////            for(int i = 200; i < 215; i += 3){
            ////                std::cout << attribute.pointer->GetValue(i) << ' ' << attribute.pointer->GetValue(i + 1) << ' ' << attribute.pointer->GetValue(i + 2) << '\n';
            ////            }
            ////            std::cout << attribute.pointer->GetName()<< " size " << attribute.pointer->GetNumberOfValues() << ' ' << "Range " << range->GetValue(0) << ' ' << range->GetValue(1) << '\n';
            ////        }
            //        resObj->SetAttributeSet(attributeSet);
            //        resObj->Modified();
            break;
        }
#endif


        //      case iGame::FileIO::STEP:
        //      {

        //      }
        //          break;
        //      case iGame::FileIO::IGES:
        //      {

        //      }
        //          break;
        case iGame::FileIO::VTS: {
            iGameVTSReader::Pointer reader = iGameVTSReader::New();
            reader->SetFilePath(file_name);
            reader->Execute();
            resObj = reader->GetOutput();
            break;
        }
        case iGame::FileIO::VTU: {
            iGameVTUReader::Pointer reader = iGameVTUReader::New();
            reader->SetFilePath(file_name);
            reader->Execute();
            resObj = reader->GetOutput();

            //        auto attributeSet = resObj->GetAttributeSet();
            //        for(int i = 0; i < attributeSet->GetAllAttributes()->GetNumberOfElements(); i ++){
            //            auto attribute = attributeSet->GetAllAttributes()->GetElement(i);
            //            attribute.updateAllDataRange();
            //            auto range = attribute.GetDataRange();
            //            std::cout << "===================\n";
            //            for(int i = 200; i < 215; i += 3){
            //                std::cout << attribute.pointer->GetValue(i) << ' ' << attribute.pointer->GetValue(i + 1) << ' ' << attribute.pointer->GetValue(i + 2) << '\n';
            //            }
            //            std::cout << attribute.pointer->GetName() << " size " << attribute.pointer->GetNumberOfValues() << ' ' << "Range " << range->GetValue(0) << ' ' << range->GetValue(1) << '\n';
            //        }
            break;
        }

        case iGame::FileIO::PVD: {
            iGamePVDReader::Pointer reader = iGamePVDReader::New();
            reader->SetFilePath(file_name);
            reader->Execute();
            resObj = reader->GetOutput();
            break;
        }
        case iGame::FileIO::VTM: {
            iGameVTMReader::Pointer reader = iGameVTMReader::New();
            reader->SetFilePath(file_name);
            reader->Execute();
            resObj = reader->GetOutput();
            break;
        }
        case iGame::FileIO::BDF: {
            NastranReader::Pointer reader = NastranReader::New();
            reader->SetFilePath(file_name);
            reader->Execute();
            resObj = reader->GetOutput();
            break;
        }
        case iGame::FileIO::CAS: {
            CASReader::Pointer reader = CASReader::New();
            reader->SetFilePath(file_name);
            reader->Execute();
            resObj = reader->GetOutput();
            break;
        }

        default:
            break;
    }

    std::filesystem::path pathObj(file_name);
    std::string baseName = pathObj.stem().string();
    if (resObj) {
        resObj->SetName(baseName);
    }

    end = clock();
    out.append(", success: ");
    out.append(resObj != nullptr ? "true" : "false");
    out.append(", time: ");
    out.append(FormatTime(end - start));
    out.append("]");
    igDebug(out);


//    if (resObj && resObj->GetAttributeSet()) { resObj->SetAttributeSet(TransformScalars2VectorArray(resObj->GetAttributeSet())); }
    return resObj;
}


bool FileIO::WriteFile(const std::string& file_name, DataObject::Pointer dataObject) {
    IGenum fileType = GetFileType(file_name);
    std::string out;
    out.append("Write file [type: ");
    out.append(GetFileTypeAsString(fileType));
    clock_t start, end;
    bool result = false;
    start = clock();
    switch (fileType) {
        case NONE: {
            IGAME_CORE_ERROR("Not support write this type!");
            break;
        }
        case VTK: {
            VTKWriter::Pointer writer = VTKWriter::New();
            result = writer->WriteToFile(dataObject, file_name);
            break;
        }
        case OBJ: {
            OBJWriter::Pointer writer = OBJWriter::New();
            result = writer->WriteToFile(dataObject, file_name);
            break;
        }
        case iGame::FileIO::OFF: {
            OFFWriter::Pointer writer = OFFWriter::New();
            result = writer->WriteToFile(dataObject, file_name);
            break;
        }
        case iGame::FileIO::MESH: {
            MESHWriter::Pointer writer = MESHWriter::New();
            result = writer->WriteToFile(dataObject, file_name);
            break;
        }
        case iGame::FileIO::STL: {
            STLWriter::Pointer writer = STLWriter::New();
            writer->SetBinary(true); // set file format
            result = writer->WriteToFile(dataObject, file_name);
            break;
        }
        case iGame::FileIO::PLY: {
            PLYWriter::Pointer writer = PLYWriter::New();
            result = writer->WriteToFile(dataObject, file_name);
            break;
        }
#if defined(CGNS_ENABLE)
        case iGame::FileIO::CGNS: {
            break;
        }
#endif

        case iGame::FileIO::INP: {
            break;
        }
        //      case iGame::FileIO::STEP:
        //      {

        //      }
        //          break;
        //      case iGame::FileIO::IGES:
        //      {

        //      }
        //          break;
        case iGame::FileIO::VTS: {

            break;
        }
        case iGame::FileIO::VTU: {

            break;
        }

        case iGame::FileIO::PVD: {

            break;
        }
        case iGame::FileIO::VTM: {
            VTMWriter::Pointer writer = VTMWriter::New();
            result = writer->WriteToFile(dataObject, file_name);
            break;
        }
        default:
            break;
    }

    end = clock();
    out.append(", success: ");
    out.append(result ? "true" : "false");
    out.append(", time: ");
    out.append(FormatTime(end - start));
    out.append("]");
    //igDebug(out);
    IGAME_CORE_DEBUG("{}", out);
    return result;
}
IGAME_NAMESPACE_END
