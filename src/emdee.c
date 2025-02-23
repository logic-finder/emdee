#include <stdio.h>
#include "test.h"
#include "misc.h"

int main(int argc, const char **argv) {
   if (argc != 2)
      ERR("usage: ./emdee testcase.txt");
   return perform_test(argv[1]);
}