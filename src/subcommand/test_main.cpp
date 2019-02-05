#include <omp.h>
#include <unistd.h>
#include <getopt.h>

#include <iostream>

#include "subcommand.hpp"

#include "unittest/driver.hpp"

using namespace std;
using namespace dg;
using namespace dg::subcommand;

// No help_test is necessary because the unit testing library takes care of
// complaining about missing options.

int main_test(int argc, char** argv){
    // Forward arguments along to the main unit test driver
    return dg::unittest::run_unit_tests(argc, argv);
}

// Register subcommand
static Subcommand dg_test("test", "run unit tests", PIPELINE, 5, main_test);

