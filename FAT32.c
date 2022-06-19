/*  Nathaniel Green 05/04/2022
    FAT32 Reader
    Partially implements a FAT32 file system. Must be passed a .img file of a FAT32 system in the command line. 
    Works when the .img file is in the same folder as the program. Passing a different path may work, but I've not tried it.
    The following commands are implemented:
    DIR: displays all non-hidden files and directories in the current directory
    EXTRACT: extract a file in the current directory into the directory that the program is running in
    (EXTRA CREDIT) CD: Change directory to a directory within the current folder, or .. to go back
*/

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <utime.h>
#include <time.h>

//make structs work correctly with memcpy
#pragma pack(1)

void displaySector(unsigned char* sector)
{
 // Display the contents of sector[] as 16 rows of 32 bytes each. Each row is shown as 16 bytes,
 // a "-", and then 16 more bytes. The left part of the display is in hex; the right part is in
 // ASCII (if the character is printable; otherwise we display ".".
 //
 for (int i = 0; i < 16; i++) // for each of 16 rows
 { //
 for (int j = 0; j < 32; j++) // for each of 32 values per row
 { //
 printf("%02X ", sector[i * 32 + j]); // Display the byte in hex
 if (j % 32 == 15) printf("- "); // At the half-way point? Show divider
 }
 printf(" "); // Separator space between hex & ASCII
 for (int j = 0; j < 32; j++) // For each of those same 32 bytes
 { //
 if (j == 16) printf(" "); // Halfway? Add a space for clarity
 int c = (unsigned int)sector[i * 32 + j]; // What IS this char's ASCII value?
 if (c >= 32 && c <= 127) printf("%1c", c); // IF printable, display it
 else printf("."); // Otherwise, display a "."
 } //
 printf("\n"); // That’s the end of this row
 }
}

struct PartitionTableEntry //An entry in the MBR's partition table
{
unsigned char bootFlag;
unsigned char CHSBegin[3];
unsigned char typeCode;
unsigned char CHSEnd[3];
unsigned int LBABegin;
unsigned int LBAEnd;
};

struct MBRStruct //used to store MBR
{
unsigned char bootCode[446];
struct PartitionTableEntry part1;
struct PartitionTableEntry part2;
struct PartitionTableEntry part3;
struct PartitionTableEntry part4;
unsigned short flag;
} MBR;

struct BPBStruct //used to store BPB
{
 unsigned char BS_jmpBoot[3]; // Jump instruction to boot code
 unsigned char BS_OEMNane[8]; // 8-Character string (not null terminated)
 unsigned short BPB_BytsPerSec; // Had BETTER be 512!
 unsigned char BPB_SecPerClus; // How many sectors make up a cluster?
 unsigned short BPB_RsvdSecCnt; // # of reserved sectors at the beginning (including the BPB)?
 unsigned char BPB_NumFATs; // How many copies of the FAT are there? (had better be 2)
 unsigned short BPB_RootEntCnt; // ZERO for FAT32
 unsigned short BPB_TotSec16; // ZERO for FAT32
 unsigned char BPB_Media; // SHOULD be 0xF8 for "fixed", but isn't critical for us
 unsigned short BPB_FATSz16; // ZERO for FAT32
 unsigned short BPB_SecPerTrk; // Don't care; we're using LBA; no CHS
 unsigned short BPB_NumHeads; // Don't care; we're using LBA; no CHS
 unsigned int BPB_HiddSec; // Don't care ?
 unsigned int BPB_TotSec32; // Total Number of Sectors on the volume
 unsigned int BPB_FATSz32; // How many sectors long is ONE Copy of the FAT?
 unsigned short BPB_Flags; // Flags (see document)
 unsigned short BPB_FSVer; // Version of the File System
 unsigned int BPB_RootClus; // Cluster number where the root directory starts (should be 2)
 unsigned short BPB_FSInfo; // What sector is the FSINFO struct located in? Usually 1
 unsigned short BPB_BkBootSec; // REALLY should be 6 – (sector # of the boot record backup)
 unsigned char BPB_Reserved[12]; // Should be all zeroes -- reserved for future use
 unsigned char BS_DrvNum; // Drive number for int 13 access (ignore)
 unsigned char BS_Reserved1; // Reserved (should be 0)
 unsigned char BS_BootSig; // Boot Signature (must be 0x29)
 unsigned char BS_VolID[4]; // Volume ID
 unsigned char BS_VolLab[11]; // Volume Label
 unsigned char BS_FilSysType[8]; // Must be "FAT32 "
 unsigned char unused[420]; // Not used
 unsigned char signature[2]; // MUST BE 0x55 AA
} BPB;





unsigned int FirstDataSector;//where the data starts
unsigned int tableOffset;//where the table starts
unsigned int currDir;//where the current directory starts


struct PartitionTableEntry * fatPartition;//will hold the correct partition
FILE* imgFile;//will hold the .img file
unsigned char userInput [281];//holds raw commmand line input
unsigned char inputCommand [10];//holds command line command
unsigned char inputFile[257];//holds command line file name
unsigned char inputFileUpper[257];//holds command line file name converted to uppercase
unsigned char dirName[256];//holds the name of the current directory




void dirCommand();
void extractCommand();//misnomer, also used for CD command



int main(int argc, char *argv[]){
    setlocale(LC_NUMERIC, "");//prints ints with commas in the right areas

    //is the input correct?
    if(argc == 2){
        
        
        
        
        imgFile = fopen(argv[1], "r");
        if(imgFile == NULL){//can't find the file
            fprintf(stderr, "Invalid Input: File not found\n");
            return 1;
        }
    }else{//someone tried weird inputs
        fprintf(stderr, "Invalid Input: please input only file name\n");
        return 1;
    }

    

    //read in MBR
    fread(&MBR.bootCode, 1, 446, imgFile);
    fread(&MBR.part1, 1, 66, imgFile);
    if(MBR.flag != 0xAA55){//if the signature is not found, probably not the right type of file
        fprintf(stderr, "MBR not found. Are you sure this is a FAT32 system image?\n");
        return 1;
    }

    //find the start of the right partition
    if(MBR.part1.typeCode = 0x0C){
        fatPartition = &(MBR.part1);
    }else if(MBR.part2.typeCode = 0x0C){
        fatPartition = &(MBR.part2);
    }else if(MBR.part3.typeCode = 0x0C){
        fatPartition = &(MBR.part3);
    }else if(MBR.part4.typeCode = 0x0C){
        fatPartition = &(MBR.part4);
    }

    unsigned int PartOffset = (fatPartition->LBABegin);//beginning of the partition

    //find BPB

    fseek(imgFile, PartOffset*512, SEEK_SET);
    fread(&BPB, 1, 512, imgFile);
    if(BPB.BPB_BytsPerSec != 512){//If BytsPerSec isn't 512, the BPB probably wasn't found correctly
        fprintf(stderr, "BPB corrupted\n");
        return 1;
    }

    FirstDataSector = BPB.BPB_RsvdSecCnt + (BPB.BPB_NumFATs * BPB.BPB_FATSz32) + PartOffset;//use BPB information to find first data sector
    tableOffset = PartOffset + BPB.BPB_RsvdSecCnt;//use BPB information to find FAT table
    currDir = FirstDataSector;//first data sector is always root directory

     
    inputFile[0] = 0x00;
    dirName[0] = 0x00;
    //start normal operation
    int currInputInd;
    int currFileInd;
    while(1){
        printf("%s\\>", dirName);//print prompt
        fgets(userInput, 280, stdin);//get input
        currInputInd = 0;
        while(userInput[currInputInd] != 0x20 && userInput[currInputInd] != '\n' && currInputInd < 9){//parse user command
            inputCommand[currInputInd] = userInput[currInputInd];
            currInputInd++;
        }
        inputCommand[currInputInd] = 0x00;
        if(userInput[currInputInd] != '\n'){//parse file input, if it exists
            currInputInd++;
            currFileInd = 0;
            while(userInput[currInputInd] != '\n' && currFileInd < 257){
                inputFile[currFileInd] = userInput[currInputInd];
                currFileInd++;
                currInputInd++;
            }
            inputFile[currFileInd] = 0x00;
        }
        currInputInd = 0;
        while(inputCommand[currInputInd] != 0x00){//convert command to lowercase
            if(inputCommand[currInputInd] >= 0x41 && inputCommand[currInputInd] <= 0x5A){
                inputCommand[currInputInd] = inputCommand[currInputInd] ^ 0x20;
            }
            currInputInd++;
        }

        if( strcmp(inputCommand, "dir") == 0 && inputFile[0] == 0x00){//look for correct command
            dirCommand();
        }else if((strcmp(inputCommand, "extract") == 0 || strcmp(inputCommand, "cd") == 0) && inputFile[0] != 0x00){
            extractCommand();//misnomer, also used for CD command
        }else{
            printf("Invalid Input.\n");
        }

        
        inputFile[0] = 0x00;
    }

}

void dirCommand(){//the directory command:
    unsigned int totalListed = 0;//total number of listed files
    unsigned int totalSize = 0;//total size of listed files
    unsigned int currClusterStart = currDir;//current cluster of directory
    unsigned int currLFNPos = 254;//LFN position
    unsigned char currLine[32];//current entry in directory
    unsigned char LFN[256];//holds LFN
    LFN[255] = 0x00;
    unsigned int currBytes = 0;//holds current position within cluster
    int currLinePos; // needs negative for while condition

    unsigned short int date;//holds entry date
    unsigned short int time;//holds entry time
    unsigned int lineSize = 0;//size of the current entry
    unsigned char eight[9];//file name before the dot
    unsigned char three[4];//file name after the  dot
    unsigned char AMPM[2];//AM/PM. will switch between AM and PM when appropriate
    AMPM[1] = 'M';

    
    
    while(1){
        fseek(imgFile, currClusterStart*BPB.BPB_BytsPerSec, SEEK_SET);//move reader to cluster start
        while(currBytes < BPB.BPB_BytsPerSec * BPB.BPB_SecPerClus){//repeat until cluster end

            fread(currLine, 1, 32, imgFile);//read in entry

            if(currLine[0] == 0x00){ // end of entries
                if(totalListed == 0){
                    printf("File not found\n");
                }else{
                printf("%'u file(s), %'u bytes \n\n", totalListed, totalSize);
                }
                break;
            }else if(currLine[0] == 0xE5){ //unused, do nothing.

            }else if(currLine[11] == 0x0F){ //it's an LFN line
                
                currLinePos = 30;
                while(currLinePos > 0){//copying the LFN backwards into the LFN array

                    LFN[currLFNPos] = currLine[currLinePos];
                    currLFNPos = currLFNPos - 1;
                    
                    if(currLinePos == 14){
                        currLinePos = currLinePos - 5;
                    }else if(currLinePos == 28){
                        currLinePos = currLinePos - 4;
                    }else{
                        currLinePos = currLinePos - 2;
                    }
                }
                
            }else if(currLine[11] == 0x08){//root volume name and ID
                unsigned int serID;
                memcpy(&serID, BPB.BS_VolID, 4);
                printf("Volume: %11s Serial #: %x \n", currLine, serID);

            }else{//Short Entry
                memcpy(&lineSize, &(currLine[28]), 4);
                if(!(currLine[11] & 0x02)){
                    totalSize += lineSize;//add size of this file to total
                    sscanf(&(currLine[0]),"%8s", eight);
                    sscanf(&(currLine[8]),"%3s", three);
                    memcpy(&time, &(currLine[22]), 2);
                    memcpy(&date, &(currLine[24]), 2);
                    
                    totalListed = totalListed + 1; // increment number of listed files
                    unsigned int min = (time & 0x07E0)>> 5 ;
                    unsigned int hour = (time & 0xF800) >> 11; // for some reason this always outputs 4 hours ahead of what Linux says?
                    
                    if(hour >= 12){//determine whether AM or PM
                        AMPM[0] = 'P';
                    }else{
                        AMPM[0] = 'A';
                    }
                    hour = hour % 12;
                    if(hour == 0){
                        hour = 12;
                    }

                    if(lineSize > 0){//print file information
                        printf("%.2u/%.2u/%u  %.2u:%.2u %.2s%'15u%12s.%s\t%s\n", (date & 0x1E0) >> 5, date & 0x1F, 1980 + ((date & 0xFE00) >> 9), hour, min, AMPM, lineSize , eight, three, &(LFN[currLFNPos+1]));
                    }else{//print directory information
                        printf("%.2u/%.2u/%u  %.2u:%.2u %.2s%15s%12s%s\t%s\n", (date & 0x1E0) >> 5, date & 0x1F, 1980 + ((date & 0xFE00) >> 9), hour, min, AMPM, "<DIR>" , eight, three, &(LFN[currLFNPos+1]));
                    }
                    
                    
                }
                currLFNPos = 254;
                LFN[currLFNPos] = 0x00;
            
            }
            currBytes = currBytes + 32;
        }
        currBytes = 0;

        unsigned int tablePos = ((currClusterStart - FirstDataSector) / BPB.BPB_SecPerClus) + 2;//determine the current table position
        fseek(imgFile, (tableOffset*BPB.BPB_BytsPerSec) + (tablePos*4), SEEK_SET);//go to that table position
        fread(&currClusterStart, 1, 4, imgFile);//read the next table position
        if(currClusterStart >= 0x0FFFFFFF){break;}//if this was the last table position, we're done. This should never be used in reality.
        currClusterStart = FirstDataSector + ((currClusterStart-2) * BPB.BPB_SecPerClus);//determine the next cluster from the new table position
    }




}


void extractFile(char* entry, char* filename){//extracts file
    unsigned int currClusterStartnum;//current table entry
    unsigned int currClusterStart; // current cluster start
    unsigned int totalsize;//holds total size of file
    unsigned int currentsize = 0;//holds current size of file.
    unsigned int copysize = BPB.BPB_BytsPerSec;//current copy size
    unsigned int sectorbytes = 0;//number of bytes read in current sector
    unsigned int tablePos;
    unsigned char sectorinfo[BPB.BPB_BytsPerSec];//buffer between reading/writing a sector
    unsigned char firstCluster[4];//holds upper/lower bytes of the first cluster
    memcpy(firstCluster, &(entry[26]), 2);//read in the bytes of the first cluster
    memcpy(&(firstCluster[2]), &(entry[20]), 2);
    memcpy(&currClusterStartnum, firstCluster, 4);//convert these bytes into an int
    unsigned short int entryDate;
    unsigned short int entryTime;

    memcpy(&totalsize, &(entry[28]), 4);//get entry total size
    FILE* outFile;
    outFile = fopen(filename,"w");//open output file
    
    while (currentsize < totalsize){//repeat until we've read in all bytes of the file
        currClusterStart = FirstDataSector + ((currClusterStartnum-2) * BPB.BPB_SecPerClus);//convert table position to cluster position
        fseek(imgFile, currClusterStart*BPB.BPB_BytsPerSec, SEEK_SET);//seek cluster start

        while(sectorbytes < BPB.BPB_BytsPerSec * BPB.BPB_SecPerClus && currentsize != totalsize){// repeat until we've read the whole cluster unless we reach the end of the file
            if(currentsize + BPB.BPB_BytsPerSec > totalsize){//if we're reaching the end of a file, we'll have to read in less than a whole sector
                copysize = totalsize - currentsize;
            }
            fread(sectorinfo, 1, copysize, imgFile);//read from FAT32
            fwrite(sectorinfo, 1, copysize, outFile);//output to our file system
            currentsize += copysize;
            sectorbytes += copysize;
        }

        sectorbytes = 0;
        
        fseek(imgFile, (tableOffset*BPB.BPB_BytsPerSec) + (currClusterStartnum*4), SEEK_SET);//determine the current table position
        fread(&currClusterStartnum, 1, 4, imgFile);//read the next table position
        if(currClusterStartnum >= 0x0FFFFFFF){break;}//if this was the last table position, we're done. This is only used if we have a file that perfectly fits a cluster
        
    }
    fclose(outFile);//close the write file
    
    time_t rawTime;
    time(&rawTime);
    struct utimbuf outTimes;
    
    struct tm* timeinfo = localtime(&rawTime);
    
    //Changing the modified/accessed times to reflect the original file
    memcpy(&entryTime, &(entry[22]), 2);
    memcpy(&entryDate, &(entry[24]), 2);
    
    timeinfo->tm_year = ((entryDate & 0xFE00) >> 9)+80;
    timeinfo->tm_mon = ((entryDate & 0x1E0) >> 5)-1;
    timeinfo->tm_mday = entryDate & 0x1F;
    timeinfo->tm_hour = (entryTime & 0xF800) >> 11;
    timeinfo->tm_min = (entryTime & 0x07E0)>> 5;
    timeinfo->tm_sec = (entryTime & 0x001F)*2;
    
    outTimes.modtime = mktime(timeinfo);

    timeinfo = localtime(&rawTime);
    
    memcpy(&entryDate, &(entry[18]), 2);
    timeinfo->tm_year = ((entryDate & 0xFE00) >> 9)+80;
    timeinfo->tm_mon = ((entryDate & 0x1E0) >> 5)-1;
    timeinfo->tm_mday = entryDate & 0x1F;
    timeinfo->tm_hour = 0;
    timeinfo->tm_min = 0;
    timeinfo->tm_sec = 0;

    outTimes.actime = mktime(timeinfo);

    utime(filename, &outTimes);//this actually sets the times
    
}






void cd(char* entry, char* newDirName){//cd command, once the correct directory is found
    
    unsigned int clusternum = 0;
    unsigned char firstCluster[4];
    memcpy(firstCluster, &(entry[26]), 2);
    memcpy(&(firstCluster[2]), &(entry[20]), 2);
    memcpy(&clusternum, firstCluster, 4);
    if(clusternum == 0){//root directory, special case
        currDir = FirstDataSector;
        dirName[0] = 0x00;
        return;
    }
    currDir = FirstDataSector + ((clusternum-2) * BPB.BPB_SecPerClus);//change current directory position
    int g = 0;
    while(newDirName[g] != 0x00){//change directory name
        dirName[g] = newDirName[g];
        g++;
    }
    dirName[g] = 0x00;
}





void extractCommand(){// finds the correct file or directory for EXTRACT and CD using user input.
    unsigned int currClusterStart = currDir;
    unsigned int currLFNPos = 254;
    unsigned char currLine[32];
    unsigned char LFN[256];
    unsigned char LFNUpper[256];
    LFN[255] = 0x00;
    unsigned int currBytes = 0;
    unsigned int entrySize = 0;
    int currLinePos; // needs negative for while condition
    unsigned char eightpointthree[13];
    eightpointthree[12] = 0x00;
    unsigned char eight[9];
    unsigned char three[4];
    memcpy(inputFileUpper, inputFile, 256);
    int g = 0;

    

    while(inputFileUpper[g] != 0x00){//copies the input file name into uppercase
        if(inputFile[g] == '\"' && inputFile[g+1] == 0x00){//if there is a quote at the end, we know we're looking for LFN
            inputFile[g] = 0x00;
            inputFileUpper[g] = 0x00;
        }
        if(inputFileUpper[g] >= 0x61 && inputFileUpper[g] <= 0x7A){
            inputFileUpper[g] = inputFileUpper[g] ^ 0x20;
        }
        g += 1;
    }




    
    while(1){
        fseek(imgFile, currClusterStart*512, SEEK_SET);//looks for correct cluster
        while(currBytes < BPB.BPB_BytsPerSec * BPB.BPB_SecPerClus){//repeat until we're done with this cluster

            fread(currLine, 1, 32, imgFile);

            if(currLine[0] == 0x00){ // end of entries
                printf("File/Directory not found\n");
                return;
            }else if(currLine[0] == 0xE5){ //unused, do nothing.

            }else if(currLine[11] == 0x0F){ //it's an LFN line
                
                currLinePos = 30;
                while(currLinePos > 0){//copying the LFN backwards into the LFN array

                    LFN[currLFNPos] = currLine[currLinePos];
                    currLFNPos = currLFNPos - 1;
                    
                    if(currLinePos == 14){
                        currLinePos = currLinePos - 5;
                    }else if(currLinePos == 28){
                        currLinePos = currLinePos - 4;
                    }else{
                        currLinePos = currLinePos - 2;
                    }
                }
                
            }else if(currLine[11] == 0x08){//volume name, don't care for this part.
                

            }else{//it's a short entry
                if(!(currLine[11] & 0x02)){
                    sscanf(&(currLine[0]),"%8s", eight);//read in name
                    sscanf(&(currLine[8]),"%3s", three);
                    
                    int i = 0;
                    int j = 0;
                    //read the name parts into one array to match user input.
                    while(eight[i] != 0x00){
                        eightpointthree[i] = eight[i];
                        i += 1;
                    }
                    eightpointthree[i] = '.';
                    
                    i += 1;
                    while(three[j] != 0x00){
                        eightpointthree[i] = three[j];
                        j += 1;
                        i += 1;
                    }



                    for(int i = currLFNPos; i < 256; i++){//copy the LFN into a different array, converting to uppercase
                        LFNUpper[i] = LFN[i];
                        if(LFN[i] >= 0x61 && LFN[i] <= 0x7A){
                            LFNUpper[i] = LFN[i] ^ 0x20;
                        }
                        

                    }

                    eightpointthree[i] = 0x00;
                    memcpy(&entrySize, &(currLine[28]), 4);
                    //printf("%s : %s \n", &(LFNUpper[currLFNPos+1]), &(inputFileUpper[1]));
                    if( strcmp(inputCommand, "extract") == 0 && (strcmp(eightpointthree, inputFileUpper) == 0 || (strcmp(&(LFNUpper[currLFNPos+1]), &inputFileUpper[1]) == 0 && inputFile[0] == '"') ) ){//correct file for EXTRACT found
                        
                        if(entrySize == 0){
                            printf("File/Directory not found\n");
                        }
                        
                        printf("File found! copying...\n");
                        if(currLFNPos == 254){
                            extractFile(currLine, eightpointthree);//this has no LFN.
                        }else{
                            extractFile(currLine, &(LFN[currLFNPos + 1]));//this has an LFN.
                        }

                        return;
                    }


                    if(strcmp(inputCommand, "cd") == 0 && entrySize == 0){//cd command
                        if(strcmp(eight, inputFileUpper) == 0){//found using 8.3
                            cd(currLine, eight);
                            return;
                        }
                        
                        

                        if(strcmp(&(LFNUpper[currLFNPos+1]), &inputFileUpper[1]) == 0 && inputFile[0] == '"'){//found using LFN
                            cd(currLine, &(LFN[currLFNPos + 1]));
                            return;
                        }

                    }


                    
                    
                }
                currLFNPos = 254;
                LFN[currLFNPos] = 0x00;
            
            }
            currBytes = currBytes + 32;
        }
        currBytes = 0;

        int tablePos = ((currClusterStart - FirstDataSector) / BPB.BPB_SecPerClus) + 2;//determine the current table position
        fseek(imgFile, (tableOffset*512) + (tablePos*4), SEEK_SET);//go to that table position
        fread(&currClusterStart, 1, 4, imgFile);//read the next table position
        if(currClusterStart >= 0x0FFFFFFF){break;}//if this was the last table position, we're done. This should never be used in reality.
        currClusterStart = FirstDataSector + ((currClusterStart-2) * BPB.BPB_SecPerClus);//determine the next cluster from the new table position
    }







}
