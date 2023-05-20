void getConnections(char* output, int** ports, char*** clientAddresses, char*** serverAddresses, int* count, int listenPort);
void freeAddresses(char** addresses, int count);
char* executeCommand(const char* command);
char* getTCPNetStat();
