#ifndef iGameFileWriter_h
#define iGameFileWriter_h
/**
 * @class   iGameFileWriter
 * @brief   It is the base class of the iGame framework string stream output file
 *
 When outputting files, the file mapping method is used,
 and the character stream of the file needs to be saved in m_Buffers
 by the developer. Developers can process data in parallel,
 but need to put the data buffer into m_Buffers in order. In subsequent development,
 users can process data in a temporary buffer pool in parallel while
 outputting data to files, and finally pass the data in the temporary buffer pool
 to m_Buffers for output to files. However, due to some problems,
 the method of parallel data processing and data output has not been used yet.
 */

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameStructuredMesh.h"
#include "Mutex/iGameAtomicMutex.h"
#include "iGameThreadPool.h"
IGAME_NAMESPACE_BEGIN
class FileWriter : public Filter {
public:
	I_OBJECT(FileWriter);
	// static Pointer New() { return new FileWriter; }

	// Pure virtual function to generate buffers. Subclasses must implement this.
	virtual bool GenerateBuffers() = 0;

	// Write the current data object to a file at the path specified by m_FilePath.
	bool WriteToFile();

	// Write the specified data object to a file at the given file path.
	bool WriteToFile(DataObject::Pointer dataobject, const std::string filePath);

	// Save the buffer data to a file using a general method.
	bool SaveBufferDataToFile();

	// Save the buffer data to a file using Windows-specific methods.
	bool SaveBufferDataToFileWithWindows();

	// Save the buffer data to a file using Linux-specific methods.
	bool SaveBufferDataToFileWithLinux();

	// Save the buffer data to a file using Mac-specific methods.
	bool SaveBufferDataToFileWithMac();

	// Set the file path where the file will be saved.
	void SetFilePath(const std::string& filePath);

	// Set the file type to either ASCII or binary.
	void SetFileType(IGenum fileType) {
		if (fileType == IGAME_BINARY) {
			m_FileType = IGAME_BINARY;
		}
		else {
			m_FileType = IGAME_ASCII;
		}
	}

	// Set the data object to be written to the file.
	void SetDataObject(DataObject::Pointer dataobject);

	// Add a string to the buffer for writing to the file.
	void AddStringToBuffer(std::string data, CharArray::Pointer buffer);

	void TransferBuffer();
	int EncodeString(char* resname, const char* name);
protected:
	// Constructor for FileWriter.
	FileWriter();

	// Destructor for FileWriter.
	~FileWriter() override;

	// Data object that will be written to the file.
	DataObject::Pointer m_DataObject{ nullptr };

	// Path to the file that will be written.
	std::string m_FilePath = "default.txt";

	// Size of the file to be written.
	size_t m_FileSize = 0;

	/*
	The char buffer used to load data into the file.
	Regardless of how different types of files are written
	in parallel to a bunch of buffers, the buffers that end up
	in m_Buffers must be in order.
	*/
	std::vector<CharArray::Pointer> m_Buffers;

	/*
	The Temporary buffers, used for parallel processing.
	When writing buffered data of m_Buffers to a file,
	the data can be pre-processed and placed in the temporary buffers.
	*/
	std::vector<CharArray::Pointer> m_TemporaryBuffers;

	iGameAtomicMutex m_Lock;
	/*
	File type, may be either ASCII or binary.
	*/
	IGenum m_FileType{ IGAME_ASCII };

	int m_MaxThreadSize = 12;
};

IGAME_NAMESPACE_END
#endif