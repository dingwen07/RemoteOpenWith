#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GetTCPConnections.h"

char* executeCommand(const char* command);


void getConnections(char* output, int** ports, char*** clientAddresses, char*** serverAddresses, int* count, int listenPort) {
    char* outputCopy = strdup(output); // Create a copy of output
    char* line;
    char* saveLine;
    line = strtok_s(outputCopy, "\n", &saveLine);

    *ports = (int*)malloc(0);
    *clientAddresses = (char**)malloc(0);
    *serverAddresses = (char**)malloc(0);
    *count = 0;

    while (line) {
        char protocol[10];
        char localAddr[80];
        char remoteAddr[80];
        char status[20];
        int pid;

        sscanf(line, "%s %[^ ] %[^ ] %s %d", protocol, localAddr, remoteAddr, status, &pid);

        char* localPortStr = strrchr(localAddr, ':') + 1;
        char* remotePortStr = strrchr(remoteAddr, ':') + 1;

        int localPort = atoi(localPortStr);
        int remotePort = atoi(remotePortStr);

        int found = 0;
        int peerPort = -1;
        char* clientAddress = NULL;
        char* serverAddress = NULL;

        if (localPort == listenPort) {
            peerPort = remotePort;
            clientAddress = remoteAddr;
            serverAddress = localAddr;

            found = 1;
        }
        else if (remotePort == listenPort) {
            peerPort = localPort;
            clientAddress = localAddr;
            serverAddress = remoteAddr;
            
            found = 1;
        }

        if (found) {
            // printf("Found Connection: %s -> %s\n", clientAddress, serverAddress);
            // Add port to the ports array
            *ports = (int*)realloc(*ports, (*count + 1) * sizeof(int));
            (*ports)[*count] = localPort;

            // Add client address to the clientAddresses array
            *clientAddresses = (char**)realloc(*clientAddresses, (*count + 1) * sizeof(char*));
            *strrchr(clientAddress, ':') = '\0'; // NULL-terminate the address at the colon
            (*clientAddresses)[*count] = strdup(clientAddress);

            // Add server address to the serverAddresses array
            *serverAddresses = (char**)realloc(*serverAddresses, (*count + 1) * sizeof(char*));
            *strrchr(serverAddress, ':') = '\0'; // NULL-terminate the address at the colon
            (*serverAddresses)[*count] = strdup(serverAddress);

            (*count)++;
        }

        line = strtok_s(NULL, "\n", &saveLine);
    }
}


void freeAddresses(char** addresses, int count) {
    for (int i = 0; i < count; i++) {
        free(addresses[i]);
    }
    free(addresses);
}


char* executeCommand(const char* command) {
    FILE* fp;
    char buffer[128];
    char* output = NULL;
    size_t outputSize = 0;

    fp = _popen(command, "r");
    if (fp == NULL) {
        perror("Failed to execute command");
        return NULL;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        size_t bufferLen = strlen(buffer);
        output = realloc(output, outputSize + bufferLen + 1);
        if (output == NULL) {
            perror("Failed to allocate memory");
            break;
        }
        strcpy(output + outputSize, buffer);
        outputSize += bufferLen;
    }

    _pclose(fp);

    return output;
}


int main() {
    // Execute the command and capture the output
    char* command = "netstat -ano | findstr TCP | findstr ESTABLISHED";
    char* output = executeCommand(command);

    // Print the raw output for debugging
    printf("Command Output:\n%s\n", output);

    // get connections
    int* clientPorts;
    char** clientAddresses;
    char** serverAddresses;
    int count;
    int listenPort = 443; // Change to the desired listen port
    getConnections(output, &clientPorts, &clientAddresses, &serverAddresses, &count, listenPort);

    // Print connection info
    printf("Connections:\n");
    for (int i = 0; i < count; i++) {
        // (client:port) -> (server:port)
        printf("%s:%d -> %s:%d\n", clientAddresses[i], clientPorts[i], serverAddresses[i], listenPort);
    }

    // Cleanup
    free(output);
    free(clientPorts);
    // freeAddresses(clientAddresses, count);
    // freeAddresses(serverAddresses, count);

    return 0;
}
