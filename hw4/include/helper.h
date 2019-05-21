#include <stdint.h>
#include <stdlib.h>

void runMain(char* line, int mode);

typedef struct typeNode {
  char* type;
  int index;
  struct typeNode* next;
} typeNode;
int typeIndex;

typeNode typeHead;

PRINTER* printerList[MAX_PRINTERS];
JOB* jobList[1024];
int jobCount;

typeNode* findType(char* typeToFind);

PRINTER* findPrinter(char* nameToFind);
int printerNum;

//Struct of conversion info
typedef struct conversionInfo {
  char* programName;
  char* proArgs[128];
} conversionInfo;

//BUFFER of allowed conversions
conversionInfo* conversionMatrix[64][64];

//helper to print out conversionMatrix
void printMatrix();

int* conversionSearch(char* typeFrom, char* typeTo, int* path);

int checkConversionSearch(char* typeFrom, char* typeTo, int* path);

PRINTER_SET* createPrinterSet(PRINTER_SET* newPrinterSet, char* fileType);

void sigchldhandler(int sig);

PRINTER* findEligiblePrinter(PRINTER_SET* printerSet, char* fileType);
void checkUpdateJobs();
