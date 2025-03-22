#ifndef CONFIG_H
#define CONFIG_H
#define COLOR_RESET   "\x1b[0m"
#define COLOR_BOLD    "\x1b[1m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_BG_BLACK "\x1b[40m"

#include <stdio.h>

#define LOGO "\n\
   ██████╗ ██████╗ ████████╗ █████╗ ██╗   ██╗███╗   ███╗\n\
  ██╔═══██╗██╔══██╗╚══██╔══╝██╔══██╗██║   ██║████╗ ████║\n\
  ██║   ██║██████╔╝   ██║   ███████║██║   ██║██╔████╔██║\n\
  ██║   ██║██╔══██╗   ██║   ██╔══██║╚██╗ ██╔╝██║╚██╔╝██║\n\
  ╚██████╔╝██║  ██║   ██║   ██║  ██║ ╚████╔╝ ██║ ╚═╝ ██║\n\
   ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝  ╚═══╝  ╚═╝     ╚═╝\n"

#define BOX_TOP    "┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓"
#define BOX_BOTTOM "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛"
#define SEPARATOR  "┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫"

void print_error(const char *message) {
    fprintf(stderr, "\n%s%s ⚠ ERROR %s %s\n", COLOR_BOLD, COLOR_RED, COLOR_RESET, message);
}

void print_success(const char *message) {
    printf("\n%s%s ✓ SUCCESS %s %s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET, message);
}

void print_info(const char *message) {
    printf("%s%s ℹ INFO %s %s\n", COLOR_BOLD, COLOR_BLUE, COLOR_RESET, message);
}

void print_progress(const char *stage, const char *detail) {
    printf("%s%s[%s%s%s] %s%s%s\n", 
           COLOR_BOLD, COLOR_YELLOW, 
           COLOR_CYAN, stage, COLOR_YELLOW,
           COLOR_RESET, COLOR_BOLD, detail);
    printf("%s   └─> %s", COLOR_BOLD, COLOR_RESET);
}

#endif // CONFIG_H
