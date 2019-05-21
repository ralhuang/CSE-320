/*
 * Error handling routines
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "error.h"

int errors;
int warnings;
int dbflag = 1;

// void fatal(fmt, a1, a2, a3, a4, a5, a6)
// char *fmt, *a1, *a2, *a3, *a4, *a5, *a6;
// {
//         fprintf(stderr, "\nFatal error: ");
//         fprintf(stderr, fmt, a1, a2, a3, a4, a5, a6);
//         fprintf(stderr, "\n");
//         exit(1);
// }
//
void fatal(char* fmt, ...) {
  fprintf(stderr, "\nFatal error: ");
  va_list variadic;
  va_start(variadic, fmt);

  vfprintf(stderr, fmt, variadic);
  va_arg(variadic, char* );
  fprintf(stderr, "\n");
  va_end(variadic);
  exit(1);
}

// void error(fmt, a1, a2, a3, a4, a5, a6)
// char *fmt, *a1, *a2, *a3, *a4, *a5, *a6;
// {
//         fprintf(stderr, "\nError: ");
//         fprintf(stderr, fmt, a1, a2, a3, a4, a5, a6);
//         fprintf(stderr, "\n");
//         errors++;
// }
//
void error(char* fmt, ...) {
  fprintf(stderr, "\nError: ");
  va_list variadic;
  va_start(variadic, fmt);

  vfprintf(stderr, fmt, variadic);
  va_arg(variadic, char* );
  fprintf(stderr, "\n");
  va_end(variadic);
  errors++;
}
// void warning(fmt, a1, a2, a3, a4, a5, a6)
// char *fmt, *a1, *a2, *a3, *a4, *a5, *a6;
// {
//         fprintf(stderr, "\nWarning: ");
//         fprintf(stderr, fmt, a1, a2, a3, a4, a5, a6);
//         fprintf(stderr, "\n");
//         warnings++;
// }
//
void warning(char* fmt, ...) {
  fprintf(stderr, "\nWarning: ");
  va_list variadic;
  va_start(variadic, fmt);

  vfprintf(stderr, fmt, variadic);
  va_arg(variadic, char* );
  fprintf(stderr, "\n");
  va_end(variadic);
  warnings++;
}
// void debug(fmt, a1, a2, a3, a4, a5, a6)
// char *fmt, *a1, *a2, *a3, *a4, *a5, *a6;
// {
//         if(!dbflag) return;
//         fprintf(stderr, "\nDebug: ");
//         fprintf(stderr, fmt, a1, a2, a3, a4, a5, a6);
//         fprintf(stderr, "\n");
// }
void debug(char* fmt, ...) {
  if(!dbflag) return;
  fprintf(stderr, "\nDebug: ");
  va_list variadic;
  va_start(variadic, fmt);

  vfprintf(stderr, fmt, variadic);
  va_arg(variadic, char* );
  fprintf(stderr, "\n");
  va_end(variadic);
}
