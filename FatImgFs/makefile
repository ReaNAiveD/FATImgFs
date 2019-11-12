fs:print.o FatImgFs.cpp FatImgFs.h print.h
	g++ -g -o fs print.o FatImgFs.cpp
print.o:print.asm
	nasm -f elf64 -o print.o print.asm
