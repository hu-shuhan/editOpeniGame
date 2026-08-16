#ifndef iGameFileReader_h
#define iGameFileReader_h

#include "iGameFilter.h"
//#include "iGameThreadPool.h"
#include "iGameDataCollection.h"
#include "iGameFlatArray.h"
#include "iGameSurfaceMesh.h"
#include <algorithm>
#include <cfloat>
#include <cstdio>

#if defined(PLATFORM_WINDOWS) || defined(_WIN32)
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MAC) || defined(__linux__) || defined(__APPLE__)
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif
IGAME_NAMESPACE_BEGIN


class FileReader : public Filter {
public:
    I_OBJECT(FileReader);
    // static Pointer New() { return new FileReader; }


    bool Execute() override;

    void SetFilePath(const std::string& filePath);
    void SetMemoryBuffer(const void* data, size_t size);
    std::string GetFilePath() const { return m_FilePath; }
    //封装了SetFilePath和Execute以及GetOutput
    DataObject::Pointer ReadFile(const std::string& filePath);

    /**
     * Internal function to read in a value.  Returns zero if there was an
     * error.
     */
    int Read(char*);
    int Read(unsigned char*);
    int Read(short*);
    int Read(unsigned short*);
    int Read(int*);
    int Read(unsigned int*);
    int Read(long*);
    int Read(unsigned long*);
    int Read(long long* result);
    int Read(unsigned long long* result);
    int Read(float*);
    int Read(double*);
    int Read(char*, size_t);
    int ReadLine(char result[256]);
    int ReadString(char result[256]);
    int ReadString(std::string& str);
    char* LowerCase(char* str, const size_t len = 256);
    //read array data, need input data type, some times will
    //have space in file string with %20, so you need to decode it in advance
    ArrayObject::Pointer ReadArray(const char* dataType, int numTuples, int numComp);
    void SkipNullData();
    void SkipDataToSpace();
    void UpdateReadProgress();
    int DecodeString(char* resname, const char* name);


protected:
    FileReader();
    ~FileReader() override = default;

    bool Open();
    bool OpenWithWindowsSystem();
    bool OpenWithLinuxOrMacSystem();
    bool OpenWithFreadType();
    bool Close();
    virtual bool Parsing() = 0;
    virtual bool CreateDataObject();

    CellArray::Pointer m_CellArray;
    DataCollection m_Data;

    DataObject::Pointer m_Output;

    std::string m_FilePath;
    std::string m_FileName;
    std::string m_FileSuffix;
    FILE* file_;
    size_t m_FileSize;
    const char* m_MemoryBuffer{nullptr};
    size_t m_MemoryBufferSize{0};
    bool m_UseMemoryBuffer{false};
    bool m_OwnsBuffer{false};

#if defined(PLATFORM_WINDOWS) || defined(_WIN32)
    HANDLE m_File{nullptr};
    HANDLE m_MapFile{nullptr};
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MAC) || defined(__linux__) || defined(__APPLE__)
    int m_File = -1;
#endif

    const char* IS;
    char* FILESTART{nullptr};
    char* FILEEND{nullptr};
    IGenum m_FileType{IGAME_ASCII};
};

inline std::string FormatTime(size_t size) {
    if (size < 1000) return std::to_string(size) + "ms";
    else if (size < 1000ull * 1000)
        return std::to_string(size / 1000.) + "s";
    else
        return std::to_string(size / 1024. / 1024) + "m";
}
/* 预先生成的9位十进制数值查询表  每行对应数字位的权重：亿、千万、百万...个位 */
inline static int morn_atoi[9][10] = {
        {0, 100000000, 200000000, 300000000, 400000000, 500000000, 600000000, 700000000, 800000000, 900000000},
        {0, 10000000, 20000000, 30000000, 40000000, 50000000, 60000000, 70000000, 80000000, 90000000},
        {0, 1000000, 2000000, 3000000, 4000000, 5000000, 6000000, 7000000, 8000000, 9000000},
        {0, 100000, 200000, 300000, 400000, 500000, 600000, 700000, 800000, 900000},
        {0, 10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000},
        {0, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000},
        {0, 100, 200, 300, 400, 500, 600, 700, 800, 900},
        {0, 10, 20, 30, 40, 50, 60, 70, 80, 90},
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}};

inline int mAtoi(const char* str) {
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;

    int flag = 0;
    unsigned char* s = (unsigned char*) str;
    if (*str == '-') {
        flag = 1;
        s++;
    } else if (*str == '+')
        s++;
    int data = morn_atoi[0][s[0] - '0'];
    if ((s[1] < '0') || (s[1] > '9')) {
        data = data / 100000000;
        return (flag) ? (0 - data) : data;
    }
    data += morn_atoi[1][s[1] - '0'];
    if ((s[2] < '0') || (s[2] > '9')) {
        data = data / 10000000;
        return (flag) ? (0 - data) : data;
    }
    data += morn_atoi[2][s[2] - '0'];
    if ((s[3] < '0') || (s[3] > '9')) {
        data = data / 1000000;
        return (flag) ? (0 - data) : data;
    }
    data += morn_atoi[3][s[3] - '0'];
    if ((s[4] < '0') || (s[4] > '9')) {
        data = data / 100000;
        return (flag) ? (0 - data) : data;
    }
    data += morn_atoi[4][s[4] - '0'];
    if ((s[5] < '0') || (s[5] > '9')) {
        data = data / 10000;
        return (flag) ? (0 - data) : data;
    }
    data += morn_atoi[5][s[5] - '0'];
    if ((s[6] < '0') || (s[6] > '9')) {
        data = data / 1000;
        return (flag) ? (0 - data) : data;
    }
    data += morn_atoi[6][s[6] - '0'];
    if ((s[7] < '0') || (s[7] > '9')) {
        data = data / 100;
        return (flag) ? (0 - data) : data;
    }
    data += morn_atoi[7][s[7] - '0'];
    if ((s[8] < '0') || (s[8] > '9')) {
        data = data / 10;
        return (flag) ? (0 - data) : data;
    }
    data += s[8] - '0';
    if ((s[9] < '0') || (s[9] > '9')) { return (flag) ? (0 - data) : data; }
    data = data * 10 + s[9] - '0';
    return (flag) ? (0 - data) : data;
}


inline const char* mAtoi(const char* str, int& val) {
    int flag = 0;
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;

    unsigned char* s = (unsigned char*) str;
    if (*str == '-') {
        flag = 1;
        s++;
    } else if (*str == '+')
        s++;
    int data = morn_atoi[0][s[0] - '0'];
    if ((s[1] < '0') || (s[1] > '9')) {
        data = data / 100000000;
        val = (flag) ? (0 - data) : data;
        return str + 1;
    }
    data += morn_atoi[1][s[1] - '0'];
    if ((s[2] < '0') || (s[2] > '9')) {
        data = data / 10000000;
        val = (flag) ? (0 - data) : data;
        return str + 2;
    }
    data += morn_atoi[2][s[2] - '0'];
    if ((s[3] < '0') || (s[3] > '9')) {
        data = data / 1000000;
        val = (flag) ? (0 - data) : data;
        return str + 3;
    }
    data += morn_atoi[3][s[3] - '0'];
    if ((s[4] < '0') || (s[4] > '9')) {
        data = data / 100000;
        val = (flag) ? (0 - data) : data;
        return str + 4;
    }
    data += morn_atoi[4][s[4] - '0'];
    if ((s[5] < '0') || (s[5] > '9')) {
        data = data / 10000;
        val = (flag) ? (0 - data) : data;
        return str + 5;
    }
    data += morn_atoi[5][s[5] - '0'];
    if ((s[6] < '0') || (s[6] > '9')) {
        data = data / 1000;
        val = (flag) ? (0 - data) : data;
        return str + 6;
    }
    data += morn_atoi[6][s[6] - '0'];
    if ((s[7] < '0') || (s[7] > '9')) {
        data = data / 100;
        val = (flag) ? (0 - data) : data;
        return str + 7;
    }
    data += morn_atoi[7][s[7] - '0'];
    if ((s[8] < '0') || (s[8] > '9')) {
        data = data / 10;
        val = (flag) ? (0 - data) : data;
        return str + 8;
    }
    data += s[8] - '0';
    if ((s[9] < '0') || (s[9] > '9')) {
        val = (flag) ? (0 - data) : data;
        return str + 9;
    }
    data = data * 10 + s[9] - '0';
    val = (flag) ? (0 - data) : data;
    return str + 10;
}

inline static double morn_atof[17][10] = {
        {0.0, 0.1000000000000000, 0.2000000000000000, 0.3000000000000000, 0.4000000000000000, 0.5000000000000000,
         0.6000000000000000, 0.7000000000000000, 0.8000000000000000, 0.9000000000000000},
        {0.0, 0.0100000000000000, 0.0200000000000000, 0.0300000000000000, 0.0400000000000000, 0.0500000000000000,
         0.0600000000000000, 0.0700000000000000, 0.0800000000000000, 0.0900000000000000},
        {0.0, 0.0010000000000000, 0.0020000000000000, 0.0030000000000000, 0.0040000000000000, 0.0050000000000000,
         0.0060000000000000, 0.0070000000000000, 0.0080000000000000, 0.0090000000000000},
        {0.0, 0.0001000000000000, 0.0002000000000000, 0.0003000000000000, 0.0004000000000000, 0.0005000000000000,
         0.0006000000000000, 0.0007000000000000, 0.0008000000000000, 0.0009000000000000},
        {0.0, 0.0000100000000000, 0.0000200000000000, 0.0000300000000000, 0.0000400000000000, 0.0000500000000000,
         0.0000600000000000, 0.0000700000000000, 0.0000800000000000, 0.0000900000000000},
        {0.0, 0.0000010000000000, 0.0000020000000000, 0.0000030000000000, 0.0000040000000000, 0.0000050000000000,
         0.0000060000000000, 0.0000070000000000, 0.0000080000000000, 0.0000090000000000},
        {0.0, 0.0000001000000000, 0.0000002000000000, 0.0000003000000000, 0.0000004000000000, 0.0000005000000000,
         0.0000006000000000, 0.0000007000000000, 0.0000008000000000, 0.0000009000000000},
        {0.0, 0.0000000100000000, 0.0000000200000000, 0.0000000300000000, 0.0000000400000000, 0.0000000500000000,
         0.0000000600000000, 0.0000000700000000, 0.0000000800000000, 0.0000000900000000},
        {0.0, 0.0000000010000000, 0.0000000020000000, 0.0000000030000000, 0.0000000040000000, 0.0000000050000000,
         0.0000000060000000, 0.0000000070000000, 0.0000000080000000, 0.0000000090000000},
        {0.0, 0.0000000001000000, 0.0000000002000000, 0.0000000003000000, 0.0000000004000000, 0.0000000005000000,
         0.0000000006000000, 0.0000000007000000, 0.0000000008000000, 0.0000000009000000},
        {0.0, 0.0000000000100000, 0.0000000000200000, 0.0000000000300000, 0.0000000000400000, 0.0000000000500000,
         0.0000000000600000, 0.0000000000700000, 0.0000000000800000, 0.0000000000900000},
        {0.0, 0.0000000000010000, 0.0000000000020000, 0.0000000000030000, 0.0000000000040000, 0.0000000000050000,
         0.0000000000060000, 0.0000000000070000, 0.0000000000080000, 0.0000000000090000},
        {0.0, 0.0000000000001000, 0.0000000000002000, 0.0000000000003000, 0.0000000000004000, 0.0000000000005000,
         0.0000000000006000, 0.0000000000007000, 0.0000000000008000, 0.0000000000009000},
        {0.0, 0.0000000000000100, 0.0000000000000200, 0.0000000000000300, 0.0000000000000400, 0.0000000000000500,
         0.0000000000000600, 0.0000000000000700, 0.0000000000000800, 0.0000000000000900},
        {0.0, 0.0000000000000010, 0.0000000000000020, 0.0000000000000030, 0.0000000000000040, 0.0000000000000050,
         0.0000000000000060, 0.0000000000000070, 0.0000000000000080, 0.0000000000000090},
        {0.0, 0.0000000000000001, 0.0000000000000002, 0.0000000000000003, 0.0000000000000004, 0.0000000000000005,
         0.0000000000000006, 0.0000000000000007, 0.0000000000000008, 0.0000000000000009},
        {0.0, 0.0000000000000000, 0.0000000000000000, 0.0000000000000000, 0.0000000000000000, 0.0000000000000000,
         0.0000000000000000, 0.0000000000000000, 0.0000000000000000, 0.0000000000000000},
};

inline static double morn_atof_k[21] = {1.0,
                                        0.1,
                                        0.01,
                                        0.001,
                                        0.0001,
                                        0.00001,
                                        0.000001,
                                        0.0000001,
                                        0.00000001,
                                        0.000000001,
                                        0.0000000001,
                                        0.00000000001,
                                        0.000000000001,
                                        0.0000000000001,
                                        0.00000000000001,
                                        0.000000000000001,
                                        0.0000000000000001,
                                        0.00000000000000001,
                                        0.000000000000000001,
                                        0.0000000000000000001};

inline double mAtof(const char* p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    int flag = 0;
    double d = 0;
    if (*p == '-') {
        flag = 1;
        p++;
    } else if (*p == '+')
        p++;
    for (; (*p >= '0') && (*p <= '9'); p++) d = d * 10.0 + (*p - '0');
    if (*p == '.') {
        p++;
        int i;
        double v = 0;
        for (i = 0; i < 20; i++) {
            if (p[i] != '0') break;
        }
        double k = morn_atof_k[i];
        p = p + i;
        for (i = 0; (p[i] >= '0') && (p[i] <= '9'); i++) { v = v + morn_atof[std::min(i, 16)][p[i] - '0']; }
        p = p + i;
        d = v * k + d;
    }
    if ((*p == 'e') || (*p == 'E')) {
        p++;
        int f = 0;
        if (*p == '-') {
            f = 1;
            p++;
        } else if (*p == '+') {
            p++;
        }
        int e = 0;
        for (; (*p >= '0') && (*p <= '9'); p++) e = e * 10 + (*p - '0');
        d = d * pow(10.0, ((f == 1) ? (0 - e) : e));
    }

    return (flag) ? (0 - d) : d;
}

inline const char* mAtof(const char* p, float& val) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;


    int flag = 0;
    double d = 0;
    if (*p == '-') {
        flag = 1;
        p++;
    } else if (*p == '+')
        p++;
    for (; (*p >= '0') && (*p <= '9'); p++) d = d * 10.0 + (*p - '0');
    if (*p == '.') {
        p++;
        int i;
        double v = 0;
        for (i = 0; i < 20; i++) {
            if (p[i] != '0') break;
        }
        double k = morn_atof_k[i];
        p = p + i;
        for (i = 0; (p[i] >= '0') && (p[i] <= '9'); i++) { v = v + morn_atof[std::min(i, 16)][p[i] - '0']; }
        p = p + i;
        d = v * k + d;
    }
    if ((*p == 'e') || (*p == 'E')) {
        p++;
        int f = 0;
        if (*p == '-') {
            f = 1;
            p++;
        } else if (*p == '+') {
            p++;
        }
        int e = 0;
        for (; (*p >= '0') && (*p <= '9'); p++) e = e * 10 + (*p - '0');
        d = d * pow(10.0, ((f == 1) ? (0 - e) : e));
    }
    val = (flag) ? (0 - d) : d;
    return p;
}

inline const char* mAtof(const char* p, double& val) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;


    int flag = 0;
    double d = 0;
    if (*p == '-') {
        flag = 1;
        p++;
    } else if (*p == '+')
        p++;
    for (; (*p >= '0') && (*p <= '9'); p++) d = d * 10.0 + (*p - '0');
    if (*p == '.') {
        p++;
        int i;
        double v = 0;
        for (i = 0; i < 20; i++) {
            if (p[i] != '0') break;
        }
        double k = morn_atof_k[i];
        p = p + i;
        for (i = 0; (p[i] >= '0') && (p[i] <= '9'); i++) { v = v + morn_atof[std::min(i, 16)][p[i] - '0']; }
        p = p + i;
        d = v * k + d;
    }
    if ((*p == 'e') || (*p == 'E')) {
        p++;
        int f = 0;
        if (*p == '-') {
            f = 1;
            p++;
        } else if (*p == '+') {
            p++;
        }
        int e = 0;
        for (; (*p >= '0') && (*p <= '9'); p++) e = e * 10 + (*p - '0');
        d = d * pow(10.0, ((f == 1) ? (0 - e) : e));
    }
    val = (flag) ? (0 - d) : d;
    return p;
}

IGAME_NAMESPACE_END
#endif
