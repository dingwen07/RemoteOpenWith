#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GetTCPConnections.h"

#define DEFAULT_PORT 443


int main(int argc, char* argv[]) {
    // Get the port to listen on from the command line
    int listenPort = DEFAULT_PORT;
    if (argc > 1) {
        listenPort = atoi(argv[1]);
    }

    // Execute the command and capture the output
    char* command = "netstat -ano | findstr TCP | findstr ESTABLISHED";
    char* output = executeCommand(command);

    // Print the raw output for debugging
    printf("All TCP Connections:\n%s\n", output);

    // get connections
    int* clientPorts;
    char** clientAddresses;
    char** serverAddresses;
    int count;
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
    freeAddresses(clientAddresses, count);
    freeAddresses(serverAddresses, count);

    return 0;
}
