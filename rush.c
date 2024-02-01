#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

// print error message to user
void printError() {
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
    fflush(stdout);
}

// return the index of redirection symbol in arg list
int findRedirectionIndex(char** args, int argCount) {
	for(int i = 0; i < argCount; i++) {
		if(strcmp(args[i], ">") == 0) {
			return i;
		}
	}
	return 0;
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
							int redirIndex = findRedirectionIndex(arguments, argCount);
							// check that redirection symbol is inputted
							if(redirIndex > 0) {
								// ensure that only 1 argument after is inputted
								if(argCount - (redirIndex + 1) != 1) {
									printError();
								} else {
									int fd = open(arguments[redirIndex+1], O_WRONLY | O_CREAT | O_TRUNC, 0644); // Open or create the output file
									dup2(fd, STDOUT_FILENO); // Redirect stdout to the file
        							close(fd); // Close the file descriptor
									arguments[redirIndex] = NULL;
									arguments[redirIndex+1] = NULL;
								}
							}
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

		for(int i = 0; i < argCount; i++) {
			arguments[i] = NULL;
		}
    }

    return 0;
}
