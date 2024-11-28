#include "iGameOFFReader.h"

IGAME_NAMESPACE_BEGIN
bool OFFReader::Parsing()
{
	int VertexNum = 0;
	int FaceNum = 0;
	int LineNum = 0;
	auto& left = this->IS;
	const char* right = this->FILEEND;

	while (left < right)
	{
		while (*left == ' ')left++;
		const char* lineEnd = strchr(left, '\n');
		if (!lineEnd) {
			lineEnd = right;
		}
		if (*left == '\r' || *left == '\n') {
			left = lineEnd + 1;
			continue;
		}
		std::string line(left, lineEnd);
		// 更新指针位置
		left = lineEnd + 1;
		if (line == "OFF" || line == "OFF\r") {
			left = ReadNums(left, VertexNum, FaceNum, LineNum);
			this->UpdateReadProgress();
			left = ReadPoints(left, VertexNum);
			this->UpdateReadProgress();
			left = ReadFaces(left, FaceNum);
			this->UpdateReadProgress();
			//left = ReadLines(left, LineNum, Lines);
			this->UpdateReadProgress();
			break;
		}
	}

	return true;
}


const char* OFFReader::ReadNums(const char* left, int& VertexNum, int& FaceNum, int& LineNum)
{
	const char* lineEnd = strchr(left, '\n') + 1;
	while (*left == ' ')left++;
	left = mAtoi(left, VertexNum) + 1;
	while (*left == ' ')left++;
	left = mAtoi(left, FaceNum) + 1;
	while (*left == ' ')left++;
	return lineEnd;
}

const char* OFFReader::ReadPoints(const char* left, int VertexNum)
{
	Points::Pointer Points = m_Data.GetPoints();
	Points->Reserve(VertexNum);
	float p[3] = { 0 };
	const char* lineEnd;
	for (int i = 0; i < VertexNum; i++)
	{
		while (*left == ' ')left++;
		lineEnd = strchr(left, '\n');
		if (*left == '\r' || *left == '\n') {
			i--;
			left = lineEnd + 1;
			continue;
		}
		for (int j = 0; j < 3; j++) {
			left = mAtof(left, p[j]) + 1;
		}
		left = lineEnd + 1;
		Points->AddPoint(p);
	}

	return left;
}

const char* OFFReader::ReadFaces(const char* left, int FaceNum)
{
	CellArray::Pointer Faces = m_Data.GetFaces();
	const char* lineEnd;
	int vhs[16] = { 0 };
	for (int i = 0; i < FaceNum; i++)
	{
		while (*left == ' ')left++;
		lineEnd = strchr(left, '\n');
		if (*left == '\r' || *left == '\n') {
			i--;
			left = lineEnd + 1;
			continue;
		}
		int Component;
		left = mAtoi(left, Component) + 1;
		for (int j = 0; j < Component; j++) {
			left = mAtoi(left, vhs[j]) + 1;
		}
		Faces->AddCellIds(vhs, Component);
		left = lineEnd + 1;
	}
	return left;
}

const char* OFFReader::ReadLines(const char* left, int LineNum, CellArray::Pointer Lines)
{
	return left;
}
IGAME_NAMESPACE_END