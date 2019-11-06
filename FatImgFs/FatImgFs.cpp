// FatImgFs.cpp: 定义应用程序的入口点。
//

#include "FatImgFs.h"

struct Date
{
	int year;
	int month;
	int day;
};

struct Time
{
	int hour;
	int minute;
	int second;
};

struct BootRecord {
	char shortCmd[3];
	char oem[8];
	char bytesPerSection[2];
	char sectionsPerCluster[1];
	char bootSize[2];
	char numFats[1];
	char rootDirMax[2];
	char sectionsCount[2];
	char mediaDescriptorType[1];
	char sectionsPerFat[2];
	char sectionsPerTrack[2];
	char numHeads[2];
	char hiddenSections[4];
	char sectionsCount32[4];
	char driverNum[1];
	char reserved[1];
	char bootSig[1];
	char volId[4];
	char volLab[11];
	char fsType[8];
	int bytsPerSec;
	int secPerClus;
	int bootSecCount;
	int fatsCount;
	int rootfileCount;
	int totalSections;
	int fatSecCount;
};

struct FsInfo {
	int secSize;
	int clusSize;
	int fatFrom;
	int dirFrom;
	int dataFrom;
	int totalSections;
};

struct EntryByte
{
	char name[13];
	char attr[1];
	char reserved[1];
	char creationSecond[1];
	char createTime[2];
	char createDate[2];
	char accessDate[2];
	char highClus[2];
	char modiTime[2];
	char modiDate[2];
	char lowClus[2];
	char size[4];
};

struct Entry {
	char name[13];
	bool readOnly;
	bool hidden;
	bool system;
	bool volumeId;
	bool directory;
	bool archieve;
	bool lfn;
	Time createTime;
	Date createDate;
	Date accessDate;
	Time modiTime;
	Date modiDate;
	int clusterNum;
	int size;
};

struct LFNEntryByte {
	char order[1];
	char first5[10];
	char attr[1];
	char checkSum[1];
	char next6[12];
	char zero[2];
	char final2[4];
};

Time convertTime(char const* time) {
	Time result;
	unsigned short timeNum = *(unsigned short*)time;
	result.hour = (int)(timeNum >> 11);
	result.minute = (int)((timeNum & 0x7ff) >> 5);
	result.second = (int)(timeNum & 0x1f) * 2;
	return result;
}

Date convertDate(char const* date) {
	Date result;
	unsigned short dateNum = *(unsigned short*)date;
	result.year = (int)(dateNum >> 9);
	result.month = (int)((dateNum & 0x1ff) >> 5);
	result.day = (int)(dateNum & 0x1f);
	return result;
}

void completeBoot(BootRecord* record) {
	record->bytsPerSec = (int) * ((short*)record->bytesPerSection);
	record->secPerClus = (int)record->sectionsPerCluster[0];
	record->bootSecCount = (int) * ((short*)record->bootSize);
	record->fatsCount = (int)record->numFats[0];
	record->rootfileCount = (int) * ((short*)record->rootDirMax); 
	record->totalSections = (int) * ((short*)record->sectionsCount);
	record->fatSecCount = (int) * ((short*)record->sectionsPerFat);
}

bool isLFN(EntryByte const* entry) {
	char READ_ONLY = 0x01;
	char HIDDEN = 0x02;
	char SYSTEM = 0x04;
	char VOLUMN_ID = 0x08;
	return *(entry->attr) & (READ_ONLY | HIDDEN | SYSTEM | VOLUMN_ID);
}

Entry buildEntry(EntryByte const* entry) {
	Entry result;
	for (int i = 0; i < 13; i++) {
		result.name[i] = entry->name[i];
	}
	char READ_ONLY = 0x01;
	char HIDDEN = 0x02;
	char SYSTEM = 0x04;
	char VOLUMN_ID = 0x08;
	char DIRECTORY = 0x10;
	char ARCHIEVE = 0x20;
	result.readOnly = *(entry->attr) & READ_ONLY;
	result.hidden = *(entry->attr) & HIDDEN;
	result.system = *(entry->attr) & SYSTEM;
	result.volumeId = *(entry->attr) & VOLUMN_ID;
	result.directory = *(entry->attr) & DIRECTORY;
	result.archieve = *(entry->attr) & ARCHIEVE;
	result.createTime = convertTime(entry->createTime);
	result.createDate = convertDate(entry->createDate);
	result.accessDate = convertDate(entry->accessDate);
	result.modiTime = convertTime(entry->modiTime);
	result.modiDate = convertDate(entry->modiDate);
	result.clusterNum = (int) * (short*)(entry->lowClus);
	result.size = *(int*)(entry->size);
}

FsInfo computeBaseInfo(BootRecord const* record) {
	FsInfo result;
	result.secSize = record->bytsPerSec;
	result.clusSize = record->secPerClus * result.secSize;
	result.fatFrom = record->bootSecCount * result.secSize;
	result.dirFrom = result.fatFrom + record->fatSecCount * result.secSize * record->fatsCount;
	result.dataFrom = result.dirFrom + record->rootfileCount * 32;
	return result;
}

inline int getDateStartAddress(int clusNum, FsInfo info) {
	return info.dataFrom + (clusNum - 2) * info.clusSize;
}

int getNext(int currentClus, std::ifstream input, FsInfo info) {
	int pos = info.fatFrom + currentClus / 2 * 3;
	input.seekg(pos);
	if (currentClus % 2 == 0){
		int result = ((unsigned int)input.get()) << 4;
		result += ((unsigned int)input.get()) >> 4;
		return result;
	}
	else {
		input.get();
		int result = ((unsigned int)(input.get() & 0xf)) << 8;
		result += (unsigned int)input.get();
	}
}

int main()
{
	std::string filename = "D:\\nice-\\Code\\FatImgFs\\a.img";
	//std::string filename = "a.img";
	std::ifstream inputFile(filename, std::ios::binary);
	BootRecord record;
	char* bootp = record.shortCmd;
	for (int i = 0; i < 62; i++) {
		*bootp = inputFile.get();
		bootp++;
	}
	completeBoot(&record);
	FsInfo fsInfo = computeBaseInfo(&record);
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
					while (*currentChar != 0 && *currentChar != ' ') {
						currentChar++;
					}
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
