#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

struct BootEntry;
struct DirEntry;
FILE * fp;
unsigned char NumFATs;
unsigned short BytsPerSec; 
unsigned char SecPerClus;
unsigned short RsvdSecCnt;
unsigned short FirstFAT;
unsigned int DataArea;
unsigned int RootClus;
unsigned char DIRName[11];
unsigned int DIRFileSize;
unsigned short DIRFstClus;
unsigned char DIRAttr;
unsigned short num;
unsigned int FatSizeInByte;
unsigned int numDir;
unsigned int *fat;
unsigned int ClusterSize;
#pragma pack(push, 1)
typedef struct	Boot_Entry{
	unsigned char BS_jmpBoot[3];/* Assembly instruction to jump to boot code */
	unsigned char BS_OEMName[8];/* OEM Name in ASCII */
	unsigned short BPB_BytsPerSec;/* Bytes per sector. Allowed values include 512, 1024, 2048, and 4096 */
	unsigned char BPB_SecPerClus;/* Sectors per cluster (data unit). Allowed values are powers of 2, but the cluster size must be 32KB or smaller */
	unsigned short BPB_RsvdSecCnt;/* Size in sectors of the reserved area */
	unsigned char BPB_NumFATs;/* Number of FATs */
	unsigned short BPB_RootEntCnt;/* Maximum number of files in the root directory for FAT12 and FAT16. This is 0 for FAT32 */
	unsigned short BPB_TotSec16;/* 16-bit value of number of sectors in file system */
	unsigned char BPB_Media;/* Media type */
	unsigned short BPB_FATSz16;/* 16-bit size in sectors of each FAT for FAT12 and FAT16. For FAT32, this field is 0 */
	unsigned short BPB_SecPerTrk;/* Sectors per track of storage device */
	unsigned short BPB_NumHeads;/* Number of heads in storage device */
	unsigned int BPB_HiddSec;/* Number of sectors before the start of partition */
	unsigned int BPB_TotSec32;/* 32-bit value of number of sectors in file system. Either this value or the 16-bit value above must be 0 */
	unsigned int BPB_FATSz32;/* 32-bit size in sectors of one FAT */
	unsigned short BPB_ExtFlags;/* A flag for FAT */
	unsigned short BPB_FSVer;/* The major and minor version number */
	unsigned int BPB_RootClus;/* Cluster where the root directory can be found */
	unsigned short BPB_FSInfo;/* Sector where FSINFO structure can be found */
	unsigned short BPB_BkBootSec;/* Sector where backup copy of boot sector is located */
	unsigned char BPB_Reserved[12];/* Reserved */
	unsigned char BS_DrvNum;/* BIOS INT13h drive number */
	unsigned char BS_Reserved1;/* Not used */
	unsigned char BS_BootSig;/* Extended boot signature to identify if the next three values are valid */
	unsigned int BS_VolID;/* Volume serial number */
	unsigned char BS_VolLab[11];/* Volume label in ASCII. User defines when creating the file system */
	unsigned char BS_FilSysType[8];/* File system type label in ASCII */
}BootEntry;
typedef struct Dir_Entry {
	unsigned char DIR_Name[11];/* File name */
	unsigned char DIR_Attr;/* File attributes */
	unsigned char DIR_NTRes;/* Reserved */
	unsigned char DIR_CrtTimeTenth;/* Created time (tenths of second) */
	unsigned short DIR_CrtTime;/* Created time (hours, minutes, seconds) */
	unsigned short DIR_CrtDate;/* Created day */
	unsigned short DIR_LstAccDate;/* Accessed day */
	unsigned short DIR_FstClusHI;/* High 2 bytes of the first cluster address */
	unsigned short DIR_WrtTime;/* Written time (hours, minutes, seconds */
	unsigned short DIR_WrtDate;/* Written day */
	unsigned short DIR_FstClusLO;/* Low 2 bytes of the first cluster address */
	unsigned int DIR_FileSize;/* File size in bytes. (0 for directories) */
}DirEntry;
#pragma pack(pop)
DirEntry *dirEntries;

void getname(int index, unsigned char *name){ 
	int i;
	for(i=0;i<8;i++){
		if (dirEntries[index].DIR_Name[i] == ' ')
			break;
                *name++ = dirEntries[index].DIR_Name[i];
	}
        if (dirEntries[index].DIR_Name[8] != ' ') {
                *name++ = '.';
                for(i=8;i<=10;i++){
			if (dirEntries[index].DIR_Name[i] == ' ') break;
                        *name++ = dirEntries[index].DIR_Name[i];
        	}
        }
        if (dirEntries[index].DIR_Attr & 0x10)
                *name++ = '/';
        *name = 0;
}
void openDevice(char * device){
        fp = fopen(device, "r");
	if(fp == NULL){
                perror("error");
                exit(1);
        }
	BootEntry *boot_entry;
        boot_entry = malloc(sizeof(BootEntry));
        fread(boot_entry, sizeof(BootEntry), 1, fp);
	NumFATs = boot_entry->BPB_NumFATs;
	BytsPerSec = boot_entry->BPB_BytsPerSec;
	SecPerClus = boot_entry->BPB_SecPerClus;
	RsvdSecCnt = boot_entry->BPB_RsvdSecCnt;
	RootClus = boot_entry->BPB_RootClus;
	FirstFAT =  BytsPerSec * RsvdSecCnt;
	DataArea = (RsvdSecCnt + NumFATs * boot_entry->BPB_FATSz32) * BytsPerSec;
        FatSizeInByte = (unsigned int)(boot_entry->BPB_FATSz32 * BytsPerSec);
        ClusterSize = BytsPerSec * SecPerClus;

        fat = malloc(FatSizeInByte);
        pread(fileno(fp), fat, FatSizeInByte,FirstFAT);

        unsigned int i;
        unsigned int rootDir = 0;
        DirEntry *tmp;
        for (i=RootClus; i<0x0FFFFFF7; i=fat[i]){rootDir++;}
        tmp = dirEntries = malloc(rootDir * ClusterSize);
        numDir = rootDir * ClusterSize / sizeof(DirEntry);
        for (i=RootClus; i<0x0FFFFFF7; i=fat[i]) {
                pread(fileno(fp), tmp, ClusterSize, DataArea + (i-2)*ClusterSize);
                tmp += ClusterSize/sizeof(DirEntry);
        }
	fclose(fp);
}
void printInfo(char *device){
        openDevice(device);
        fprintf(stdout,"Number of FATs = %d\n",NumFATs);
        fprintf(stdout,"Number of bytes per sector = %d\n",BytsPerSec);
        fprintf(stdout,"Number of sectors per cluster = %d\n",SecPerClus);
        fprintf(stdout,"Number of reserved sectors = %d\n",RsvdSecCnt);
        fprintf(stdout,"First FAT starts at byte = %d\n",FirstFAT);
        fprintf(stdout,"Data area starts at byte = %d\n",DataArea);
}
void listRootDirectory(char * device){
	openDevice(device);
        int j; 
	unsigned int count = 1;
       	unsigned char name[15];
        for (j=0; j<numDir; j++) {
                if (dirEntries[j].DIR_Name[0] == 0)
			continue;
		if (dirEntries[j].DIR_Attr == 0x0f){
                        printf("%u, LFN entry\n",count++);
			continue;
		}
                getname(j, name);
		if(dirEntries[j].DIR_Name[0] == 0xe5){
			name[0] = '?';
		}
                printf("%u, %s", count++, name);
                printf(", %u, %u\n", dirEntries[j].DIR_FileSize, (dirEntries[j].DIR_FstClusHI<<16)+dirEntries[j].DIR_FstClusLO);
        }
}

void recoverFile(char *device,char target[13],char dest[200]){
	openDevice(device);
	int i,j,k;
	int count=0;
	int find=0;
	char tmp[13];
	char *tmp2;
	char *Data;
	FILE *rf;
	fp = fopen(device,"r");
	for(i=1;i<13;i++){
		tmp[count] = target[i];
		count++;
	}
	for(j=0;j<numDir;j++){
		tmp2 = malloc(13);
		count = 0;
		if(dirEntries[j].DIR_Name[0] == 0xe5){
			for(k=1;k<8;k++){
				if(dirEntries[j].DIR_Name[k] == ' ')break;
				tmp2[count] = dirEntries[j].DIR_Name[k];
				count++;
			}
			if(dirEntries[j].DIR_Name[8] != ' '){
				tmp2[count]='.';
				count++;
				for(k=8;k<11;k++){
					if(dirEntries[j].DIR_Name[k] == ' ')break;
					tmp2[count] = dirEntries[j].DIR_Name[k];
					count++;
				}
			}
			tmp2[count] = 0;
		

		if(strcmp(tmp2,tmp) == 0){
			find++;
			DIRFstClus = (dirEntries[j].DIR_FstClusHI<<16)+dirEntries[j].DIR_FstClusLO;
				

			if(fat[DIRFstClus] == 0){
				Data = malloc(ClusterSize);
				rf = fopen(dest,"w+");
				if(rf == NULL){
					fprintf(stdout,"%s: failed to open\n",dest);
					exit(1);
				}
				fseek(fp,(DataArea + (DIRFstClus -2)*ClusterSize),SEEK_SET);
				fread(Data,dirEntries[j].DIR_FileSize,1,fp);
				fwrite(Data,dirEntries[j].DIR_FileSize, 1, rf);
				fclose(rf);
				fprintf(stdout,"%s: recovered\n",target);
				exit(1);
			}else{
				if((dirEntries[j].DIR_FileSize == 0)&&(DIRFstClus == 0)){
					rf = fopen(dest,"w+");
					if(rf == NULL){
                                        fprintf(stdout,"%s: failed to open\n",dest);
                                        exit(1);
                              		}
					fclose(rf);
					fprintf(stdout,"%s: recovered\n",target);
	                                exit(1);

				}else{

				fprintf(stdout,"%s: error - fail to recover\n",target);
				exit(1);
				}
			}
		}
	}
}
	if(find == 0){
		fprintf(stdout,"%s: error - file not found\n",target);
		exit(1);
	}	
}
void cleanseFile(char *device,unsigned char target[13]){
        openDevice(device);
        int i,j,k;
        int count=0;
        int find=0;
        unsigned char tmp[13];
        char *tmp2;
        for(i=1;i<13;i++){
                tmp[count] = target[i];
                count++;
        }
        for(j=0;j<numDir;j++){
                tmp2 = malloc(13);
                count = 0;
                if(dirEntries[j].DIR_Name[0] == 0xe5){
                        for(k=1;k<8;k++){
                                if(dirEntries[j].DIR_Name[k] == ' ')break;
                                tmp2[count] = dirEntries[j].DIR_Name[k];
                                count++;
                        }
                if(dirEntries[j].DIR_Name[8] != ' '){
                                tmp2[count]='.';
                                count++;
                                for(k=8;k<11;k++){
                                        if(dirEntries[j].DIR_Name[k] == ' ')break;
                                        tmp2[count] = dirEntries[j].DIR_Name[k];
                                        count++;
				}
                }
                tmp2[count] = 0;
                

                if(strcmp(tmp2,tmp) == 0){
                        find++;
                        DIRFstClus = (dirEntries[j].DIR_FstClusHI<<16)+dirEntries[j].DIR_FstClusLO;
                        if(fat[DIRFstClus] == 0){
				if(dirEntries[j].DIR_FileSize != 0){
					fp = fopen(device,"r+");
					char Data[dirEntries[j].DIR_FileSize];
					for(i=0;i<dirEntries[j].DIR_FileSize;i++){
						Data[i] = 0;
					}
                                	if(fp == NULL){
                                		perror("error");
                                		exit(1);
                               		}
                                	fseek(fp,(DataArea + (DIRFstClus -2)*ClusterSize),SEEK_SET);
                                	fwrite(Data,dirEntries[j].DIR_FileSize, 1, fp);
                                	fclose(fp);
                                	fprintf(stdout,"%s: cleansed\n",target);
                                	exit(1);
				 }else{
                	                fprintf(stdout,"%s: error - fail to cleanse\n",target);
                        	        exit(1);
                        	}

                        }else{
                                fprintf(stdout,"%s: error - fail to cleanse\n",target);
                                exit(1);
                        }
                }
        }
}
        if(find == 0){
                fprintf(stdout,"%s: error - file not found\n",target);
                exit(1);
        }
	

}
void UsageOfProgram(char *c){
	fprintf(stdout,"Usage: %s -d [device filename] [other arguments]\n",c);
        fprintf(stdout,"-i                   Print file system information\n");
        fprintf(stdout,"-l                   List the root directory\n");
        fprintf(stdout,"-r target -o dest    Recover the target deleted file\n");
        fprintf(stdout,"-x target            Cleanse the target deleted file\n");

}
int main(int argc,char *argv[]){
        if((argc >= 4)&&(strcmp(argv[1],"-d")==0)){
		if((argc==4)&&(strcmp(argv[3],"-i")==0))
		{
			printInfo(argv[2]);
		}
		else if((argc==4)&&(strcmp(argv[3],"-l")==0))
		{
			listRootDirectory(argv[2]);
		}
		//Recover the target delted file
		else if((argc==7)&&(strcmp(argv[3],"-r")==0)&&(strcmp(argv[5],"-o")==0)){
			recoverFile(argv[2],argv[4],argv[6]);
		}
		//Cleanse the target deleted file
		else if((argc==5)&&(strcmp(argv[3],"-x")==0)){
			cleanseFile(argv[2],argv[4]);
		}
		else {UsageOfProgram(argv[0]);}
        } 
	else {UsageOfProgram(argv[0]);}
        return 0;
}
