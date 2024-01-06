// Server.cpp
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <vector>

#pragma comment(lib, "ws2_32.lib") // windows下socket编程需要引入该库

#define DEFAULT_PORT 5270 // 服务器端口
#define DEFAULT_ADDR "127.0.0.1" // 服务器地址
#define BUFFER_SIZE 1500 // 最大传输长度

using namespace std;

vector<thread> clientThreads;

// 再在外部定义函数
void ServerSend(int sock) {
    char buffer[BUFFER_SIZE];
    while(true){
        memset(buffer, 0, sizeof(buffer));
        cin.getline(buffer, sizeof(buffer));
        if(send(sock, buffer, sizeof(buffer), 0) == -1){
            cout << "无法向客户端发送消息。" << endl;
            break;
        }
        cout << "发送成功!" << endl;
    }
}

void ServerRecv(int sock) {
    char buffer[BUFFER_SIZE];
    while(true){
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            cout << "与客户端的连接断开或出错。" << endl;
            break;
        }
        cout << "Client: " << buffer << endl;
    }
}

int main(){
    system("chcp 65001");
    // 初始化winsock库
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        printf("初始化winsock失败，错误代码: %d\n", err);
        return 1;
    }

    // 创建socket
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        cout << "创建socket失败！" << endl;
        return 1;
    }

    // 配置socket信息
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(DEFAULT_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(DEFAULT_ADDR);

    // 绑定ip与端口
    int bind_res = bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (bind_res == -1) {
        cout << "绑定IP和端口号失败！" << endl;
        return 1;
    }

    // 监听 ip:port
    listen(sock, 20);

    // 等待一个链接
    struct sockaddr_in server_addr;
    socklen_t server_addr_size = sizeof(server_addr);
    int client_sock = accept(sock, (struct sockaddr *) &server_addr, &server_addr_size);

    if (client_sock == -1) {
        cout << "接收客户端连接失败。" << endl;
        closesocket(client_sock);
        return 1;
    }

    clientThreads.emplace_back(ServerSend, client_sock);
    clientThreads.emplace_back(ServerRecv, client_sock);

    // 结束进程
    for(auto & clientThread : clientThreads){
        clientThread.join();
    }

    // 关闭
    closesocket(client_sock);
    closesocket(sock);
    WSACleanup();

    system("pause");
    return 0;
}