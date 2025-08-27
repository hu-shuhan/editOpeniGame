//
// Created by m_ky on 2025/3/19.
//

/**
 * @class   iGameXMLUtils
 * @brief   iGameXMLUtils's brief
 */
#pragma once

#include "iGameBase64Util.h"

IGAME_NAMESPACE_BEGIN
template<typename T>
static void ReadRawBinaryPoints(bool is_header_8_byte, char *p, Points::Pointer pointSet) {
    auto* current = reinterpret_cast<unsigned char*>(p);
    size_t point_byte_len = sizeof (T) * 3;
    uint64_t byte_size;
    if(is_header_8_byte){
        byte_size = bytes_to_target<uint64_t>(p);
        current += 8;
    } else {
        byte_size = bytes_to_target<uint32_t>(p);
        current += 4;
    }
    uint64_t pointNum = byte_size / point_byte_len;
    T pointList[3];
    for(int i = 0; i < pointNum; i ++){
        std::memcpy(pointList, current, point_byte_len);
        pointSet->AddPoint(pointList);
        current += point_byte_len;
    }
}

    template<typename T>
    static void ReadRawBinaryArray(bool is_header_8_byte, char* p, typename FlatArray<T>::Pointer arr){
        char* current = p;
        uint64_t byte_size;
        if(is_header_8_byte){
            byte_size = bytes_to_target<uint64_t>(p);
            current += 8;
        } else {
            byte_size = bytes_to_target<uint32_t>(p);
            current += 4;
        }
        size_t elemSize = sizeof(T) * arr->GetDimension();
        uint64_t elemNum = byte_size / elemSize;

        // 解决方案1: 使用std::vector
        std::vector<T> elemVector(arr->GetDimension());

        for(int i = 0; i < elemNum; i++){
            std::memcpy(elemVector.data(), current, elemSize);
            arr->AddElement(elemVector);
            current += elemSize;
        }

        // 或者解决方案2: 如果有接受T*类型的重载，可以这样做
        // for(int i = 0; i < elemNum; i++){
        //     T* elemPtr = reinterpret_cast<T*>(current);
        //     arr->AddElement(elemPtr);  // 如果有接受T*的重载的话
        //     current += elemSize;
        // }
    }
IGAME_NAMESPACE_END