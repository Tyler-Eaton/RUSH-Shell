#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void printError() {
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
    fflush(stdout);
}

int main(int argc, char** argv) {

    // buffer for cmd input
    char* cmd = NULL;
    char* arg = NULL;
    size_t cmdSize = 0;
	size_t argCount = 0;
	size_t pathCount = 1;
	char** arguments = NULL;
	// allocate space for path and set initial path to /bin
	char** path = (char**)malloc(sizeof(char*));
	path[0] = "/bin";

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

		// parse input command and store into array of strings
        char* temp = cmd;
		argCount = 0;
        while ((arg = strsep(&temp, " \n\t")) != NULL) {
            if (strlen(arg) > 0) {
				argCount++;
                arguments = (char**)realloc(arguments, argCount * sizeof(char*));
                arguments[argCount - 1] = strdup(arg);
            }
        }

		// ensure that arguments were passed
		if(argCount > 0) {
			// check if command is built in first
			if (strcmp(arguments[0], "exit") == 0) {
				if(argCount > 1) {
					printError();
				} else {
					exit(0);
				}
			} else if (strcmp(arguments[0], "cd") == 0) {
				if(argCount != 2) {
					printError();
				} else {
					int dir = chdir(arguments[1]);
					if(dir == -1) {
						printError();
					}
				}
			} else if (strcmp(arguments[0], "path") == 0) {
				path = (char**)realloc(path, (argCount - 1) * sizeof(char*));
				for(int i = 1; i < argCount; i++) {
					path[i-1] = arguments[i];
				}
				pathCount = argCount - 1;
			} else {
				int flag = 0;
				for(int i = 0; i < pathCount; i++) {
					char* res = strdup(path[i]);
					strcat(res, "/");
					strcat(res, arguments[0]);
					int canAccess = access(res, X_OK);
					if(canAccess == 0) {
						flag = 1;
						pid_t pid = fork();
						// check if fork failed
						if (pid == -1) {
							printError();
							break;
						}
						// child process exec command
						else if(pid == 0) {
							execv(res, arguments);
						}
						// parent should wait for child to finish and break out of loop
						else {
							int status;
							waitpid(pid, &status, 0);
							break;
						}
					}
				}
				if(flag == 0) {
					printError();
				}
			}
		}
    }

    return 0;
}
