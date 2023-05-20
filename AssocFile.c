#pragma comment(lib, "advapi32.lib")

#include <windows.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <ProgID base> <extension> <binary path>\n", argv[0]);
        return 1;
    }

    const char* progIdBase = argv[1];
    const char* extension = argv[2];
    const char* binaryPath = argv[3];

    // Check if the binary exists
    DWORD attributes = GetFileAttributes(binaryPath);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "The binary does not exist: %s\n", binaryPath);
        return 1;
    }

    // Extract the basename of the binary path
    const char* binaryBaseName = strrchr(binaryPath, '\\');
    if (binaryBaseName == NULL) {
        binaryBaseName = strrchr(binaryPath, '/');
    }
    if (binaryBaseName == NULL) {
        binaryBaseName = binaryPath;
    }
    else {
        binaryBaseName++;  // Move past the slash character
    }

    // Create the ProgID
    char progId[256];
    snprintf(progId, sizeof(progId), "%s.%s", progIdBase, extension);

    HKEY key;

    // Create the HKEY_CURRENT_USER\Software\Classes\<ProgID>\shell\open\command key
    char progIdKeyPath[256];
    snprintf(progIdKeyPath, sizeof(progIdKeyPath), "Software\\Classes\\%s\\shell\\open\\command", progId);

    if (RegCreateKey(HKEY_CURRENT_USER, progIdKeyPath, &key) == ERROR_SUCCESS) {
        // Set the default value of the command key to the path of the binary
        char value[256];
        snprintf(value, sizeof(value), "\"%s\" \"%%1\"", binaryPath);

        if (RegSetValueEx(key, "", 0, REG_SZ, (BYTE*)value, strlen(value) + 1) != ERROR_SUCCESS) {
            fprintf(stderr, "Failed to set the default value of the ProgID command key\n");
        }

        RegCloseKey(key);
    }
    else {
        fprintf(stderr, "Failed to create the ProgID command key\n");
        return 1;
    }

    // Create the HKEY_CURRENT_USER\Software\Classes\Applications\<binaryBaseName>\shell\open\command key
    char appsKeyPath[256];
    snprintf(appsKeyPath, sizeof(appsKeyPath), "Software\\Classes\\Applications\\%s\\shell\\open\\command", binaryBaseName);

    if (RegCreateKey(HKEY_CURRENT_USER, appsKeyPath, &key) == ERROR_SUCCESS) {
        // Set the default value of the command key to the path of the binary
        char value[256];
        snprintf(value, sizeof(value), "\"%s\" \"%%1\"", binaryPath);

        if (RegSetValueEx(key, "", 0, REG_SZ, (BYTE*)value, strlen(value) + 1) != ERROR_SUCCESS) {
            fprintf(stderr, "Failed to set the default value of the Applications command key\n");
        }

        RegCloseKey(key);
    }
    else {
        fprintf(stderr, "Failed to create the Applications command key\n");
        return 1;
    }
    
    // Create the HKEY_CURRENT_USER\Software\Classes\.<extension>\OpenWithProgids key
    char openWithProgidsKeyPath[256];
    snprintf(openWithProgidsKeyPath, sizeof(openWithProgidsKeyPath), "Software\\Classes\\.%s\\OpenWithProgids", extension);

    if (RegCreateKey(HKEY_CURRENT_USER, openWithProgidsKeyPath, &key) == ERROR_SUCCESS) {
        // Add the ProgID to the OpenWithProgids key
        if (RegSetValueEx(key, progId, 0, REG_NONE, NULL, 0) != ERROR_SUCCESS) {
            fprintf(stderr, "Failed to add the ProgID to the OpenWithProgids key\n");
        }

        RegCloseKey(key);
    }
    else {
        fprintf(stderr, "Failed to create the OpenWithProgids key\n");
        return 1;
    }

    // Create the HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.extension\OpenWithProgids key
    // char openWithProgidsKeyPath[256];
    snprintf(openWithProgidsKeyPath, sizeof(openWithProgidsKeyPath), "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.%s\\OpenWithProgids", extension);

    if (RegCreateKey(HKEY_CURRENT_USER, openWithProgidsKeyPath, &key) == ERROR_SUCCESS) {
        // Add the ProgID to the OpenWithProgids key
        if (RegSetValueEx(key, progId, 0, REG_NONE, NULL, 0) != ERROR_SUCCESS) {
            fprintf(stderr, "Failed to add the ProgID to the OpenWithProgids key\n");
        }

        RegCloseKey(key);
    }
    else {
        fprintf(stderr, "Failed to create the OpenWithProgids key\n");
        return 1;
    }

    return 0;
}
