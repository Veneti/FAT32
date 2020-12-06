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

#define MAX_NUM_ARGUMENTS 3

#define WHITESPACE " \t\n" // We want to split our command line up into tokens \
                           // so we need to define what delimits our tokens.   \
                           // In this case  white space                        \
                           // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255 // The maximum command-line size




int16_t BPB_BytsPerSec;
int8_t BPB_SecPerClus;
int16_t BPB_RsvdSecCnt;
int8_t BPB_NumFATs;
int16_t BPB_FATSz32;

bool file_isOpen = false;

int compare(char *cmdString, char *dirString)
{
    char *dotdot = "..";

    if (strncmp(cmdString, dirString, 2) == 0)
    {
        if (strncpy(cmdString, dirString, 2) == 0)
        {
            return 1;
        }
        else
        {
            return 0;  
        }
    }

    char imgName[12];

    memset(imgName, '\0', 11);
    strncpy(imgName, dirString, 11);

    char input[12];
    memset(input, '\0', 12);
    strncpy(input, cmdString, strlen(cmdString));

    char expandedName[12];
    memset(expandedName, '\0', 12);

    //removes the .png in the file name
    char *tokenized = strtok(input, ".");
    strncpy(expandedName, tokenized, strlen(cmdString));

    tokenized = strtok(NULL, ".");

    if (tokenized)
    {
        strncpy( (char*)(expandedName+8), tokenized, strlen(tokenized));
    }

    int i;
    for (i = 0; i < 11; i++)
    {
        expandedName[i] = toupper(expandedName[i]);
    }

    if (strncmp(expandedName, imgName, 11) == 0)
    {
        return 0;
    }

    return 0;
}

int main()
{
    FILE *filePtr;
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
                filePtr = fopen(token[1], "w");
                if (filePtr == NULL)
                {
                    printf("Error: Improper format. Could not do 'open <%s>'\n", token[1]);
                }
                else
                {
                    file_isOpen = true;
                }

                //read bpb section
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

            }
            else if (strcmp(token[0], "info") == 0)
            {
                // info();
            }
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
                    perror("Error: Improper format. Please put in format close <filename>\n");
                }

                //print out bpb section in decimal and hex

                printf("BPB_BytsPerSec:     %d  %x\n",BPB_BytsPerSec,BPB_BytsPerSec);
                printf("BPB_SecPerClus:     %d  %x\n",BPB_SecPerClus,BPB_SecPerClus);
                printf("BPB_RsvdSecCnt:     %d  %x\n",BPB_RsvdSecCnt,BPB_RsvdSecCnt);
                printf("BPB_NumFATs:        %d  %x\n",BPB_NumFATs,BPB_NumFATs);
                printf("BPB_FATSz32:        %d  %x\n",BPB_FATSz32,BPB_FATSz32);
                

            }
            else if (strcmp(token[0], "stat") == 0)
            {
                if (token[1] != NULL)
                {
                    // stat(token[1]);
                }
                else
                {
                    printf("Error: Improper format. Please put in format stat <filename> or <directory name>\n");
                }
            }
            else if (strcmp(token[0], "get") == 0)
            {
                if (token[1] != NULL)
                {
                    // get(token[1]);
                }
                else
                {
                    printf("Error: Improper format. Please put in format get <filename>\n");
                }
            }
            else if (strcmp(token[0], "cd") == 0)
            {
                if (token[1] != NULL)
                {
                    // cd(token[1]);
                    //call compare
                }
                else
                {
                    printf("Error: Improper format. Please put in format cd <directory>\n");
                }
            }
            else if (strcmp(token[0], "ls") == 0)
            {
                /*
                int i;
                for (i = 0; i < NUM_ENTRIES; i++)
                {
                    char fileName[12];
                    strncpy();
                }*/
            }
            else if (strcmp(token[0], "read") == 0)
            {
                if (token[1] != NULL && token[2] != NULL && token[3] != NULL)
                    printf("read\n");
                    // read(token[1], atoi(token[2]), atoi(token[3]));
                else
                    printf("Error: Improper format. Please put in format read <filename> <position> <number of bytes>\n");
            }
            else if (strcmp(token[0], "exit") == 0)
            {
                // exit(0);
            }
            else
            {
                printf("Error: Invalid Command\n");
            }
        }
  }

  return 0;
}