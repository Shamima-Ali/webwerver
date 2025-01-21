#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fstream>
#include <cstdlib>
#include <thread> 

#include <sys/types.h>


using namespace std;


void process_clientBuffer(char clientBuffer[1024], int clientSocket) {
    if (clientBuffer[5] == ' ') {
        cout << "GET request" << endl;
        

        const char *path= "./www/index.html";
        ifstream indexFile;
        indexFile.open (path);

        if (indexFile.is_open()) {
            indexFile.seekg (0, indexFile.end);
            int length = indexFile.tellg();
            indexFile.seekg (0, indexFile.beg);

            const char* httpResponse = "HTTP/1.1 200 OK\r\n\r\n";
            
            char * buffer = new char [length+1024];
            memcpy(buffer, httpResponse, strlen(httpResponse));
            indexFile.read (buffer + strlen(httpResponse), length);

            if (!indexFile) {
                cout << "Error: index.html " << indexFile.gcount() << " could be read";
            }
            indexFile.close();
            
            thread::id child_thread_id = this_thread::get_id();
            
            cout << "Child Thread ID: " << child_thread_id << endl;

            // // Convert thread ID to string
            // std::stringstream ss;
            // ss << child_thread_id;
            // std::string id_str = ss.str();

            // // Allocate a char* buffer and copy the string
            // char* c_id = new char[1024];
            // std::strncpy(c_id, id_str.c_str(), 1023); // Use strncpy for safety
            // c_id[1023] = '\0'; // Null-terminate the string

            // // Cleanup dynamically allocated memory
            // delete[] c_id;
    
            this_thread::sleep_for (std::chrono::seconds(20));
            send(clientSocket, buffer, strlen(buffer), 0);
            delete[] buffer;

        } else {
            cout << "Error opening file." << endl;
        }
        

    } else {
        const char *buffer = "HTTP/1.1 400 OK\r\n\r\nNot Found";
        send(clientSocket, buffer, strlen(buffer), 0);
    }

    cout << "Closing client socket now. " << endl;
    close(clientSocket);
}

void createChildProcess(int clientSocket) {
    pid_t c_pid = fork();
    cout << c_pid << endl;

    if (c_pid == -1) {
        cout << "Error creating child" << endl;
        return;
    } else if (c_pid == 0) {
        cout << "I am child " << getpid() << endl;

        char clientBuffer[1024] = {0};
        ssize_t bytesReceived = recv(clientSocket, clientBuffer, sizeof(clientBuffer), 0);
        if (bytesReceived == -1) {
            cout << "Error receiving message from client. " << strerror(errno) << endl;
            return;
        }

        process_clientBuffer(clientBuffer, clientSocket);

        cout << "Closing client socket now. " << endl;
        close(clientSocket);

    } else {
        close(clientSocket); 
    }

    return;
}
int createSocket() {
    int serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    return serverSocketFd;
}


int main() {
    int serverSocketFd = createSocket();
    if (serverSocketFd == -1) {
        cout << "Error creating socket. "  << strerror(errno) << endl;
    }

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
        if ((serverSocketFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        if (::bind(serverSocketFd, p->ai_addr, p->ai_addrlen) == -1) {
            cout << "Error binding to socket. " << strerror(errno) << endl;
            close(serverSocketFd);
            continue;
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
        cout << "Waiting for connection..." << endl;
        int clientSocket = accept(serverSocketFd, nullptr, nullptr);
        if (clientSocket == -1) {
            cout << "Error accepting connection." << strerror(errno) << endl;
            return 1;
        }

        cout << "Accepted connection..." << endl;
        char clientBuffer[1024] = {0};
        ssize_t bytesReceived = recv(clientSocket, clientBuffer, sizeof(clientBuffer), 0);
        if (bytesReceived == -1) {
            cout << "Error receiving message from client. " << strerror(errno) << endl;
            return 1;
        }

        cout << "Creating thread..." << endl;
        thread clientThread(process_clientBuffer, clientBuffer, clientSocket);
        clientThread.detach();

        // cout << "Client thread sent res. Waiting for thread client to complete" << clientThread.get_id() << endl;
        // clientThread.join();
        // createChildProcess(clientSocket); // Alternative to threading        
    
    }

    close(serverSocketFd);
    return 0;

}
