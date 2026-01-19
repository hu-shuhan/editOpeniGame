#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <../IO/IGC/iGameIGCWriter.h>
#include "iGameFileReader.h"
#include <future>
#pragma comment(lib, "ws2_32.lib")


inline std::string GetFileExtension(const std::string& path) {
    size_t pos = path.find_last_of(".");
    if (pos == std::string::npos) return "";
    return path.substr(pos);
}

inline std::string clientThread(int dummyIdx, std::string filePath) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return nullptr;

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "[Client] Socket error\n";
        WSACleanup();
        return nullptr;
    }
    //auto obj = iGame::FileIO::ReadFile(filePath);


    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(34567);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*) &serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[Client] Connect failed\n";
        closesocket(clientSocket);
        WSACleanup();
        return nullptr;
    }

    std::cout << "[Client] Connected to server\n";
    std::string ext = GetFileExtension(filePath);
    uint32_t extLen = ext.size();
    // 先发送扩展名长度
    send(clientSocket, (char*) &extLen, sizeof(extLen), 0);
    // 再发送扩展名字符串
    send(clientSocket, ext.c_str(), extLen, 0);

   
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[Client] Failed to open file: " << filePath << "\n";
        closesocket(clientSocket);
        WSACleanup();
        return nullptr;
    }

    uint64_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    send(clientSocket, (char*) &fileSize, sizeof(fileSize), 0);

    // 2. 发送文件内容
    char buf[65536];
    while (!file.eof()) {
        file.read(buf, sizeof(buf));
        int n = (int) file.gcount();
        send(clientSocket, buf, n, 0);
    }
    file.close();
    std::cout << "[Client] File sent to server\n";

    // 3. 接收文件大小
    uint64_t recvSize = 0;
    int r = recv(clientSocket, (char*) &recvSize, sizeof(recvSize), 0);
    if (r <= 0) {
        std::cerr << "[Client] Failed to receive file size from server\n";
        closesocket(clientSocket);
        WSACleanup();
        return nullptr;
    }

    std::cout << "[Client] Receiving file of size " << recvSize << "\n";

    // 4. 接收文件内容
    std::string localFileName = "./clientFile"+ext;
    std::ofstream outFile(localFileName, std::ios::binary);
    uint64_t received = 0;
    while (received < recvSize) {
        int toRead = (recvSize - received > sizeof(buf)) ? sizeof(buf) : (int) (recvSize - received);
        int n = recv(clientSocket, buf, toRead, 0);
        if (n <= 0) break;
        outFile.write(buf, n);
        received += n;
    }
    outFile.close();
    std::cout << "[Client] File received\n";

    closesocket(clientSocket);
    WSACleanup();
    return localFileName;
}
