/**
 * server_sock.hpp
 * Hunkar Ciplak, hunkarciplak@hotmail.com
*/


#ifndef _SERVER_SOCK_H_

#define _SERVER_SOCK_H_

#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <mutex>
#include <queue>

typedef struct{
    struct sockaddr_in client_addr;
    int client_fd;
}CLIENT_INFO_T;

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
        CLIENT_INFO_T *client_socks = nullptr;
        std::queue<std::string> qmessage_to_send;
        std::mutex mbuf_lock, mclient_lock;

    public:
        ServerSocket(void);
        ~ServerSocket();
        void CreateSocket(const SERVER_SOCK_INFO_T *);    //configres the socket and sets the socket opts.
        void StartServer(void);
        void SendQueueToAllClients(void);
        SERVER_SOCK_INFO_T GetSocketOptions(void)const;
        CLIENT_INFO_T * GetClientSocketBuf(void)const;
    };

#endif