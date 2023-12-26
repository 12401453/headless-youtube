#include "TcpListener.h"

int TcpListener::init() {

    m_socket = socket(AF_INET, SOCK_STREAM, 0); //AF_INET and SOCK_STREAM are both constants defined by a #define macro to be = 1
    if (m_socket == -1) {
        std::cerr << "socket() failed" << std::endl;
        return -1;
    }
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(m_port); //host-to-network-short converts little-endian binary representation of the port-number to big-endian
    //inet_pton is "internet pointer-to-a-string-to-a-number", and it assigns an IP address to the sin_addr field of the sockaddr_in hint struct
    inet_pton(AF_INET, m_ipAddress, &hint.sin_addr); //converts IP in format 127.0.0.1 to an array of integers. The third parameter is a pointer to the field of the hint struct which we are filling out with the format-converted IP address

    //the second argument of bind() is the pointer to the sockaddr_in called hint, except it only accepts pointers to structs of type sockaddr, not sockaddr_in, so we have to cast the mem address of our hint struct to a pointer to a plain sockaddr struct, which is what the (sockaddr*) operator does
    //the bind() and listen() functions are called simultaneously with checking their return values, which is a bit unclear imo, but it saves declaring unneccesary variables to hold their return values
    if (bind(m_socket, (sockaddr*)&hint, sizeof(hint)) == -1) {
        std::cerr << "Can't bind to IP/port" << std::endl;
        return -2;
    }
    if (listen(m_socket, SOMAXCONN) == -1) {
        std::cerr << "listen() failed" << std::endl;
        return -3;
    }

    FD_ZERO(&m_master);

    FD_SET(m_socket, &m_master);

    return 0;

}

int TcpListener::run() {

    bool running = true;

    while(running) {
        fd_set master_copy = m_master;

        //select will only return when a file descriptor in the fd_set master_copy gets data on it to read (the initial connection will put data to read on the listening-socket, m_socket, but once we call accept() on that and add the new client-socket to our fd_set, the client will write its actual message-data to the new client-socket, and nothing will be left on the listening-socket, so select() on its second go-through the loop will only signal data on the client-socket and will yeet any barren sockets (i.e. the listening-socket m_master) from its fd_set argument)
        if(select(FD_SETSIZE, &master_copy, nullptr, nullptr, nullptr) < 0) {
            std::cerr << "select() is fucked";
            return -4;
        }
        //select() modifies the master_copy variable so it only includes those file-descriptors (sockets) which are ready for I/O, in this case reading, which means on the second-pass of this loop the listening-socket is yeeted from master_copy and only the client-socket remains

        for(int i = 0; i < FD_SETSIZE; i++) {
            int sock = i;//master_copy.fd_array[i];

            //select() has yeeted any non-readable sockets from master_copy, so once accept() has been called on the listening-socket and the client-socket added to m_master, this FD_ISSET() call only returns true for the client-socket 
            if(FD_ISSET(sock, &master_copy)) {

                //this only gets called if the listening socket is determined to have some data for us to read by select(), which only happens when a client connects to the server on the listening socket, so this only triggers upon new client connections
                if (sock == m_socket) {
                    std::cout << "sock == listening - Client connected" << std::endl;
                    int client = accept(m_socket, nullptr, nullptr);
                    FD_SET(client, &m_master);

                    onClientConnected(client);

                }

                //if the socket determined to have data to read in it by select() is not the listening socket then we assume first that it is a client socket and we try to read the bytes on it into the buf array using recv()
                else {
                    std::cout << "sock != listening" << std::endl;
                    char buf[4096];
                    memset(buf, 0, 4096);
                    int bytesIn = recv(sock, buf, 4095, 0); //ensure that the last byte of msg is the null-byte

                    //if there were in fact no bytes to be read off the client socket, even though select told us there was, then this is because the client has disconnected from the socket and we need to close it and remove it from fd_set using FD_CLR()
                    if(bytesIn <= 0) {
                        std::cout << "bytesIn <= 0\nClient disconnected\n" << std::endl;
                        onClientDisconnected(sock);
                        close(sock);
                        FD_CLR(sock, &m_master);
                    }
                    else {
                        std::cout << "bytesIn > 0" << std::endl;
                        onMessageReceived(sock, buf, bytesIn);
                    }
                }
            }
        }
    }
    return 0;
}

void TcpListener::sendToClient(int clientSocket, const char* msg, int length) {

    send(clientSocket, msg, length, 0);
}

/*
void TcpListener::broadcastToClients(int sendingClient, const char* msg, int length) {
    for (int i = 0; i < FD_SETSIZE; i++) {
        int outSock = i;//master.fd_array[i];

        //the outSock is another connected client socket, so this code is only called if more than one client is connected (if the outSock is a member of the fd_set master). It loops round and sends the incoming data in buf to each outSock it finds so that the message gets to each client, but it has to differentiate between other connected clients and the main listening socket (which listens for new client connections) and the client which is currently writing to the server (stored in sendingClient), which is why we have the second if-statement; it writes the contents of buf to all the sockets in fd_set master EXCEPT the listening socket and the client-socket which wrote the message

        if(outSock != m_socket && outSock != sendingClient) {
            sendToClient(outSock, msg, length);
        }

    }

} */

void TcpListener::onMessageReceived(int clientSocket, const char* msg, int length) {

}

void TcpListener::onClientConnected(int clientSocket) {

}

void TcpListener::onClientDisconnected(int clientSocket) {

}
