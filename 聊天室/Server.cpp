// Server.cpp
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 5270
#define DEFAULT_ADDR "127.0.0.1"
#define BUFFER_SIZE 1500

using namespace std;

// client 标记
struct ClientInfo {
    string userName;
    in_addr ipAddress;
    int port;
};

// 客户端报文
struct ClientMsg {
    in_addr ipAddress;
    unsigned int time, online;
    char SourceName[20], Message[1024];
};

// 将结构体序列化为字节流
void decode(char *buffer, ClientMsg &obj) {
    memcpy(buffer, &obj, sizeof(ClientMsg));
}

// 将字节流反序列化为结构体
void encode(char *buffer, ClientMsg &obj) {
    memcpy(&obj, buffer, sizeof(ClientMsg));
}

// 时间戳转时间字符串
string timeToTimeStr(time_t timeStamp) {
    struct tm *timeinfo = nullptr;
    char buffer[80];
    timeinfo = localtime(&timeStamp);
    strftime(buffer, 80, "%H:%M:%S", timeinfo);
    return string(buffer);
}

vector<thread> clientThreads; // 为每个用户分配一个线程
unordered_set<int> connectedClients; // 存储所有连接的客户端套接字

// 广播消息给所有客户端
void broadcastMessage(ClientMsg &message, int currentSock) {
    char buffer[BUFFER_SIZE]{0};
    decode(buffer, message);
    for (int clientSock: connectedClients) {
        if (clientSock != currentSock) {
            send(clientSock, buffer, sizeof(buffer), 0);
        }
    }
}

// 服务器主动发送广播报文
void ServerBroadcastSend(void) {
    char buffer[BUFFER_SIZE];
    while(true) {
        memset(buffer, 0, sizeof(buffer));
        cin.getline(buffer, sizeof(buffer));
//        getchar(); // 接受遗留的换行符

        in_addr serverAddr;
        inet_pton(AF_INET, DEFAULT_ADDR, &serverAddr);
        ClientMsg sendBag{serverAddr, (unsigned long)time(NULL),
                          (unsigned long)connectedClients.size(), "Server]: "};
        memcpy(sendBag.Message, buffer, strlen(buffer));
        memset(buffer, 0, sizeof(buffer));
        decode(buffer, sendBag);

        for(int clientSock : connectedClients){
            send(clientSock, buffer, sizeof(buffer), 0);
        }
    }
}

// 单独处理每个加入的客户端
void handleClient(int client_sock, const ClientInfo &cInfo) {
    // 将新连接的客户端套接字加入全局列表
    connectedClients.insert(client_sock);
    cout << "[" << timeToTimeStr(time(NULL)) << " "
         << inet_ntoa(cInfo.ipAddress) << ":" << cInfo.port << " "
         << cInfo.userName << "] 加入了连接。当前在线："
         << connectedClients.size() << " 人" << endl;

    auto broadcast = [&](int x){
        // 广播新用户 [加入 or 离开] 聊天室通知
        ClientMsg joinLink{cInfo.ipAddress, (unsigned int) time(NULL), (unsigned int) connectedClients.size()};
        memcpy(joinLink.SourceName, cInfo.userName.c_str(), sizeof(joinLink.SourceName));
        string str = string((x == 0) ? "] 加入" : "] 离开") + "了聊天室。当前在线：";
        string strTemp = str + to_string(connectedClients.size() - x) + " 人";
        memcpy(joinLink.Message, strTemp.c_str(), strlen(strTemp.c_str()));
        broadcastMessage(joinLink, client_sock);
    };

    broadcast(0);

    char buffer[BUFFER_SIZE];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(client_sock, buffer, sizeof(buffer), 0);
        // 连接断开
        if (bytesRead <= 0) {
            cout << "[" << timeToTimeStr(time(NULL)) << " "
                 << inet_ntoa(cInfo.ipAddress) << ":" << cInfo.port << " "
                 << cInfo.userName << "] 断开了连接。当前在线："
                 << connectedClients.size() - 1 << " 人" << endl;

            broadcast(1);
            break;
        }

        ClientMsg cmsg;
        encode(buffer, cmsg);
        memset(cmsg.SourceName, 0, sizeof(cmsg.SourceName));

        cout << "[" << timeToTimeStr(cmsg.time) << " "
             << inet_ntoa(cInfo.ipAddress) << ":" << cInfo.port << " "
             << cInfo.userName << cmsg.Message << endl;

        // 待广播包
        cmsg.ipAddress = cInfo.ipAddress;
        cmsg.online = connectedClients.size();
        memcpy(cmsg.SourceName, cInfo.userName.c_str(), cInfo.userName.length());

        // 广播消息给所有客户端
        broadcastMessage(cmsg, client_sock);
    }

    // 删除当前套接字
    connectedClients.erase(client_sock);
    // 关闭连接
    closesocket(client_sock);
}


int main() {
    system("chcp 65001");

    // 定义变量->获取winsock版本->加载winsock库->初始化->创建套接字->设置套接字选项->特定操作->关闭套接字->卸载winsock库

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
        return 0;
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
        return 0;
    }

    // 监听 ip:port
    cout << "服务器启动，开始监听5270端口。" << endl;
    listen(sock, 20);

    thread ServerSend(ServerBroadcastSend); // 服务器广播报文

    while (true) {
        struct sockaddr_in server_addr;
        socklen_t server_addr_size = sizeof(server_addr);
        int client_sock = accept(sock, (struct sockaddr *) &server_addr, &server_addr_size);

        if (client_sock == -1) {
            cout << "接收客户端连接失败。" << endl;
            closesocket(client_sock);
            break;
        }

        char userName[BUFFER_SIZE]{0};
        int bytesRead = recv(client_sock, userName, sizeof(userName), 0);
        if (bytesRead <= 0) {
            cout << "与客户端的连接断开或出错。" << endl;
            break;
        }
        // 保存客户端信息
        ClientInfo cInfo{userName, server_addr.sin_addr, ntohs(server_addr.sin_port)};
        // 启动一个新的线程处理客户端连接
        clientThreads.emplace_back(handleClient, client_sock, cInfo);
    }

    // 等待所有线程结束
    for (auto &thread: clientThreads) {
        thread.join();
    }

    ServerSend.join();

    closesocket(sock);
    WSACleanup();

    return 0;
}
