#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <signal.h>

using namespace std;



int main() {
    int serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketFd == -1) {
        cout << "Error creating socket. "  << strerror(errno) << endl;
        return 1;
    }

    int sockfd;  
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP address

    if ((rv = getaddrinfo(NULL, "8080", &hints, &servinfo)) != 0) {
        cout << stderr << " getaddrinfo: " << gai_strerror(rv) << endl;
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((serverSocketFd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        if (bind(serverSocketFd, p->ai_addr, p->ai_addrlen) == -1) {
            cout << "Error binding to socket. " << strerror(errno) << endl;
            close(serverSocketFd);
            continue;;
        }

        break; // if we get here, we must have connected successfully
    }

    if (p == NULL) {
    // looped off the end of the list with no successful bind
        fprintf(stderr, "failed to bind socket\n");
        return 1;
    }

    freeaddrinfo(servinfo); // all done with this structure
    
    if (listen(serverSocketFd, 5) == -1) {
        cout << "Error listening on socket " << strerror(errno) << endl;
        return 1;
    }

    cout << "Server is running on 127.0.0.1:8080" << endl;

    while (true) {
        int clientSocket = accept(serverSocketFd, nullptr, nullptr);
        if (clientSocket == -1) {
            cout << "Error accepting connection." << strerror(errno) << endl;
            return 1;
        }

        char clientBuffer[1024] = {0};
        ssize_t bytesReceived = recv(clientSocket, clientBuffer, sizeof(clientBuffer), 0);
        if (bytesReceived == -1) {
            cout << "Error receiving message from client. " << strerror(errno) << endl;
            return 1;
        }
        cout << "Message from client: " << clientBuffer << endl;

        char ackBuff[1024] = "HTTP/1.1 200 OK\r\n\r\nMessage received";
        send(clientSocket, &ackBuff, strlen(ackBuff), 0);
        close(clientSocket);

    }

    close(serverSocketFd);
    return 0;

}
