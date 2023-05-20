#include <stdlib.h>
#include <string.h>

#include "HashFile.h"

char* hashFile(char* filename) {
    // placeholder
    char* placeholder = "placeholder";
    char* hash = (char*)malloc(strlen(placeholder) + 1);
    strcpy(hash, placeholder);
    hash[strlen(placeholder)] = '\0';
    return hash;
}
