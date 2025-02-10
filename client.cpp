#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <thread> 
#include <sys/types.h>

using namespace std;

typedef uint32_t u_int32_t;
typedef uint16_t u_int16_t;

#define htons(x) ((u_int16_t)(x))

void send_requestBuffer(int clientSocketFd) {
    char message[1024] = "GET / HTTP/1.1";
    send(clientSocketFd, &message, strlen(message), 0);
    if (errno == -1) {
        cout << "Error sending message to server. " << strerror(errno) << endl;
        return;
    }

    cout << "receving message from sever..." << endl;
    
    char serverBuffer[183+1024] = {0};
    ssize_t bytesReceived = recv(clientSocketFd, serverBuffer, sizeof(serverBuffer), 0);
    if (bytesReceived == -1) {
        cout << "Error receiving message from server. " << strerror(errno) << endl;
        return;
    }

    // cout << "Client thread sleeping for 30 sec..." << endl;
    // this_thread::sleep_for (std::chrono::seconds(5));
    cout << "Server response: " << serverBuffer << endl;

    cout << "Closing client socket" << endl;
    close(clientSocketFd);

    return;
}

int main() {
    for (int i = 0; i < 3; i++) {
        int clientSocketFd = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocketFd == -1) {
            cout << "Error creating client. "  << strerror(errno) << endl;
            return 1;
        }

        struct addrinfo hints;
        struct addrinfo *servinfo;
        struct addrinfo *p;
        int status;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        if ((status = getaddrinfo(NULL, "8080", &hints, &servinfo)) != 0) {
            cout << "Error getting address info " << gai_strerror(status) << endl;
            return 1;
        }

        for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((clientSocketFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        if (connect(clientSocketFd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("connect");
            close(clientSocketFd);
            continue;
        }

            break; // if we get here, we must have connected successfully
        }

        cout << "Connected to server. " << endl;

        if (p == NULL) {
        // looped off the end of the list with no connection
            cout << (stderr, "failed to connect\n");
            exit(2);
        }

        freeaddrinfo(servinfo); // all done with servinfo

    
        cout << "sending message " << i << " to sever..." << endl;
        thread clientThread(send_requestBuffer, clientSocketFd);
        clientThread.join();
        
    }

    

    return 0;

}