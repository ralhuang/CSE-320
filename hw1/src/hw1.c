    #include <stdlib.h>

    #include "debug.h"
    #include "hw1.h"

    #ifdef _STRING_H
    #error "Do not #include <string.h>. You will get a ZERO."
    #endif

    #ifdef _STRINGS_H
    #error "Do not #include <strings.h>. You will get a ZERO."
    #endif

    #ifdef _CTYPE_H
    #error "Do not #include <ctype.h>. You will get a ZERO."
    #endif

 /*
     * You may modify this file and/or move the functions contained here
     * to other source files (except for main.c) as you wish.
     *
     * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
     * you MAY NOT declare any arrays or allocate any storage with malloc().
     * The purpose of this restriction is to force you to use pointers.
     * Variables to hold the content of three frames of audio data and
     * two annotation fields have been pre-declared for you in const.h.
     * You must use those variables, rather than declaring your own.
     * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
     *
     * IMPORTANT: You MAY NOT use floating point arithmetic or declare
     * any "float" or "double" variables.  IF YOU VIOLATE THIS RESTRICTION,
     * YOU WILL GET A ZERO!
     */

    /**
     * @brief Validates command line arguments passed to the program.
     * @details This function will validate all the arguments passed to the
     * program, returning 1 if validation succeeds and 0 if validation fails.
     * Upon successful return, the selected program options will be set in the
     * global variables "global_options", where they will be accessible
     * elsewhere in the program.
     *
     * @param argc The number of arguments passed to the program from the CLI.
     * @param argv The argument strings passed to the program from the CLI.
     * @return 1 if validation succeeds and 0 if validation fails.
     * Refer to the homework document for the effects of this function on
     * global variables.
     * @modifies global variable "global_options" to contain a bitmap representing
     * the selected options.
     */
    int validargs(int argc, char ** argv) {
      // for each argument, number determined by argc, check the argv and determine
      // if it is valid.
      // TEST IF ONLY 1 argument
      numargs = argc;
      int currentArg = 1;
      unsigned long int x = 0x8000000000000000;

      for (int i = 0; i < argc - 1; i++) {
        char b = *( *(argv + 1 + i));
        if (b == '-') {
          b = *( *(argv + 1 + i) + 2);
          if (b != '\0') {
            return 0;
          }
        }
      }

      if (argc <= 1) {
        return 0;
      }

      if (argc == 2) {
        char * firstarg = *(argv + currentArg);
        char b = * firstarg;
        char c = *(firstarg + 1);

        if (b == '-') {
          if (c == 'h' || c == 'u' || c == 'd') {
            if (c == 'h') {
              global_options = global_options | x;
            }
            if (c == 'u') {
              x = 0x4000000000000000;
              global_options = global_options | x;
              // printf("%lu\n", global_options);
              x = 0x4C00FFFFFFFFFFFF;
              global_options = global_options & x;
              // printf("%lu\n", global_options);
            }
            if (c == 'd') {
              x = 0x2000000000000000;
              global_options = global_options | x;
              // printf("%lu\n", global_options);
              x = 0x2C00FFFFFFFFFFFF;
              global_options = global_options & x;
            }
            return 1;
          }
        }

        // printf("%s\n", "Incorrect argument");
        return 0;

      } else if (argc > 2) {
        char * argpointer = *(argv + currentArg);
        char b = * argpointer;
        char c = *(argpointer + 1);

        if (b == '-') {
          // IF IT STARTS WITH -h
          if (c == 'h') {
            // printf("%s\n", "help menu success");
            // printf("%lu\n", global_options);
            global_options = global_options | x;
            // printf("%lu\n", global_options);
            return 1;
          } else if (argc > 5) { //Max # of args for other than H
            return 0;
          }
          // IF IT STARTS WITH -u or -d
          else if (c == 'u' || c == 'd') {
            // printf("%s\n", "in u/d");
            unsigned long int y = 0;
            if (c == 'u')
              y = 0x4000000000000000;
            else
              y = 0x2000000000000000;

            currentArg++;
            argpointer = *(argv + currentArg);
            b = * argpointer;
            c = *(argpointer + 1);
            if (b != '-') {
              // printf("%s\n", "Invalid argument");
              return 0;
            } else {
              if (c == 'p') {
                y = y | 0x800000000000000;
                currentArg++;
                // printf("%d\n", currentArg);
                // printf("%d\n", argc);
                if (currentArg != argc) {
                  argpointer = *(argv + currentArg);
                  c = *(argpointer + 1);
                  printf("%c\n", c);
                } else {
                  global_options = y;
                  return 1;
                }
              }
              if (c == 'f') { //need to validate factor
                currentArg++;
                int sum = 0;
                int numDigits = 0;
                argpointer = *(argv + currentArg);
                int x = 0;
                for (int i = 0; i <= numDigits; i++) {
                  x = *(argpointer + i);
                  if (x != 0) {
                    numDigits++;
                  }
                }
                if (numDigits > 4 || numDigits == 0) {
                  return 0;
                }

                for (int i = 0; numDigits != 0; i++) {
                  int n = *(argpointer + i) - 48;
                  if (n < 0 || n > 9)
                    return 0;
                  if (numDigits == 4)
                    sum += n * 1000;
                  else if (numDigits == 3)
                    sum += n * 100;
                  else if (numDigits == 2)
                    sum += n * 10;
                  else if (numDigits == 1)
                    sum += n;
                  numDigits--;
                }
                if (sum < 1 || sum > 1024) {
                  return 0;
                } else {
                  factor = sum;
                  currentArg++;
                  if (argc == currentArg) {
                    sum--;
                    long z = (long) sum;
                    z = z << 48;
                    y = y | z;
                    global_options = y;
                    return 1;
                  } else {
                    argpointer = *(argv + currentArg);
                    b = *(argpointer);
                    c = *(argpointer + 1);
                    if (b != '-' || c != 'p') {
                      return 0;
                    } else {
                      y = y | 0x800000000000000;
                      sum--;
                      long z = (long) sum;
                      z = z << 48;
                      global_options = y | z;
                      return 1;
                    }
                  }
                }
              }
              return 0;
            }
          } else if (c == 'c') {
            unsigned long int y = 0x1000000000000000;
            currentArg++;
            argpointer = *(argv + currentArg);
            b = * argpointer;
            c = *(argpointer + 1);
            if (b != '-') {
              return 0;
            } else {
              currentArg++;
              if (c == 'p') {
                y = y | 0x800000000000000;
                argpointer = *(argv + currentArg);
                b = * argpointer;
                c = *(argpointer + 1);
                currentArg++;
                if (b != '-') {
                  return 0;
                }
              }
              if (c == 'k') {
                long orKey = 0;
                int numKeyDigits = 0;
                for (int i = 0; i < 8; i++) {
                  argpointer = *(argv + currentArg);
                  int x = *(argpointer + i);
                  if (x < 48 || x > 102)
                    i = 8;
                  else if (x > 57 && x < 65)
                    i = 8;
                  else if (x > 70 && x < 97)
                    i = 8;
                  else
                    numKeyDigits++;
                }
                for (int i = 0; i < numKeyDigits; i++) {
                  orKey = orKey << 4;
                  argpointer = *(argv + currentArg);

                  int x = *(argpointer + i);
                  if (x < 48 || x > 102)
                    return 0;
                  if (x > 57 && x < 65)
                    return 0;
                  if (x > 70 && x < 97)
                    return 0;
                  if (x >= 97)
                    x = x - 87;
                  else if (x >= 65)
                    x = x - 55;
                  else if (x >= 48)
                    x = x - 48;
                  orKey = orKey | x;
                }


                currentArg++;
                if (argc == currentArg) {
                  y = y | orKey;
                  global_options = y;
                  return 1;
                } else {
                  argpointer = *(argv + currentArg);
                  b = * argpointer;
                  c = *(argpointer + 1);
                  if (b != '-' || c != 'p') {
                    return 0;
                  } else
                    y = y | 0x800000000000000;
                  y = y | orKey;
                  global_options = y;
                  return 1;
                }
              } else
                return 0;
            }
          } else
            return 0;
        }
      }

      return 0;
    }

    /**
     * @brief  Recodes a Sun audio (.au) format audio stream, reading the stream
     * from standard input and writing the recoded stream to standard output.
     * @details  This function reads a sequence of bytes from the standard
     * input and interprets it as digital audio according to the Sun audio
     *(.au) format.  A selected transformation (determined by the global variable
     * "global_options") is applied to the audio stream and the transformed stream
     * is written to the standard output, again according to Sun audio format.
     *
     * @param  argv  Command-line arguments, for constructing modified annotation.
     * @return 1 if the recoding completed successfully, 0 otherwise.
     */
    int recode(char ** argv) {
      unsigned long x = global_options & 0x800000000000000;
      AUDIO_HEADER testHeader;
      AUDIO_HEADER * testHeaderPointer;
      testHeaderPointer = & testHeader;

      int counter = 0;
      if (x != 0x800000000000000) {

        for (int i = 0; i < numargs; i++) {
          int boolean = 0;
          int incrementer = 0;
          while (boolean == 0) {
            char b = *( *(argv + i) + incrementer);
            *(output_annotation + (counter * 8)) = b;
            counter++;
            if (( *( *(argv + i) + incrementer + 1)) == '\0') {
              b = 0x20;
              if (i != numargs - 1) {
                *(output_annotation + (counter * 8)) = b;
                counter++;
              } else {
                b = 0x0a;
                *(output_annotation + (counter * 8)) = b;
                counter++;
              }
              boolean = 1;
            } else
              incrementer++;
          }
        }
        testHeaderPointer->data_offset += counter;
        if (read_header(testHeaderPointer) == 0)
          return 0;
        int annotationsize = testHeaderPointer -> data_offset - sizeof(AUDIO_HEADER);
        if (annotationsize > ANNOTATION_MAX) {
          return 0;
        }
        numargbsize = counter;
        if (read_annotation(input_annotation, annotationsize) == 0)
          return 0;
        testHeaderPointer->data_offset += (newannotationsize + counter);
        if (write_header(testHeaderPointer) == 0)
          return 0;
        if (counter + annotationsize > ANNOTATION_MAX)
          return 0;
        if (write_annotation(output_annotation, counter) == 0)
          return 0;
        if (write_annotation(input_annotation, newannotationsize) == 0)
          return 0;
      } else {
        if (read_header(testHeaderPointer) == 0)
          return 0;
        int annotationsize = testHeaderPointer -> data_offset - sizeof(AUDIO_HEADER);
        if (annotationsize > ANNOTATION_MAX) {
          return 0;
        }

        if (write_header(testHeaderPointer) == 0)
          return 0;
        if (read_annotation(input_annotation, annotationsize) == 0)
          return 0;
        if (write_annotation(input_annotation, annotationsize) == 0)
          return 0;
      }

      //SPEEDUP
      x = global_options & 0x4000000000000000;
      if (x == 0x4000000000000000) {
        int fcounter = 0;
        int framelooper = 0;
        while (fcounter < ((testHeaderPointer -> data_size) / ((testHeaderPointer->channels)* (testHeaderPointer->encoding - 1)))) {
          if(read_frame((int*) input_frame, testHeaderPointer -> channels, (testHeaderPointer -> encoding - 1)) == 0)
            return 0;
          if ((framelooper % factor) == 0) {
            if(write_frame((int*) input_frame, testHeaderPointer -> channels, (testHeaderPointer -> encoding - 1)) == 0)
              return 0;
            fcounter++;
          }
          framelooper++;
        }
      }

      //SLOWDOWN
      if ((global_options & 0x2000000000000000) == 0x2000000000000000) {
        int fcounter = 0;
        while (fcounter < ((testHeaderPointer -> data_size) / ((testHeaderPointer->channels)* (testHeaderPointer->encoding - 1)))) {
          if(fcounter == 0) {
            if (read_frame((int *) input_frame, testHeaderPointer -> channels, (testHeaderPointer -> encoding - 1)) == 0)
              return 0;
            if (write_frame((int *) input_frame, testHeaderPointer -> channels, (testHeaderPointer -> encoding - 1)) == 0)
              return 0;
            fcounter++;
          }

          framecopy((int*) input_frame, (int *) previous_frame, testHeaderPointer -> channels, (testHeaderPointer -> encoding - 1));
            if (read_frame((int *) input_frame, testHeaderPointer -> channels, (testHeaderPointer -> encoding - 1)) == 0)
              return 0;
            fcounter++;
            if (fcounter != ((testHeaderPointer -> data_size) - 1)) {
              if (interpolate((int *)input_frame, (int *)previous_frame, testHeaderPointer -> channels, (testHeaderPointer -> encoding - 1), factor) == 0)
                return 0;
              fcounter += (factor - 1);
            }
            if (write_frame((int *) input_frame, testHeaderPointer->channels, (testHeaderPointer->encoding -1)) == 0)
              return 0;
        }

      }

      //encrypt
      if ((global_options & 0x1000000000000000) == 0x1000000000000000) {
        int key = global_options & 0xffffffff;
        int fcounter = 0;
        while (fcounter < ((testHeaderPointer -> data_size) / ((testHeaderPointer->channels)* (testHeaderPointer->encoding - 1)))) {
            read_frame((int*)input_frame, testHeaderPointer->channels, (testHeaderPointer->encoding - 1));
            encrypt((int*) input_frame, testHeaderPointer->channels, (testHeaderPointer->encoding -1 ), key);
            write_frame((int*) output_frame, testHeaderPointer->channels, (testHeaderPointer->encoding - 1));
            fcounter++;
        }
      }

      return 1;
    }

    int read_header(AUDIO_HEADER * hp) {
      unsigned int magicNum = 0;
      unsigned int dataOff = 0;
      unsigned int dataSize = 0;
      unsigned int encoding = 0;
      unsigned int samRate = 0;
      unsigned int channels = 0;
      int c = 0;
      int structcount = 0;
      for (int i = 0; i < 24; i++) {
        int d = getchar();
        if (d == EOF) {
          return 0;
        }
        c = c | d;
        if ((i + 1) % 4 == 0 && i > 0) {
          if (structcount == 0) {
            magicNum = c;
            c = 0;
          } else if (structcount == 1) {
            dataOff = c;
            c = 0;
          } else if (structcount == 2) {
            dataSize = c;
            c = 0;
          } else if (structcount == 3) {
            encoding = c;
            c = 0;
          } else if (structcount == 4) {
            samRate = c;
            c = 0;
          } else if (structcount == 5) {
            channels = c;
            c = 0;
          }
          structcount++;
        } else
          c = c << 8;
      }
      /*MUST VALIDATE IF magicNum is right,
        if dataOff is divisible by 8,
        if encoding is of 2,3,4,5
        if channels is either 1 or 2
    */
      if (magicNum != 0x2e736e64)
        return 0;
      if (dataOff % 8 != 0)
        return 0;
      if (encoding != 2 && encoding != 3 && encoding != 4 && encoding != 5)
        return 0;
      if (channels != 1 && channels != 2)
        return 0;
      hp -> magic_number = magicNum;
      hp -> data_offset = dataOff;
      hp -> data_size = dataSize;
      hp -> encoding = encoding;
      hp -> sample_rate = samRate;
      hp -> channels = channels;

      //update size based on annotation, and speedup/slowdown
      if((global_options & 0x4000000000000000) == 0x4000000000000000) {
        if((dataSize/((encoding - 1) * channels)) % 2 == 0)
          hp->data_size = (dataSize/factor);
        else
          hp->data_size = ((dataSize/factor) + channels);
            }
      if((global_options & 0x2000000000000000) == 0x2000000000000000)
              hp->data_size = ((dataSize/((encoding - 1) * channels) + (((dataSize/((encoding - 1) * channels) - 1)) * (factor - 1))) * ((encoding - 1) * channels));
      return 1;
    }

    int write_header(AUDIO_HEADER * hp) {
      int z = 24;
      int p = 0;
      for (int j = 0; j < 6; j++) {
        unsigned int * x = ( & (hp -> magic_number));
        * x = *(x + j);
        for (int i = 0; i < 4; i++) {
          int y = * x << (8 * i);
          y = y >> z;

          if ((p = putchar(y)) == EOF)
            return 0;
        }
      }
      return 1;
    }

    int read_annotation(char * ap, unsigned int size) {

      if (size == 0 && (global_options & 0x800000000000000) != 0x800000000000000) {
        int tempsize = size;
        int i = 0;
        while ((tempsize + numargbsize) % 8 != 0) {
          *(ap + ((size + i) * 8)) = 0;
          i++;
          tempsize++;
        }
        newannotationsize = tempsize;
        return 1;
      }

      int x = 0;
      for (int i = 0; i < size; i++) {
        x = getchar();
        if (x == EOF) {
          return 0;
        }
        *(ap + (i * 8)) = x;
        if ((i == (size - 1)) & (size % 8 == 0) & (x != 0))
          return 0;
      }

      if ((global_options & 0x800000000000000) != 0x800000000000000) {
        int tempsize = size;
        int i = 0;
        while ((tempsize + numargbsize) % 8 != 0) {
          *(ap + ((size + i) * 8)) = 0;
          i++;
          tempsize++;
        }
        newannotationsize = tempsize;
        return 1;
      }
      return 0;
    }

    int write_annotation(char * ap, unsigned int size) {
      int p = 0;
      for (int i = 0; i < size; i++) {
        int x = *(ap + (i * 8));
        if ((p = putchar(x)) == EOF)
          return 0;
      }
      return 1;
    }

    int read_frame(int * fp, int channels, int bytes_per_sample) {
      int e = 0;
      int bytes = channels * bytes_per_sample;
      for (int i = 0; i < bytes; i++) {
        if ((e = getchar()) == EOF) {
          return 0;
        } else
          *(fp + (i * bytes)) = e;
      }
      return 1;
    }

    int write_frame(int * fp, int channels, int bytes_per_sample) {
      int e = 0;
      int bytes = channels * bytes_per_sample;
      for (int i = 0; i < bytes; i++) {
        int op = *(fp + (i * bytes));
        if ((e = putchar(op)) == EOF) {
          return 0;
        }
      }
      return 1;
    }

    int framecopy(int* fp, int* fp2, int channels, int bytes_per_sample) {
      int bytes = channels * bytes_per_sample;
      for(int i = 0; i < bytes; i++) {
        int op = *(fp + (i * bytes));
        *(fp2 + (i * bytes)) = op;
      }
      return 1;
    }

    int interpolate(int* cfp, int* pfp, int channels, int bytes_per_sample, int factor) {
      debug("factor %d", factor);
      int bytes = channels * bytes_per_sample;

      int s = 0;
      int t = 0;
      int totalFrameS = 0;
      int totalFrameT = 0;
      for(int i = 0; i < bytes; i++) {
        s += *(pfp + (i * bytes));
        t += *(cfp + (i * bytes));

        if(i != bytes - 1) {
          s = s << 8;
          t = t << 8;
        }
      }
      totalFrameS = s;
      totalFrameT = t;

      int totalDiff = totalFrameT - totalFrameS;
      //generate and output frame
      for(int k = 1; k < factor; k++) {
        int totalNewFrame = totalFrameS + ((totalDiff * k)/factor);
        debug("total new frame: %d", totalNewFrame);

        for(int i = 0; i < bytes; i++) {
          if(i % 2 == 0) {
            *((int*)output_frame + (bytes * i)) = totalNewFrame >> ((bytes * bytes * channels) * 3/4);
            debug("shifting right: %d", ((bytes * bytes * channels) * 3/4));
          }
          else {
            *((int*)output_frame + (bytes * i)) = (totalNewFrame << ((bytes * bytes * channels) * 1/4) )>> (((bytes * bytes * channels) / 2) + ((bytes * bytes * channels) * 1/4));
            debug("shifting left: %d", ((bytes * bytes * channels) * 1/4));
            debug("then shifting right: %d", (((bytes * bytes * channels) / 2) + ((bytes * bytes * channels) * 1/4)));
          }
        }

        if (write_frame( (int*)(output_frame), channels, bytes_per_sample) == 0)
          return 0;
      }
      return 1;
    }

    //darn
    int encrypt(int* inputframe, int channels, int bytes_per_sample, int key) {
        int bytes = channels * bytes_per_sample;
        int encryptedvalue = 0;
        for(int i = 0; i < bytes; i++) {
            encryptedvalue = *(inputframe + (i * bytes)) ^ key;
            *(output_frame + (i * bytes)) = encryptedvalue;
        }

        return 1;
    }



