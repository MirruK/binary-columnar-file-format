#pragma once

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG > 0
#define debug_log(fmt, ...) \
printf("\n%s:%d: @ %s():\n\t " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__);
#else
#define debug_log(fmt, ...) ;
#endif

