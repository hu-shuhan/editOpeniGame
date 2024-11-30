#include "iGameOBJReader.h"

IGAME_NAMESPACE_BEGIN
bool OBJReader::Parsing()
{
	int VertexNum = 0;
	int FaceNum = 0;

	Points::Pointer points = m_Data.GetPoints();
	CellArray::Pointer Faces = m_Data.GetFaces();
	igIndex vhs[256];
	char line[256];
	float coord[3] = { .0 };
	igIndex size = 0;
	const char* lineEnd = nullptr;
	igIndex lineNum=0;
	int UpdateSize=100000;
	while (this->IS<this->FILEEND&& this->ReadString(line)) {
		if (lineNum++ == UpdateSize) {
			lineNum = 0;
			this->UpdateReadProgress();
		}
		if (line[0] == '\0') {
			continue;
		}
		if (line[0] == 'v' && line[1] == '\0') {
			for (int i = 0; i < 3; i++) {
				if (!this->Read(coord + i)) {
					return false;
				}
			}
			points->AddPoint(coord);
			VertexNum++;
		}
		else if (line[0] == 'f' && line[1] == '\0') {
			size = 0;
			lineEnd = strchr(this->IS, '\n');
			if (!lineEnd)lineEnd = FILEEND;
			while (this->IS < lineEnd) {
				if (!this->Read(vhs + size)) {
					return false;
				}
				vhs[size]--;
				size++;
				this->SkipDataToSpace();
				this->SkipNullData();
			}
			this->IS = lineEnd + 1;
			Faces->AddCellIds(vhs, size);
			FaceNum++;
		}
	}
	this->UpdateProgress(1.0);
	return true;




	//auto& left = this->FILESTART;
	//char* right = this->FILEEND;


	//int vhs[64] = { 0 };
	//const char* data_st;
	//float data[3] = { 0,0,0 };
	//while (left < right) {
	//	// 查找行结束位置
	//	char* lineEnd = strchr(left, '\n');
	//	if (!lineEnd) {
	//		lineEnd = right;
	//	}
	//	std::string line(left, lineEnd);
	//	// 更新指针位置		
	//	if (line.size() < 1) {
	//		left = lineEnd + 1;
	//		continue;
	//	}
	//	if (line[0] == 'v' && line[1] == ' ') {
	//		data_st = left + 2;
	//		for (int i = 0; i < 3; i++) {
	//			data_st = mAtof(data_st, data[i]) + 1;
	//		}
	//		points->AddPoint(data);
	//		VertexNum++;
	//	}
	//	else if (line[0] == 'f' && line[1] == ' ') {
	//		int vh;
	//		data_st = left + 2;
	//		int size = 0;
	//		while (data_st < lineEnd && *data_st != '\r') {
	//			data_st = mAtoi(data_st, vh);
	//			if (*data_st != ' ') {
	//				data_st = strchr(data_st, ' ');
	//			}
	//			vhs[size++] = vh - 1;
	//			if (!data_st) {
	//				data_st = right;
	//			}
	//			else data_st++;
	//		}
	//		Faces->AddCellIds(vhs, size);
	//		FaceNum++;
	//	}
	//	left = lineEnd + 1;
	//}
	return true;
}
IGAME_NAMESPACE_END