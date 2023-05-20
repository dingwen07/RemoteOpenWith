#pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "GetTCPConnections.h"
#include "HashFile.h"

#define SERVER_PORT 2335
#define MAX_BUFFER_LEN 4096
#define NUM_PORTS 10
#define MAX_REQUEST_SIZE 1024
#define MAX_FILE_SIZE 1048576 // 1MB

void clinetCommunication(char* addr, char* filename, char* hash, int* clientPorts, char* requestID);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    char* filename = argv[1];
    // check if file exists
    DWORD attributes = GetFileAttributes(filename);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "The file does not exist: %s\n", filename);
        return 1;
    }

    // compute hash
    char* hash = hashFile(filename);

    // get connections
    int listenPort = 3389;
    char* output = getTCPNetStat();
    int* clientPorts;
    char** clientAddresses;
    char** serverAddresses;
    int count;
    getConnections(output, &clientPorts, &clientAddresses, &serverAddresses, &count, listenPort);

    // for each client address
    for (int i = 0; i < count; i++) {
        // (client:port) -> (server:port)
        printf("%s:%d -> %s:%d\n", clientAddresses[i], clientPorts[i], serverAddresses[i], listenPort);
        // send hash to client
        clinetCommunication(clientAddresses[i], filename, hash, clientPorts, "1234567890");
    }

    return 0;
}

void clinetCommunication(char* addr, char* filename, char* hash, int* clientPorts, char* requestID) {
    char* sendData = NULL;
    char* receiveData = NULL;
    int iResult;
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL, *ptr = NULL, hints;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    // convert SERVER_PORT to string
    char serverPortStr[10];
    sprintf(serverPortStr, "%d", SERVER_PORT);
    if (getaddrinfo(addr, serverPortStr, &hints, &result) != 0) {
        printf("getaddrinfo failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return;
        }

        // Connect to server.
        if (connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return;
    }

    // At this point, the connection is established
    printf("Connected to the server.\n");


    // Step 1: Handshake
    // Wait for "RemoteOpenWith Server 0.1" from server
    receiveData = (char*)malloc(MAX_BUFFER_LEN);  // allocate buffer
    memset(receiveData, 0, MAX_BUFFER_LEN);
    iResult = recv(ConnectSocket, receiveData, MAX_BUFFER_LEN, 0);
    if (iResult <= 0) {
        printf("recv failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }
    if (strcmp(receiveData, "RemoteOpenWith Server 0.1\r\n") != 0) {
        printf("Server handshake failed: %s\n", receiveData);
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }
    printf("Received handshake from server.\n");

    // Send "RemoteOpenWith 0.1" to server
    sendData = "RemoteOpenWith 0.1\r\n";
    iResult = send(ConnectSocket, sendData, strlen(sendData), 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }
    printf("Sent handshake to server.\n");

    // Step 2: Verify
    // Wait for "VERIFY" from server
    memset(receiveData, 0, MAX_BUFFER_LEN);
    iResult = recv(ConnectSocket, receiveData, MAX_BUFFER_LEN, 0);
    if (iResult <= 0) {
        printf("recv failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }
    if (strcmp(receiveData, "VERIFY\r\n") != 0) {
        printf("Server verification request failed: %s\n", receiveData);
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }
    printf("Received verification request from server.\n");

    // Send ports to server
    char ports[100] = {0};
    for(int i = 0; i < NUM_PORTS; i++) {
        char portStr[10];
        sprintf(portStr, "%d,", clientPorts[i]);
        strcat(ports, portStr);
    }
    ports[strlen(ports)-1] = '\0';  // remove trailing comma
    strcat(ports, "\r\n");  // add TELNET EOL
    iResult = send(ConnectSocket, ports, strlen(ports), 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }
    printf("Sent ports to server for verification.\n");

    // Wait for "OK" from server
    memset(receiveData, 0, MAX_BUFFER_LEN);
    iResult = recv(ConnectSocket, receiveData, MAX_BUFFER_LEN, 0);
    if (iResult <= 0) {
        printf("recv failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }
    if (strcmp(receiveData, "OK\r\n") != 0) {
        printf("Server verification failed: %s\n", receiveData);
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }
    printf("Server verification succeeded.\n");

    // Step 3: Send file
    // Open file
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Unable to open file: %s\n", filename);
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    rewind(fp);

    // Get file basename
    // Extract the basename of the binary path
    const char* fileBaseName = strrchr(filename, '\\');
    if (fileBaseName == NULL) {
        fileBaseName = strrchr(filename, '/');
    }
    if (fileBaseName == NULL) {
        fileBaseName = filename;
    }
    else {
        fileBaseName++;  // Move past the slash character
    }

    // Send Request ID, filename, fileSize and fileHash to server
    char fileDetails[512];
    sprintf(fileDetails, "%s:%s:%ld:%s\r\n", requestID, fileBaseName, fileSize, hash);
    iResult = send(ConnectSocket, fileDetails, strlen(fileDetails), 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        fclose(fp);
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }
    printf("Sent file details to server.\n");

    // Wait for "OK" from server
    memset(receiveData, 0, MAX_BUFFER_LEN);
    iResult = recv(ConnectSocket, receiveData, MAX_BUFFER_LEN, 0);
    if (iResult <= 0) {
        printf("recv failed with error: %d\n", WSAGetLastError());
        fclose(fp);
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }
    if (strcmp(receiveData, "OK\r\n") != 0) {
        printf("Server refused file details: %s\n", receiveData);
        fclose(fp);
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }
    printf("Server accepted file details.\n");

    // Send file data to server
    char fileData[MAX_BUFFER_LEN];
    for (long i = 0; i < fileSize; i += MAX_BUFFER_LEN) {
        size_t dataSize = fread(fileData, 1, MAX_BUFFER_LEN, fp);
        iResult = send(ConnectSocket, fileData, dataSize, 0);
        if (iResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            fclose(fp);
            closesocket(ConnectSocket);
            WSACleanup();
            return;
        }
    }
    printf("Sent file data to server.\n");
    fclose(fp);

    // Wait for "OK" from server
    memset(receiveData, 0, MAX_BUFFER_LEN);
    iResult = recv(ConnectSocket, receiveData, MAX_BUFFER_LEN, 0);
    if (iResult <= 0) {
        printf("recv failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }
    if (strcmp(receiveData, "OK\r\n") != 0) {
        printf("Server refused file data: %s\n", receiveData);
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }
    printf("Server accepted file data.\n");

    // Step 4: Exchange File Update Server Port
    // Send "0" to server
    sendData = "0\r\n";
    iResult = send(ConnectSocket, sendData, strlen(sendData), 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }
    printf("Sent File Update Server Port to server.\n");


    // Close connection and cleanup
    free(receiveData);
    closesocket(ConnectSocket);
    WSACleanup();
}
