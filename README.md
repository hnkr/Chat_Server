# Chat Server
Chat Server of which socket can be configured / customized in C++.

## Requirements
- Ubuntu

## Working Principle
- Select based.
- Thread based. 2 thread will be in use:
    - 1 for listening & conn. / disconn. reqs. & receiving data
    - 1 for sending the received data to all connected clients

## Build & Run
- Build:
    -  g++ main.cpp -o ChatServer server_sock.cpp -pthread
- Run:
    - ./ChatServer

## Helpful Commands
- $ lsof -i:PORT_NUMBER can be used to see the list of PIDs which have work on PORT_NUMBER.
- $ sudo kill -9 PID_TO_KILL can be used to kill all / previous one before starting ChatServer.
