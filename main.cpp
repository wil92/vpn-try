/*
 * Copyright 2023 Guillermo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/netfilter_ipv4.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "base64.cpp"
#include "protocol.cpp"

#define LISTEN_BACKLOG 50
#define MAX_DATA_SIZE 100

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

int serverPortNo = 4332;
int clientPortNo = 4333;

void listeningOnPort(int port, void (*newConnection)(void *args)) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr{};
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
//    addr.sin_addr.s_addr = inet_addr("207.180.211.97");
    // depending on the system modify the function to call (in mac is different)
    addr.sin_port = htons(port);

    if (bind(sockfd, (sockaddr *) &addr, sizeof(addr)) == -1) {
        handle_error("bind");
    }

    if (listen(sockfd, LISTEN_BACKLOG) == -1) {
        handle_error("listen");
    }

    while (true) {
        struct sockaddr_storage addrNew{};
        // accept connections
        socklen_t addrNewSize = sizeof(addrNew);
        int socknew = accept(sockfd, (sockaddr *) &addrNew, &addrNewSize);
        if (socknew == -1) {
            handle_error("accept");
        }
        newConnection(&socknew);
    }
}

void *handleClientConnection(void *args) {
    int sockFd = *(int *) args;
    std::cout << "connected to client with socket " << sockFd << std::endl;

    in_addr_t addrRedirection;
    in_port_t portRedirection;

    char buff[MAX_DATA_SIZE];
    ssize_t numBytes;
    while (true) {
        if ((numBytes = recv(sockFd, buff, MAX_DATA_SIZE - 20, 0)) == -1) {
            handle_error("receiving message");
        }
        if (numBytes == 0) {
            std::cout << "Socket: " << sockFd << " is disconnected" << std::endl;
            break;
        }
        buff[numBytes] = '\0';
        char message[MAX_DATA_SIZE];
        int messageLen;

        protocol::from(&addrRedirection, &portRedirection, buff, (int) numBytes, message, &messageLen);
        char addrMsg[20];
        inet_ntop(AF_INET, &addrRedirection, addrMsg, sizeof(addrMsg));
        std::cout << "Socket: " << sockFd << ", Addr:" << addrMsg << ", Port: " << ntohs(portRedirection) << ", Message: " << message
                  << std::endl;
    }

    return nullptr;
}

void extractDestAddr(int fd, struct sockaddr_in *dest) {
    socklen_t destLen = sizeof(*dest);
    if (getsockopt(fd, SOL_IP, SO_ORIGINAL_DST, dest, &destLen) == -1) {
        std::cout << "Error with getAddrDest" << std::endl;
    }
    char ip[30];
    inet_ntop(AF_INET, &dest->sin_addr.s_addr, ip, sizeof(ip));
    std::cout << "-----------------------" << std::endl;
    std::cout << ip << std::endl;
    std::cout << ntohs(dest->sin_port) << std::endl;
    std::cout << "-----------------------" << std::endl;
}

struct serverConnection {
private:
    int sockfd;
    struct sockaddr_in addr{};
    struct sockaddr_in addrOriginal;
    bool firstMessage = true;
public:
    explicit serverConnection(const sockaddr_in &addrOriginal) : addrOriginal(addrOriginal) {}

    void connectToServer() {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(serverPortNo);
        addr.sin_addr.s_addr = INADDR_ANY;
//      addr.sin_addr.s_addr = inet_addr("207.180.211.97");

        if (connect(sockfd, (sockaddr *) &addr, sizeof(addr)) == -1) {
            handle_error("connect");
        }
    }

    void redirectDataToServer(char *msg) {
        char buffer[1000];
        int bufferLen = 0;
        if (firstMessage) {
            firstMessage = false;
            protocol::to(&addrOriginal.sin_addr.s_addr, &addrOriginal.sin_port, msg, (int) strlen(msg), buffer,
                         &bufferLen);
        } else {
            protocol::to(nullptr, nullptr, msg, (int) strlen(msg), buffer,
                         &bufferLen);
        }
        ssize_t res = send(sockfd, buffer, bufferLen, 0);
        std::cout << res << std::endl;
        if (res == -1) {
            handle_error("recv");
        }
        std::cout << buffer << std::endl;
        sleep(1);
    }
};

void *handleApplicationConnection(void *args) {
    int sockFd = *(int *) args;
    std::cout << "connected to application with socket " << sockFd << std::endl;

    sockaddr_in originalDest = sockaddr_in();
    extractDestAddr(sockFd, &originalDest);

    serverConnection sc{originalDest};
    sc.connectToServer();

    char buff[MAX_DATA_SIZE];
    ssize_t numBytes;
    while (true) {
        if ((numBytes = recv(sockFd, buff, MAX_DATA_SIZE - 1, 0)) == -1) {
            handle_error("receiving message");
        }
        if (numBytes == 0) {
            std::cout << "Socket: " << sockFd << " is disconnected" << std::endl;
            break;
        }
        buff[numBytes] = '\0';

        sc.redirectDataToServer(buff);
//        in_addr_t addr;
//        in_port_t port;
//        char addrMsg[20];
//        inet_ntop(AF_INET, &addr, addrMsg, sizeof(addrMsg));
//        std::cout << "Socket: " << sockFd << ", Addr:" << addrMsg << ", Port: " << port << ", Message: " << std::endl;
    }

    return nullptr;
}

void createNewClientConnectionThread(void *args) {
    pthread_t newThread = pthread_t();
    pthread_create(&newThread, nullptr, &handleClientConnection, args);
}

void createNewApplicationConnectionThread(void *args) {
    pthread_t newThread = pthread_t();
    pthread_create(&newThread, nullptr, &handleApplicationConnection, args);
}

int startServer() {
    std::cout << "Server started" << std::endl;

    listeningOnPort(serverPortNo, createNewClientConnectionThread);

    return 0;
}

int startClient() {
    std::cout << "Client started" << std::endl;

    listeningOnPort(clientPortNo, createNewApplicationConnectionThread);

    return 0;
}

int main(int argc, char *argv[]) {
    bool serverMode = false;

    // check application arguments
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--server-mode") == 0) {
            serverMode = true;
        }
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            // todo: show help here
            return 0;
        }
    }

    if (serverMode) {
        return startServer();
    }
    return startClient();
}
