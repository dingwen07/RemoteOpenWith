#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void getClientPorts(char* output, int** ports, int* count, int listenPort);
void getServerAddresses(char* output, char*** addresses, int* count, int listenPort);
void removeDuplicateAddresses(char*** addresses, int* count);
void removeDuplicatePorts(int** ports, int* count);
char* executeCommand(const char* command);

void getClientPorts(char* output, int** ports, int* count, int listenPort) {
    char* outputCopy = strdup(output); // Create a copy of output
    char* line;
    char* saveLine;
    line = strtok_s(outputCopy, "\n", &saveLine);

    *ports = (int*)malloc(0);
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

        if (localPort == listenPort) {
            *ports = (int*)realloc(*ports, (*count + 1) * sizeof(int));
            (*ports)[*count] = remotePort;
            (*count)++;
        }
        else if (remotePort == listenPort) {
            *ports = (int*)realloc(*ports, (*count + 1) * sizeof(int));
            (*ports)[*count] = localPort;
            (*count)++;
        }

        line = strtok_s(NULL, "\n", &saveLine);
    }

    // Remove duplicates from the ports array
    removeDuplicatePorts(ports, count);
}


void getServerAddresses(char* output, char*** addresses, int* count, int listenPort) {
    char* outputCopy = strdup(output); // Create a copy of output
    char* line;
    char* saveLine;
    line = strtok_s(outputCopy, "\n", &saveLine);

    *addresses = (char**)malloc(0);
    *count = 0;

    while (line != NULL) {
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
        // printf("Local Port: %d\n", localPort);
        // printf("Remote Port: %d\n", remotePort);

        if (localPort == listenPort) {
            *addresses = (char**)realloc(*addresses, (*count + 1) * sizeof(char*));
            *strrchr(localAddr, ':') = '\0'; // NULL-terminate the address at the colon
            (*addresses)[*count] = strdup(localAddr);
            (*count)++;
        }
        else if (remotePort == listenPort) {
            *addresses = (char**)realloc(*addresses, (*count + 1) * sizeof(char*));
            *strrchr(remoteAddr, ':') = '\0'; // NULL-terminate the address at the colon
            (*addresses)[*count] = strdup(remoteAddr);
            (*count)++;
        }

        line = strtok_s(NULL, "\n", &saveLine);
    }

    // Remove duplicates from the addresses array
    removeDuplicateAddresses(addresses, count);
}


void removeDuplicateAddresses(char*** addresses, int* count) {
    if (*count <= 1) {
        return; // No duplicates to remove
    }

    char** uniqueAddresses = (char**)malloc(*count * sizeof(char*));
    int uniqueCount = 0;

    // Iterate over the addresses and add only unique ones to the new array
    for (int i = 0; i < *count; i++) {
        int isDuplicate = 0;

        // Check if the current address is already in the uniqueAddresses array
        for (int j = 0; j < uniqueCount; j++) {
            if (strcmp((*addresses)[i], uniqueAddresses[j]) == 0) {
                isDuplicate = 1;
                break;
            }
        }

        if (!isDuplicate) {
            uniqueAddresses[uniqueCount] = strdup((*addresses)[i]);
            uniqueCount++;
        }
    }

    // Free the original addresses
    for (int i = 0; i < *count; i++) {
        free((*addresses)[i]);
    }
    free(*addresses);

    // Update the addresses and count with the unique ones
    *addresses = uniqueAddresses;
    *count = uniqueCount;
}



void removeDuplicatePorts(int** ports, int* count) {
    if (*count <= 1) {
        return; // No duplicates to remove
    }

    int* uniquePorts = (int*)malloc(*count * sizeof(int));
    int uniqueCount = 0;

    // Iterate over the ports and add only unique ones to the new array
    for (int i = 0; i < *count; i++) {
        int isDuplicate = 0;

        // Check if the current port is already in the uniquePorts array
        for (int j = 0; j < uniqueCount; j++) {
            if ((*ports)[i] == uniquePorts[j]) {
                isDuplicate = 1;
                break;
            }
        }

        if (!isDuplicate) {
            uniquePorts[uniqueCount] = (*ports)[i];
            uniqueCount++;
        }
    }

    // Free the original ports array
    free(*ports);

    // Update the ports and count with the unique ones
    *ports = uniquePorts;
    *count = uniqueCount;
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

    // Parse TCP ports of the client
    int* clientPorts;
    int clientCount;
    int listenPort = 3389; // Change to the desired listen port
    getClientPorts(output, &clientPorts, &clientCount, listenPort);

    // Print TCP ports of the client
    printf("\nClient TCP Ports:\n");
    for (int i = 0; i < clientCount; i++) {
        printf("%d\n", clientPorts[i]);
    }

    // Parse IP addresses of the server
    char** serverAddresses;
    int serverCount;
    getServerAddresses(output, &serverAddresses, &serverCount, listenPort);

    // Print IP addresses of servers
    printf("\nServer IP Addresses:\n");
    for (int i = 0; i < serverCount; i++) {
        printf("%s\n", serverAddresses[i]);
    }

    // Cleanup
    free(output);
    free(clientPorts);
    for (int i = 0; i < serverCount; i++) {
        free(serverAddresses[i]);
    }
    free(serverAddresses);

    return 0;
}
