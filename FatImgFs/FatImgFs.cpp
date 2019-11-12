// FatImgFs.cpp: 定义应用程序的入口点。
//

#include "FatImgFs.h"
#include "print.h"

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
	char name[11];
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
	char name[11];
	bool readOnly;
	bool hidden;
	bool system;
	bool volumeId;
	bool directory;
	bool archieve;
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
	char entryType[1];
	char checkSum[1];
	char next6[12];
	char zero[2];
	char final2[4];
};

struct Node {
	char name[53] = { 0 };
	Entry entry;
	Node* next;
	Node* child;
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
	for (int i = 0; i < 11; i++) {
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
	return result;
}

void getLFName(LFNEntryByte const* entry, char* name) {
	for (int i = 0; i < 5; i++) {
		*name = entry->first5[i * 2];
		name++;
	}
	for (int i = 0; i < 6; i++) {
		*name = entry->next6[i * 2];
		name++;
	}
	for (int i = 0; i < 2; i++) {
		*name = entry->final2[i * 2];
		name++;
	}
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

inline int getDataStartAddress(int clusNum, FsInfo info) {
	return info.dataFrom + (clusNum - 2) * info.clusSize;
}

int getNext(int currentClus, std::ifstream &input, FsInfo info) {
	int pos = info.fatFrom + currentClus / 2 * 3;
	input.seekg(pos);
	if (currentClus % 2 == 0){
		int result = ((unsigned int)input.get());
		result += ((unsigned int)(input.get() & 0xf)) << 8;
		return result;
	}
	else {
		input.get();
		int result = ((unsigned int)input.get()) >> 4;
		result += ((unsigned int)input.get()) << 4;
		return result;
	}
}

Node* getDirStructure(std::ifstream& input, FsInfo info, int clus) {
	Node* first = nullptr;
	char name[53] = { 0 };
	char* lfnNameSave = &name[52];
	Node* latestNode = nullptr;
	int currentClus = clus;
	while (currentClus < 0xFF7) {
		input.seekg(getDataStartAddress(currentClus, info));
		for (int i = 0; i < info.clusSize / sizeof(EntryByte); i++) {
			EntryByte entry;
			char* currentRead = (char*)&entry;
			for (int i = 0; i < sizeof(EntryByte); i++) {
				*currentRead = input.get();
				currentRead++;
			}
			if (entry.name[0] == 15 || entry.name[0] == 0) {
				continue;
			}
			if (isLFN(&entry)) {
				lfnNameSave -= 13;
				getLFName((LFNEntryByte*)&entry, lfnNameSave);
			}
			else {
				Entry normalEntry = buildEntry(&entry);
				Node* node = new Node;
				node->entry = normalEntry;
				if (latestNode != nullptr) {
					latestNode->next = node;
					latestNode = node;
				}
				else {
					first = node;
					latestNode = node;
				}
				node->next = nullptr;
				if (lfnNameSave == &name[52]) {
					char* namep = node->name;
					char* nameEntryp = entry.name;
					if (!node->entry.directory) {
						for (int i = 0; i < 8 && *nameEntryp != 0 && *nameEntryp != 0x20; i++) {
							*namep = *nameEntryp;
							namep++;
							nameEntryp++;
						}
						*namep = '.';
						namep++;
						nameEntryp = &entry.name[8];
						for (int i = 0; i < 3 && *nameEntryp != 0 && *nameEntryp != 0x20; i++) {
							*namep = *nameEntryp;
							namep++;
							nameEntryp++;
						}
					}
					else {
						for (int i = 0; i < 11 && *nameEntryp != 0 && *nameEntryp != 0x20; i++) {
							*namep = *nameEntryp;
							namep++;
							nameEntryp++;
						}
					}
				}
				else {
					char* namep = node->name;
					while (*lfnNameSave != 0) {
						*namep = *lfnNameSave;
						namep++;
						lfnNameSave++;
					}
					lfnNameSave = &name[52];
				}
			}
		}
		currentClus = getNext(currentClus, input, info);
	}
	Node* current = first;
	if (current != nullptr) {
		current->child = first;
		current = current->next;
	}
	if (current != nullptr) {
		current = current->next;
	}
	while (current != nullptr) {
		if (current->entry.directory) {
			current->child = getDirStructure(input, info, current->entry.clusterNum);
			if (current->child != nullptr && current->child->next != nullptr) {
				current->child->next->child = first;
			}
		}
		else {
			current->child = nullptr;
		}
		current = current->next;
	}
	return first;
}

Node* getDirStructure(std::ifstream &input, FsInfo info) {
	input.seekg(info.dirFrom);
	Node* first = nullptr;
	char name[53] = { 0 };
	char* lfnNameSave = &name[52];
	Node* latestNode = nullptr;
	while(input.tellg() < info.dataFrom) {
		EntryByte entry;
		char* currentRead = (char*)&entry;
		for (int i = 0; i < sizeof(EntryByte); i++) {
			*currentRead = input.get();
			currentRead++;
		}
		if (entry.name[0] == 15) {
			continue;
		}
		if (isLFN(&entry)) {
			lfnNameSave -= 13;
			getLFName((LFNEntryByte*)&entry, lfnNameSave);
		}
		else {
			if (entry.lowClus[0] == 0 && entry.lowClus[1] == 0) {
				continue;
			}
			Entry normalEntry = buildEntry(&entry);
			Node* node = new Node;
			node->entry = normalEntry;
			if (latestNode != nullptr) {
				latestNode->next = node;
				latestNode = node;
			}
			else {
				first = node;
				latestNode = node;
			}
			node->next = nullptr;
			if (lfnNameSave == &name[52]) {
				char* namep = node->name;
				char* nameEntryp = entry.name;
				if (!node->entry.directory) {
					for (int i = 0; i < 8 && *nameEntryp != 0 && *nameEntryp != 0x20; i++) {
						*namep = *nameEntryp;
						namep++;
						nameEntryp++;
					}
					*namep = '.';
					namep++;
					nameEntryp = &entry.name[8];
					for (int i = 0; i < 3 && *nameEntryp != 0 && *nameEntryp != 0x20; i++) {
						*namep = *nameEntryp;
						namep++;
						nameEntryp++;
					}
				}
				else {
					for (int i = 0; i < 11 && *nameEntryp != 0 && *nameEntryp != 0x20; i++) {
						*namep = *nameEntryp;
						namep++;
						nameEntryp++;
					}
				}
			}
			else {
				char* namep = node->name;
				while (*lfnNameSave != 0) {
					*namep = *lfnNameSave;
					namep++;
					lfnNameSave++;
				}
				lfnNameSave = &name[52];
			}
		}
	}
	Node* current = first;
	while (current != nullptr) {
		if (current->entry.directory) {
			current->child = getDirStructure(input, info, current->entry.clusterNum);
			if (current->child != nullptr && current->child->next != nullptr) {
				current->child->next->child = first;
			}
		}
		else {
			current->child = nullptr;
		}
		current = current->next;
	}
	return first;
}

Node* resolve(char* path, Node* root, bool isDir) {
	char* pathp = path;
	Node* dir = root;
	if (*pathp == 0 || path[1] == 0) {
		if (isDir) {
			return root;
		}
		else {
			return nullptr;
		}
	}
	pathp++;
	while (true) {
		char fileName[53] = { 0 };
		char* cpp = &fileName[0];
		bool end = false;
		while (*pathp != 0 && *pathp != '/') {
			*cpp = *pathp;
			cpp++;
			pathp++;
		}
		if (*pathp == 0) {
			end = true;
		}
		else {
			pathp++;
			if (*pathp == 0) {
				end = true;
			}
		}
		while (dir != nullptr) {
			cpp = &fileName[0];
			char* dirName = dir->name;
			while (*dirName == *cpp && *cpp != 0) {
				dirName++;
				cpp++;
			}
			if (*dirName == 0 && *cpp == 0) {
				if (dir->entry.directory && !end) {
					dir = dir->child;
					break;
				}
				else if (!dir->entry.directory && end && !isDir) {
					return dir;
				}
				else if (dir->entry.directory && end && isDir) {
					return dir->child;
				}
			}
			dir = dir->next;
		}
		if (dir == nullptr) {
			return nullptr;
		}
	}
}

int dirCount(Node* node) {
	Node* currentNode = node;
	int result = 0;
	while (currentNode != nullptr){
		if (currentNode->entry.directory && !(currentNode->name[0] == '.' && currentNode->name[1] == 0) 
			&& !(currentNode->name[0] == '.' && currentNode->name[1] == '.' && currentNode->name[2] == 0))
			result++;
		currentNode = currentNode->next;
	}
	return result;
}

int fileCount(Node* node) {
	Node* currentNode = node;
	int result = 0;
	while (currentNode != nullptr) {
		if (!currentNode->entry.directory)
			result++;
		currentNode = currentNode->next;
	}
	return result;
}

void printDir(Node* node, char* related,bool detailed) {
	myPrint(related);
	if (detailed) {
		myPrint(" ");
		char count[20];
		snprintf(count, 20, "%d", dirCount(node));
		myPrint(count);
		myPrint(" ");
		snprintf(count, 20, "%d", fileCount(node));
		myPrint(count);
	}
	myPrint(":\n");
	Node* currentNode = node;
	while (currentNode != nullptr) {
		if (currentNode->entry.directory) {
			redPrint(currentNode->name);
			myPrint(" ");
			if (detailed && !(currentNode->name[0] == '.' && currentNode->name[1] == 0)
				&& !(currentNode->name[0] == '.' && currentNode->name[1] == '.' && currentNode->name[2] == 0)) {
				char count[20];
				snprintf(count, 20, "%d", dirCount(currentNode->child));
				myPrint(count);
				myPrint(" ");
				snprintf(count, 20, "%d", fileCount(currentNode->child));
				myPrint(count);
			}
			if (detailed) {
				myPrint("\n");
			}
		}
		else {
			myPrint(currentNode->name);
			myPrint(" ");
			if (detailed) {
				char count[20];
				snprintf(count, 20, "%d", currentNode->entry.size);
				myPrint(count);
				myPrint("\n");
			}
		}
		currentNode = currentNode->next;
	}
	myPrint("\n");

	currentNode = node;
	while (currentNode != nullptr) {
		if (currentNode->entry.directory && !(currentNode->name[0] == '.' && currentNode->name[1] == 0)
			&& !(currentNode->name[0] == '.' && currentNode->name[1] == '.' && currentNode->name[2] == 0)) {
			char relatedStr[50] = { 0 };
			char* pathp = related;
			char* relatedStrp = &relatedStr[0];
			while (*pathp != 0) {
				*relatedStrp = *pathp;
				relatedStrp++;
				pathp++;
			}
			pathp = currentNode->name;
			while (*pathp != 0) {
				*relatedStrp = *pathp;
				relatedStrp++;
				pathp++;
			}
			*relatedStrp = '/';
			relatedStrp++;
			printDir(currentNode->child, relatedStr, detailed);
		}
		currentNode = currentNode->next;
	}
}

void ls(char* path, bool detailed, Node* root){
	Node* related = resolve(path, root, true);
	if (related == nullptr) {
		myPrint("cannot resolve path\n");
		return;
	}
	char relatedStr[50] = { 0 };
	char* pathp = path;
	char* relatedStrp = &relatedStr[0];
	while (*pathp != 0) {
		*relatedStrp = *pathp;
		relatedStrp++;
		pathp++;
	}
	if (path[0] != 0) {
		relatedStrp--;
		if (*relatedStrp != '/') {
			relatedStrp++;
			*relatedStrp = '/';
		}
	}
	else {
		*relatedStrp = '/';
	}
	relatedStrp++;
	printDir(related, relatedStr, detailed);
}

void cat(char* path, Node* root, std::ifstream& input, FsInfo info) {
	Node* fileNode = resolve(path, root, false);
	if (fileNode == nullptr) {
		myPrint("cannot resolve path\n");
		return;
	}
	int currentClus = fileNode->entry.clusterNum;
	char* buffer = new char[info.clusSize + 1];
	buffer[info.clusSize] = 0;
	while (currentClus < 0xFF7) {
		input.seekg(getDataStartAddress(currentClus, info));
		char* writerp = buffer;
		for (int i = 0; i < info.clusSize; i++) {
			buffer[i] = input.get();
		}
		myPrint(buffer);
		currentClus = getNext(currentClus, input, info);
	}
	delete[]buffer;
}

int main()
{
	std::string filename = "/home/naived/FATImgFs/ref.img";
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
	Node* root = getDirStructure(inputFile, fsInfo);
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
				myPrint("exit command need no args\n");
				myPrint("command: exit\n");
				continue;
			}
			else if (flags[0] != 0){
				myPrint("exit command cannot recognize flag\n");
				continue;
			}
			else{
				break;
			}
		}
		else if (cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 't' && cmd[3] == 0) {
			if (arg[0] == 0){
				myPrint("cat command need file path\n");
				myPrint("command: cat [file]\n");
				continue;
			}
			else if (tooManyArgs){
				myPrint("cat command need exactly one file path\n");
				myPrint("command: cat [file]\n");
				continue;
			}
			else if (flags[0] != 0){
				myPrint("cat command cannot recognize flag\n");
				continue;
			}
			else{
				cat(arg, root, inputFile, fsInfo);
			}
		}
		else if (cmd[0] == 'l' && cmd[1] == 's' && cmd[2] == 0){
			if (tooManyArgs){
				myPrint("ls command need no more than one file path\n");
				myPrint("command: ls [file] [options]\n");
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
				myPrint("ls command cannot recognize flag\n");
				continue;
			}
			if (arg[0] == 0){
				arg[0] = '/';
			}
			ls(arg, detailed, root);
		}
	}
	return 0;
}
