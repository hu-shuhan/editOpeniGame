//#pragma once
//#include <iGameUnstructuredMesh.h>
//#include <iostream>
//#include <thread>
//#include <winsock2.h>
//#include <filesystem> // 需要C++17支持
//namespace fs = std::filesystem;
//#pragma comment(lib, "ws2_32.lib")
//#ifndef OPENCMD_H
//#define OPENCMD_H
//#include <string>
//#include "iGameFileIO.h"
//#include "Spline XML/iGameNurbsReader.h"
//#include "iGameMeshCodec/iGameMeshEncoder.h"
//#include "iGameMeshCodec/iGameMeshDecoder.h"
//class OpenCmd {
//public:
//    int selected_idx;
//    std::string filePath;
//
//    // 序列化函数
//    std::string serialize() const {
//        // 将 selected_idx 和 filePath 序列化为一个字符串
//        return std::to_string(selected_idx) + "|" + filePath;
//    }
//
//    // 反序列化函数
//    void deserialize(const std::string& data) {
//        size_t pos = data.find("|");
//        if (pos != std::string::npos) {
//            selected_idx = std::stoi(data.substr(0, pos));
//            filePath = data.substr(pos + 1);
//        }
//    }
//};
//#endif
////static DataObject::Pointer _obj;
//
//void clientThread(int selected_idx, std::string filePath) {
//    OpenCmd m_openCmd;
//    m_openCmd.selected_idx = selected_idx;
//    m_openCmd.filePath = filePath;
//
//    // 序列化 OpenCmd
//    std::string serializedData = m_openCmd.serialize();
//
//    WSADATA wsaData;
//    WSAStartup(MAKEWORD(2, 2), &wsaData);
//
//    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//    if (clientSocket == INVALID_SOCKET) {
//        MessageBox(NULL, "Invalid Socket!", "错误", MB_ICONERROR);
//        return;
//    }
//
//    sockaddr_in serverAddr;
//    serverAddr.sin_family = AF_INET;
//    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
//    serverAddr.sin_port = htons(12345);
//
//    if (connect(clientSocket, (sockaddr*) &serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
//        MessageBox(NULL, "Connect Error!!", "错误", MB_ICONERROR);
//        closesocket(clientSocket);
//        return;
//    }
//
//    // 发送序列化后的数据
//    send(clientSocket, serializedData.c_str(), serializedData.size() + 1, 0);
//    std::string saveDir = "D:";
//    std::string savePath = saveDir + "/ReceivedFile.igc"; // 保存文件名
//
//    // 2. 创建目录（如果不存在）
//    if (!fs::exists(saveDir)) {
//        fs::create_directories(saveDir);
//        if (!fs::exists(saveDir)) {
//            MessageBox(NULL, "无法创建目录!", "错误", MB_ICONERROR);
//            closesocket(clientSocket);
//            return;
//        }
//    }
//
//    // 3. 接收文件
//    std::ofstream file(savePath, std::ios::binary);
//    if (!file.is_open()) {
//        MessageBox(NULL, "文件创建失败!", "错误", MB_ICONERROR);
//        closesocket(clientSocket);
//        return;
//    }
//
//    // 接收文件大小（假设服务器先发送文件大小）
//    std::streamsize fileSize;
//    if (recv(clientSocket, (char*) &fileSize, sizeof(fileSize), 0) <= 0) {
//        MessageBox(NULL, "接收文件大小失败!", "错误", MB_ICONERROR);
//        file.close();
//        closesocket(clientSocket);
//        return;
//    }
//
//    // 循环接收数据
//    constexpr size_t BUFFER_SIZE = 65536;
//    char buffer[BUFFER_SIZE];
//    std::streamsize totalReceived = 0;
//
//    while (totalReceived < fileSize) {
//        int bytesToRead = (fileSize - totalReceived > BUFFER_SIZE) ? BUFFER_SIZE : fileSize - totalReceived;
//        int bytesReceived = recv(clientSocket, buffer, bytesToRead, 0);
//        if (bytesReceived <= 0) {
//            MessageBox(NULL, "接收中断!", "错误", MB_ICONERROR);
//            break;
//        }
//        file.write(buffer, bytesReceived);
//        totalReceived += bytesReceived;
//    }
//
//    file.close();
//    closesocket(clientSocket);
//    WSACleanup();
//}