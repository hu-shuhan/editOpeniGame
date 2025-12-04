#pragma once

#if defined(_WIN32) || defined(_WIN64)

#include <iGameUnstructuredMesh.h>
#include <iostream>
#include <thread>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include "IGC/iGameIGCWriter.h"
#include "MeshCodec/iGameMeshDecoderFilter.h"
#include "Spline XML/iGameSplineReaderCPU.h"
#include "iGameFileIO.h"
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
        return std::to_string(selected_idx) + "|" + filePath;3
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
iGame::DataObject::Pointer OpenFile(const std::string& filePath) {
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
    auto writer = iGame::IGCWriter::New();
    writer->SetCodecControlParams(iGame::MeshEncoderFilter<iGame::EncodeOutputBinaryArray>::GenerateDefaultCodecParams(tem));

    if (!writer->WriteToFile(tem, "./CScomp.igc")) {
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
    std::cout << "listening..." << std::endl;

    while (true) {
        SOCKET clientSocket = INVALID_SOCKET;
        sockaddr_in clientAddr;
        int iAddrLength = sizeof(clientAddr);
        std::cout << "wait for login..." << std::endl;
        clientSocket = accept(serverSocket, (SOCKADDR*) &clientAddr, &iAddrLength);

        if (clientSocket == INVALID_SOCKET) {
            std::cout << "Accept Error !" << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }
        std::cout << "client addres：" << inet_ntoa(clientAddr.sin_addr) << std::endl;

        char buffer[1024];
        int iLenOfRecvData = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (iLenOfRecvData > 0) {
            // 反序列化数据
            OpenCmd recCmd;
            recCmd.deserialize(buffer);
            std::cout << recCmd.filePath << std::endl;
            auto check = LoadAndCompress(recCmd.filePath);
            std::ifstream file("./CScomp.igc", std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                std::cerr << "cant open file: "
                          << "./CScomp.igc" << std::endl;
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
            std::cout << "sever stoped..." << std::endl;
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

#endif
