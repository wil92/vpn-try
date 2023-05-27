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
#include <utility>
#include <map>

#include <sys/socket.h>
#include <poll.h>
#include <netinet/in.h>
#include <linux/netfilter_ipv4.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <semaphore.h>

#include "protocol.cpp"

#define LISTEN_BACKLOG 50
#define MAX_DATA_SIZE 100

#define socks_t std::pair<int, int>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

int serverPortNo = 4332;
int clientPortNo = 4333;

std::map<int, sem_t*> sems;
void addSem(sem_t *s, int sock) {
    sem_init(s, 0, 0);
    sems.insert(std::make_pair(sock, s));
}
//sem_t s;

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

void *handleDestMsg(void *args) {
    socks_t socks = *((socks_t *) args);
    int sockFd = socks.first;
    int sockClientFd = socks.second;
    std::cout << "--------- " << sockClientFd << std::endl;
    std::cout << "connected to destination with socket " << sockFd << std::endl;

    sem_t *s = sems.find(sockFd)->second;
    sem_post(s);

    char buff[MAX_DATA_SIZE];
//    struct pollfd check{sockFd, POLLIN};
    while (true) {
        ssize_t numBytes;
//        poll(&check, 1, 1000);

        if ((numBytes = recv(sockFd, buff, MAX_DATA_SIZE - 35, 0)) == -1) {
            handle_error("receiving msg");
        }
        if (numBytes == 0) {
            std::cout << "Socket: " << sockFd << " is disconnected" << std::endl;
            close(sockClientFd);
            sems.erase(sockFd);
            break;
        }
        buff[numBytes] = '\0';

        std::cout << "Destination msg: " << buff << std::endl;
        char msg[1000];
        int msgLen;
        protocol::to(nullptr, nullptr, buff, (int) numBytes, msg, &msgLen);
        if (send(sockClientFd, msg, msgLen, 0) == -1) {
            handle_error("sending destination response");
        }
    }

    return nullptr;
}

struct destConnection {
private:
    int sockfd;
    struct sockaddr_in addr{};
    bool isConnected = false;
    sem_t s;
public:
    int clientSockFd;
    in_addr_t addrRedirection;
    in_port_t portRedirection;

    void createRecvThread() {
        pthread_t newThread = pthread_t();
        socks_t socks{sockfd, clientSockFd};
        std::cout << "//////// " << clientSockFd << std::endl;
        pthread_create(&newThread, NULL, &handleDestMsg, &socks);
    }

    void connectToDestination() {
        if (!isConnected) {
            isConnected = true;

            sockfd = socket(AF_INET, SOCK_STREAM, 0);

            addSem(&s, sockfd);

            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = portRedirection;
            addr.sin_addr.s_addr = addrRedirection;
//          addr.sin_addr.s_addr = inet_addr("207.180.211.97");

            if (connect(sockfd, (sockaddr *) &addr, sizeof(addr)) == -1) {
                handle_error("connect");
            }

            char addrMsg[20];
            inet_ntop(AF_INET, &addrRedirection, addrMsg, sizeof(addrMsg));
            std::cout << "Connected to Addr:" << addrMsg << ", Port: " << ntohs(portRedirection) << ", Sock: " << sockfd
                      << std::endl;

            createRecvThread();
        }
    }

    void sendData(char *data, int dataLen) {
        sem_wait(&s);
        ssize_t res = send(sockfd, data, dataLen, 0);
        std::cout << "sent data to destination, Sock: " << sockfd << std::endl;
        if (res == -1) {
            handle_error("recv destination");
        }
        sem_post(&s);
    }
};

void *handleClientConnection(void *args) {
    int sockFd = *(int *) args;
    std::cout << "connected to client with socket " << sockFd << std::endl;

    destConnection dc{};
    dc.clientSockFd = sockFd;
    std::cout << "++++++++ " << sockFd << std::endl;

    char buff[MAX_DATA_SIZE];
    ssize_t numBytes;
    while (true) {
        if ((numBytes = recv(sockFd, buff, MAX_DATA_SIZE, 0)) == -1) {
            handle_error("receiving message");
        }
        if (numBytes == 0) {
            std::cout << "Socket: " << sockFd << " is disconnected" << std::endl;
            break;
        }
        buff[numBytes] = '\0';
        char message[MAX_DATA_SIZE];
        int messageLen;

        protocol::from(&dc.addrRedirection, &dc.portRedirection, buff, (int) numBytes, message, &messageLen);

        std::cout << "- " << message << std::endl;

        dc.connectToDestination();
        dc.sendData(message, messageLen);
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

void *handleServerMsg(void *args) {
    socks_t socks = *((socks_t *) args);
    int sockFd = socks.first;
    int sockApplicationFd = socks.second;
    std::cout << "AAAA server: " << sockFd << ", app: " << sockApplicationFd << std::endl;
    std::cout << "connected to server waiting form response in socket: " << sockFd << std::endl;

    sem_t *s = sems.find(sockFd)->second;
    sem_post(s);

    char buff[MAX_DATA_SIZE];
    ssize_t numBytes;
    while (true) {
        if ((numBytes = recv(sockFd, buff, MAX_DATA_SIZE, 0)) == -1) {
            handle_error("receiving message");
        }
        if (numBytes == 0) {
            std::cout << "Socket: " << sockFd << " is disconnected" << std::endl;
            close(sockApplicationFd);
            sems.erase(sockFd);
            break;
        }
        buff[numBytes] = '\0';

        char msg[1000];
        int msgLen;
        protocol::from(nullptr, nullptr, buff, (int) numBytes, msg, &msgLen);
        std::cout << "Server msg: " << msg << std::endl;
        if (send(sockApplicationFd, msg, msgLen, 0) == -1) {
            handle_error("sending destination response");
        }
    }

    return nullptr;
}

struct serverConnection {
private:
    int sockfd;
    struct sockaddr_in addr{};
    struct sockaddr_in addrOriginal;
    bool firstMessage = true;
    sem_t s;
public:
    int applicationSockFd;

    explicit serverConnection(const sockaddr_in &addrOriginal) : addrOriginal(addrOriginal) {}

    void createRecvThread() {
        pthread_t newThread = pthread_t();
        socks_t socks{sockfd, applicationSockFd};
        std::cout << "AAAA server: " << sockfd << ", app: " << applicationSockFd << std::endl;
        pthread_create(&newThread, NULL, &handleServerMsg, &socks);
    }

    void connectToServer() {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        addSem(&s, sockfd);

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(serverPortNo);
        addr.sin_addr.s_addr = INADDR_ANY;
//      addr.sin_addr.s_addr = inet_addr("207.180.211.97");

        if (connect(sockfd, (sockaddr *) &addr, sizeof(addr)) == -1) {
            handle_error("connect");
        }

        createRecvThread();
    }

    void sendData(char *msg) {
        sem_wait(&s);
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
        std::cout << "-- " << msg << std::endl;
        std::cout << "---" << buffer << std::endl;
        sem_post(&s);
    }
};

void *handleApplicationConnection(void *args) {
    int sockFd = *(int *) args;
    std::cout << "connected to application with socket " << sockFd << std::endl;

    sockaddr_in originalDest = sockaddr_in();
    extractDestAddr(sockFd, &originalDest);

    serverConnection sc{originalDest};
    sc.applicationSockFd = sockFd;
    sc.connectToServer();

    char buff[MAX_DATA_SIZE];
    ssize_t numBytes;
    while (true) {
        if ((numBytes = recv(sockFd, buff, MAX_DATA_SIZE - 35, 0)) == -1) {
            handle_error("receiving message");
        }
        if (numBytes == 0) {
            std::cout << "Socket: " << sockFd << " is disconnected" << std::endl;
            break;
        }
        buff[numBytes] = '\0';

        sc.sendData(buff);
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
