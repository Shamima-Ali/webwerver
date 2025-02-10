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

void sendIndexFile(int clientSocket) {
    cout << "GET index file request" << endl;
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

        // this_thread::sleep_for (std::chrono::seconds(20));
        send(clientSocket, buffer, strlen(buffer), 0);
        delete[] buffer;

        cout << "Closing client socket " << clientSocket << " now. " << endl;
        close(clientSocket);

    }  else {
        cout << "Error opening index file." << endl;
        cout << "Closing client socket " << clientSocket << " now. " << endl;
        close(clientSocket);
    }
}

void ignoreFavIcon(int clientSocket) {
    cout << "Ignoring favicon.ico request." << endl;
    close(clientSocket);
}

void displayNotFound(int clientSocket) {
    const char *buffer = "HTTP/1.1 400 OK\r\n\r\nNot Found";
    send(clientSocket, buffer, strlen(buffer), 0);
    close(clientSocket);
}

void process_clientBuffer(string clientBuffer, int clientSocket) {
    stringstream ss(clientBuffer);
    string word = "";
    string req = "";
    string fileToExec = "hello.alicgi";
    int len = fileToExec.length();

    if (clientBuffer.empty()) {
        cout << "Empty client message" << endl;
        close(clientSocket);
        return;
    }

    while (ss >> word) {
        if (word == "Host:") {
            break;
        }
        req += word + " ";
    }

    string reqPath = req.substr(5, req.length() - 6);
    cout << "Client Requested Path --------> " << reqPath << endl;
 
    if (clientBuffer[5] == ' ' || clientBuffer[5] == '/') {
        sendIndexFile(clientSocket);
        return;
        
    } else if (reqPath.substr(0, 12) == fileToExec) {
        string queryWithVersion = reqPath.substr(13, reqPath.length() - len);
        string versionHttp = "HTTP/1.1";
        int versionLen = versionHttp.length();
        int endOfQuery = queryWithVersion.length() - versionLen;

        string query = queryWithVersion.substr(0, endOfQuery);
        
        cout << "executing hello" << endl;
        const char *envVarName = "QUERY_STRING";
        const char *envFileDesc = "FILE_DESCRIPTOR";


        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();// create child
        if (pid == -1) {
            cout << "Error creating child" << endl;
            close(clientSocket);
            return;
        } else if (pid == 0) {
            cout << "Child process created" << endl;
            char cgiBuff;
            dup2(pipefd[1], STDOUT_FILENO);
            setenv(envVarName, query.c_str(), 1); // set its env
            
            const char *path= "./cgi-bin/hello.alicgi"; // get path of executable
            const char* args[] = {path, nullptr};
        
            execvp(path, const_cast<char* const*>(args)); // execute

            
        } else {
            close(pipefd[1]);
            int status;

            const char* httpHeaders = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
            send(clientSocket, httpHeaders, strlen(httpHeaders), 0);

            char buffer[1024];
            ssize_t bytesRead;
            while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
                send(clientSocket, buffer, bytesRead, 0);
            }

            close(pipefd[0]);
            waitpid(pid, &status, 0);

            if (WIFEXITED(status)) {
                std::cout << "Child process exited with status: " << WEXITSTATUS(status) << "\n";
            } else {
                std::cerr << "Child process did not exit normally.\n" << WEXITSTATUS(status) << "\n";
            }
            close(clientSocket);
        }
    } else if (reqPath == "favicon.ico HTTP/1.1") {
        ignoreFavIcon(clientSocket);
        return;
    }
    else {
        displayNotFound(clientSocket);
        return;
    }

   return;
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
    // Set up socket properties
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

    // Create and bind socket
    int serverSocketFd;
    if ((serverSocketFd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
        perror("socket");
        //continue;
    }

    // use "::" to access theglobal namespace bind
    if (::bind(serverSocketFd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        cout << "Error binding to socket. " << strerror(errno) << endl;
        close(serverSocketFd);
    }


    if (servinfo == NULL) {
        fprintf(stderr, "failed to bind socket\n");
        return 1;
    }

    freeaddrinfo(servinfo); // all done with this structure

    // Listen to socket
    
    if (listen(serverSocketFd, 5) == -1) {
        cout << "Error listening on socket " << strerror(errno) << endl;
        return 1;
    }

    cout << "Server is running on 127.0.0.1:8080" << endl;

    while (true) {
        cout << "Waiting for connection..." << endl;

        // Accept client
        int clientSocket = accept(serverSocketFd, nullptr, nullptr);
        if (clientSocket == -1) {
            cout << "Error accepting connection." << strerror(errno) << endl;
            return 1;
        }

        // Receive message from client
        char tempBuffer[1024] = {0};
        ssize_t bytesReceived = recv(clientSocket, tempBuffer, sizeof(tempBuffer), 0);
        if (bytesReceived == -1) {
            cout << "Error receiving message from client. " << strerror(errno) << endl;
            return 1;
        }

        // Create a string copy of the buffer for the thread
        string clientBuffer(tempBuffer, bytesReceived);

        thread clientThread(process_clientBuffer, clientBuffer, clientSocket);
        clientThread.detach();     
    
    }

    close(serverSocketFd);
    return 0;

}
