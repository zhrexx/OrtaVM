#ifndef XHEX_H
#define XHEX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>

typedef struct {
    FILE *input;
    FILE *output;
    bool show_ascii;
    bool reverse;
    bool uppercase;
    bool include_header;
    int columns;
    int group_size;
    unsigned long start_offset;
    unsigned long length;
    const char *var_name;
    bool c_style_output;
} xhex_options_t;

static void xhex_init_options(xhex_options_t *options) {
    options->input = stdin;
    options->output = stdout;
    options->show_ascii = true;
    options->reverse = false;
    options->uppercase = false;
    options->include_header = true;
    options->columns = 16;
    options->group_size = 2;
    options->start_offset = 0;
    options->length = 0;
    options->var_name = "hexdump";
    options->c_style_output = false;
}

static char xhex_to_printable(char c) {
    return (isprint((unsigned char)c)) ? c : '.';
}

static char xhex_to_hex_char(unsigned char nibble, bool uppercase) {
    nibble &= 0x0F;
    if (nibble < 10) {
        return '0' + nibble;
    } else {
        return (uppercase ? 'A' : 'a') + (nibble - 10);
    }
}

static void xhex_byte_to_hex(unsigned char byte, char *output, bool uppercase) {
    output[0] = xhex_to_hex_char((byte >> 4), uppercase);
    output[1] = xhex_to_hex_char(byte, uppercase);
}

static int xhex_hex_to_byte(const char *hex) {
    int byte = 0;
    
    for (int i = 0; i < 2; i++) {
        char c = hex[i];
        byte <<= 4;
        
        if (c >= '0' && c <= '9') {
            byte |= c - '0';
        } else if (c >= 'a' && c <= 'f') {
            byte |= c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            byte |= c - 'A' + 10;
        } else {
            return -1;
        }
    }
    
    return byte;
}

static void xhex_print_header(xhex_options_t *options, unsigned long offset) {
    if (options->c_style_output) {
        fprintf(options->output, "  /* %08lx */", offset);
    } else {
        fprintf(options->output, "%08lx:", offset);
    }
}

static void xhex_print_hex_dump(xhex_options_t *options) {
    unsigned char buffer[options->columns];
    size_t bytes_read;
    unsigned long total_read = 0;
    unsigned long offset = options->start_offset;
    bool done = false;
    
    if (options->c_style_output) {
        fprintf(options->output, "unsigned char %s[] = {\n", options->var_name);
    }
    
    if (options->start_offset > 0) {
        fseek(options->input, options->start_offset, SEEK_SET);
    }
    
    while (!done && (bytes_read = fread(buffer, 1, options->columns, options->input)) > 0) {
        if (options->length > 0 && total_read + bytes_read > options->length) {
            bytes_read = options->length - total_read;
            done = true;
        }
        
        if (bytes_read == 0) break;
        
        if (options->include_header) {
            xhex_print_header(options, offset);
        }
        
        for (size_t i = 0; i < options->columns; i++) {
            if (i < bytes_read) {
                char hex[3] = {0};
                xhex_byte_to_hex(buffer[i], hex, options->uppercase);
                
                if (options->c_style_output) {
                    fprintf(options->output, "0x%s%s ", hex, (i == bytes_read - 1 && total_read + bytes_read == options->length) ? "" : ",");
                } else {
                    fprintf(options->output, " %s", hex);
                }
                
                if (options->group_size > 0 && (i + 1) % options->group_size == 0) {
                    fprintf(options->output, " ");
                }
            } else {
                if (options->c_style_output) {
                } else {
                    fprintf(options->output, "   ");
                    if (options->group_size > 0 && (i + 1) % options->group_size == 0) {
                        fprintf(options->output, " ");
                    }
                }
            }
        }
        
        if (options->show_ascii && !options->c_style_output) {
            fprintf(options->output, " |");
            for (size_t i = 0; i < bytes_read; i++) {
                fprintf(options->output, "%c", xhex_to_printable(buffer[i]));
            }
            fprintf(options->output, "|");
        }
        
        fprintf(options->output, "\n");
        
        total_read += bytes_read;
        offset += bytes_read;
        
        if (options->length > 0 && total_read >= options->length) {
            break;
        }
    }
    
    if (options->c_style_output) {
        fprintf(options->output, "};\n");
        fprintf(options->output, "const size_t %s_size = %lu;\n", options->var_name, total_read);
    }
}

static void xhex_reverse_hex_dump(xhex_options_t *options) {
    char line[1024];
    unsigned char output_buf[1024];
    size_t output_idx = 0;
    
    while (fgets(line, sizeof(line), options->input) != NULL) {
        char *p = line;
        
        if (options->include_header) {
            while (*p && *p != ':') p++; // Skip the offset
            if (*p == ':') p++;          // Skip the colon too
        }
        
        while (*p) {
            if (isspace(*p)) {
                p++;
                continue;
            }
            
            if (*p == '|') {
                // Skip ASCII representation
                while (*p && *p != '\n') p++;
                continue;
            }
            
            if (isxdigit(*p) && isxdigit(*(p+1))) {
                int byte = xhex_hex_to_byte(p);
                if (byte != -1) {
                    output_buf[output_idx++] = (unsigned char)byte;
                    if (output_idx >= sizeof(output_buf)) {
                        fwrite(output_buf, 1, output_idx, options->output);
                        output_idx = 0;
                    }
                }
                p += 2;
            } else {
                p++;
            }
        }
    }
    
    if (output_idx > 0) {
        fwrite(output_buf, 1, output_idx, options->output);
    }
}

static void xhex_display_help() {
    printf("xhex - Hexadecimal dump utility\n");
    printf("Usage: xhex [options] [infile [outfile]]\n");
    printf("\nOptions:\n");
    printf("  -a, --no-ascii     Don't display ASCII representation\n");
    printf("  -r, --reverse      Reverse operation: convert hex dump to binary\n");
    printf("  -u, --uppercase    Use uppercase hex characters\n");
    printf("  -n, --no-header    Don't display offset headers\n");
    printf("  -c, --columns NUM  Set number of bytes per line (default: 16)\n");
    printf("  -g, --group NUM    Group bytes by this number (default: 2)\n");
    printf("  -s, --seek OFFSET  Start at OFFSET bytes into input\n");
    printf("  -l, --length NUM   Process only NUM bytes of input\n");
    printf("  -C, --c-array      Output in C array format\n");
    printf("  -v, --var NAME     Variable name for C array (default: hexdump)\n");
    printf("  -h, --help         Display this help and exit\n");
}

static int xhex_parse_options(int argc, char *argv[], xhex_options_t *options) {
    static struct option long_options[] = {
        {"no-ascii",   no_argument,       0, 'a'},
        {"reverse",    no_argument,       0, 'r'},
        {"uppercase",  no_argument,       0, 'u'},
        {"no-header",  no_argument,       0, 'n'},
        {"columns",    required_argument, 0, 'c'},
        {"group",      required_argument, 0, 'g'},
        {"seek",       required_argument, 0, 's'},
        {"length",     required_argument, 0, 'l'},
        {"c-array",    no_argument,       0, 'C'},
        {"var",        required_argument, 0, 'v'},
        {"help",       no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    int c;
    int option_index = 0;
    
    while ((c = getopt_long(argc, argv, "arunc:g:s:l:Cv:h", long_options, &option_index)) != -1) {
        switch (c) {
            case 'a':
                options->show_ascii = false;
                break;
            case 'r':
                options->reverse = true;
                break;
            case 'u':
                options->uppercase = true;
                break;
            case 'n':
                options->include_header = false;
                break;
            case 'c':
                options->columns = atoi(optarg);
                if (options->columns <= 0) {
                    fprintf(stderr, "Error: Column count must be positive\n");
                    return 1;
                }
                break;
            case 'g':
                options->group_size = atoi(optarg);
                break;
            case 's':
                options->start_offset = strtoul(optarg, NULL, 0);
                break;
            case 'l':
                options->length = strtoul(optarg, NULL, 0);
                break;
            case 'C':
                options->c_style_output = true;
                break;
            case 'v':
                options->var_name = optarg;
                break;
            case 'h':
                xhex_display_help();
                exit(0);
            case '?':
                return 1;
            default:
                abort();
        }
    }
    
    int remaining_args = argc - optind;
    
    if (remaining_args >= 1) {
        if (strcmp(argv[optind], "-") != 0) {
            options->input = fopen(argv[optind], "rb");
            if (!options->input) {
                fprintf(stderr, "Error: Could not open input file '%s'\n", argv[optind]);
                return 1;
            }
        }
    }
    
    if (remaining_args >= 2) {
        if (strcmp(argv[optind + 1], "-") != 0) {
            options->output = fopen(argv[optind + 1], options->reverse ? "wb" : "w");
            if (!options->output) {
                fprintf(stderr, "Error: Could not open output file '%s'\n", argv[optind + 1]);
                if (options->input != stdin) fclose(options->input);
                return 1;
            }
        }
    }
    
    return 0;
}

static void xhex_cleanup(xhex_options_t *options) {
    if (options->input != stdin) {
        fclose(options->input);
    }
    if (options->output != stdout) {
        fclose(options->output);
    }
}

int xhex_run(int argc, char *argv[]) {
    xhex_options_t options;
    xhex_init_options(&options);
    
    if (xhex_parse_options(argc, argv, &options) != 0) {
        return 1;
    }
    
    if (options.reverse) {
        xhex_reverse_hex_dump(&options);
    } else {
        xhex_print_hex_dump(&options);
    }
    
    xhex_cleanup(&options);
    return 0;
}

#endif /* XHEX_H */
