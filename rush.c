#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    // buffer for cmd input
    char* cmd = NULL;
    char* token = NULL;
    size_t cmdSize = 0;

    // calling rush with arguments causes an error
    if (argc > 1) {
        printf("An error has occurred\n");
        return 1;
    }

    while (1) {
        // prompt user for input and get that input and store in buffer
        printf("rush> ");
        if (getline(&cmd, &cmdSize, stdin) == -1) {
            // Handle error or EOF
            break;
        }

        // Tokenize the command
        token = strtok(cmd, " \n\t");
        while (token != NULL) {
            printf("%s\n", token);
            token = strtok(NULL, " \n\t");
        }

        // check if command is built in first
        if (strcmp(cmd, "exit") == 0) {
            free(cmd); // Free the dynamically allocated memory
            exit(0);
        }
    }

    free(cmd); // Free the dynamically allocated memory

    return 0;
}
