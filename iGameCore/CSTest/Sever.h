#pragma once
#include <fstream>
#include <iostream>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "iGameFileReader.h"
#pragma comment(lib, "ws2_32.lib")

inline void serverThread() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return;

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "[Server] Socket error\n";
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(34567);

    if (bind(serverSocket, (sockaddr*) &serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[Server] Bind failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        std::cerr << "[Server] Listen failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    std::cout << "[Server] Listening on port 34567...\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) continue;

        std::cout << "[Server] Client connected\n";
        // 1. 接收扩展名长度
        uint32_t extLen = 0;
        recv(clientSocket, (char*) &extLen, sizeof(extLen), 0);

        // 2. 接收扩展名
        std::string ext(extLen, '\0');
        recv(clientSocket, ext.data(), extLen, 0);

        // 3. 构建本地文件名
        std::string localFile = "./ServerFile" + ext;
        std::ofstream outFile(localFile, std::ios::binary);
        // 4. 接收文件大小
        uint64_t fileSize = 0;
        int r = recv(clientSocket, (char*) &fileSize, sizeof(fileSize), 0);
        if (r <= 0) {
            std::cerr << "[Server] Failed to receive file size\n";
            closesocket(clientSocket);
            continue;
        }

        std::cout << "[Server] Receiving file of size " << fileSize << "\n";

        // 5. 接收文件内容
        uint64_t received = 0;
        char buf[65536];
        while (received < fileSize) {
            int toRead = (fileSize - received > sizeof(buf)) ? sizeof(buf) : (int) (fileSize - received);
            int n = recv(clientSocket, buf, toRead, 0);
            if (n <= 0) break;
            outFile.write(buf, n);
            received += n;
        }
        outFile.close();
        std::cout << "[Server] File received\n";


        // 6. 回传文件给客户端
        std::ifstream inFile(localFile, std::ios::binary | std::ios::ate);
        if (!inFile.is_open()) {
            std::cerr << "[Server] Failed to open file to send\n";
            closesocket(clientSocket);
            continue;
        }

        uint64_t sendSize = inFile.tellg();
        inFile.seekg(0, std::ios::beg);

        send(clientSocket, (char*) &sendSize, sizeof(sendSize), 0);

        while (!inFile.eof()) {
            inFile.read(buf, sizeof(buf));
            int n = (int) inFile.gcount();
            send(clientSocket, buf, n, 0);
        }

        inFile.close();
        closesocket(clientSocket);
        std::cout << "[Server] File sent back to client\n";
    }

    closesocket(serverSocket);
    WSACleanup();
}
