#include "bincoff.h"
#include "bincoff_internal.h"
#include "debug_macro.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>



int main() {
  // Debug broken bincoff library functions here
  debug_log("Hello, context for this printf call should be visible at the start of the line\n");
  return 0;
}
