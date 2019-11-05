// FatImgFs.cpp: 定义应用程序的入口点。
//

#include "FatImgFs.h"


int main()
{
	while (true) {
		char commandLine[100];
		std::cin.getline(commandLine, 100);
		char cmd[20] = {0};
		char flags[20] = {0};
		char arg[50] = {0};

		char* currentChar = commandLine;
		char* cmdp = cmd;
		char* flagp = flags;
		char* argp = arg;
		bool tooManyArgs = false;
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
				if (*currentChar != 0 && arg[0] == 0){
					while (*currentChar != 0 && *currentChar != ' ') {
						*argp = *currentChar;
						argp++;
						currentChar++;
					}
				}
				else if (*currentChar != 0) {
					tooManyArgs = true;
				}
			}
		}
		
		if (cmd[0] == 'e' && cmd[1] == 'x' && cmd[2] == 'i' && cmd[3] == 't' && cmd[4] == 0) {
			if (arg[0] != 0){
				std::cout << "exit command need no args" << std::endl;
				std::cout << "command: exit" << std::endl;
				continue;
			}
			else if (flags[0] != 0){
				std::cout << "exit command cannot recognize flag" << std::endl;
				continue;
			}
			else{
				break;
			}
		}
		else if (cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 't' && cmd[3] == 0) {
			if (arg[0] == 0){
				std::cout << "cat command need file path" << std::endl;
				std::cout << "command: cat [file]" << std::endl;
				continue;
			}
			else if (tooManyArgs){
				std::cout << "cat command need exactly one file path" << std::endl;
				std::cout << "command: cat [file]" << std::endl;
				continue;
			}
			else if (flags[0] != 0){
				std::cout << "cat command cannot recognize flag" << std::endl;
				continue;
			}
			else{
				
			}
		}
		else if (cmd[0] == 'l' && cmd[1] == 's' && cmd[2] == 0){
			if (tooManyArgs){
				std::cout << "ls command need no more than one file path" << std::endl;
				std::cout << "command: ls [file] [options]" << std::endl;
				continue;
			}
			bool detailed = false;
			bool flagErr = false;
			char* currentFlag = flags;
			while (*currentFlag != 0){
				if (*currentFlag == 'l'){
					detailed = true;
				}
				else {
					flagErr = true;
					break;
				}
				currentFlag++;
			}
			if (flagErr){
				std::cout << "ls command cannot recognize flag" << std::endl;
				continue;
			}
			if (arg[0] == 0){
				arg[0] == '/';
			}
		}
	}
	return 0;
}
