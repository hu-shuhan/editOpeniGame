#include "iGameFileIO.h"

#include "VTK/iGameVTKReader.h"
#include "iGameMeshCodec/iGameMeshDecoder.h"
#include "VTK/iGameVTKWriter.h"
#include "OFF/iGameOFFReader.h"
#include "OFF/iGameOFFWriter.h"
#include "MESH/iGameMESHReader.h"
#include "MESH/iGameMESHWriter.h"
#include "OBJ/iGameOBJReader.h"
#include "OBJ/iGameOBJWriter.h"
#include "STL/iGameSTLReader.h"
#include "STL/iGameSTLWriter.h"
#include "PLY/iGamePLYReader.h"
#include "PLY/iGamePLYWriter.h"
#include "VTK XML/iGamePVDReader.h"
#include "VTK XML/iGameVTSReader.h"
#include "VTK XML/iGameVTUReader.h"
#include "VTK XML/iGameVTMReader.h"
#include "INP/iGameINPReader.h"
#include "CGNS/iGameCGNSReader.h"
#include "Abaqus/iGameODBReader.h"
#include "FFMPEG/iGameFFMPEGVideoWriter.h"

IGAME_NAMESPACE_BEGIN
IGenum FileIO::GetFileType(const std::string& file_name)
{
	const char* pos = strrchr(file_name.data(), '.');
	const char* fileEnd = file_name.data() + file_name.size();
	std::string FileSuffix;
	if (pos != nullptr) {
		FileSuffix = std::string(pos + 1, fileEnd);
	}
	if (FileSuffix == "vtk") {
		return VTK;
	} 
	else if (FileSuffix == "igc") {
        return IGC;
    }
	else if (FileSuffix == "obj") {
		return OBJ;
	}
	else if (FileSuffix == "mesh" || FileSuffix == "MESH") {
		return MESH;
	}
	else if (FileSuffix == "off") {
		return OFF;
	}
	else if (FileSuffix == "stl") {
		return STL;
	}
	else if (FileSuffix == "ply") {
		return PLY;
	}
	else if (FileSuffix == "step") {
		return STEP;
	}
	else if (FileSuffix == "igs" || FileSuffix == "iges" || FileSuffix == "IGS" || FileSuffix == "IGES") {
		return IGES;
	}
	else if (FileSuffix == "pvd") {
		return PVD;
	}
	else if (FileSuffix == "vts") {
		return VTS;
	}
	else if (FileSuffix == "vtu") {
		return VTU;
	}
	else if (FileSuffix == "vtm") {
		return VTM;
	}
	else if (FileSuffix == "e" || FileSuffix == "ex2" || FileSuffix == "EX2") {
		return EX2;
	}
	else if (FileSuffix == "cgns") {
		return CGNS;
	}
	else if (FileSuffix == "inp") {
		return INP;
	} else if (FileSuffix == "odb") {
		return ODB;
	}
	return NONE;
}

std::string FileIO::GetFileTypeAsString(IGenum type)
{
	switch (type)
	{
	case NONE:return "NONE";
	case VTK:return "VTK";
	case OBJ:return "OBJ";
	case OFF:return "OFF";
	case MESH:return "MESH";
	case STL:return "STL";
	case PLY:return "PLY";
	case STEP:return "STEP";
	case IGES:return "IGES";
	case PVD:return "PVD";
	case VTS:return "VTS";
	case VTM:return "VTM";
	case VTU:return "VTU";
	case CGNS:return "CGNS";
	case INP:return "INP";
	case ODB:return "ODB";
	default:return "NONE";
	}
}

DataObject::Pointer FileIO::ReadFile(const std::string& file_name)
{
	IGenum fileType = GetFileType(file_name);
	std::string out;
	out.append("Read file [type: ");
	out.append(GetFileTypeAsString(fileType));
	clock_t start, end;
	DataObject::Pointer resObj = nullptr;

	start = clock();
	switch (fileType)
	{
	case NONE:
	{
		break;
	}
	case VTK:
	{
		VTKReader::Pointer reader = VTKReader::New();
		resObj = reader->ReadFile(file_name);
		break;
	}
    case IGC: {
        MeshDecoder::Pointer reader = MeshDecoder::New();
        resObj = reader->ReadFile(file_name);
        break;
    }
	case OBJ:
	{
		OBJReader::Pointer reader = OBJReader::New();
		reader->SetFilePath(file_name);
		reader->Execute();
		resObj = reader->GetOutput();
		break;
	}
	case iGame::FileIO::OFF:
	{
		OFFReader::Pointer reader = OFFReader::New();
		reader->SetFilePath(file_name);
		reader->Execute();
		resObj = reader->GetOutput();
		break;
	}
	case iGame::FileIO::MESH:
	{
		MESHReader::Pointer reader = MESHReader::New();
		reader->SetFilePath(file_name);
		reader->Execute();
		resObj = reader->GetOutput();
		break;
	}
	case iGame::FileIO::STL:
	{
		STLReader::Pointer reader = STLReader::New();
		reader->SetFilePath(file_name);
		reader->Execute();
		resObj = reader->GetOutput();
		break;
	}
	case iGame::FileIO::PLY:
	{
		PLYReader::Pointer reader = PLYReader::New();
		resObj = reader->ReadFile(file_name);
		break;
	}
#if defined(CGNS_ENABLE)
	case iGame::FileIO::CGNS:
	{
		iGameCGNSReader::Pointer reader = iGameCGNSReader::New();
		resObj = reader->ReadFile(file_name);
		break;
	}
#endif

	case iGame::FileIO::INP:
	{
		INPReader::Pointer reader = INPReader::New();
		resObj = reader->ReadFile(file_name);
		break;
	}
#if defined(AbqSDK_ENABLE)
    case iGame::FileIO::ODB:
	{
        ODBReader::Pointer reader = ODBReader::New();
        resObj = reader->ReadOdbFirstFrameMesh(file_name);

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
	case iGame::FileIO::VTS:
	{
		iGameVTSReader::Pointer reader = iGameVTSReader::New();
		reader->SetFilePath(file_name);
		reader->Execute();
		resObj = reader->GetOutput();
		break;
	}
	case iGame::FileIO::VTU:
	{
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

	case iGame::FileIO::PVD:
	{
		iGamePVDReader::Pointer reader = iGamePVDReader::New();
		reader->SetFilePath(file_name);
		reader->Execute();
		resObj = reader->GetOutput();
		break;
	}
	case iGame::FileIO::VTM:
	{
		iGameVTMReader::Pointer reader = iGameVTMReader::New();
		reader->SetFilePath(file_name);
		reader->Execute();
		resObj = reader->GetOutput();
		break;
	}
	default:
		break;
	}

	end = clock();
	out.append(", success: ");
	out.append(resObj != nullptr ? "true" : "false");
	out.append(", time: ");
	out.append(FormatTime(end - start));
	out.append("]");
	//igDebug(out);
	std::cout << out << std::endl;
	return resObj;
}


bool  FileIO::WriteFile(const std::string& file_name, DataObject::Pointer dataObject)
{
	IGenum fileType = GetFileType(file_name);
	std::string out;
	out.append("Write file [type: ");
	out.append(GetFileTypeAsString(fileType));
	clock_t start, end;
	bool result = false;
	start = clock();
	switch (fileType)
	{
	case NONE:
	{
		std::cout << "Not support write this type!\n";
		break;
	}
	case VTK:
	{
		VTKWriter::Pointer writer = VTKWriter::New();
		result = writer->WriteToFile(dataObject, file_name);
		break;
	}
	case OBJ:
	{
		OBJWriter::Pointer writer = OBJWriter::New();
		result = writer->WriteToFile(dataObject, file_name);
		break;
	}
	case iGame::FileIO::OFF:
	{
		OFFWriter::Pointer writer = OFFWriter::New();
		result = writer->WriteToFile(dataObject, file_name);
		break;
	}
	case iGame::FileIO::MESH:
	{
		MESHWriter::Pointer writer = MESHWriter::New();
		result = writer->WriteToFile(dataObject, file_name);
		break;
	}
	case iGame::FileIO::STL:
	{
		STLWriter::Pointer writer = STLWriter::New();
		writer->SetBinary(true); // set file format
		result = writer->WriteToFile(dataObject, file_name);
		break;
	}
	case iGame::FileIO::PLY:
	{
		PLYWriter::Pointer writer = PLYWriter::New();
		result = writer->WriteToFile(dataObject, file_name);
		break;
	}
#if defined(CGNS_ENABLE)
	case iGame::FileIO::CGNS:
	{
		break;
	}
#endif

	case iGame::FileIO::INP:
	{
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
	case iGame::FileIO::VTS:
	{

		break;
	}
	case iGame::FileIO::VTU:
	{

		break;
	}

	case iGame::FileIO::PVD:
	{

		break;
	}
	case iGame::FileIO::VTM:
	{

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
	std::cout << out << std::endl;
	return result;
}
IGAME_NAMESPACE_END
