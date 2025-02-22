#pragma once
#include<Client.h>
#include<Sever.h>
void CSTest(int selected_idx, std::string filePath) {
    std::thread server_thread(serverThread);

    std::thread client_thread(clientThread,selected_idx,filePath);

    // 等待线程完成
    server_thread.join();
    client_thread.join();
    return ;
}