////////////////////////////////
//    Name: Brian Truong      //
//    ID: 1001549574          //
//                            //
//    Name: Tyler Do          //
//    ID: 1001553345          //
//                            //
//    Class: CSE 3301 - 002   //
//                            //
////////////////////////////////

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>

#define MAX_NUM_ARGUMENTS 16

#define WHITESPACE " \t\n" // We want to split our command line up into tokens \
                           // so we need to define what delimits our tokens.   \
                           // In this case  white space                        \
                           // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255 // The maximum command-line size


//Structure from twitch office hour
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEM     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHINVE   0x20
//0x40 and 0x80 are unused


//Structure from twitch office hour
struct __attribute__ ( ( __packed__ ) ) DirectoryEntry
{
    char    DIR_NAME[11];
    uint8_t DIR_Attr;
    uint8_t Unused1[8];
    uint16_t DIR_FirstClusterHigh;
    uint8_t Unused2[4];
    uint16_t DIR_FirstClusterLow;
    uint32_t DIR_FileSize;
};
struct DirectoryEntry dir[16];


int16_t  BPB_BytsPerSec;
int8_t   BPB_SecPerClus;
int16_t  BPB_RsvdSecCnt;
int8_t   BPB_NumFATs;
int16_t  BPB_FATSz32;

bool file_isOpen = false;
FILE *filePtr;

char correctDirectory[12];
void directoryFormat(char *dirString);
// void getFile(char *originalFilename, char *newFilename);
// void get(char *dirname);
int32_t clusterAtribute(char *dirString);
int32_t getSizeOfCluster(int32_t cluster);

/*
    Finds the starting address of the block of the sector (FAT slides)
*/
int32_t LBAToOffset(int32_t sector)
{
    return ((sector -2)* BPB_BytsPerSec) + (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) 
        + (BPB_RsvdSecCnt * BPB_BytsPerSec);
}

/*
    looks up into first FAT and returns logical block address (FAT slides)
*/
int16_t nextLB(int32_t sector)
{
    uint32_t FATAddress = (BPB_BytsPerSec * BPB_RsvdSecCnt) + (sector * 4);
    int16_t val;
    fseek(filePtr, FATAddress, SEEK_SET);
    fread( &val, 2, 1, filePtr);
    return val;
}

//from twitch office hours
int compare(char *cmdString, char *dirString)
{
    char *dotdot = "..";

    if (strncmp(dotdot, cmdString, 2) == 0)
    {
        if (strncmp(cmdString, dirString, 2) == 0)
        {
            return 1;
        }
        else
        {
            return 0;  
        }
    }

    char imgName[12];

    strncpy(imgName, dirString, 11);
    imgName[11] = '\0';
    

    char input[16];
    memset(input, 0, 16);
    strncpy(input, cmdString, strlen(cmdString));
    //expandedName[11] = '\0';


    char expandedName[12];
    memset(expandedName, ' ', 12);

    //removes the .png in the file name
    char *tokenized = strtok(input, ".");
    strncpy(expandedName, tokenized, strlen(cmdString));

    tokenized = strtok(NULL, ".");

    if (tokenized)
    {
        strncpy( (char*)(expandedName+8), tokenized, strlen(tokenized));
    }
    expandedName[11] = '\0';

    int i;
    for (i = 0; i < 11; i++)
    {
        expandedName[i] = toupper(expandedName[i]);
    }
    //printf("%s\n", expandedName);
    if (strncmp(expandedName, imgName, 11) == 0)
    {
        return 1;
    }

    return 0;
}

int main()
{
    char *cmd_str = (char *)malloc(MAX_COMMAND_SIZE);

    while (1)
    {
        // Print out the mfs prompt
        printf("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while (!fgets(cmd_str, MAX_COMMAND_SIZE, stdin))
      ;

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int token_count = 0;

    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;

    char *working_str = strdup(cmd_str);

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while (((arg_ptr = strsep(&working_str, WHITESPACE)) != NULL) &&
           (token_count < MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup(arg_ptr, MAX_COMMAND_SIZE);
      if (strlen(token[token_count]) == 0)
      {
        token[token_count] = NULL;
      }
      token_count++;
    }

     if (token[0] != NULL)
        {
            if (strcmp(token[0], "open") == 0)
            {
                if (file_isOpen == true)
                {
                    printf("Error: File system image already open.\n");
                }
                else
                {
                    filePtr = fopen(token[1], "r");
                    if (filePtr == NULL)
                    {
                        printf("Error: Improper format. Could not do 'open <%s>'\n", token[1]);
                    }
                    else
                    {
                        file_isOpen = true;
                    }

                    //read bpb section from reference byte
                    fseek(filePtr, 11, SEEK_SET);
                    fread(&BPB_BytsPerSec, 1, 2, filePtr);

                    fseek(filePtr, 13, SEEK_SET);
                    fread(&BPB_SecPerClus, 1, 2, filePtr);

                    fseek(filePtr, 14, SEEK_SET);
                    fread(&BPB_RsvdSecCnt, 1, 2, filePtr);

                    fseek(filePtr, 16, SEEK_SET);
                    fread(&BPB_NumFATs, 1, 2, filePtr);

                    fseek(filePtr, 36, SEEK_SET);
                    fread(&BPB_FATSz32, 1, 4, filePtr);

                    //root dir address is losted in both reserved sector and FATs
                    int rootAddress = ( BPB_RsvdSecCnt * BPB_BytsPerSec) + 
                            (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec);

                    //printf("%x\n", rootAddress);
                    fseek(filePtr, rootAddress, SEEK_SET);
                    fread(dir, sizeof(struct DirectoryEntry), 16, filePtr);
                }
                


            }
            // m
            else if (strcmp(token[0], "close") == 0)
            {
                if (file_isOpen)
                {
                    fclose(filePtr);
                    
                    filePtr = NULL;
                    file_isOpen = 0;
                }
                else
                {
                    perror("Error: File system not open.\n");
                }
            }
            else if (strcmp(token[0], "bpb") == 0)
            {
                if (file_isOpen)
                {
                    // char *line = "----------------------------------------------";
                    //print out bpb section in decimal and hex
                    // printf("\nBPB Stats Output    %-8s  %-10s\n%s\n","Decimal","Hex","----------
                    // ------------------------------------");
                    printf("BPB_BytsPerSec:     %-8d  0x%-8x\n",BPB_BytsPerSec,BPB_BytsPerSec);
                    printf("BPB_SecPerClus:     %-8d  0x%-8x\n",BPB_SecPerClus,BPB_SecPerClus);
                    printf("BPB_RsvdSecCnt:     %-8d  0x%-8x\n",BPB_RsvdSecCnt,BPB_RsvdSecCnt);
                    printf("BPB_NumFATs:        %-8d  0x%-8x\n",BPB_NumFATs,BPB_NumFATs);
                    printf("BPB_FATSz32:        %-8d  0x%-8x\n",BPB_FATSz32,BPB_FATSz32);
                }
                else
                {
                    printf("Error: File img is not open yet.\n");

                }
            }
            else if (strcmp(token[0], "stat") == 0)
            {
                if (file_isOpen)
                {
                    int cluster = clusterAtribute(token[1]);
                    
                    int i;
                    for (i = 0; i < 16; i++)
                    {
                        if (cluster == dir[i].DIR_FirstClusterLow)
                        {
                            printf("File Attribute: %d\n", dir[i].DIR_Attr);
                            printf("File Size: %d\n", dir[i].DIR_FileSize);
                            printf("Starting Cluster Number: %d\n", cluster);
                        }
                    }
                    
                }
                else
                {
                    printf("Error: No files are currently open.\n");
                }
            }
            else if (strcmp(token[0], "get") == 0)
            {
                if (token[1] == NULL)
                {
                    printf("Error: Missing Parameters - get <filename>\n");
                }
                else if (file_isOpen)
                {
                    char *dirstring = (char *)malloc(strlen(token[1]));
                    strncpy(dirstring, token[1], strlen(token[1]));
                    int cluster = clusterAtribute(dirstring);
                    int size = getSizeOfCluster(cluster);
                    FILE *newfp = fopen(token[1], "w");
                    if(token[2] == NULL)
                    {
                        fseek(filePtr, LBAToOffset(cluster), SEEK_SET);
                        unsigned char *ptr = new unsigned char(size);
                        fread(ptr, size, 1, filePtr);
                        fwrite(ptr, size, 1, newfp);
                        fclose(newfp);
                    }
                    else
                    {
                        newfp = fopen(token[2], "w");
                        fseek(filePtr, LBAToOffset(cluster), SEEK_SET);
                        unsigned char *ptr = new unsigned char(size);
                        fread(ptr, size, 1, filePtr);
                        fwrite(ptr, size, 1, newfp);
                        fclose(newfp);
                    }
                                
                }
                else
                {
                    printf("Error: No files are currently open.\n");
                }
            }

            //from twitch office hours
            else if (strcmp(token[0], "cd") == 0)
            {
                if (file_isOpen)
                {
                    int i = 0, found = 0;
                    for (i = 0; i < 16; i++)
                    {
                        if (compare(token[1], dir[i].DIR_NAME))
                        {
                            int clusterLow = dir[i].DIR_FirstClusterLow;

                            if (clusterLow == 0)
                            {
                                clusterLow = 2 ;
                            }

                            /*int offset = ((clusterLow -2)* BPB_BytsPerSec) + 
                                    (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) + 
                                    (BPB_RsvdSecCnt * BPB_BytsPerSec);*/
                            
                            int offset = LBAToOffset(clusterLow);

                            fseek(filePtr, offset, SEEK_SET);
                            fread(dir, sizeof(struct DirectoryEntry), 16, filePtr);

                            found = 1;
                            break;
                        }
                    }
                    if (found == 0)
                    {
                        printf("Error: The directory is not found.\n");
                    }
                }
                else
                {
                    printf("Error: File img is not open yet.\n");
                }
            }
            else if (strcmp(token[0], "ls") == 0)
            {
                if (file_isOpen)
                {
                    int i = 0;
                    for (i = 0; i < 16; i++)
                    {
                        char fileName[12];
                        memset(fileName, '\0', 11);
                        strncpy(fileName, dir[i].DIR_NAME, 11);

                        if ((dir[i].DIR_Attr == ATTR_READ_ONLY || dir[i].DIR_Attr == ATTR_DIRECTORY 
                            || dir[i].DIR_Attr == ATTR_ARCHINVE) && fileName[0] != 0xffffffe5)
                        {
                            printf("%s\n", fileName);
                        }
                    }
                }
                else
                {
                    printf("Error: File img is not open yet.\n");
                }
            }
            else if (strcmp(token[0], "read") == 0)
            {
                if (token[1] == NULL || token[2] == NULL || token[3] == NULL)
                {
                    printf("Error: Missing Parameters - read <filename> <position> <number of bytes>\n");
                }
                else if (file_isOpen)
                {
                    int i = 0;
                    int found = 0;
                    
                    int requestedOffset = atoi(token[2]);

                    int requestedBytes = atoi(token[3]);
                    // printf("compare.\n");

                    int bytesRaminingToRead = requestedBytes;

                    if (requestedOffset < 0 )
                    {
                        printf("Error: Requested offset can not be less than 0.\n");
                        printf("break.\n");

                        break;
                    }

                    for (i = 0; i < 16; i++)
                    {
                        char *fileToken = token[1];
                        char readfile[12];
                        strncpy(readfile, token[1], 12);
                        char DIRNAME[12];
                        
                        memcpy(DIRNAME, dir[i].DIR_NAME, 12);
												directoryFormat(readfile);
                        
                        if (compare(readfile, dir[i].DIR_NAME))
                        {
                            printf("hey we didnt get read but the code is the comments\n");
                            char readResults;
                            int cluster = dir[i].DIR_FirstClusterLow;
                            int clusterOffset = LBAToOffset(cluster);
                            int counter;

                            // for (counter = 0; counter < dir[i].DIR_FileSize; counter++)
                            // {
                            // 	if (counter % 512 == 0 && counter != 0)
                            // 	{
                            // 		cluster = nextLB(cluster);
                            // 		clusterOffset = LBAToOffset(cluster);
                            // 	}
                            // 	fseek(filePtr, clusterOffset++, SEEK_SET);
                            // 	fread(&readResults, 1, 1, filePtr);
                            // 	if (counter >= requestedOffset && counter < (requestedOffset + requestedBytes))
                            // 	{
                            // 		printf("%s ", readResults);
                            // 	}



                            // found = 1;
                            // int searchSize = requestedOffset;

                            // while (searchSize >= BPB_BytsPerSec)
                            // {
                            //     cluster = nextLB(cluster);
                            //     searchSize = searchSize - BPB_BytsPerSec;

                            // }

                            // int offset = LBAToOffset(cluster);
                            // int byteOffset = (requestedOffset % BPB_BytsPerSec);

                            // fseek(filePtr, offset + byteOffset, SEEK_SET);

                            // unsigned char buffer[BPB_BytsPerSec];
                            // fread(buffer, 1, byteOffset, filePtr);

                            // for (i = 0; i < byteOffset; i++);
                            // {
                            //     printf("%x ", buffer[i]);
                            // }
                            // printf("\n");
                        }
                    }
                }
            }
                else
                {
                    printf("Error: No files are currently open.\n");
                }
            }
            else if (strcmp(token[0], "quit") == 0 || strcmp(token[0], "exit") == 0)
            {
                exit(0);
            }
            else
            {
                printf("Error: Invalid Command\n");
            }
        }
  return 0;
}

void directoryFormat(char *dirString)
{
	char source[12];
	memset(source, ' ', 12);
	
	char *fileName = strtok(dirString, ".");

	if(fileName)
	{
		strncpy(source, fileName, strlen(fileName));
		fileName = strtok(NULL, ".");

		if(fileName)
		{
			strncpy((char *)(source + 8), fileName, strlen(fileName));
		}
		
		source[11] = '\0';

		int i;
		for (i = 0; i < 11; i++)
		{
			source[i] = toupper(source[i]);
		}
	}
	else
	{
		strncpy(source, dirString, strlen(dirString));
		source[11] = '\0';
	}

	strncpy(correctDirectory, source, 12);
}

int32_t clusterAtribute(char *dirString)
{
	directoryFormat(dirString);
	int i;
	for (i = 0; i < 16; i++)
	{
		char *directory = (char*)malloc(11);
		memset(directory, '\0', 11);
		memcpy(directory, dir[i].DIR_NAME, 11);
		
		if (strncmp(directory, correctDirectory, 11) == 0)
		{
			int firstClusterLow = dir[i].DIR_FirstClusterLow;
			return firstClusterLow;
		}
	}
	return -1;
}

int32_t getSizeOfCluster(int32_t cluster)
{
    int i;
    for (i = 0; i < 16; i++)
    {
        if (cluster == dir[i].DIR_FirstClusterLow)
        {
            int size = dir[i].DIR_FileSize;
            return size;
        }
    }
    return -1;
}