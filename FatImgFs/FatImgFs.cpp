// FatImgFs.cpp: 定义应用程序的入口点。
//

#include "FatImgFs.h"


int main()
{
	while (true) {
		char commandLine[100];
		std::cin.getline(commandLine, 100);
		char command[20];
		char flags[20];
		char arg[50];

		char* currentChar = commandLine;
		char* cmdp = command;
		char* flagp = flags;
		char* argp = arg;
		while (*currentChar != 0 && *currentChar != ' ') {
			*cmdp = *currentChar;
			cmdp++;
			currentChar++;
		}
		while (*currentChar != 0) {
			while (*currentChar == ' ') {
				currentChar++;
			}
			if (*currentChar == '-') {
				currentChar++;
				while (*currentChar != 0 && *currentChar != ' ') {
					*flagp = *currentChar;
					flagp++;
					currentChar++;
				}
			}
			else {
				while (*currentChar != 0 && *currentChar != ' ') {
					*argp = *currentChar;
					argp++;
					currentChar++;
				}
			}
		}
	}
	return 0;
}
