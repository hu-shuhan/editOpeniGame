#pragma once
#include <iGameUnstructuredMesh.h>
#include <iostream>
#include <thread>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include "iGameFileIO.h"
#include "Spline XML/iGameNurbsReader.h"
#include "iGameMeshCodec/iGameMeshEncoder.h"
#include "iGameMeshCodec/iGameMeshDecoder.h"
#ifndef OPENCMD_H
#define OPENCMD_H
#include <string>

class OpenCmd {
public:
    int selected_idx;
    std::string filePath;

    // 序列化函数
    std::string serialize() const {
        // 将 selected_idx 和 filePath 序列化为一个字符串
        return std::to_string(selected_idx) + "|" + filePath;
    }

    // 反序列化函数
    void deserialize(const std::string& data) {
        size_t pos = data.find("|");
        if (pos != std::string::npos) {
            selected_idx = std::stoi(data.substr(0, pos));
            filePath = data.substr(pos + 1);
        }
    }
};
#endif
iGame::DataObject::Pointer  OpenFile(const std::string& filePath) {
    using namespace iGame;
    if (filePath.empty() || strrchr(filePath.data(), '.') == nullptr) return nullptr;

    auto obj = iGame::FileIO::ReadFile(filePath);
    //_obj = obj;
    if (obj == nullptr) {
        igDebug("This file read error.");
        return nullptr;
    }
    auto filename = filePath.substr(filePath.find_last_of('/') + 1);
    obj->SetName(filename.substr(0, filename.find_last_of('.')).c_str());
    obj->GetPropertys()->AddProperty(Variant::String, "FilePath")->SetValue(filePath);
    //Q_EMIT AddFileToModelList(QString(filePath.substr(filePath.find_last_of('/') + 1).c_str()));
    //return;
    return obj;
}
bool LoadAndCompress(std::string filePath) {
    auto tem = OpenFile(filePath);
    iGame::MeshEncoder::Pointer filter = iGame::MeshEncoder::New();
    filter->m_PointQuantMode = iGame::QuantMode::None;
    filter->m_PointQuantizedBits = 16;
    filter->m_AttrbQuantMode = iGame::QuantMode::None;
    filter->m_AttrbQuantizedBits = 16;

    filter->SetNumberOfInputs(1);
    filter->SetSaveFilePath("D:/SendTest.igc");
    filter->SetInput(tem);

    if (!filter->Execute()) {
        igDebug("Compress File Error\n");
        return false;
    }
    return true;
}
void serverThread() {
    system("cls");
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(sockVersion, &wsaData) != 0) return;

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cout << "socket error:" << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
    serverAddr.sin_port = htons(12345);

    if (bind(serverSocket, (SOCKADDR*) &serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "Bind Error!" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        std::cout << "Listen Error !" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return;
    }
    std::cout << "正在监听..." << std::endl;

    while (true) {
        SOCKET clientSocket = INVALID_SOCKET;
        sockaddr_in clientAddr;
        int iAddrLength = sizeof(clientAddr);
        std::cout << "等待登录..." << std::endl;
        clientSocket = accept(serverSocket, (SOCKADDR*) &clientAddr, &iAddrLength);

        if (clientSocket == INVALID_SOCKET) {
            std::cout << "Accept Error !" << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }
        std::cout << "客户端地址：" << inet_ntoa(clientAddr.sin_addr) << std::endl;

        char buffer[1024];
        int iLenOfRecvData = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (iLenOfRecvData > 0) {
            // 反序列化数据
            OpenCmd recCmd;
            recCmd.deserialize(buffer);
            auto check = LoadAndCompress(recCmd.filePath);
            std::ifstream file("D:/SendTest.igc", std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                std::cerr << "文件打开失败: "
                          << "D:/SendTest.igc" << std::endl;
                return;
            }

            // 1. 发送文件大小
            std::streamsize fileSize = file.tellg();
            file.seekg(0, std::ios::beg);
            send(clientSocket, (char*) &fileSize, sizeof(fileSize), 0);

            // 2. 发送文件内容
            constexpr size_t BUFFER_SIZE = 65536;
            char buffer[BUFFER_SIZE];
            while (!file.eof()) {
                file.read(buffer, BUFFER_SIZE);
                send(clientSocket, buffer, file.gcount(), 0);
            }

            file.close();

        } else {
            std::cout << "服务器断开，无接收..." << std::endl;
            break;
        }
        closesocket(clientSocket);
        closesocket(serverSocket);
        WSACleanup();
        return;     
    }
    closesocket(serverSocket);
    WSACleanup();
}