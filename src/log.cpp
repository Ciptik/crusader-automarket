#include "log.hpp"
#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>

UDPLogger::UDPLogger(const std::string& ip, int port) 
    : recipient_ip(ip), recipient_port(port) {
    // Инициализируем Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return;
    }

    // Создаем UDP-сокет
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Error creating socket" << std::endl;
        WSACleanup();
    }
}

UDPLogger::~UDPLogger() {
    closesocket(sock);
    WSACleanup();
}

void UDPLogger::sendMessage(const std::string& message) {
    // Настраиваем адрес получателя
    struct sockaddr_in recipient;
    memset(&recipient, 0, sizeof(recipient));
    recipient.sin_family = AF_INET;
    recipient.sin_addr.s_addr = inet_addr(recipient_ip.c_str());
    recipient.sin_port = htons(recipient_port);

    // Отправляем сообщение
    sendto(sock, message.c_str(), message.length(), 0, (struct sockaddr*)&recipient, sizeof(recipient));
}