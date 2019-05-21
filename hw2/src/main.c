
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "version.h"
#include "global.h"
#include "gradedb.h"
#include "stats.h"
#include "read.h"
#include "write.h"
#include "normal.h"
#include "sort.h"
#include "error.h"
#include "report.h"

/*
 * Course grade computation program
 */

#define REPORT          0
#define COLLATE         1
#define FREQUENCIES     2
#define QUANTILES       3
#define SUMMARIES       4
#define MOMENTS         5
#define COMPOSITES      6
#define INDIVIDUALS     7
#define HISTOGRAMS      8
#define TABSEP          9
#define ALLOUTPUT      10
#define SORTBY         11
#define NONAMES        12
#define OUTPUT         13
#define BADOPT         14

static struct option_info {
        unsigned int val;
        char *name;
        char chr;
        int has_arg;
        char *argname;
        char *descr;
} option_table[] = {
 {REPORT,         "report",    'r',      no_argument, NULL,
                  "Process input data and produce specified reports."},
 {COLLATE,        "collate",   'c',      no_argument, NULL,
                  "Collate input data and dump to standard output."},
 {FREQUENCIES,    "freqs",     0,        no_argument, NULL,
                  "Print frequency tables."},
 {QUANTILES,      "quants",    0,        no_argument, NULL,
                  "Print quantile information."},
 {SUMMARIES,      "summaries", 0,        no_argument, NULL,
                  "Print quantile summaries."},
 {MOMENTS,        "stats",     0,        no_argument, NULL,
                  "Print means and standard deviations."},
 {COMPOSITES,     "comps",     0,        no_argument, NULL,
                  "Print students' composite scores."},
 {INDIVIDUALS,    "indivs",    0,        no_argument, NULL,
                  "Print students' individual scores."},
 {HISTOGRAMS,     "histos",    0,        no_argument, NULL,
                  "Print histograms of assignment scores."},
 {TABSEP,         "tabsep",    0,        no_argument, NULL,
                  "Print tab-separated table of student scores."},
 {ALLOUTPUT,      "all",       'a',      no_argument, NULL,
                  "Print all reports."},
 {NONAMES,        "nonames",   'n',      no_argument, NULL,
                  "Suppress printing of students' names."},
 {SORTBY,         "sortby",    'k',      required_argument, "key",
                  "Sort by {name, id, score}."},
 {OUTPUT,         "output",    'o',      required_argument, "outfile",
                  "Write output to file, rather than standard output."},
 {BADOPT,         0, 0, no_argument, NULL, "Invalid option"}
};

#define NUM_OPTIONS (15)

static char *short_options = "rcank:o:";
static struct option long_options[NUM_OPTIONS];

static void init_options() {
    for(unsigned int i = 0; i < NUM_OPTIONS; i++) {
        struct option_info *oip = &option_table[i];
        struct option *op = &long_options[i];
        op->name = oip->name;
        op->has_arg = oip->has_arg;
        op->flag = NULL;
        op->val = oip->val;
    }
}

static int report, collate, freqs, quantiles, summaries, moments,
           scores, composite, histograms, tabsep, nonames, output,
           sortby, all;

static void usage();

int main(argc, argv)
int argc;
char *argv[];
{
        FILE* filepointer;
        Course *c;
        Stats *s;
        extern int errors, warnings;
        char optval;
        int (*compare)() = comparename;
        fprintf(stderr, BANNER);
        init_options();
        if(argc <= 1) usage(argv[0]);
        while(optind < argc) {
            if((optval = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
                if(optind == 2) {
                  if (optval != REPORT && optval != 'r' && optval != COLLATE && optval != 'c') {
                    fprintf(stderr, "'%s' or '%s' must be the first option\n\n",
                        option_table[REPORT].name, option_table[COLLATE].name);
                    usage(argv[0]);
                  }
                }
                // printf("OPTVAL : %d\n", optval);
                switch(optval) {
                case REPORT: report++; break;
                case 'r': report++; break;
                case COLLATE: collate++; break;
                case 'c': collate++; break;
                case TABSEP: tabsep++; break;
                case NONAMES: nonames++; break;
                case 'n': nonames++; break;
                case SORTBY:
                    sortby++;
                    if(!strcmp(optarg, "name"))
                        compare = comparename;
                    else if(!strcmp(optarg, "id"))
                        compare = compareid;
                    else if(!strcmp(optarg, "score"))
                        compare = comparescore;
                    else {
                        fprintf(stderr,
                                "Option '%s' requires argument from {name, id, score}.\n\n",
                                option_table[(int)optval].name);
                        usage(argv[0]);
                    }
                    break;
                case 'k':
                    sortby++;
                    if(!strcmp(optarg, "name"))
                        compare = comparename;
                    else if(!strcmp(optarg, "id"))
                        compare = compareid;
                    else if(!strcmp(optarg, "score"))
                        compare = comparescore;
                    else {
                        fprintf(stderr,
                                "Option '-k (sortby)' requires argument from {name, id, score}.\n\n");
                        usage(argv[0]);
                    }
                    break;
                case FREQUENCIES: freqs++; break;
                case QUANTILES: quantiles++; break;
                case SUMMARIES: summaries++; break;
                case MOMENTS: moments++; break;
                case COMPOSITES: composite++; break;
                case INDIVIDUALS: scores++; break;
                case HISTOGRAMS: histograms++; break;
                case ALLOUTPUT:
                    all++;
                    break;
                case 'a':
                    all++;
                    break;
                case OUTPUT:
                    output++;
                    filepointer = fopen(optarg, "w");
                    break;
                case 'o':
                    output++;
                    filepointer = fopen(optarg, "w");
                    break;
                case '?':
                    usage(argv[0]);
                    break;
                default:
                    break;
                }
            } else {
                break;
            }
        }

        if(report > 1 || collate > 1 || tabsep > 1 || nonames > 1 || sortby > 1 || output > 1 ||
          all > 1 || histograms > 1 || scores > 1 || composite > 1 || moments > 1 || freqs > 1 ||
          quantiles > 1 || scores > 1 || summaries > 1) {
            fprintf(stderr, "Do not repeat flags.");
            usage(argv[0]);
          }

        if(all == 1) {
          freqs++; quantiles++; summaries++; moments++;
          composite++; scores++; histograms++; tabsep++;
        }

        if(optind == argc) {
                fprintf(stderr, "No input file specified.\n\n");
                usage(argv[0]);
        }
        char *ifile = argv[optind];
        if(report == collate) {
                fprintf(stderr, "Exactly one of '%s' or '%s' is required.\n\n",
                        option_table[REPORT].name, option_table[COLLATE].name);
                usage(argv[0]);
        }

        fprintf(stderr, "Reading input data...\n");
        c = readfile(ifile);
        if(errors) {
           printf("%d error%s found, so no computations were performed.\n",
                  errors, errors == 1 ? " was": "s were");
           exit(EXIT_FAILURE);
        }

        fprintf(stderr, "Calculating statistics...\n");
        s = statistics(c);
        if(s == NULL) fatal("There is no data from which to generate reports.");
        normalize(c);//, s);
        composites(c);
        sortrosters(c, comparename);
        checkfordups(c->roster);
        if(collate) {
                fprintf(stderr, "Dumping collated data...\n");
                if(output) {
                  writecourse(filepointer, c);
                } else
                writecourse(stdout, c);
                exit(errors ? EXIT_FAILURE : EXIT_SUCCESS);
        }
        sortrosters(c, compare);

        fprintf(stderr, "Producing reports...\n");
        if(output) {
          reportparams(filepointer, ifile, c);
          if(moments) reportmoments(filepointer, s);
          if(composite) reportcomposites(filepointer, c, nonames);
          if(freqs) reportfreqs(filepointer, s);
          if(quantiles) reportquantiles(filepointer, s);
          if(summaries) reportquantilesummaries(filepointer, s);
          if(histograms) reporthistos(filepointer, c, s);
          if(scores) reportscores(filepointer, c, nonames);
          if(tabsep) reporttabs(filepointer, c);
          fclose(filepointer);
        } else {
        reportparams(stdout, ifile, c);
        if(moments) reportmoments(stdout, s);
        if(composite) reportcomposites(stdout, c, nonames);
        if(freqs) reportfreqs(stdout, s);
        if(quantiles) reportquantiles(stdout, s);
        if(summaries) reportquantilesummaries(stdout, s);
        if(histograms) reporthistos(stdout, c, s);
        if(scores) reportscores(stdout, c, nonames);
        if(tabsep) reporttabs(stdout, c);//, nonames);
      }

        fprintf(stderr, "\nProcessing complete.\n");
        printf("%d warning%s issued.\n", warnings+errors,
               warnings+errors == 1? " was": "s were");
        exit(errors ? EXIT_FAILURE : EXIT_SUCCESS);
}

void usage(name)
char *name;
{
        struct option_info *opt;

        fprintf(stderr, "Usage: %s [options] <data file>\n", name);
        fprintf(stderr, "Valid options are:\n");
        for(unsigned int i = 0; i < NUM_OPTIONS-1; i++) {
                opt = &option_table[i];
                char optchr[5] = {' ', ' ', ' ', ' ', '\0'};
                if(opt->chr)
                  sprintf(optchr, "-%c, ", opt->chr);
                char arg[32];
                if(opt->has_arg)
                    sprintf(arg, " <%.10s>", opt->argname);
                else
                    sprintf(arg, "%.13s", "");
                fprintf(stderr, "\t%s--%-10s%-13s\t%s\n",
                            optchr, opt->name, arg, opt->descr);
                opt++;
        }
        exit(EXIT_FAILURE);
}
