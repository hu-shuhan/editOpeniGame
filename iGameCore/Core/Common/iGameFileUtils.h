/**
* @class   iGameFileUtils
* @brief
*/
#ifndef iGameFileUtils_h
#define iGameFileUtils_h

#include <string>
#include <iostream>

inline std::string GetFilenameName(const std::string& filename)
{
#if defined(_WIN32)
   const char* separators = "/\\";
#else
   char separators = '/';
#endif
   std::string::size_type slash_pos = filename.find_last_of(separators);
   if (slash_pos != std::string::npos) {
       return filename.substr(slash_pos + 1);
   }
   else {
       return filename;
   }
}

inline void OutputWindowDisplayErrorText(const char* file, int line, const char* msg) {
   std::cout << "[Error  ] In " << file << ", line " << line << ": " << msg << '\n';
}

inline void OutputWindowDisplayWarningText(const char* file, int line, const char* msg) {
   std::cout << "[Warning] In " << file << ", line " << line << ": " << msg << '\n';
}

inline void OutputWindowDisplayDebugText(const char* file, int line, const char* msg) {
   //std::cout << "[Debug  ] In " << file << ", line " << line << ": " << msg << '\n';
   std::cout << "[Debug  ] " << msg << '\n';
}

inline void OutputWindowDisplayInfoText(const char* file, int line, const char* msg) {
   std::cout << "[Info   ] In " << file << ", line " << line << ": " << msg << '\n';
}

inline std::string FormatByte(size_t size) {
   if (size < 1024)
       return std::to_string(size) + "B";
   else if (size < 1024ull * 1024)
       return std::to_string(size / 1024) + "KB";
   else if (size < 1024ull * 1024 * 1024)
       return std::to_string(size / 1024 / 1024) + "MB";
   else
       return std::to_string(size / 1024 / 1024 / 1024) + "GB";
}

inline std::string FormatTime(size_t size) {
   if (size < 1000)
       return std::to_string(size) + "ms";
   else if (size < 1000ull * 60)
       return std::to_string(size / 1000.) + "s";
   else
       return std::to_string(size / 1024. / 60) + "m";
}

#endif