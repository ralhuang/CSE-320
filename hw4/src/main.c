#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>

#include "imprimer.h"
#include "helper.h"
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/signal.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>


//HELPER FUNCTIONS DEFINED


typeNode* findType(char* typeToFind) {
  typeNode* node_ptr = &typeHead;
  if(node_ptr->next == NULL) {
    return NULL;
  }
  node_ptr = node_ptr->next;
  while(node_ptr->next != NULL) {
    if(strcmp(node_ptr->type, typeToFind) != 0) {
      node_ptr = node_ptr->next;
    }
    else {
      return node_ptr;
    }
  }
  if(strcmp(node_ptr->type, typeToFind) == 0) {
    return node_ptr;
  }
  else {
    return NULL;
  }
}

PRINTER* findPrinter(char* nameToFind) {
  for(int i = 0; i < printerNum; i++) {
    if(printerList[i]->name == NULL) {
      return NULL;
    }
    if(strcmp(printerList[i]->name, nameToFind) == 0) {
      return printerList[i];
    }
  }

  return NULL;
}

void printMatrix() {
  for(int i = 0; i < typeIndex; i++) {
    for(int j = 0; j < typeIndex; j++) {
      if(conversionMatrix[i][j] != NULL) {
        printf("%s ", (conversionMatrix[i][j])->programName);
      }
      else
        printf("0 ");
    }
    printf("\n");
  } printf("\n");
  return;
}

int* conversionSearch(char* typeFrom, char* typeTo, int* path) {
  int indexFrom = findType(typeFrom)->index;
  int targetIndex = findType(typeTo)->index;
  int found = 0;
  int visitedCount = 0;
  int visited[typeIndex];
  for (int i = 0; i < typeIndex; i++) {
    visited[i] = -1;
  }
  int queueCount = 0;
  int queue[typeIndex];

  //find list of conversions for typeFrom, in order, add them to a queue, until no more conversions
  visited[indexFrom] = 1;
  visitedCount++;
  queue[queueCount] = indexFrom;
  queueCount++;

  while(visitedCount != typeIndex || found == 0) { //generate path
    //DEQUEUE
    int dequeue = queue[0];
    for(int i = 0; i < queueCount; i++) {
      if(i == (queueCount - 1))
        queue[i] = -1;
      else
        queue[i] = queue[i+1];
    }
    queueCount--;

    //enqueue children of thedequeued
    for(int i = 0; i < typeIndex; i++) {
      if(conversionMatrix[dequeue][i] != NULL) {
        if(visited[i] != 1) { //only enqueue if we haven't visited it before
          queue[queueCount] = i;
          queueCount++;
          path[i] = dequeue;
          visited[i] = 1;
          visitedCount++;
          if(i == targetIndex)
            found = 1;
        }
      }
    }
  }
  return path;
}

int checkConversionSearch(char* typeFrom, char* typeTo, int* path) {
  int indexFrom = findType(typeFrom)->index;
  int targetIndex = findType(typeTo)->index;

  int counter = targetIndex;
  while(path[counter] >= 0) {
    // printf("%d %d\n", counter, path[counter]);
    counter = path[counter];
  }
  // printf("%d %d\n\n", counter, path[counter]);

  if(counter == indexFrom)
    return 1;
  else
    return 0;
}

PRINTER* findEligiblePrinter(PRINTER_SET* printerSet, char* fileType) {

  for(int i = 0; i < printerNum; i++) {
    uint32_t x = *printerSet & ((0x1) << printerList[i]->id);
    if((x != 0x0) && printerList[i]->enabled == 1) { //enabled printer in printerset
      //check if possible to use printer to print (check type and possible conversion)
      int path[typeIndex];
      // printf("typeindex = %d\n", typeIndex);
      for(int i = 0; i < typeIndex; i++) {
          path[i] = -1;
      }
      // printf("%s %s\n", printerList[i]->type, fileType);
      if((strcmp(printerList[i]->type, fileType) == 0) || checkConversionSearch(fileType, printerList[i]->type,
        conversionSearch(fileType, printerList[i]->type, path)) == 1)
          return printerList[i];
    }
  }

  return NULL;
}

void sigchldhandler(int sig) {
  int status;
  while ((status = waitpid((pid_t) (-1), 0, WNOHANG)) > 0) {}
}

void checkUpdateJobs() {
  for(int i = 0; i < jobCount; i++) {
    if(jobList[i]->status == QUEUED && findEligiblePrinter(&(jobList[i]->eligible_printers), jobList[i]->file_type) != NULL) {
      (jobList[i]->status) = RUNNING;
      jobList[i]->chosen_printer = findEligiblePrinter(&(jobList[i]->eligible_printers), jobList[i]->file_type);
      jobList[i]->chosen_printer->busy = 1;

      struct timeval newtime;
      gettimeofday(&newtime, NULL);
      jobList[i]->change_time = newtime;
      char buffer[1024];
      printf("%s\n", imp_format_printer_status(jobList[i]->chosen_printer, buffer, 1024));

      //CREATE SIG HANDLER to capture signals from master process
      pid_t master_pid = 0;

      signal(SIGCHLD, sigchldhandler);
      if((master_pid = fork()) == 0) { //MASTER PROCESS
        setpgid(getpgid(getpid()), getpid());
        jobList[i]->pgid = getpgid(getpid());
        printf("%s\n", imp_format_job_status(jobList[i], buffer, 1024));
        pid_t child_pid;
        int childstatus;
        //IF DO NOT NEED CONVERSION
        if(strcmp(jobList[i]->chosen_printer->type, jobList[i]->file_type) == 0) {

          if((child_pid = fork()) == 0) { //CHILD PROCESS
            int in = open(jobList[i]->file_name, O_RDONLY);
            int connect;
              if((connect = imp_connect_to_printer(jobList[i]->chosen_printer, PRINTER_NORMAL)) != -1) {//if (connect != -1) {
              char* argarray[] = {"/bin/cat", NULL};

              dup2(in, 0); //SWAP STDIN OF this child WITH file
              dup2(connect, 1); //swap STDOUT of this child with connect printer
              if(execv("/bin/cat", argarray) < 0)
                exit(EXIT_FAILURE);
              exit(EXIT_SUCCESS);
            }
            else
              exit(EXIT_FAILURE);
          } else {  //back to master process
            int childstatus2;
            sleep(1);
            waitpid(child_pid, &childstatus, 0);
            printf("Finished waiting for child\n");
            printf("is exited? = %d\n", WIFEXITED(childstatus2));
            printf("exit status = %d\n", WEXITSTATUS(childstatus2));
            if(WEXITSTATUS(childstatus2) == EXIT_SUCCESS) {
              // printf("exit status child = %d\n", WIFEXITED(childstatus2));
              exit(EXIT_SUCCESS);
            }
            else
              exit(EXIT_FAILURE);
          }

        } else {

        //IF NEED CONVERSION

          if((child_pid = fork()) == 0) { //CHILD PROCESS
            exit(EXIT_SUCCESS);

          } else {
            int childstatus2;
            sleep(1);
            waitpid(child_pid, &childstatus2, 0);
            if(WIFEXITED(childstatus2) == EXIT_SUCCESS)
              exit(EXIT_SUCCESS);
            else
              exit(EXIT_FAILURE);
          }
        }

      } else {  //MAIN PROCESS
        int masterstatus;
        sleep(2);
        if(WEXITSTATUS(masterstatus) == EXIT_SUCCESS) {
          printf("%d\n", WEXITSTATUS(masterstatus));
          (jobList[i]->status) = COMPLETED;
          jobList[i]->chosen_printer->busy = 0;
          jobList[i]->chosen_printer = NULL;
          gettimeofday(&newtime, NULL);
          jobList[i]->change_time = newtime;
          printf("%s\n", imp_format_job_status(jobList[i], buffer, 1024));
        }

        else if(WEXITSTATUS(masterstatus) == EXIT_FAILURE) {
          printf("%d\n", WEXITSTATUS(masterstatus));
          (jobList[i]->status) = ABORTED;

          jobList[i]->chosen_printer->busy = 0;
          jobList[i]->chosen_printer = NULL;
          gettimeofday(&newtime, NULL);
          jobList[i]->change_time = newtime;
          printf("%s\n", imp_format_job_status(jobList[i], buffer, 1024));
        }

      }
    }
  }
}

void runMain(char* inFile, int mode) {
  int runningStatus = 1;
  char* tokens;
  char* prompt;
  printerNum = 0;
  typeIndex = 0;
  jobCount = 0;
  FILE* out = stdout;
  FILE* f;
  if(mode == 1) {
    f = fopen(inFile, "r");
  } else if (mode == 2) {
    out = fopen(inFile, "w");
  }


  while(runningStatus == 1) {
    if(inFile == NULL) {
      prompt = readline("imp> ");
    }
    else if (mode == 1) { //BATCH MODE
      if(f == NULL) {
        printf("File does not exist\n");
        runningStatus = 0;
        break;
      }
      if(runningStatus == 1) {
        char lineBuffer[256];
        prompt = fgets(lineBuffer, 256, f);
        if(prompt != NULL)
          prompt = strtok(prompt, "\n");
      }
      if (prompt == NULL) {
        runningStatus = 0;
      }
    } else if (mode == 2) { //OUTPUT FILE
      printf("in output\n");
      prompt = readline("imp> ");

      if(out == NULL) {
        printf("File does not exist\n");
        runningStatus = 0;
        break;
      }
    }

    void* promptPointer = prompt;
    if(runningStatus == 1)
      tokens = strtok(prompt, " ");
    if(tokens != NULL) {
    if (strcmp(tokens, "quit") == 0) { //QUIT

      //FREE THE TYPE LIST
      typeNode* node_ptr = &typeHead;
      free(promptPointer);
      while(node_ptr->next != NULL) {
        free(node_ptr->next->type);
        node_ptr = node_ptr->next;
      }

      //FREE THE PRINTERS
      for(int i = 0; i < printerNum; i++) {
        free(printerList[i]->name);
        free(printerList[i]->type);
        free(printerList[i]);
      }

      fprintf(out, "Quitting...\n");
      runningStatus = 0;
      break;
    } else if (strcmp(tokens, "help") == 0) { //HELP
      if(strtok(NULL, " ") == NULL)
        fprintf(out, "List of commands: \n- help\n- quit\n- type\n- printer\n- conversion\n- printers\n- jobs\n- print\n- cancel\n- pause\n- resume\n- disable\n- enable\n");
      else {
        char errorbuf[1024];
        fprintf(out, "%s\n", imp_format_error_message("<help> takes no args\n", errorbuf, 1024));
      }
    } else if (strcmp(tokens, "type") == 0) { //TYPE

      char* typeToken = strtok(NULL, " ");
      if(typeToken != NULL) {
        if(strtok(NULL, " ") == NULL) {
          typeNode* node_ptr = &(typeHead);
          int found = 0;
          while(node_ptr->next != NULL) {

            if(strcmp(node_ptr->next->type, typeToken) == 0) {
              char errorbuf[1024];
              fprintf(out, "%s\n", imp_format_error_message("Type already exists\n", errorbuf, 1024));
              found = 1;
              break;
            }
            node_ptr = node_ptr->next;
          }

          if(found == 0) {

            void* newType = calloc(1, sizeof(typeNode));
            void* typeValue = calloc(1, strlen(typeToken)+1);
            strcpy((char*)typeValue, typeToken);

            ((typeNode*)newType)->type = (char*)typeValue;
            if(typeHead.next != NULL) {
              ((typeNode*)newType)->next = typeHead.next;
            }
            else
              ((typeNode*)newType)->next = NULL;
            typeHead.next = (typeNode*)newType;
            ((typeNode*)newType)->index = typeIndex;
            typeIndex++;
          }
          node_ptr = &(typeHead);
          while(node_ptr->next != NULL) {
            node_ptr = node_ptr->next;
          }
        } else {
          char errorbuf[1024];
          fprintf(out, "%s\n", imp_format_error_message("<type> should only have one file type argument\n", errorbuf, 1024));
        }
      } else {
        char errorbuf[1024];
        fprintf(out, "%s\n", imp_format_error_message("<type> needs another argument\n", errorbuf, 1024));
        }

    } else if (strcmp(tokens, "printer") == 0) { //PRINTER//
        char* printerTok = strtok(NULL, " ");
        char* fileTok = strtok(NULL, " ");
        if(printerTok != NULL && fileTok != NULL) {
        if(strtok(NULL, " ") != NULL) {
          char errorbuf[1024];
          fprintf(out, "%s\n", imp_format_error_message("Usage: <printer> <printer name> <printer file type>\n", errorbuf, 1024));
        } else {
          if(printerNum >= 32) {
          char errorbuf[1024];
          fprintf(out, "%s\n", imp_format_error_message("Too many printers.", errorbuf, 1024));
        }
          else {
            if(findPrinter(printerTok) != NULL) {
              char errorbuf[1024];
              fprintf(out, "%s\n", imp_format_error_message("There already exists a printer with that name.\n", errorbuf, 1024));
            } else {
              if(findType(fileTok) != NULL) { //CREATE A NEW PRINTER
                void* printerName = calloc(1, strlen(printerTok)+1);
                void* fileType = calloc(1, strlen(fileTok)+1);
                strcpy(printerName, printerTok);
                strcpy(fileType, fileTok);
                void* newPP = malloc(sizeof(PRINTER));
                ((PRINTER*)newPP)->id = printerNum;
                ((PRINTER*)newPP)->name = printerName;
                ((PRINTER*)newPP)->type = fileType;
                ((PRINTER*)newPP)->enabled = 0;
                ((PRINTER*)newPP)->busy = 0;
                printerList[printerNum] = ((PRINTER*)newPP);
                char printerStatusBuffer[64];
                int size = 64;
                fprintf(out,"%s\n", imp_format_printer_status(printerList[printerNum], printerStatusBuffer, size));
                printerNum++;
              } else {
                char errorbuf[1024];
                fprintf(out, "%s\n", imp_format_error_message("Type is not supported by program. Please add it by typing: type <type_name>", errorbuf, 1024));
              }
            }
          }
        }
      } else {
        char errorbuf[1024];
        fprintf(out, "%s\n", imp_format_error_message("Invalid command.\nDid you forget to type the printer name and/or support file type?", errorbuf, 1024));
      }
    } else if (strcmp(tokens, "printers") == 0){ //PRINTERS//
      if(strtok(NULL, " ") != NULL) {
        char errorbuf[1024];
        fprintf(out, "%s\n", imp_format_error_message("USAGE: <printers> does not take args", errorbuf, 1024));
      }
      else {
        if(printerNum == 0) {
          char errorbuf[1024];
          fprintf(out, "%s\n", imp_format_error_message("There are no printers\n", errorbuf, 1024));
        }
        for(int i = 0; i < printerNum; i++) {
          char printerStatusBuffer[64];
          int size = 64;
          fprintf(out,"%s\n", imp_format_printer_status(printerList[i], printerStatusBuffer, size));
        }
      }
    } else if (strcmp(tokens, "conversion") == 0) { //CONVERSION

      char* convertFrom = strtok(NULL, " ");
      char* convertTo = strtok(NULL, " ");
      char* programTok = strtok(NULL, " ");
      if (convertFrom == NULL || convertTo == NULL || programTok == NULL) { //if any arguments are not present, print error
        char errorbuf[1024];
        fprintf(out, "%s\n", imp_format_error_message("USAGE: conversion <file_type1> <file_type2> <program name>", errorbuf, 1024));
      } else {  //if all args are present, continue
        if(strcmp(convertFrom, convertTo) != 0) {
          typeNode* typeFrom = findType(convertFrom);
          typeNode* typeTo = findType(convertTo);
          if(typeFrom != NULL && typeTo != NULL) {
            if(conversionMatrix[typeFrom->index][typeTo->index] == NULL) {
              void* newConversion = calloc(1, sizeof(conversionInfo));
              void* programName = calloc(1, strlen(programTok)+1);
              strcpy(programName, programTok);
              ((conversionInfo*)newConversion)->programName = programName;
              char* argTok = strtok(NULL, " ");
              int argLoop = 1;
              int argIndex = 0;
              while(argLoop == 1) { //SET ARG ARRAY
                if(argTok != NULL) {
                  void* conversionArg = malloc(strlen(argTok)+1);
                  strcpy(conversionArg, argTok);
                  ((conversionInfo*)newConversion)->proArgs[argIndex] = (char*)conversionArg;
                  argTok = strtok(NULL, " ");
                  argIndex++;
                } else
                  argLoop = 0;
              }

              conversionMatrix[typeFrom->index][typeTo->index] = ((conversionInfo*)newConversion);

              //CHECK IF ANY QUEUED JOBS CAN NOW BE STARTED
              checkUpdateJobs();
            } else {
              if(conversionMatrix[typeFrom->index][typeTo->index]->programName != NULL) {
                char errorbuf[1024];
                fprintf(out, "%s\n", imp_format_error_message("There is already a program that converts these two types\n", errorbuf, 1024));
              }
            }
          } else {
              char errorbuf[1024];
              fprintf(out, "%s\n", imp_format_error_message("At least one of those types do not exist.\n", errorbuf, 1024));
            }
          } else {
            char errorbuf[1024];
            fprintf(out, "%s\n", imp_format_error_message("Please do not try to convert identical files\n", errorbuf, 1024));
          }
        }
      // printMatrix(); // TESTING CONVERSION MATRIX
    } else if (strcmp(tokens, "print") == 0) {  //PRINT JOB//
      int printerError = 0;
      while(printerError == 0) {
        //VALIDATE FILE AND SEPARATE INTO FILE NAME AND TYPE
        char* printFile = strtok(NULL, " ");
        char* filePointer = printFile;
        if(strrchr(filePointer, '.') != NULL) {
          if(printFile != NULL) {
            void* newJob = calloc(1, sizeof(JOB));

            char* specifiedPrinter = strtok(NULL, " ");
            // if(specifiedPrinter != NULL && findPrinter(specifiedPrinter) != NULL) {
              PRINTER_SET newPrinterSet = 0;

              ((JOB*)newJob)->jobid = jobCount;
              ((JOB*)newJob)->status = QUEUED;

              //DEFINING ELIGIBLE PRINTERS
              if(specifiedPrinter != NULL) {
                // printf("test\n");
                while(specifiedPrinter != NULL) {
                  PRINTER* x = findPrinter(specifiedPrinter);
                  if(x == NULL) {
                    char errorbuf[1024];
                    fprintf(out, "%s\n", imp_format_error_message("Printer not found\n", errorbuf, 1024));
                    free(newJob);
                    printerError = 1;
                    jobCount--;
                    break;
                  }

                  newPrinterSet ^= (0x1) << findPrinter(specifiedPrinter)->id;
                  specifiedPrinter = strtok(NULL, " ");
                }
                if(printerError == 1)
                  break;
              } else
                newPrinterSet = ANY_PRINTER;


                //setting file name and extension
              char* fileName = (char*)malloc(strlen(printFile)+1);
              strcpy(fileName, printFile);
              strtok(printFile, ".");
              char* fileExtTok = strtok(NULL, ".");
              void* fileExt = calloc(1, strlen(fileExtTok)+1);
              strcpy(fileExt, fileExtTok);
              ((JOB*)newJob)->file_name = fileName;
              ((JOB*)newJob)->file_type = fileExt;

              ((JOB*)newJob)->eligible_printers = newPrinterSet;
              struct timeval newtv;
              gettimeofday(&newtv, NULL);
              ((JOB*)newJob)->creation_time = newtv;
              ((JOB*)newJob)->change_time = newtv;
              jobList[jobCount] = ((JOB*)newJob);
              jobCount++;
              for(int i = 0; i < jobCount; i++) {
                char buffer[1024];
                fprintf(out, "%s\n", imp_format_job_status(jobList[i], buffer, 1024));
              }
              checkUpdateJobs();
              printerError = 1;

          } else {
            char errorbuf[1024];
            fprintf(out, "%s\n", imp_format_error_message("USAGE: <print> <file_name> [optional args: <printer_name1> ... ]", errorbuf, 1024));
            printerError = 1;
          }
        } else {
          char errorbuf[1024];
          fprintf(out, "%s\n", imp_format_error_message("File name needs an extension.\n", errorbuf, 1024));
          printerError = 1;
        }
      }

    } else if (strcmp(tokens, "jobs") == 0) {
      for(int i = 0; i < jobCount; i++) {
        // printf("eligible_printers %x\n", jobList[i]->eligible_printers);
        char buffer[1024];
        fprintf(out, "%s\n", imp_format_job_status(jobList[i], buffer, 1024));
      }

    } else if (strcmp(tokens, "enable") == 0) { //ENABLE PRINTER
      char* printerName = strtok(NULL," ");
      if(strtok(NULL, " ") != NULL) {
        char errorbuf[1024];
        fprintf(out, "%s\n", imp_format_error_message("USAGE: <enable> <printer_name>\n", errorbuf, 1024));
      }
      if(findPrinter(printerName) != NULL) {

        if(findPrinter(printerName)->enabled == 0) {
          findPrinter(printerName)->enabled = 1;
          char buffer[1024];
          fprintf(out, "%s\n", imp_format_printer_status(findPrinter(printerName), buffer, 1024));
          checkUpdateJobs();
        }
        else {
          char errorbuf[1024];
          fprintf(out, "%s\n", imp_format_error_message("Already enabled\n", errorbuf, 1024));
        }
      } else {
        char errorbuf[1024];
        fprintf(out, "%s\n", imp_format_error_message("No printer with that name found\n", errorbuf, 1024));
      }

    } else if (strcmp(tokens, "disable") == 0) { //DISABLE PRINTER
      char* printerName = strtok(NULL," ");
      if(strtok(NULL, " ") != NULL) {
        char errorbuf[1024];
        fprintf(out, "%s\n", imp_format_error_message("USAGE: <disable> <printer_name>\n", errorbuf, 1024));
      }
      if(findPrinter(printerName) != NULL) {

        if (findPrinter(printerName)->enabled == 1) {
          findPrinter(printerName)->enabled = 0;
          char buffer[1024];
          fprintf(out, "%s\n", imp_format_printer_status(findPrinter(printerName), buffer, 1024));
        }
        else {
          char errorbuf[1024];
          fprintf(out, "%s\n", imp_format_error_message("Already disabled\n", errorbuf, 1024));
        }
      } else {
        char errorbuf[1024];
        fprintf(out, "%s\n", imp_format_error_message("No printer with that name found\n", errorbuf, 1024));
      }

    } else if (strcmp(tokens, "pause") == 0) {
      fprintf(out, "Sorry, this function is not implemented yet.\n");

    } else if (strcmp(tokens, "cancel") == 0) {

      fprintf(out, "Sorry, this function is not implemented yet.\n");

    } else if (strcmp(tokens, "resume") == 0) {
      fprintf(out, "Sorry, this function is not implemented yet.\n");

    }
     else { //INVALID COMMAND
      if(runningStatus == 1)
        fprintf(out, "List of valid commands: \n- help\n- quit\n- type\n- printer\n- conversion\n- printers\n- jobs\n- print\n- cancel\n- pause\n- resume\n- disable\n- enable\n");
    }
  }
    if(mode != 1)
      free(promptPointer);
  }

}
/*
 * "Imprimer" printer spooler.
 */

int main(int argc, char *argv[]) {

  char optval;
  //DEFAULT CASE WITHOUT ANY ARGS TO BIN/IMPRIMER
  if(argc == 1) {
    //run shell
    runMain(NULL, 0);
    return 1;
  }

  while(optind < argc) {
  	if((optval = getopt(argc, argv, "i:o:?")) != -1) {
  	    switch(optval) {
          case 'i':
            runMain(optarg, 1);
            break;
          case 'o':
            runMain(optarg, 2);
            break;
    	    case '?':
            fprintf(stderr, "Usage: %s [-i <cmd_file>] [-o <out_file>]\n", argv[0]);
    		    exit(EXIT_FAILURE);
    		    break;
    	    default:
    		    break;
  	    }
  	}
  }
  exit(EXIT_SUCCESS);
}
