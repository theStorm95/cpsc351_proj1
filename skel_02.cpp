#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

// child to parent communication pipe
int childToParent[2];

// parent to child communication pipe
int parentToChild[2];

// max file name length
#define FILE_NAME_MAX 1000

// write end pipe
#define PIPE_WRITE_END 1

// read end pipe
#define PIPE_READ_END 0

// max size of array of hash programs
#define HASH_ARRAY_SIZE 6

// max hash value length
#define HASH_VAL_LENGTH 1000


// array of hash programs (alphanumerical order)
const string hashProgram[] = {"md5sum", "sha1sum", "sha224sum", "sha256sum", "sha384sum", "sha512sum"};

//toHash called by child
void toHash(const string& programName)
{

        // buffer for hash value
        char hashVal[HASH_VAL_LENGTH];

        //received file name (as a string)
        char file_Name_Input[FILE_NAME_MAX];

        //Zeros inserted into the buffer
        memset(file_Name_Input, (char)NULL, FILE_NAME_MAX);

        //Reads message received from parent
        if(read(parentToChild[PIPE_READ_END], file_Name_Input, FILE_NAME_MAX) < 0){
                perror("Read");
                exit(-1);
        }

       // command line is put together
        string cmdLine(programName);
        cmdLine += " ";
        cmdLine += file_Name_Input;

        //open pipe to program using command line
        FILE* hashOut = popen(cmdLine.c_str(), "r");
        if(!hashOut){
                perror("popen");
                exit(-1);
        }

        // Reset the hash buffer
        memset(hashVal, (char)NULL, HASH_VAL_LENGTH);
        if(fread(hashVal, sizeof(char), sizeof(char) * HASH_VAL_LENGTH, hashOut) < 0){
                perror("Fread");
                exit(-1);
        }

        if(pclose(hashOut) < 0){
                perror("Pclose");
                exit(-1);
        }

        //sends string to parents
        if(write(childToParent[PIPE_WRITE_END], hashVal, HASH_VAL_LENGTH) < 0){
                perror("Write");
                exit(-1);
        }

        //child terminated
        exit(0);

}


int main(int argc, char** argv)
{

        //error checking
        if(argc < 2)
        {
                fprintf(stderr, "USE: %s <FILE NAME>\n", argv[0]);
                exit(-1);
        }

        //saves the file name
        string file_name(argv[1]);

        // creates process ID (see pid fork example)
        pid_t pid;

        // uses for loop to run program for each hashing algortihm
        for(int hashAlg = 0; hashAlg < HASH_ARRAY_SIZE; ++hashAlg)
        {

                
                // Create Parent Child Pipe(s)
                if(pipe(parentToChild) < 0 || pipe(childToParent) < 0){
                        perror("Pipe");
                        exit(-1);
                }

                //forks child process and saves id
                if((pid = fork()) < 0)
                {
                        perror("Fork");
                        exit(-1);
                }
                //is the child
                else if(pid == 0)

                {
                        //closes hanging ends of pipe
                        if(close(parentToChild[PIPE_WRITE_END]) < 0 ||
                                close(childToParent[PIPE_READ_END]) < 0){
                                perror("Close");
                                exit(-1);
                        }

                        toHash(hashProgram[hashAlg]);
                }

                //is the Parent

                //closes hanging ends of pipes
                if(close(parentToChild[PIPE_READ_END]) < 0 ||
                        close(childToParent[PIPE_WRITE_END]) < 0){
                        perror("Close");
                        exit(-1);
                }

                // string received from child placed in buffer
                char hashVal[HASH_VAL_LENGTH];

                // resets hash buffer
                memset(hashVal, (char)NULL, HASH_VAL_LENGTH);

                //sends string to child
                if(write(parentToChild[PIPE_WRITE_END], file_name.c_str(), sizeof(file_name)) < 0){
                        perror("Write");
                        exit(-1);
                }

                // now, read string send by child
                if(read(childToParent[PIPE_READ_END], hashVal, HASH_VAL_LENGTH) < 0){
                        perror("Read");
                        exit(-1);
                }

                //Prints outputted hash value
                fprintf(stdout, "%s HASH VALUE is: %s\n", hashProgram[hashAlg].c_str(), hashVal);
                fflush(stdout);

                if(wait(NULL) < 0)
                {
                        perror("Wait");
                        exit(-1);
                }
        }

        return 0;
}
