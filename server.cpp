#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <stdint.h>
#include <arpa/inet.h>

using namespace std;

int main() {
    int serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketFd == -1) {
        cout << "Error creating socket. "  << strerror(errno) << endl;
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr.s_addr);
    memset(serverAddress.sin_zero, '0', 8); // can igore this line if ye want

    
    if (bind(serverSocketFd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cout << "Error binding to socket. " << strerror(errno) << endl;
        close(serverSocketFd);
        return 1;
    }

    
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
        close(clientSocket);

    }

    close(serverSocketFd);
    return 0;

}
