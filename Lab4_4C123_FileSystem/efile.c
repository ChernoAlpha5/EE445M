// filename ************** eFile.c *****************************
// High-level routines to implement a solid-state disk 
// Jonathan W. Valvano 3/9/17

#include "edisk.h"
#include "UART2.h"
#include <stdio.h>
#include "string.h"

#define SUCCESS 0
#define FAIL 1

#define MAXNUMBLOCKS 2048
#define NUMFATBLOCKS 8
#define NUMDIRBLOCKS 1

struct dir{
	char name[8];
	uint16_t bytesWritten;
	uint16_t blockNum;
};
typedef struct dir dirType;

#define To16Bit(X) (Hi ## X<<8) | Lo ## X


dirType directory[NUMDIRBLOCKS*42];	//42 file directory entries fit into a single block
uint8_t fileBuffer[512];
uint8_t FAT[NUMFATBLOCKS*512];

#define HiBytesWritten curBufEntry[8]
#define LoBytesWritten curBufEntry[9]
#define HiBlockNum curBufEntry[10]
#define LoBlockNum curBufEntry[11]

void LoadDirectory(){
	for(int k=0; k<NUMDIRBLOCKS; k++){
		eDisk_ReadBlock(fileBuffer, k);
		uint8_t *curBufEntry = fileBuffer;
		for(int i=0; i< 42; i++){
			for(int j=0; j<8; j++){
				directory[42*k+i].name[j] = curBufEntry[j];
			}
			directory[42*k+i].bytesWritten = To16Bit(BytesWritten);
			directory[42*k+i].blockNum  = To16Bit(BlockNum);
			curBufEntry+=12;
		}
	}
}
void LoadFAT(){
	eDisk_Read(0, FAT, NUMDIRBLOCKS, NUMFATBLOCKS);
	/*for(int i = 0; i<NUMFATBLOCKS; i++){
		eDisk_ReadBlock(FAT + 512*i, 0);
	}*/
}

//---------- eFile_Init-----------------
// Activate the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure (already initialized)
int eFile_Init(void){ // initialize file system
	eDisk_Init(0);
	LoadDirectory();
	LoadFAT();
  return SUCCESS;
}

void ClearDir(){
	for(int i=0; i<42; i++){
		directory[i].name[0] = 0;
	}
	directory[0].name[0] = '*';//'*' reserved for free space filename
	directory[0].blockNum = NUMDIRBLOCKS + NUMFATBLOCKS;
}
void ClearFAT(){
	for(int i=0; i<NUMDIRBLOCKS + NUMFATBLOCKS; i++){
		FAT[i] = 0;
	}
	for(int i=NUMDIRBLOCKS + NUMFATBLOCKS; i<MAXNUMBLOCKS; i++){
		FAT[i] = i+1;
	}
	FAT[MAXNUMBLOCKS-1] = 0;
}
void StoreDir(){
	for(int k=0; k<NUMDIRBLOCKS; k++){
		uint8_t *curBufEntry = fileBuffer;
		for(int i=0; i< 42; i++){
			for(int j=0; j<8; j++){
				curBufEntry[j] = directory[42*k+i].name[j];
			}
			HiBytesWritten = (directory[42*k+i].bytesWritten&0xFF00)>>8;
			LoBytesWritten = (directory[42*k+i].bytesWritten&0x00FF);
			HiBlockNum = (directory[42*k+i].blockNum&0xFF00)>>8;
			LoBlockNum = (directory[42*k+i].blockNum&0x00FF);
			curBufEntry+=12;
		}
		eDisk_WriteBlock(fileBuffer, k);
	}
}

void StoreFAT(){
	eDisk_Write(0,FAT,NUMDIRBLOCKS,NUMFATBLOCKS);
}

//---------- eFile_Format-----------------
// Erase all files, create blank directory, initialize free space manager
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Format(void){ // erase disk, add format
	ClearDir();
	ClearFAT();
	StoreDir();
	StoreFAT();
  return SUCCESS;   // OK
}



//---------- eFile_Create-----------------
// Create a new, empty file with one allocated block
// Input: file name is an ASCII string up to seven characters 
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Create( char name[]){  // create new file, make it empty 
	int curDirEntry;
	for(curDirEntry = 1; curDirEntry < 42*NUMDIRBLOCKS; curDirEntry++){	//skip file 0 - free space manager
		if(directory[curDirEntry].name[0] == 0){
			break;
		}
	}
	for(int i = 0; i<8; i++){
		directory[curDirEntry].name[i] = name[i];
	}
	
	int freeSpaceBlockNum = directory[0].blockNum;
	directory[curDirEntry].blockNum = freeSpaceBlockNum;
	directory[curDirEntry].bytesWritten = 0;	
	directory[0].blockNum = FAT[freeSpaceBlockNum];
	FAT[freeSpaceBlockNum] = 0;
  return SUCCESS;     
}

int FindLastBlock(int dirEntry){
	int curBlock = directory[dirEntry].blockNum;
	while(FAT[curBlock] != 0){
		curBlock = FAT[curBlock];
	}
	return curBlock;
}

int findFileName(char *name){
	int curDirEntry;
	for(curDirEntry = 1; curDirEntry < 42*NUMDIRBLOCKS; curDirEntry++){ //skip file 0 - free space manager
		if(strncmp(directory[curDirEntry].name,name,8) == 0){	//equal
			return curDirEntry;
		}
	}
	return 0;
}

int OpenFile = 0;
int FileBlock;
//---------- eFile_WOpen-----------------
// Open the file, read into RAM last block
// Input: file name is an ASCII string up to seven characters
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WOpen(char name[]){      // open a file for writing 
	if(OpenFile){
		return FAIL;
	}
	OpenFile = findFileName(name);
	if(!OpenFile) return FAIL;
	int curBlock = FindLastBlock(OpenFile);
	eDisk_ReadBlock(fileBuffer,curBlock);
	FileBlock = curBlock;
	return SUCCESS;  
}

//things messed up, correct block num for writing/reading
//---------- eFile_Write-----------------
// save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Write(char data){
	if(directory[OpenFile].bytesWritten == 512 - 1){		
		eDisk_WriteBlock(fileBuffer, directory[OpenFile].blockNum);	//save block to SD card

		//allocate new block
		int freeSpaceBlockNum = directory[0].blockNum;
		FAT[directory[OpenFile].blockNum] = freeSpaceBlockNum;
		directory[0].blockNum = FAT[freeSpaceBlockNum];
		FAT[freeSpaceBlockNum] = 0;
		directory[OpenFile].bytesWritten = 0;
	}
	fileBuffer[directory[OpenFile].bytesWritten] = data;
	directory[OpenFile].bytesWritten++;
  return SUCCESS;  
}


//---------- eFile_Close-----------------
// Deactivate the file system
// Input: none
// Output: 0 if successful and 1 on failure (not currently open)
int eFile_Close(void){ 
	StoreDir();
	StoreFAT();
  return SUCCESS;     
}

//---------- eFile_WClose-----------------
// close the file, left disk in a state power can be removed
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WClose(void){ // close the file for writing
  if(!OpenFile){
		return FAIL;
	}
	else{
		eDisk_WriteBlock(fileBuffer, directory[OpenFile].blockNum);	//save block to SD card
		OpenFile = 0;
	}
  return SUCCESS;     
}

int ReadDataIndex = 0;
//---------- eFile_ROpen-----------------
// Open the file, read first block into RAM 
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble read to flash)
int eFile_ROpen( char name[]){      // open a file for reading 
	if(OpenFile){
		return FAIL;
	}
	OpenFile = findFileName(name);
	if(!OpenFile) return FAIL;
	eDisk_ReadBlock(fileBuffer,directory[OpenFile].blockNum);
	ReadDataIndex = 0;
  return SUCCESS;     
}
 
//---------- eFile_ReadNext-----------------
// retreive data from open file
// Input: none
// Output: return by reference data
//         0 if successful and 1 on failure (e.g., end of file)
int eFile_ReadNext( char *pt){       // get next byte 
	if(ReadDataIndex == 512 - 1){
		eDisk_ReadBlock(fileBuffer, directory[OpenFile].
	}
	*pt = fileBuffer[ReadDataIndex];
	ReadDataIndex++;
  return SUCCESS; 
}

    
//---------- eFile_RClose-----------------
// close the reading file
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_RClose(void){ // close the file for writing
	if(!OpenFile){
		return FAIL;
	}
	else{
		OpenFile = 0;
	}
  return SUCCESS;
}


//---------- eFile_Directory-----------------
// Display the directory with filenames and sizes
// Input: pointer to a function that outputs ASCII characters to display
// Output: none
//         0 if successful and 1 on failure (e.g., trouble reading from flash)
int eFile_Directory(void(*fp)(char)){   
	for(int i = 1; i<42*NUMDIRBLOCKS; i++){ //skip file 0 - free space manager
		if(directory[i].name[0]){
			for(int j=0; j<8; j++){
				if(!directory[i].name[j]){
					break;
				}
				fp(directory[i].name[j]);
			}
			fp('\n');
		}
	}
  return SUCCESS;
}

//---------- eFile_Delete-----------------
// delete this file
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Delete( char name[]){  // remove this file 

  return SUCCESS;    // restore directory back to flash
}

int StreamToFile=0;                // 0=UART, 1=stream to file

int eFile_RedirectToFile(char *name){
  eFile_Create(name);              // ignore error if file already exists
  if(eFile_WOpen(name)) return 1;  // cannot open file
  StreamToFile = 1;
  return 0;
}

int eFile_EndRedirectToFile(void){
  StreamToFile = 0;
  if(eFile_WClose()) return 1;    // cannot close file
  return 0;
}

int fputc (int ch, FILE *f) { 
  if(StreamToFile){
    if(eFile_Write(ch)){          // close file on error
       eFile_EndRedirectToFile(); // cannot write to file
       return 1;                  // failure
    }
    return 0; // success writing
  }

   // regular UART output
  UART_OutChar(ch);
  return 0; 
}

int fgetc (FILE *f){
  char ch = UART_InChar();  // receive from keyboard
  UART_OutChar(ch);            // echo
  return ch;
}
