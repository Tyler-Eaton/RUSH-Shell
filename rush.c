#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void printError() {
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
    fflush(stdout);
}

int main(int argc, char** argv) {


    // buffer for cmd input
    char* cmd = NULL;
    char* token = NULL;
    size_t cmdSize = 0;
	size_t tokenCount = 0;
	char** arguments = NULL;

    // calling rush with arguments causes an error
    if (argc > 1) {
		printError();
		exit(1);
    }

    while (1) {
        // prompt user for input and get that input and store in buffer
        printf("rush> ");
        if (getline(&cmd, &cmdSize, stdin) == -1) {
			printError();
			exit(1);
        }

		// tokenize the input
        char* temp = cmd;
		tokenCount = 0;
        while ((token = strsep(&temp, " \n\t")) != NULL) {
            if (strlen(token) > 0) {
				tokenCount++;
                arguments = (char**)realloc(arguments, tokenCount * sizeof(char*));
                arguments[tokenCount - 1] = strdup(token);
            }
        }

		if(tokenCount > 0) {
			// check if command is built in first
			if (strcmp(arguments[0], "exit") == 0) {
				if(tokenCount > 1) {
					printError();
				} else {
					exit(0);
				}
			}
		}
    }

    return 0;
}
