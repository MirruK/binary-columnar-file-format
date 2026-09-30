#pragma once

#include <string.h>

#define STRIP_NEWLINE(string) string[strcspn(string, "\n")] = '\0'
