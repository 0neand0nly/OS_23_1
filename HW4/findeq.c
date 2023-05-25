#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_THREADS 64
#define DEFAULT_BYTES 1024
#define MAX_FILES 1000
#define MAX_PATH_LENGTH 256

int numThreads;
int minSize = DEFAULT_BYTES;
char* outputPath;
int file_out =0;


typedef struct 
{
    char paths[MAX_FILES][MAX_PATH_LENGTH];
    int count;
} FileList;


typedef struct 
{
    FileList * FileList;
    int start;
    int end;
}ThreadArgs;



void getinputs(int argc, char * argv[])
{
    for(int i=1;i<argc;i++)
    {
        if(strcmp(argv[i],"-t")==0)
        {
            i++;
            numThreads= atoi(argv[i]);

            #ifdef DEBUG
            printf("Num Threads: %d\n",numThreads);
            #endif
        }

        if(strcmp(argv[i],"-m")==0)
        {
            i++;
            minSize = atoi(argv[i]);

            #ifdef DEBUG
            printf("Min Bytes: %d\n",minSize);
            #endif
        }

        if(strcmp(argv[i],"-o")==0)
        {
            i++;
            outputPath = argv[i];

            // change the prog to produce output via file
            file_out =1;
        }
        else
        {
            printf("Invalid input USAGE: ./findeq -t [numthreads] -m [mim_size of bytes] -o[outputpath]\n DIR(directory to search)\n");
            break;
        }
    }
}


int main(int argc, char* argv[])
{
    getinputs(argc, argv);

    FileList filelist;
    filelist.count=0;



    return 0;
}