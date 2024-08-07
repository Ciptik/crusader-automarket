#ifndef LOG_HPP
#define LOG_HPP

#include <string>

class UDPLogger {
public:
    UDPLogger(const std::string& ip, int port);
    ~UDPLogger();
    
    void sendMessage(const std::string& message);

private:
    int sock;
    std::string recipient_ip;
    int recipient_port;
};

#endif // LOG_HPP