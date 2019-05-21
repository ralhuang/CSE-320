#include <stdio.h>
#include <sys/wait.h>

#include <criterion/criterion.h>
#include <criterion/logging.h>

#include "gradedb.h"
#include "read.h"
#include "write.h"
#include "sort.h"
#include "stats.h"

#define ERR_RETURN_SUCCESS "Program exited with 0x%x instead of EXIT_SUCCESS"
#define ERR_RETURN_FAILURE "Program exited with 0x%x instead of EXIT_FAILURE"
#define ERR_RETURN_SIGNAL "Program crashed (status 0x%x)"
#define ERR_PRINTS_WRONG_REPORT_ALL "Program prints %d wrong output for --report and --all"
#define ERR_PRINTS_WRONG_REPORT "Program prints %d wrong output for --report"

extern int errors, warnings;

void assert_matching_content(char *out, char *ref) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "cmp -s %s tests/rsrc/%s", out, ref);
    int ret = system(cmd);
    cr_assert_eq(WEXITSTATUS(ret), EXIT_SUCCESS,
		 "Output file %s does not match expected tests/rsrc/%s", out, ref);
}

void run_comparison_test(char *s, char *out, char *ref) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), s, out);
    int ret = system(cmd);
    cr_assert(!WIFSIGNALED(ret), "Program crashed (status 0x%x): %d", cmd, ret);
    cr_assert_eq(WEXITSTATUS(ret), EXIT_SUCCESS, "Exit status (0x%x) is not EXIT_SUCCESS: %s",
		 WEXITSTATUS(ret), cmd);
    assert_matching_content(out, ref);
}


// tokenbuf tests
// test with cse307_extra_large_token_0chars.dat
// test with cse307_extra_large_token_1000chars.dat
// test with cse307_extra_large_token_10000chars.dat
Test(hw2_tests_suite, tokenbuf_test_0chars){
    char *cmd = "./bin/grades --report ./tests/rsrc/cse307_extra_large_token_0chars.dat >/dev/null 2>&1";
    int ret = system(cmd);
    cr_assert(!WIFSIGNALED(ret), ERR_RETURN_SIGNAL, ret);
    cr_assert_eq(WEXITSTATUS(ret), EXIT_SUCCESS, ERR_RETURN_SUCCESS, ret);
}
Test(hw2_tests_suite, tokenbuf_test_1000chars){
    char *cmd = "./bin/grades --report ./tests/rsrc/cse307_extra_large_token_1000chars.dat >/dev/null 2>&1";
    int ret = system(cmd);
    cr_assert(!WIFSIGNALED(ret), ERR_RETURN_SIGNAL, ret);
    cr_assert_eq(WEXITSTATUS(ret), EXIT_SUCCESS, ERR_RETURN_SUCCESS, ret);
}
Test(hw2_tests_suite, tokenbuf_test_10000chars){
    char *cmd = "./bin/grades --report ./tests/rsrc/cse307_extra_large_token_10000chars.dat >/dev/null 2>&1";
    int ret = system(cmd);
    cr_assert(!WIFSIGNALED(ret), ERR_RETURN_SIGNAL, ret);
    cr_assert_eq(WEXITSTATUS(ret), EXIT_SUCCESS, ERR_RETURN_SUCCESS, ret);
}

// use after free
Test(hw2_tests_suite, valgrind_use_after_free){
    char *cmd = "valgrind --leak-check=full --show-leak-kinds=all ./bin/grades --freqs ./cse307a.dat 2>&1 | grep 'invalid read'";
    int ret = WEXITSTATUS(system(cmd));
    cr_assert_neq(ret, EXIT_SUCCESS, ERR_RETURN_FAILURE, ret);
}
// memory leak
Test(hw2_tests_suite, valgrind_memory_leak){
    char *cmd = "valgrind --leak-check=full ./bin/grades --report ./cse307.dat 2>&1 | grep 'still reachable: 0 bytes'";
    int ret = WEXITSTATUS(system(cmd));
    cr_assert_eq(ret, EXIT_FAILURE, ERR_RETURN_FAILURE, ret);
}

// options test for --report
Test(hw2_tests_suite, end_2_end_report_short){
    run_comparison_test("./bin/grades -r cse307.dat 2>/dev/null | grep -v 'RUN DATE' > %s 2>/dev/null",
			"testreportshort", "testreport");
}
Test(hw2_tests_suite, end_2_end_report){
    run_comparison_test("./bin/grades --report cse307.dat 2>/dev/null | grep -v 'RUN DATE' > %s 2>/dev/null",
			"testreport", "testreport");
}

// options test for --report and -all
Test(hw2_tests_suite, end_to_end_report_all_short){
    run_comparison_test("./bin/grades --report -a cse307.dat 2>/dev/null | grep -v 'RUN DATE' > %s 2>/dev/null",
			"testreportallshort", "testreportall");
}
Test(hw2_tests_suite, end_to_end_report_all){
    run_comparison_test("./bin/grades --report --all cse307.dat 2>/dev/null | grep -v 'RUN DATE' > %s 2>/dev/null",
			"testreportall", "testreportall");
}

// options test for --report and -sortby
Test(hw2_tests_suite, end_to_end_report_sortby_short){
    run_comparison_test("./bin/grades --report --comps -k name cse307.dat 2>/dev/null | grep -v 'RUN DATE' > %s 2>/dev/null",
			"testreportsortbyshort", "testreportsortby");
}
Test(hw2_tests_suite, end_to_end_report_sortby){
    run_comparison_test("./bin/grades --report --comps --sortby name cse307.dat 2>/dev/null | grep -v 'RUN DATE' > %s 2>/dev/null",
			"testreportsortby", "testreportsortby");
}

// options test for --collate
Test(hw2_tests_suite, end_to_end_collate_short){
    run_comparison_test("./bin/grades -c cse307.dat > %s 2>/dev/null",
			"testcollateshort", "testcollate");
}
Test(hw2_tests_suite, end_to_end_collate){
    run_comparison_test("./bin/grades --collate cse307.dat > %s 2>/dev/null",
			"testcollate", "testcollate");
}

// options test for --o option
Test(hw2_tests_suite, end_to_end_o_option_short){
    run_comparison_test("./bin/grades --report --comps -o tmp cse307.dat >/dev/null 2>&1 && cat tmp | grep -v 'RUN DATE' > %s 2>/dev/null",
			"testreportcompsoutputshort", "testreportcompsoutput");
}
Test(hw2_tests_suite, end_to_end_o_option){
    run_comparison_test("./bin/grades --report --comps --output tmp cse307.dat >/dev/null 2>&1 && cat tmp | grep -v 'RUN DATE' > %s 2>/dev/null",
			"testreportcompsoutput", "testreportcompsoutput");
}

// options test for --long_options segfault
Test(hw2_tests_suite, end_to_end_long_options){
    char *cmd = "./bin/grades --report --comps --sortby name --quiet cse307.dat >/dev/null 2>&1";
    int ret = system(cmd);
    cr_assert(!WIFSIGNALED(ret), ERR_RETURN_SIGNAL, ret);
    cr_assert_eq(WEXITSTATUS(ret), EXIT_FAILURE, ERR_RETURN_FAILURE, ret);
}

// options test for summaries
Test(hw2_tests_suite, end_to_end_summaries){
    run_comparison_test("./bin/grades --report --summaries cse307.dat 2>/dev/null | grep -v 'RUN DATE' > %s 2>/dev/null",
			"testsummaries", "testsummaries");
}

// options test for freqs
Test(hw2_tests_suite, end_to_end_freqs){
    run_comparison_test("./bin/grades --report --freqs cse307.dat 2>/dev/null | grep -v 'RUN DATE' > %s 2>/dev/null",
			"testfreqs", "testfreqs");
}

// options test for quants 
Test(hw2_tests_suite, end_to_end_quants){
    run_comparison_test("./bin/grades --report --quants cse307.dat 2>/dev/null | grep -v 'RUN DATE' > %s 2>/dev/null",
			"testquants", "testquants");
}

// options test for stats 
Test(hw2_tests_suite, end_to_end_stats){
    run_comparison_test("./bin/grades --report --stats cse307.dat 2>/dev/null | grep -v 'RUN DATE' > %s 2>/dev/null",
			"teststats", "teststats");
}

// options test for comps
Test(hw2_tests_suite, end_to_end_comps){
    run_comparison_test("./bin/grades --report --comps cse307.dat 2>/dev/null | grep -v 'RUN DATE' > %s 2>/dev/null",
			"testcomps", "testcomps");
}

// options test for indivs
Test(hw2_tests_suite, end_to_end_indivs){
    run_comparison_test("./bin/grades --report --indivs cse307.dat 2>/dev/null | grep -v 'RUN DATE' > %s 2>/dev/null",
			"testindivs", "testindivs");
}

// options test for histos
Test(hw2_tests_suite, end_to_end_histos){
    run_comparison_test("./bin/grades --report --histos cse307.dat 2>/dev/null | grep -v 'RUN DATE' > %s 2>/dev/null",
			"testhistos", "testhistos");
}

// options test for tabsep
Test(hw2_tests_suite, end_to_end_tabsep){
    run_comparison_test("./bin/grades --report --tabsep cse307.dat 2>/dev/null | grep -v 'RUN DATE' > %s 2>/dev/null",
			"testtabsep", "testtabsep");
}

// options test for nonames collate
Test(hw2_tests_suite, end_to_end_nonames_collate_short){
    run_comparison_test("./bin/grades --collate -n cse307.dat 2>/dev/null | grep -v 'RUN DATE' > %s 2>/dev/null",
			"testnonamesshort", "testnonames");
}
Test(hw2_tests_suite, end_to_end_nonames_collate){
    run_comparison_test("./bin/grades --collate --nonames cse307.dat 2>/dev/null | grep -v 'RUN DATE' > %s 2>/dev/null",
			"testnonames", "testnonames");
}
