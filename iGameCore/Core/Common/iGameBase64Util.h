//
// Created by m_ky on 2024/8/3.
//

/**
 * @class   iGameBase64Util
 * @brief   iGameBase64Util's brief
 */
#pragma once

#include "iGamePoints.h"
#include "iGameFlatArray.h"

IGAME_NAMESPACE_BEGIN

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";


template<typename T>
static T bytes_to_target(unsigned char* bytes) {
    T res;
    std::memcpy(&res, bytes, sizeof(T));

    return res;
}
template<typename T>
static T bytes_to_target(const char* bytes) {
    T res;
    std::memcpy(&res, bytes, sizeof(T));

    return res;
}
//template<typename T>
//static void Read(const char*p, const T* bytes, int length) {
//    std::memcpy(bytes, p, length);
//}


/*
 * Template of Base64 parser
 * */
template<typename T>
static void Base64_convert(const char* p, int offset_byte_num, uint64_t byte_length) {
    int i = 0;
    unsigned char char_array_4[4], char_array_3[3];
    int type_size = sizeof(T);
    unsigned char* target_bytes = new unsigned char[type_size];

    int target_byte_count = 0;
    uint64_t total_byte_count = 0;
    uint64_t sum_byte = byte_length + offset_byte_num;
    while (p != nullptr && total_byte_count < sum_byte) {
        char_array_4[i++] = *p;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++) {
                total_byte_count++;
                if (total_byte_count > offset_byte_num && total_byte_count <= sum_byte) {
                    target_bytes[target_byte_count++] = char_array_3[i];
                    if (target_byte_count == type_size) {
                        T value = bytes_to_target<T>(target_bytes);
                        std::cout << "value : " << value << '\n';
                        target_byte_count = 0;
                    }
                }
            }
            i = 0;
        }
        p++;
    }
    delete[] target_bytes;
}


template<typename T>
static T Base64_Convert_TargetValue(const char* p) {
    int i = 0;
    unsigned char char_array_4[4], char_array_3[3];
    int type_size = sizeof(T);
    auto* target_bytes = new unsigned char[type_size];

    T res = 0;
    int target_byte_count = 0;
    while (p != nullptr) {
        char_array_4[i++] = *p;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++) {
                target_bytes[target_byte_count++] = char_array_3[i];
                if (target_byte_count == type_size) {
                    res  = bytes_to_target<T>(target_bytes);
                    delete[] target_bytes;
                    return res;
                }
            }
            i = 0;
        }
        p ++;
    }
    delete [] target_bytes;
    return res;
}

template<class T>
static void Base64_ConvertTo_Points(const char* p, int offset_byte_num, uint64_t byte_length, Points::Pointer pointSet){
    int i = 0;
    unsigned char char_array_4[4], char_array_3[3];
    int type_size = sizeof(T);
    unsigned char* target_bytes = new unsigned char[type_size];
    T point_array[3] = {0};
    int target_byte_count = 0;
    int point_count = 0;
    uint64_t total_byte_count = 0;
//    uint64_t sum_byte = byte_length / 2 + offset_byte_num;
    uint64_t sum_byte = byte_length + offset_byte_num;
    while (total_byte_count <= sum_byte) {
        char_array_4[i++] = *p;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++) {
                total_byte_count++;
                if (total_byte_count > offset_byte_num && total_byte_count <= sum_byte) {
                    target_bytes[target_byte_count++] = char_array_3[i];
                    if (target_byte_count == type_size) {
                        target_byte_count = 0;
                        point_array[point_count ++] = bytes_to_target<T>(target_bytes);
                        if(point_count == 3){
                            pointSet->AddPoint(point_array);
                            point_count = 0;
                        }
                    }
                }
            }
            i = 0;
        }
        p ++;
    }
    delete[] target_bytes;
}
//
//static void Base64_ConvertTo_Float_Points(const char* p, int offset_byte_num, uint32_t byte_length, Points::Pointer pointSet){
//    int i = 0;
//    unsigned char char_array_4[4], char_array_3[3];
//    int type_size = sizeof(float);
//    unsigned char* target_bytes = new unsigned char[type_size];
//    float point_array[3] = {0};
//    int target_byte_count = 0;
//    int point_count = 0;
//    uint32_t total_byte_count = 0;
//    uint32_t sum_byte = byte_length + offset_byte_num;
//    while (total_byte_count <= sum_byte) {
//        char_array_4[i++] = *p;
//        if (i == 4) {
//            for (i = 0; i < 4; i++)
//                char_array_4[i] = base64_chars.find(char_array_4[i]);
//
//            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
//            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
//            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
//
//            for (i = 0; (i < 3); i++) {
//                total_byte_count++;
//                if (total_byte_count > offset_byte_num && total_byte_count <= sum_byte) {
//                    target_bytes[target_byte_count++] = char_array_3[i];
//                    if (target_byte_count == type_size) {
//                        target_byte_count = 0;
//                        point_array[point_count ++] = bytes_to_target<float>(target_bytes);
//                        if(point_count == 3){
//                            pointSet->AddPoint(point_array);
//                            point_count = 0;
//                        }
//                    }
//                }
//            }
//            i = 0;
//        }
//        p ++;
//    }
//    delete[] target_bytes;
//}

template<typename T>
static void Base64_ConvertTo_FlatArray(const char* p, int offset_byte_num, uint64_t byte_length, typename FlatArray<T>::Pointer flatArray){
    int i = 0;
    unsigned char char_array_4[4], char_array_3[3];
    int type_size = sizeof(T);
    unsigned char* target_bytes = new unsigned char[type_size];
    int target_byte_count = 0;
    uint64_t total_byte_count = 0;
    uint64_t sum_byte = byte_length + offset_byte_num;

    while (total_byte_count <= sum_byte) {
        char_array_4[i++] = *p;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++) {
                total_byte_count++;
                if (total_byte_count > offset_byte_num  && total_byte_count <= sum_byte) {
                    target_bytes[target_byte_count++] = char_array_3[i];
                    if (target_byte_count == type_size) {
                        target_byte_count = 0;
                        flatArray->AddValue(bytes_to_target<T>(target_bytes));
                    }
                }
            }
            i = 0;
        }
        p ++;
    }
    delete[] target_bytes;
}

/*
 * Template of Base64 Encoded.
 * */

template<typename  T>
static void ReadBase64EncodedPoints(bool is_header_8_byte, const char *p, Points::Pointer pointSet) {
    if(is_header_8_byte){
        auto byte_size = Base64_Convert_TargetValue<uint64_t>(p);
        Base64_ConvertTo_Points<T>(p, 8, byte_size, pointSet);
    } else {
        auto byte_size = Base64_Convert_TargetValue<uint32_t>(p);
        Base64_ConvertTo_Points<T>(p, 4, byte_size, pointSet);
    }
}

template<typename T>
static void ReadBase64EncodedArray(bool is_header_8_byte, const char* p, typename FlatArray<T>::Pointer arr){
    if(is_header_8_byte){
        auto byte_size =  Base64_Convert_TargetValue<uint64_t>(p);
        Base64_ConvertTo_FlatArray<T>(p, 8, byte_size, arr);
    } else {
        auto byte_size =  Base64_Convert_TargetValue<uint32_t>(p);
        Base64_ConvertTo_FlatArray<T>(p, 4, byte_size, arr);
    }
}

IGAME_NAMESPACE_END