
#ifndef _SERVER_SOCK_H_

#define _SERVER_SOCK_H_

#include <iostream>
#include <netinet/in.h>

typedef struct{
    uint16_t    PORT_NO;
    uint16_t    MAX_CLIENT_COUNT;
    uint16_t    LISTEN_BACKLOG;
    uint8_t     TCP_SOCKET:1,   //1:TCP, 0:UDP
                REUSE_ADDR:1,
                REUSE_PORT:1,
                BLOCKING:1; //1:Blocking, 0: Non-blocking

}SERVER_SOCK_INFO_T;

class ServerSocket {
    private:
        SERVER_SOCK_INFO_T server_sock_info;
        int16_t sock_fd = 0;
        struct sockaddr_in server_addr;
        int *client_socks = nullptr; //will handle the client sockets..

    public:
        ServerSocket(void);
        ~ServerSocket();
        void CreateSocket(const SERVER_SOCK_INFO_T *socket_config_);    //configres the socket and sets the socket opts.
        void StartServer(void);
        void SendToAllClients(std::uint8_t msg_to_send_)const;
        SERVER_SOCK_INFO_T GetSocketOptions(void)const;
    };

#endif