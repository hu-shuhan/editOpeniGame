#include "iGameFileWriter.h"

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MAC)
#include <fcntl.h>    
#include <sys/mman.h> 
#include <unistd.h>   
#include <sys/types.h>
#include <sys/stat.h>
#endif

IGAME_NAMESPACE_BEGIN

FileWriter::FileWriter()
{

}
FileWriter::~FileWriter()
{
	std::vector<CharArray::Pointer> temp;
	m_Buffers.swap(temp);
	m_TemporaryBuffers.swap(temp);
}
bool FileWriter::WriteToFile()
{
	if (!GenerateBuffers()) {
		igError("Could not generate buffer to load.");
		return false;
	}
	return SaveBufferDataToFile();

}
bool FileWriter::WriteToFile(DataObject::Pointer dataObject, std::string filePath)
{
	this->m_DataObject = dataObject;
	this->m_FilePath = filePath;
	return WriteToFile();
}

bool FileWriter::SaveBufferDataToFile()
{
	FILE* file = fopen(this->m_FilePath.c_str(), "wb");
	if (file == NULL) {
		perror("fopen failed");
		return false;
	}
	size_t fileSize = 0;
	for (int i = 0; i < m_Buffers.size(); i++) {
		if (m_Buffers[i]) {
			fileSize += m_Buffers[i]->GetNumberOfValues();
		}
	}
	for (int i = 0; i < m_Buffers.size(); i++) {
		if (m_Buffers[i] && m_Buffers[i]->RawPointer()) {
			size_t bufferSize = m_Buffers[i]->GetNumberOfValues();
			if (fwrite(m_Buffers[i]->RawPointer(), 1, bufferSize, file) != bufferSize) {
				perror("fwrite failed");
				fclose(file);
				return false;
			}
		}
	}
	m_Buffers.clear();
	fclose(file);
	return true;
//下面的是内存映射方式，效率没有fwrite高
#ifdef PLATFORM_WINDOWS
	return SaveBufferDataToFileWithWindows();
#elif defined(PLATFORM_LINUX)
	return SaveBufferDataToFileWithLinux();
#elif defined(PLATFORM_MAC)
	return SaveBufferDataToFileWithMac();
#endif
}
bool FileWriter::SaveBufferDataToFileWithWindows()
{
#ifdef PLATFORM_WINDOWS
	clock_t time_1 = clock();
	// 打开文件

	HANDLE hFile = CreateFile(this->m_FilePath.data(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		_tprintf(_T("CreateFile failed with error: %lu\n"), GetLastError());
		return false;
	}

	// 计算新的文件大小
	m_FileSize = 0;
	for (int i = 0; i < m_Buffers.size(); i++) {
		if (m_Buffers[i]) {
			m_FileSize += m_Buffers[i]->GetNumberOfValues();
		}
	}

	// 创建文件映射，大小为新的文件大小
	HANDLE hMapFile = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, m_FileSize, NULL);
	if (hMapFile == NULL) {
		_tprintf(_T("CreateFileMapping failed with error: %lu\n"), GetLastError());
		CloseHandle(hFile);
		return false;
	}

	// 将文件映射到内存
	LPVOID lpBaseAddress = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, m_FileSize);
	if (lpBaseAddress == NULL) {
		_tprintf(_T("MapViewOfFile failed with error: %lu\n"), GetLastError());
		CloseHandle(hMapFile);
		CloseHandle(hFile);
		return false;
	}

	// 将缓冲区数据写入文件
	size_t offset = 0;
	for (int i = 0; i < m_Buffers.size(); i++) {
		if (m_Buffers[i] && m_Buffers[i]->RawPointer()) {
			memcpy((char*)lpBaseAddress + offset, m_Buffers[i]->RawPointer(), m_Buffers[i]->GetNumberOfValues());
			offset += m_Buffers[i]->GetNumberOfValues();
		}
	}
	m_Buffers.clear();
	// 解除映射
	UnmapViewOfFile(lpBaseAddress);
	CloseHandle(hMapFile);
	CloseHandle(hFile);

	clock_t time_2 = clock();
	std::cout << "Write buffer to file cost " << time_2 - time_1 << "ms\n";
#endif
	return true;
}

bool FileWriter::SaveBufferDataToFileWithLinux()
{
#ifdef PLATFORM_LINUX

	// 打开文件
	int fd = open(this->m_FilePath.c_str(), O_RDWR | O_CREAT, 0666);
	if (fd == -1) {
		perror("open failed");
		return false;
	}

	// 计算文件大小
	m_FileSize = 0;
	for (int i = 0; i < m_Buffers.size(); i++) {
		if (m_Buffers[i]) {
			m_FileSize += m_Buffers[i]->GetNumberOfValues();
		}
	}

	// 调整文件大小
	if (ftruncate(fd, m_FileSize) == -1) {
		perror("ftruncate failed");
		close(fd);
		return false;
	}

	// 使用mmap将文件映射到内存
	void* map = mmap(NULL, m_FileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		perror("mmap failed");
		close(fd);
		return false;
	}

	// 将缓冲区数据写入文件
	size_t offset = 0;
	for (int i = 0; i < m_Buffers.size(); i++) {
		if (m_Buffers[i] && m_Buffers[i]->RawPointer()) {
			memcpy((char*)map + offset, m_Buffers[i]->RawPointer(), m_Buffers[i]->GetNumberOfValues());
			offset += m_Buffers[i]->GetNumberOfValues();
		}
	}
	m_Buffers.clear();
	// 清除映射并关闭文件
	if (munmap(map, m_FileSize) == -1) {
		perror("munmap failed");
	}
	close(fd);

	std::cout << "Write buffer to file with mmap (Linux) successful.\n";

#endif
	return true;
}
bool FileWriter::SaveBufferDataToFileWithMac()
{
#ifdef PLATFORM_MAC
	// 使用mmap在macOS实现文件映射
	int fd = open(this->m_FilePath.c_str(), O_RDWR | O_CREAT, 0666);
	if (fd == -1) {
		perror("open failed");
		return false;
	}

	// 计算文件大小
	m_FileSize = 0;
	for (int i = 0; i < m_Buffers.size(); i++) {
		if (m_Buffers[i]) {
			m_FileSize += m_Buffers[i]->GetNumberOfValues();
		}
	}

	// 调整文件大小
	if (ftruncate(fd, m_FileSize) == -1) {
		perror("ftruncate failed");
		close(fd);
		return false;
	}

	// 使用mmap将文件映射到内存
	void* map = mmap(NULL, m_FileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		perror("mmap failed");
		close(fd);
		return false;
	}

	// 写入缓冲区数据到文件
	size_t offset = 0;
	for (int i = 0; i < m_Buffers.size(); i++) {
		if (m_Buffers[i] && m_Buffers[i]->RawPointer()) {
			memcpy((char*)map + offset, m_Buffers[i]->RawPointer(), m_Buffers[i]->GetNumberOfValues());
			offset += m_Buffers[i]->GetNumberOfValues();
		}
	}
	m_Buffers.clear();
	// 清除映射并关闭文件
	if (munmap(map, m_FileSize) == -1) {
		perror("munmap failed");
	}
	close(fd);

	std::cout << "Write buffer to file with mmap (macOS) successful.\n";
#endif
	return true;
}

void FileWriter::SetFilePath(const std::string& filePath)
{
	this->m_FilePath = filePath;
}

void FileWriter::SetDataObject(DataObject::Pointer dataObject)
{
	this->m_DataObject = dataObject;
}
void FileWriter::AddStringToBuffer(std::string data, CharArray::Pointer buffer)
{
	if (!buffer)return;
	for (int i = 0; i < data.size(); i++) {
		buffer->AddValue(data[i]);
	}
	return;
}

void FileWriter::TransferBuffer()
{
	for (int i = 0; i < m_TemporaryBuffers.size(); i++) {
		m_Buffers.emplace_back(m_TemporaryBuffers[i]);
	}
	m_TemporaryBuffers.clear();
}
int FileWriter::EncodeString(char* resname, const char* name)
{
	if (!resname || !name)
	{
		return 0;
	}

	std::ostringstream str;
	size_t cc = 0;
	size_t reslen = 0;
	char buffer[4]; // To hold "%xx" format

	while (name[cc])
	{
		unsigned char ch = static_cast<unsigned char>(name[cc]);

		// Encode only non-alphanumeric characters (you can adjust this condition as needed)
		if (!isalnum(ch))
		{
			snprintf(buffer, sizeof(buffer), "%%%02X", ch);
			str << buffer;
			reslen += 3; // "%xx" is 3 characters
		}
		else
		{
			str << name[cc];
			reslen++;
		}
		cc++;
	}

	strncpy(resname, str.str().c_str(), reslen + 1);
	resname[reslen] = 0;

	return static_cast<int>(reslen);
}
IGAME_NAMESPACE_END