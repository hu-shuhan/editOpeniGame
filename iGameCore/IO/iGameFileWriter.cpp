#include "iGameFileWriter.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>

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

namespace {

/*
写文件（保存网格）期间的进度上报状态。

为什么要用线程局部变量而不是成员：下面这几个量描述的是「一次保存操作」，
而保存可能是嵌套的（iGameVTMWriter / iGameIGCMWriter 内部会逐块 new 一个子 writer
再 Execute）。嵌套时只允许最外层那一个 writer 负责进度与文案，否则子文件写完后会把
进度推回 0%（进度条组件的 100% 会自动归零），并覆盖外层「多块编码 3/10」的文案。
*/
thread_local const FileWriter* t_WriteProgressOwner = nullptr;
thread_local std::chrono::steady_clock::time_point t_WriteStartTime{};
thread_local int t_WriteTickCount = 0;
thread_local double t_WriteLastTickElapsed = -1.0;

// 「生成数据」阶段的进度曲线：无法预知最终输出多少字节，只能按时间渐近推进，
// 上限留在 0.75，剩下的交给真正写盘（0.78 → 0.97）与收尾的 1.0。
constexpr double kGenerateProgressBegin = 0.02;
constexpr double kGenerateProgressEnd = 0.75;
constexpr double kGenerateTimeConstant = 5.0; // 秒
// 写盘阶段映射到的区间。
constexpr double kFlushProgressShift = 0.78;
constexpr double kFlushProgressScale = 0.19;
// 节流：每 kTickCallStride 次写入才查一次时钟，两次上报至少间隔 kTickInterval 秒。
// AddStringToBuffer 是热循环里每行都会调用的，不能每次都取时钟、更不能每次都发事件。
constexpr int kTickCallStride = 512;
constexpr double kTickInterval = 0.1;

// 取扩展名 / 文件名用于状态栏文案（纯字符串处理，不经过文件系统，避免编码问题）。
std::string FileSuffixOf(const std::string& path) {
    const auto slash = path.find_last_of("/\\");
    const auto dot = path.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) { return std::string(); }
    return path.substr(dot + 1);
}

std::string FileNameOf(const std::string& path) {
    const auto slash = path.find_last_of("/\\");
    return (slash == std::string::npos) ? path : path.substr(slash + 1);
}

} // namespace

FileWriter::FileWriter()
{
	this->m_FileSize=0;
	this->SetNumberOfInputs(1);
	this->SetNumberOfOutputs(0);
}
FileWriter::~FileWriter()
{
	std::vector<CharArray::Pointer> temp;
	m_Buffers.swap(temp);
	m_TemporaryBuffers.swap(temp);
}
bool FileWriter::Execute()
{
	this->m_DataObject = this->m_Inputs->GetElement(0);
	if(!m_DataObject){
		igDebug("could not write nullptr object!");
		return false;
	}
	this->BeginWriteProgress();
	// 生成数据阶段：无法预知总输出量，进度由 TickWriteProgress 按时间渐近推进。
	if (!GenerateBuffers()) {
		IGAME_CORE_ERROR("Could not generate buffer to load.");
		this->EndWriteProgress();
		return false;
	}
	// 写盘阶段：把 SaveBufferDataToFile 内部 0→1 的进度映射到 0.78→0.97 这一段。
	// 一次写出几百 MB 时这段同样耗时，不能没有反馈。
	this->SetProgressRange(kFlushProgressShift, kFlushProgressScale);
	const bool saved = SaveBufferDataToFile();
	this->EndWriteProgress();
	return saved;
}

void FileWriter::BeginWriteProgress()
{
	// 嵌套写出（.igcm/.vtm 内部的子 block）不接管进度，交给最外层。
	if (t_WriteProgressOwner != nullptr) { return; }
	if (m_ProgressObserver == nullptr) { return; }

	t_WriteProgressOwner = this;
	t_WriteStartTime = std::chrono::steady_clock::now();
	t_WriteTickCount = 0;
	t_WriteLastTickElapsed = -1.0;

	// 生成阶段同样阻塞 GUI 线程：置位后进度条组件才会把进度真正画出来
	// （见 ProgressObserver::SetBlockingGuiThread）。
	m_ProgressObserver->SetBlockingGuiThread(true);

	const std::string suffix = FileSuffixOf(m_FilePath);
	m_ProgressObserver->UpdateText(suffix.empty() ? std::string("正在生成文件数据…")
	                                              : "正在生成 " + suffix + " 文件数据…");
	// 先给一个非 0 值：进度条组件把 0 当作「复位」，不会显示。
	this->UpdateProgress(kGenerateProgressBegin);
}

void FileWriter::TickWriteProgress()
{
	// 任何 writer（含嵌套的子 writer）都能推动同一条时间曲线，它们在等长的时间段里
	// 干的都是「生成数据」这件事；但上报对象始终是最外层那一次保存。
	if (t_WriteProgressOwner == nullptr) { return; }
	if (++t_WriteTickCount < kTickCallStride) { return; }
	t_WriteTickCount = 0;

	const double elapsed =
			std::chrono::duration<double>(std::chrono::steady_clock::now() - t_WriteStartTime).count();
	if (t_WriteLastTickElapsed >= 0.0 && elapsed - t_WriteLastTickElapsed < kTickInterval) { return; }
	t_WriteLastTickElapsed = elapsed;

	auto* observer = ProgressObserver::Instance();
	if (observer == nullptr) { return; }
	const double fraction = kGenerateProgressBegin + (kGenerateProgressEnd - kGenerateProgressBegin) *
	                                                     (1.0 - std::exp(-elapsed / kGenerateTimeConstant));
	// 直接发给全局观察者：本对象可能不是进度区间（shift/scale）的拥有者。
	observer->UpdateProgress(fraction);
}

void FileWriter::EndWriteProgress()
{
	// 只由最外层收尾；子 writer 结束后不能把外层的进度状态清掉。
	if (t_WriteProgressOwner != this) { return; }
	t_WriteProgressOwner = nullptr;
	t_WriteTickCount = 0;
	t_WriteLastTickElapsed = -1.0;

	// 复位区间，避免把 shift/scale 留给下一次复用同一个 writer 的调用。
	m_Progress = 0.0;
	m_ProgressShift = 0.0;
	m_ProgressScale = 1.0;

	if (m_ProgressObserver == nullptr) { return; }
	m_ProgressObserver->SetBlockingGuiThread(false);
	m_ProgressObserver->UpdateProgress(1.0); // 让进度条走到「完成」分支（会自行归零并隐藏）
	m_ProgressObserver->UpdateText("");      // 清空文案，避免状态栏残留「正在写入…」
}
bool FileWriter::WriteToFile(DataObject::Pointer dataObject,const std::string filePath)
{
	this->SetInput(dataObject);
	this->SetFilePath(filePath);
	return Execute();
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
	if (t_WriteProgressOwner == this) {
		const std::string fileName = FileNameOf(m_FilePath);
		m_ProgressObserver->UpdateText(fileName.empty() ? std::string("正在写入文件…")
		                                                : "正在写入文件 " + fileName + "…");
	}
	const size_t bufferCount = m_Buffers.size();
	// 大缓冲区按块写出：一次 fwrite 几百 MB 时中间没有任何进度可上报，
	// 分块后每块都能回报一次（Execute() 已把本阶段映射到 0.78→0.97）。
	constexpr size_t kWriteChunkSize = 8u * 1024u * 1024u;
	for (int i = 0; i < m_Buffers.size(); i++) {
		const bool reportProgress = (t_WriteProgressOwner == this) && bufferCount > 0;
		const char* source = (m_Buffers[i] != nullptr) ? m_Buffers[i]->RawPointer() : nullptr;
		const size_t bufferSize = (source != nullptr) ? m_Buffers[i]->GetNumberOfValues() : 0;
		if (bufferSize == 0) {
			if (reportProgress) { this->UpdateProgress(static_cast<double>(i + 1) / static_cast<double>(bufferCount)); }
			continue;
		}
		for (size_t written = 0; written < bufferSize;) {
			const size_t chunkSize = std::min(kWriteChunkSize, bufferSize - written);
			if (fwrite(source + written, 1, chunkSize, file) != chunkSize) {
				perror("fwrite failed");
				fclose(file);
				return false;
			}
			written += chunkSize;
			if (reportProgress) {
				const double bufferFraction = static_cast<double>(written) / static_cast<double>(bufferSize);
				this->UpdateProgress((static_cast<double>(i) + bufferFraction) / static_cast<double>(bufferCount));
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
    IGAME_CORE_TRACE("Write buffer to file cost {} ms", time_2 - time_1);
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

    IGAME_CORE_TRACE("Write buffer to file with mmap (Linux) successful.");

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
    IGAME_CORE_TRACE("Write buffer to file with mmap (macOS) successful.")
#endif
	return true;
}

void FileWriter::SetFilePath(const std::string& filePath)
{
	this->m_FilePath = filePath;
}

void FileWriter::SetDataObject(DataObject::Pointer dataObject)
{
	this->SetInput(0, dataObject);
}
void FileWriter::AddStringToBuffer(std::string data, CharArray::Pointer buffer)
{
	if (!buffer)return;
	for (int i = 0; i < data.size(); i++) {
		buffer->AddValue(data[i]);
	}
	// 所有 ASCII/XML writer 都是一行一次地调这里，正好是「生成数据」阶段唯一的公共钩子：
	// 借它按时间把进度条往前推（内部有次数+时间双重节流，几乎零开销）。
	this->TickWriteProgress();
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