#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define cmdLen 256

int main(int argc, char** argv) {
	// buffer for cmd input
	char* cmd = (char *)malloc(cmdLen * sizeof(char));
	size_t cmdSize = cmdLen;
	if(cmd == NULL) {
		perror("Unable to allocate space for command input.");
		return 1;
	}

	while(1) {
		printf("rush> ");
		getline(&cmd, &cmdSize, stdin);
	}

	return 0;
}