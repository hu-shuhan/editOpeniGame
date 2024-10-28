#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #if defined(_DEBUG)
        #define IG_QT_MODULE_EXPORT
    #elif defined(IG_QT_COMPILE_EXPORT)
        #define IG_QT_MODULE_EXPORT __declspec(dllexport)
    #else
        #define IG_QT_MODULE_EXPORT __declspec(dllimport)
#endif
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
    #if defined(IG_QT_COMPILE_EXPORT)
        #define IG_QT_MODULE_EXPORT __attribute__((visibility("default")))
    #else
        #define IG_QT_MODULE_EXPORT
#endif
#elif defined(EMSCRIPTEN)
    #define IG_QT_MODULE_EXPORT
#endif



