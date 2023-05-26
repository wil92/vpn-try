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
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/netfilter_ipv4.h>
#include <arpa/inet.h>
#include <pthread.h>

#define LISTEN_BACKLOG 50
#define MAX_DATA_SIZE 100

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

int serverPortNo = 4332;
int clientPortNo = 4333;

void listeningOnPort(int port, void *args, void (*newConnection)(void *args)) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr{};
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
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
        void *passThrow = &socknew;
        (((int*)args) + 1) = args;
        newConnection(passThrow);
    }
}

#ifdef SERVER

void *handleClientConnection(void *args) {
    int sockFd = *(int *) args;
    std::cout << "connected to client with socket " << sockFd << std::endl;

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
        std::cout << "Socket: " << sockFd << ", Message: " << buff << std::endl;
    }

    return nullptr;
}

void createNewConnectionThread(int *sockfd) {
    pthread_t newThread = pthread_t();
    pthread_create(&newThread, nullptr, &handleClientConnection, sockfd);
}

int main() {
    std::cout << "Server started" << std::endl;

    listeningOnPort(serverPortNo, createNewConnectionThread);

    return 0;
}

#else

void getAddrDest(int fd, struct sockaddr_in *dest) {
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

void *handleConnection(void *args) {
    int sockFd = *(int *) args;
    std::cout << "connected to client with socket " << sockFd << std::endl;

    sockaddr_in originalDest = sockaddr_in();
    getAddrDest(sockFd, &originalDest);

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
        std::cout << "Socket: " << sockFd << ", Message: " << buff << std::endl;
    }

    return nullptr;
}

void createNewConnectionThread(void *args) {
    int *sockFd = (int *)args;
    pthread_t newThread = pthread_t();
    pthread_create(&newThread, nullptr, &handleConnection, sockFd);
}

int main() {
    std::cout << "Client started" << std::endl;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(serverPortNo);
    addr.sin_addr.s_addr = INADDR_ANY;
//    addr.sin_addr.s_addr = inet_addr("207.180.211.97");

    if (connect(sockfd, (sockaddr *) &addr, sizeof(addr)) == -1) {
        handle_error("connect");
    }

    listeningOnPort(clientPortNo, NULL, createNewConnectionThread);

//    while (true)
//    {
//        char* buffer = "Hello world!!!";
//        size_t bufferSize = strlen(buffer);
//        ssize_t res = send(sockfd, buffer, bufferSize, 0);
//        std::cout<<res<<std::endl;
//        if (res == -1) {
//            handle_error("recv");
//        }
//        std::cout<<buffer<<std::endl;
//        sleep(1);
//    }
}
#endif
