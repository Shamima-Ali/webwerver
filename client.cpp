#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <stdint.h>
#include <arpa/inet.h>

using namespace std;

typedef uint32_t u_int32_t;
typedef uint16_t u_int16_t;

#define htons(x) ((u_int16_t)(x))

int main() {
    int clientSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocketFd == -1) {
        cout << "Error creating client. "  << strerror(errno) << endl;
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr.s_addr);
    // serverAddress.sin_addr.s_addr = INADDR_ANY;

    connect(clientSocketFd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (errno == -1) {
        cout << "Error connecting to server socket " << strerror(errno) << endl;
        return 1;
    }

    
    char message[1024] = {0};
    cin >> message;
    send(clientSocketFd, &message, strlen(message), 0);
    if (errno == -1) {
        cout << "Error sending message to server. " << strerror(errno) << endl;
        return 1;
    }

    close(clientSocketFd);

    return 0;

}