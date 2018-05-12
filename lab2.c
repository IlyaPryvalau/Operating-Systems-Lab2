/*
Вариант 8
Подсчитать для заданного каталога (первый аргумент командной строки) и всех его подкаталогов суммарный размер занимаемого файлами на диске пространства в байтах
и суммарный размер файлов. Вычислить коэффициент использования дискового пространства в %. Для получения размера занимаемого файлами на диске пространства
использовать команду stat.
*/

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <limits.h>
#define errorPath "/tmp/error_log.txt"
#define blockSize 512

dev_t rootDev;
ino_t *listPtr = NULL;
int listItemCount = 0;
long totalSize = 0;
long totalBlocks = 0;
long actualSize = 0;
long actualBlocks = 0;

void dumpError(FILE *errorLog)
{
  fseek(errorLog, 0, SEEK_SET);
  int str = fgetc(errorLog);
  while (str != EOF)
  {
    fputc(str, stderr);
    str = fgetc(errorLog);
  }
  fclose(errorLog);
  remove(errorPath);
}

int listPush(ino_t **listBase, ino_t value) {
	ino_t *newPtr;
	if ((newPtr = (ino_t *)realloc(*listBase, sizeof(ino_t) * (listItemCount + 1))) == NULL)
		return 0;
	*listBase = newPtr;
	newPtr[listItemCount++] = value;
	printf("%d\n", (int)newPtr[listItemCount - 1]);
	return 1;
}

int inList(ino_t *listBase, ino_t value) {
    if (!listBase)
        return 0;
    for(int i = 0; i <= listItemCount; i++) {
        if (value == listBase[i]) 
            return 1;
    }
    return 0;
}

void dirExplore(char *programName, char *dirName, FILE *errorLog, long *siz, long* blocks) {
    DIR *dir;
    struct dirent *dirEntry;
    struct stat dirEntryStat;

    if ((dir = opendir(dirName)) != NULL){
        while ((dirEntry = readdir(dir)) != NULL) {
            //forming the dir entry absolute path
            char *entryPath = (char *)malloc((strlen(dirName) + strlen(dirEntry->d_name) + 2) * sizeof(char));
            strcpy(entryPath, dirName);
            if (dirName[strlen(dirName) - 1] != '/')
                strcat(entryPath, "/");
            strcat(entryPath, dirEntry->d_name);
            if (lstat(entryPath, &dirEntryStat) == 0) {
                if (dirEntryStat.st_dev == rootDev){
                    //if file is a directory
                    if (dirEntry->d_type == DT_DIR) {
                        if (strcmp(dirEntry->d_name, ".") == 0 || strcmp(dirEntry->d_name, "..") == 0)
                            continue;
                        else {
                            long *tempsiz = siz;
                            long *tempblocks = blocks;
                            long siz = 0;
                            long blocks = 0;
                            dirExplore(programName, entryPath, errorLog, &siz, &blocks);
                            siz += dirEntryStat.st_size;
                            blocks += dirEntryStat.st_blocks;
                            printf("%s %ld %ld\n", entryPath, siz, blocks * blockSize);
                            *tempsiz += siz;
                            *tempblocks +=blocks;
                        }
                    }
                    //if file is a regular file
                    if (dirEntry->d_type == DT_REG) {
                        //if multiple hard links
                        if (dirEntryStat.st_nlink > 1) {
                            if (!inList(listPtr, dirEntryStat.st_ino)) {
                                *siz += dirEntryStat.st_size;
                                *blocks += dirEntryStat.st_blocks;
                                listPush(&listPtr, dirEntryStat.st_ino);
                                //printf("%s %ld %ld\n", entryPath, dirEntryStat.st_size, dirEntryStat.st_blocks * blockSize);
                                //*siz, *blocks * blockSize);
                            }
                        } else {
                            *siz += dirEntryStat.st_size;
                            *blocks += dirEntryStat.st_blocks;
                            //printf("%s %ld %ld\n", entryPath, dirEntryStat.st_size, dirEntryStat.st_blocks * blockSize);  
                        }
                    }
                    //if file is a sym link
                    if (dirEntry->d_type == DT_LNK) {
                        *siz += dirEntryStat.st_size;
                        *blocks += dirEntryStat.st_blocks;
                        //printf("%s %ld %ld\n", entryPath, dirEntryStat.st_size, dirEntryStat.st_blocks * blockSize);
                    }
                    free(entryPath);
                } 
            } else
                fprintf(errorLog, "%s: %s %s\n", programName, dirName, strerror(errno)); 

        }
        closedir(dir);
    } else {
        fprintf(errorLog, "%s: %s %s\n", programName, dirName, strerror(errno));
        /*lstat(dirName, &dirEntryStat);
        totalSize += dirEntryStat.st_size;
        totalBlocks += dirEntryStat.st_blocks;
        printf("%s %ld %ld %ld\n", dirName, dirEntryStat.st_size, totalSize, totalBlocks * blockSize);*/
    }
}

int main(int argc, char *argv[]) {
    FILE *errorLog;
    char *programName = basename(argv[0]);
    long totalSize = 0;
    long totalBlocks = 0;

    if ((errorLog = fopen(errorPath, "w+")) == NULL) {
        fprintf(stderr, "%s: Unable create error log: %s\n", programName, errorPath);
        exit(EXIT_FAILURE);
    }
    if (argc != 2) {
        fprintf(stderr, "%s: %s ./%s [%s]\n", programName, "Incorrect amount of parameters.\nCorrect usage:", programName, "directory");
        exit(EXIT_FAILURE);
    }
    char *dirName;
    dirName = realpath(argv[1], NULL);
    if (dirName == NULL) {
        printf("%s: %s %s\n", programName, argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }
    struct stat entryStatRoot;
    if (lstat(dirName, &entryStatRoot) != 0) {
        fprintf(errorLog, "%s: %s %s\n", programName, dirName, strerror(errno));
        exit(EXIT_FAILURE);
    }
    else
        rootDev = entryStatRoot.st_dev;
    dirExplore(programName, dirName, errorLog, &actualSize, &actualBlocks);
    totalSize = entryStatRoot.st_size + actualSize;
    totalBlocks = entryStatRoot.st_blocks + actualBlocks;
    printf("%s %ld %ld %ld\n", dirName, entryStatRoot.st_size, totalSize, totalBlocks * blockSize);    
    float diskUsageRatio = 0;
    if (totalBlocks == 0)
        diskUsageRatio = 0;
    else
        diskUsageRatio = ((float)(totalSize * 100) / (float)(totalBlocks * blockSize));
    printf("Disk usage coefficient = %f%%\n", diskUsageRatio); 
    dumpError(errorLog);
    free(listPtr);     
}