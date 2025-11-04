#include "iGameFileReader.h"
#include "iGameByteSwap.h"
#include "iGameStringArray.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN
FileReader::FileReader() {
    this->SetNumberOfInputs(0);
    this->SetNumberOfOutputs(1);
    this->IS = nullptr;
    this->file_ = nullptr;
    this->m_FileSize = 0;
}
DataObject::Pointer FileReader::ReadFile(const std::string& filePath) {
    SetFilePath(filePath);
    Execute();
    return this->GetOutput();
}
bool FileReader::Execute() {
    clock_t start, end;
    start = clock();

    clock_t time1 = clock();
    if (!Open()) {
        std::cerr << "Open failure\n";
        return false;
    }
    clock_t time2 = clock();
    //std::cout << "Read file to buffer Cost " << time2 - time1 << "ms\n";
    if (!Parsing()) {
        std::cerr << "Parsing failure\n";
        return false;
    }
    if (!Close()) {
        std::cerr << "Close failure\n";
        return false;
    }
    if (!CreateDataObject()) {
        std::cerr << "Generate DataObject failure\n";
        return false;
    }
    clock_t time3 = clock();
    //std::cout << "Generate DataObject Cost " << time3 - time2 << "ms\n";
    
    if (m_Output) {
        m_Output->GetPropertys()->AddProperty(Variant::LongLong, "FileSize")->SetValue(static_cast<long long>(m_FileSize));
    }
    
    this->SetOutput(0, m_Output);
    end = clock();
    std::cout << "Read file success! The time cost: " << end - start << "ms" << std::endl;
    return true;
}

bool FileReader::Open() {
    if (m_FilePath.empty()) { return false; }
    bool isOpenOK = false;

#ifdef PLATFORM_WINDOWS
    isOpenOK = OpenWithWindowsSystem();
#elif defined(PLATFORM_LINUX)
    isOpenOK = OpenWithLinuxOrMacSystem();
#elif defined(PLATFORM_MAC)
    isOpenOK = OpenWithLinuxOrMacSystem();
#endif
    if (!isOpenOK) { return OpenWithFreadType(); }

    return true;
}

bool FileReader::OpenWithFreadType() {
    // 打开文件，使用二进制模式读取
    file_ = fopen(m_FilePath.c_str(), "rb");
    if (file_ == nullptr) {
        std::cerr << "fopen failed to open the file.\n";
        return false;
    }

    // 获取文件大小
    fseek(file_, 0, SEEK_END); // 移动到文件末尾
    m_FileSize = ftell(file_); // 获取文件大小
    fseek(file_, 0, SEEK_SET); // 移回文件开头

    if (m_FileSize == 0) {
        std::cerr << "File size is 0.\n";
        fclose(file_);
        return false;
    }

    // 分配内存并读取文件内容
    FILESTART = (char*) malloc(m_FileSize);
    if (FILESTART == nullptr) {
        std::cerr << "Memory allocation failed.\n";
        fclose(file_);
        return false;
    }

    size_t bytesRead = fread(FILESTART, 1, m_FileSize, file_);
    if (bytesRead != m_FileSize) {
        std::cerr << "fread failed to read the entire file.\n";
        free(FILESTART);
        fclose(file_);
        return false;
    }

    // 设置文件开始、结束和当前指针
    IS = FILESTART;
    FILEEND = FILESTART + m_FileSize;

    // 关闭文件
    fclose(file_);
    return true;
}
bool FileReader::OpenWithWindowsSystem() {
#ifdef PLATFORM_WINDOWS
    // 打开文件
    this->m_File = CreateFile(m_FilePath.data(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (m_File == INVALID_HANDLE_VALUE) {
        _tprintf(_T("CreateFile failed with error: %lu\n"), GetLastError());
        return false;
    }

    // 获取文件大小
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(m_File, &fileSize)) {
        _tprintf(_T("GetFileSizeEx failed with error: %lu\n"), GetLastError());
        CloseHandle(m_File);
        return false;
    }
    this->m_FileSize = fileSize.QuadPart; // 将文件大小以字节为单位赋值给 m_FileSize

    // 创建文件映射对象
    this->m_MapFile = CreateFileMapping(m_File, NULL, PAGE_READONLY, 0, 0, NULL);
    if (m_MapFile == NULL) {
        _tprintf(_T("CreateFileMapping failed with error: %lu\n"), GetLastError());
        CloseHandle(m_File);
        return false;
    }

    // 将文件映射到进程的虚拟地址空间
    this->FILESTART = (char*) MapViewOfFile(m_MapFile, FILE_MAP_READ, 0, 0, 0);
    if (this->FILESTART == NULL) {
        _tprintf(_T("MapViewOfFile failed with error: %lu\n"), GetLastError());
        CloseHandle(m_MapFile);
        CloseHandle(m_File);
        return false;
    }
    this->IS = this->FILESTART;
    this->FILEEND = this->FILESTART + this->m_FileSize;
#endif
    return true;
}
bool FileReader::OpenWithLinuxOrMacSystem() {
#if defined(PLATFORM_LINUX) || defined(PLATFORM_MAC)
    // 打开文件
    m_File = open(m_FilePath.c_str(), O_RDONLY);
    if (m_File == -1) {
        perror("open");
        return false;
    }

    // 获取文件大小
    struct stat st;
    if (fstat(m_File, &st) == -1) {
        perror("fstat");
        close(m_File);
        return false;
    }
    m_FileSize = st.st_size;

    // 映射文件到内存
    FILESTART = (char*) mmap(NULL, m_FileSize, PROT_READ, MAP_PRIVATE, m_File, 0);
    if (FILESTART == MAP_FAILED) {
        perror("mmap");
        close(m_File);
        return false;
    }
    this->IS = this->FILESTART;
    FILEEND = FILESTART + m_FileSize;

#endif
    return true;
}
bool FileReader::Close() {
#ifdef PLATFORM_WINDOWS
    if (this->m_MapFile) {
        UnmapViewOfFile(this->IS);
        CloseHandle(this->m_MapFile);
        CloseHandle(this->m_File);
    }

#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MAC)
    if (m_File != -1) { close(m_File); }
#endif
    return true;
}

bool FileReader::CreateDataObject() {
    if (this->GetOutput()) {
        m_Output = this->GetOutput();
        return true;
    }
    int numFaces = m_Data.GetNumberOfFaces();
    int numVolumes = m_Data.GetNumberOfVolumes();

    if (numFaces && numVolumes) {
        VolumeMesh::Pointer mesh = VolumeMesh::New();
        mesh->SetPoints(m_Data.GetPoints());
        mesh->SetFaces(m_Data.GetFaces());
        mesh->SetVolumes(m_Data.GetVolumes());
        mesh->SetAttributeSet(m_Data.GetData());
        m_Output = mesh;
    }

    else if (numFaces) {
        SurfaceMesh::Pointer mesh = SurfaceMesh::New();
        mesh->SetPoints(m_Data.GetPoints());
        mesh->SetFaces(m_Data.GetFaces());
        mesh->SetAttributeSet(m_Data.GetData());
        m_Output = mesh;
    }

    else if (numVolumes) {
        VolumeMesh::Pointer mesh = VolumeMesh::New();
        mesh->SetPoints(m_Data.GetPoints());
        mesh->SetVolumes(m_Data.GetVolumes());
        mesh->SetAttributeSet(m_Data.GetData());
        m_Output = mesh;
    } else if (m_Data.GetPoints()) {
        PointSet::Pointer mesh = PointSet::New();
        mesh->SetPoints(m_Data.GetPoints());
        mesh->SetAttributeSet(m_Data.GetData());
        m_Output = mesh;
    } else {
        m_Output = nullptr;
    }
    return m_Output;
}

//------------------------------------------------------------------------------
// Internal function to read in an integer value.
// Returns zero if there was an error.
int FileReader::Read(char* result) {
    if (this->IS < this->FILEEND) {
        *result = (char) *this->IS;
        return 1;
    }
    return 0;
}
//------------------------------------------------------------------------------
int FileReader::Read(char* data, size_t n) {
    if (this->IS + n > this->FILEEND) { n = this->FILEEND - this->IS; }
    if (n > 0) {
        memcpy(data, this->IS, n);
        this->IS += n;
        return 1;
    }
    return 0;
}
//------------------------------------------------------------------------------
int FileReader::Read(unsigned char* result) {
    if (this->IS < this->FILEEND) {
        *result = (unsigned char) *this->IS;
        return 1;
    }
    return 0;
}

//------------------------------------------------------------------------------
int FileReader::Read(short* result) {
    int intData;
    this->IS = mAtoi(this->IS, intData);
    *result = (short) intData;
    return 1;
}

//------------------------------------------------------------------------------
int FileReader::Read(unsigned short* result) {
    int intData;
    this->IS = mAtoi(this->IS, intData);
    *result = (unsigned short) intData;
    return 1;
}

//------------------------------------------------------------------------------
int FileReader::Read(int* result) {
    this->IS = mAtoi(this->IS, *result);
    return 1;
}

//------------------------------------------------------------------------------
int FileReader::Read(unsigned int* result) {
    int intData;
    this->IS = mAtoi(this->IS, intData);
    *result = (unsigned int) intData;
    return 1;
}

//------------------------------------------------------------------------------
int FileReader::Read(long* result) {
    int intData;
    this->IS = mAtoi(this->IS, intData);
    *result = (long) intData;
    return 1;
}

//------------------------------------------------------------------------------
int FileReader::Read(unsigned long* result) {
    int intData;
    this->IS = mAtoi(this->IS, intData);
    *result = (unsigned long) intData;
    return 1;
}

//------------------------------------------------------------------------------
int FileReader::Read(long long* result) {
    int intData;
    this->IS = mAtoi(this->IS, intData);
    *result = (long long) intData;
    return 1;
}

//------------------------------------------------------------------------------
int FileReader::Read(unsigned long long* result) {
    int intData;
    this->IS = mAtoi(this->IS, intData);
    *result = (unsigned long long) intData;
    return 1;
}

//------------------------------------------------------------------------------
int FileReader::Read(float* result) {
    this->IS = mAtof(this->IS, *result);
    return 1;
}

//------------------------------------------------------------------------------
int FileReader::Read(double* result) {
    this->IS = mAtof(this->IS, *result);
    return 1;
}

//------------------------------------------------------------------------------
// Internal function to read in a line up to 256 characters.
// Returns zero if there was an error.
int FileReader::ReadLine(char result[256]) {
    const char* lineEnd = strchr(this->IS, '\n');
    if (!lineEnd) lineEnd = this->FILEEND;
    if (!this->IS || this->IS >= lineEnd) {
        result[0] = '\0';
        return 0;
    }
    int slen = lineEnd - this->IS;
    std::memcpy(result, this->IS, slen);
    result[slen] = '\0';
    // remove '\r', if present.
    if (slen > 0 && result[slen - 1] == '\r') { result[slen - 1] = '\0'; }
    this->IS = lineEnd + 1;
    return 1;
}

//------------------------------------------------------------------------------
// Internal function to read in a string up to 256 characters.
// Returns zero if there was an error.
int FileReader::ReadString(char result[256]) {
    // Force the parameter to be seen as a 256-byte array rather than a decayed
    // pointer.
    this->SkipNullData();
    const char* op = strchr(this->IS, ' ');
    const char* lineEnd = strchr(this->IS, '\n');

    if (lineEnd == nullptr) { lineEnd = FILEEND; }
    if (!op || op > lineEnd) { op = lineEnd; }
    int slen = 0;
    if (!op) { op = this->FILEEND + 1; }
    slen = op - this->IS;
    if (slen < 1) {
        result[0] = '\0';
        return 0;
    }
    std::memcpy(result, this->IS, slen);
    result[slen] = '\0';

    if (slen > 0 && result[slen - 1] == '\r') {
        result[slen - 1] = '\0';
        slen--;
    }
    for (int i = 0; i < slen; i++) {
        if (result[i] == '\t') {
            result[i] = '\0';
            op = this->IS + i;
            break;
        }
    }
    this->IS = op;
    if (result[0] == '\0') { this->IS++; }
    return 1;
}
int FileReader::ReadString(std::string& result) {
    this->SkipNullData();
    const char* op = strchr(this->IS, ' ');
    const char* lineEnd = strchr(this->IS, '\n');
    if (!op || op > lineEnd) { op = lineEnd; }
    if (!op) op = this->FILEEND;

    if (this->IS > op) {
        result[0] = '\0';
        return 0;
    }
    result = std::string(this->IS, op);
    int slen = result.length();
    if (slen > 0 && result[slen - 1] == '\r') { result = std::string(this->IS, op - 1); }
    for (int i = 0; i < slen; i++) {
        if (result[i] == '\t') {
            result[i] = '\0';
            op = this->IS + i;
            break;
        }
    }
    this->IS = op;
    if (result[0] == '\0') { this->IS++; }
    return 1;
}

char* FileReader::LowerCase(char* str, const size_t len) {
    size_t i;
    char* s;

    for (i = 0, s = str; *s != '\0' && i < len; s++, i++) { *s = tolower(*s); }
    return str;
}

// Please ensure *self->IS is data, not '\r' or '\n'
template<class T>
int iGameReadBinaryData(FileReader::Pointer self, T* data, int numTuples, int numComp) {
    if (numTuples == 0 || numComp == 0) {
        // nothing to read here.
        return 1;
    }
    char line[256];
    if (!self->Read((char*) data, sizeof(T) * numComp * numTuples)) {
        igDebug("Error reading binary data");
        return 0;
    }
    return 1;
}

template<class T>
int iGameReadASCIIData(FileReader::Pointer self, T* data, int numTuples, int numComp) {
    int i, j;
    static int UpdatePerNum = 50000;
    for (i = 0; i < numTuples; i++) {
        if (i % UpdatePerNum == 0) { self->UpdateReadProgress(); }
        for (j = 0; j < numComp; j++) {
            if (!self->Read(data)) {
                igDebug("Error reading ascii data");
                return 0;
            }
            data++;
        }
    }
    return 1;
}

ArrayObject::Pointer FileReader::ReadArray(const char* dataType, int numTuples, int numComp) {
    char* type = strdup(dataType);
    type = this->LowerCase(type);
    this->SkipNullData();
    ArrayObject::Pointer array{nullptr};
    if (!strncmp(type, "bit", 3)) {
        // process in fulture
    } else if (!strncmp(type, "unsigned_char", 13) || !strncmp(type, "unsigned char", 13)) {
        UnsignedCharArray::Pointer arr = UnsignedCharArray::New();
        arr->SetDimension(numComp);
        arr->Resize(numTuples);
        unsigned char* ptr = arr->RawPointer();
        if (m_FileType == IGAME_BINARY) {
            iGameReadBinaryData(this, ptr, numTuples, numComp);
        } else {
            iGameReadASCIIData(this, ptr, numTuples, numComp);
        }
        array = arr;
    } else if (!strncmp(type, "char", 4) || !strncmp(type, "signed_char", 11) || !strncmp(type, "signed char", 11)) {
        CharArray::Pointer arr = CharArray::New();
        arr->SetDimension(numComp);
        arr->Resize(numTuples);
        char* ptr = arr->RawPointer();
        if (m_FileType == IGAME_BINARY) {
            iGameReadBinaryData(this, ptr, numTuples, numComp);
        } else {
            iGameReadASCIIData(this, ptr, numTuples, numComp);
        }
        array = arr;
    } else if (!strncmp(type, "unsigned_short", 14) || !strncmp(type, "unsigned short", 14)) {
        UnsignedShortArray::Pointer arr = UnsignedShortArray::New();
        arr->SetDimension(numComp);
        arr->Resize(numTuples);
        unsigned short* ptr = arr->RawPointer();
        if (m_FileType == IGAME_BINARY) {
            iGameReadBinaryData(this, ptr, numTuples, numComp);
            ByteSwap::Swap2BERange((short*) ptr, numTuples * numComp);
        } else {
            iGameReadASCIIData(this, ptr, numTuples, numComp);
        }
        array = arr;
    } else if (!strncmp(type, "short", 5)) {
        ShortArray::Pointer arr = ShortArray::New();
        arr->SetDimension(numComp);
        arr->Resize(numTuples);
        short* ptr = arr->RawPointer();
        if (m_FileType == IGAME_BINARY) {
            iGameReadBinaryData(this, ptr, numTuples, numComp);
            ByteSwap::Swap2BERange(ptr, numTuples * numComp);
        } else {
            iGameReadASCIIData(this, ptr, numTuples, numComp);
        }
        array = arr;
    }
    //when read vtk file, it will maybe exist vtkidtype,
    //this type usually means int ,and some times may be long long, it need be process in fulture
    else if (!strncmp(type, "vtkidtype", 9)) {
        IntArray::Pointer arr = IntArray::New();
        arr->SetDimension(numComp);
        std::vector<int> buffer(numTuples * numComp);
        if (m_FileType == IGAME_BINARY) {
            iGameReadBinaryData(this, buffer.data(), numTuples, numComp);
            ByteSwap::Swap4BERange(buffer.data(), numTuples * numComp);
        } else {
            iGameReadASCIIData(this, buffer.data(), numTuples, numComp);
        }
        arr->Resize(numTuples);
        int* ptr2 = arr->RawPointer();
        for (int idx = 0; idx < numTuples * numComp; idx++) { ptr2[idx] = buffer[idx]; }
        array = arr;
    } else if ((!strncmp(type, "unsigned_int", 12)) || (!strncmp(type, "unsigned int", 12))) {
        UnsignedIntArray::Pointer arr = UnsignedIntArray::New();
        arr->SetDimension(numComp);
        arr->Resize(numTuples);
        unsigned int* ptr = arr->RawPointer();
        if (m_FileType == IGAME_BINARY) {
            iGameReadBinaryData(this, ptr, numTuples, numComp);
            ByteSwap::Swap4BERange((int*) ptr, numTuples * numComp);
        } else {
            iGameReadASCIIData(this, ptr, numTuples, numComp);
        }
        array = arr;
    } else if (!strncmp(type, "int", 3)) {
        IntArray::Pointer arr = IntArray::New();
        arr->SetDimension(numComp);
        arr->Resize(numTuples);
        int* ptr = arr->RawPointer();
        if (m_FileType == IGAME_BINARY) {
            iGameReadBinaryData(this, ptr, numTuples, numComp);
            ByteSwap::Swap4BERange(ptr, numTuples * numComp);
        } else {
            iGameReadASCIIData(this, ptr, numTuples, numComp);
        }
        array = arr;
    }
    //when read vtk file, it will maybe exist vtktypeint64,
    //this type  means unsigned long long
    else if (!strncmp(type, "vtktypeuint64", 13)) {
        UnsignedLongLongArray::Pointer arr = UnsignedLongLongArray::New();
        arr->SetDimension(numComp);
        arr->Resize(numTuples);
        auto ptr = arr->RawPointer();
        if (m_FileType == IGAME_BINARY) {
            iGameReadBinaryData(this, ptr, numTuples, numComp);
            ByteSwap::Swap8BERange(ptr, numTuples * numComp);
        } else {
            iGameReadASCIIData(this, ptr, numTuples, numComp);
        }
        array = arr;
    }
    //when read vtk file, it will maybe exist vtktypeint64,
    //this type  means long long
    else if (!strncmp(type, "vtktypeint64", 12)) {
        LongLongArray::Pointer arr = LongLongArray::New();
        arr->SetDimension(numComp);
        arr->Resize(numTuples);
        auto ptr = arr->RawPointer();
        if (m_FileType == IGAME_BINARY) {
            iGameReadBinaryData(this, ptr, numTuples, numComp);
            ByteSwap::Swap8BERange(ptr, numTuples * numComp);
        } else {
            iGameReadASCIIData(this, ptr, numTuples, numComp);
        }
        array = arr;
    } else if (!strncmp(type, "float", 5)) {
        FloatArray::Pointer arr = FloatArray::New();
        arr->SetDimension(numComp);
        arr->Resize(numTuples);
        float* ptr = arr->RawPointer();
        if (m_FileType == IGAME_BINARY) {
            iGameReadBinaryData(this, ptr, numTuples, numComp);
            ByteSwap::Swap4BERange(ptr, numTuples * numComp);
        } else {
            iGameReadASCIIData(this, ptr, numTuples, numComp);
        }
        array = arr;
    } else if (!strncmp(type, "double", 6)) {
        DoubleArray::Pointer arr = DoubleArray::New();
        arr->SetDimension(numComp);
        arr->Resize(numTuples);
        double* ptr = arr->RawPointer();
        if (this->m_FileType == IGAME_BINARY) {
            iGameReadBinaryData(this, ptr, numTuples, numComp);
            ByteSwap::Swap8BERange(ptr, numTuples * numComp);
        } else {
            iGameReadASCIIData(this, ptr, numTuples, numComp);
        }
        array = arr;
    } else {
        igError("Unsupported data type: " << type);
        free(type);
        return nullptr;
    }
    free(type);
    this->UpdateReadProgress();
    return array;
}

void FileReader::SetFilePath(const std::string& filePath) {
    this->m_FilePath = filePath;
    this->m_FileName = filePath.substr(filePath.find_last_of('/') + 1, filePath.size());
    igDebug("Reading file: "+ filePath);
}

void FileReader::SkipNullData() {
    while (this->IS < this->FILEEND &&
           (*this->IS == ' ' || *this->IS == '\r' || *this->IS == '\n' || *this->IS == '\t'))
        this->IS++;
}
void FileReader::SkipDataToSpace() {
    while (this->IS < this->FILEEND && *this->IS != ' ') this->IS++;
}
void FileReader::UpdateReadProgress() {
    if (!this->IS) return;
    double progress = 1.0 * (this->IS - this->FILESTART) / m_FileSize;
    this->UpdateProgress(progress);
}
int FileReader::DecodeString(char* resname, const char* name) {
    if (!resname || !name) { return 0; }
    std::ostringstream str;
    size_t cc = 0;
    unsigned int ch;
    size_t len = strlen(name);
    size_t reslen = 0;
    char buffer[10] = "0x";
    while (name[cc]) {
        if (name[cc] == '%') {
            if (cc <= (len - 3)) {
                buffer[2] = name[cc + 1];
                buffer[3] = name[cc + 2];
                buffer[4] = 0;
                sscanf(buffer, "%x", &ch);
                str << static_cast<char>(ch);
                cc += 2;
                reslen++;
            }
        } else {
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