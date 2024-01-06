// Client.cpp
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <string>
#include <unistd.h>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 5270
#define DEFAULT_ADDR "127.0.0.1"
#define BUFFER_SIZE 1500

using namespace std;

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

string timeToTimeStr(time_t timeStamp) {
    struct tm *timeinfo = nullptr;
    char buffer[80];
    timeinfo = localtime(&timeStamp);
    strftime(buffer, 80, "%H:%M:%S", timeinfo);
    return string(buffer);
}

// 接收线程
void receiveThread(int sock) {
    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            cout << "与服务器的连接断开或出错。" << endl;
            break;
        }

        ClientMsg cmsg;
        encode(buffer, cmsg);
        cout << "[" << timeToTimeStr(cmsg.time) << " "
             << inet_ntoa(cmsg.ipAddress) << " "
             << cmsg.SourceName << cmsg.Message << endl;
    }
}

// 发送线程
void sendThread(int sock) {
    char buffer[BUFFER_SIZE];
    ClientMsg cmsg;
    while (true) {
        cin.sync(); // 清空输入缓存
        // 清空缓存
        memset(buffer, 0, sizeof(buffer));
        memset(cmsg.SourceName, 0, sizeof(cmsg.SourceName));
        memset(cmsg.Message, 0, sizeof(cmsg.Message));

        cin.getline(buffer, sizeof(buffer));
//        getchar();

        if(strlen(buffer) == 0){
            cout << "禁止非空输入！" << endl;
            continue;
        }

        if (strcmp(buffer, "quit") == 0) {
            exit(0);
        }

        string userInput(string("]: ") + buffer);
        cmsg.time = time(NULL);
        memcpy(cmsg.Message, userInput.c_str(), userInput.length());

        decode(buffer, cmsg);
        if (send(sock, buffer, sizeof(buffer), 0) == -1) {
            cout << "无法向服务器发送消息。" << endl;
            break;
        } else {
            cout << "发送成功！" << endl;
        }
    }
}

int main() {
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
        return 0;
    }

    // 配置socket信息
    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(DEFAULT_PORT);
    client_addr.sin_addr.s_addr = inet_addr(DEFAULT_ADDR);

    int connect_res = connect(sock, (struct sockaddr *) &client_addr, sizeof(client_addr));
    if (connect_res == -1) {
        cout << "连接服务器失败！可能服务器未开启。" << endl;
        closesocket(connect_res);
        closesocket(sock);
        WSACleanup();
        system("pause");
        return 0;
    }

    cout << "\n请输入你的昵称：";
    char userName[BUFFER_SIZE];
    cin >> userName;

    if (send(sock, userName, strlen(userName), 0) == -1) {
        cout << "无法向服务器发送消息。" << endl;
        closesocket(connect_res);
        closesocket(sock);
        WSACleanup();
        return 0;
    }
    cout << "您好：" << userName << "，欢迎使用聊天室！" << endl;

    // 启动接收和发送线程
    thread receiveThreadObj(receiveThread, sock);
    thread sendThreadObj(sendThread, sock);

    // 等待线程结束
    receiveThreadObj.join();
    sendThreadObj.join();

    closesocket(connect_res);
    closesocket(sock);
    WSACleanup();

    return 0;
}
