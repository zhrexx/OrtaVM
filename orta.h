#ifndef ORTA_H
#define ORTA_H
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>

#define STACK_CAPACITY 200000

typedef enum {
    RTS_OK,
    RTS_STACK_OVERFLOW,
    RTS_STACK_UNDERFLOW,
    RTS_ERROR,
    RTS_UNDEFINED_INSTRUCTION,
    RTS_INVALID_TYPE,
} RTStatus;

char *error_to_string(RTStatus status) {
    switch (status) {
    case RTS_OK:
        return "OK";
    case RTS_STACK_OVERFLOW:
        return "Stack Overflow";
    case RTS_STACK_UNDERFLOW:
        return "Stack Underflow";
    case RTS_ERROR:
        return "Error";
    case RTS_UNDEFINED_INSTRUCTION:
        return "Undefined Instruction";
    case RTS_INVALID_TYPE:
        return "Invalid Type";
    default:
        return "Unknown Error";
    }
}

typedef enum {
    I_NOP,            // Used to jmp
    I_IGNORE,         // Skips next instruction
    I_IGNORE_IF,      // Skips next instruction if cond ==

    // Stack
    I_PUSH,           // Push an number to stack
    I_PUSH_STR,       // Push a string to stack
    I_DROP,           // Drops last item
    I_JMP,            // JMPs to given index
    I_JMP_IF,         // JMP to given index while != cond
    I_DUP,            // Duplicates last item
    I_SWAP,           // Swaps last 2 elements
    I_EQ,             // returns 1 if the latest and second item equals
    I_LT,             // Less than (returns 1 if less else 0)
    I_GT,             // Greater that (returns 1 if greater else 0)
    I_GET,            // Gets an item at given index

    I_CLEAR_STACK,    // Clears stack

    // Math
    I_ADD,            // +
    I_SUB,            // -
    I_MUL,            // *
    I_DIV,            // /

    // Syscall
    I_EXIT,           // Exit with given code
    I_EXIT_IF,        // Exit with given code if == cond

    // Functions
    I_STR_LEN,        // Pushes the len of string out of stack to the stack


    // Terminal
    I_INPUT,          // Read input from stdin (ld)
    I_INPUT_STR,      // Read input from stdin (char *)
    I_PRINT,          // For Ints
    I_CAST_AND_PRINT, // Casting Ptr to char * and print
    I_PRINT_STR,      // Print string
    I_COLOR,          // Change Output Color
    I_CLEAR,          // Clear Terminal
    I_SLEEP,          // Sleep for x ms
    I_EXECUTE_CMD,    // Allows to execute an command

    // File
    I_FILE_OPEN,      // Pushes FILE * to the stack
    I_FILE_WRITE,     // Writes a given text to file (appends)
    I_FILE_READ,      // Reads a file and pushes it to the stack

    // Random
    I_RANDOM_INT,     // Pushes an random int to stack
} Instruction;

// Flags
size_t level = 0;
size_t limit = 1000;

void insecure_function(int line_count, char *func) {
    if (level == 1) {
        printf("\033[33mWarning: Usage of insecure function at token %d: %s\033[0m\n", line_count, func);
    }
}

typedef struct {
    int64_t dest;
    int64_t cond;
} JumpIfArgs;

typedef struct {
    int64_t min;
    int64_t max;
} RandomInt;

typedef struct {
    int64_t exit_code;
    int64_t cond;
} ExitIf;

typedef struct {
    Instruction inst;
    union {
        int64_t int_value;
        char str_value[1024];
        JumpIfArgs jump_if_args;
        RandomInt random_int;
        ExitIf exit_if;
    };
} Token;

typedef struct {
    Token tokens[1024];
    size_t size;
} Program;

typedef struct {
    int64_t stack[STACK_CAPACITY];
    size_t stack_size;

    Program program;
} OrtaVM;

RTStatus push(OrtaVM *vm, int64_t value) {
    if (vm->stack_size >= STACK_CAPACITY) {
        return RTS_STACK_OVERFLOW;
    }
    vm->stack[vm->stack_size++] = value;
    return RTS_OK;
}

RTStatus push_str(OrtaVM *vm, const char *str) {
    if (vm->stack_size >= STACK_CAPACITY) {
        return RTS_STACK_OVERFLOW;
    }
    char *dynamic_str = strdup(str);
    if (!dynamic_str) {
        return RTS_ERROR;
    }
    vm->stack[vm->stack_size++] = (int64_t)dynamic_str;
    return RTS_OK;
}

char *extract_content(const char *line) {
    const char *start = strchr(line, '"');
    const char *end = strrchr(line, '"');
    if (!start || !end || start == end) {
        return "No text provided";
    }
    size_t length = end - start - 1;
    static char content[1024];
    if (length >= sizeof(content)) {
        return "Sorry Content is to long!";
    }
    strncpy(content, start + 1, length);
    content[length] = '\0';
    return content;
}


RTStatus add(OrtaVM *vm) {
    if (vm->stack_size < 2) {
        return RTS_STACK_UNDERFLOW;
    }
    int64_t a = vm->stack[--vm->stack_size];
    int64_t b = vm->stack[--vm->stack_size];
    return push(vm, a + b);
}

RTStatus sub(OrtaVM *vm) {
    if (vm->stack_size < 2) {
        return RTS_STACK_UNDERFLOW;
    }
    int64_t a = vm->stack[--vm->stack_size];
    int64_t b = vm->stack[--vm->stack_size];
    return push(vm, b - a);
}

RTStatus print(OrtaVM *vm) {
    if (vm->stack_size == 0) {
        return RTS_STACK_UNDERFLOW;
    }
    printf("%ld\n", vm->stack[vm->stack_size - 1]);
    return RTS_OK;
}

RTStatus OrtaVM_execute(OrtaVM *vm) {
    Program program = vm->program;
    size_t iterations = 0;
    for (size_t i = 0; i < program.size && iterations < limit*4; i++) {
        iterations++;
        Token token = program.tokens[i];
        switch (token.inst) {
            case I_PUSH:
                if (push(vm, token.int_value) != RTS_OK) {
                    return RTS_STACK_OVERFLOW;
                }
                break;
            case I_ADD:
                if (add(vm) != RTS_OK) {
                    return RTS_STACK_UNDERFLOW;
                }
                break;
            case I_SUB:
                if (sub(vm) != RTS_OK) {
                    return RTS_STACK_UNDERFLOW;
                }
                break;
            case I_PRINT:
                if (print(vm) != RTS_OK) {
                    return RTS_STACK_UNDERFLOW;
                }
                break;
            case I_MUL:
                if (vm->stack_size < 2) {
                    return RTS_STACK_UNDERFLOW;
                }
                int64_t a = vm->stack[--vm->stack_size];
                int64_t b = vm->stack[--vm->stack_size];
                if (push(vm, a * b) != RTS_OK) {
                    return RTS_STACK_OVERFLOW;
                }
                break;
            case I_DIV:
                if (vm->stack_size < 2) {
                    return RTS_STACK_UNDERFLOW;
                }
                int64_t c = vm->stack[--vm->stack_size];
                int64_t d = vm->stack[--vm->stack_size];
                if (d == 0) {
                    return RTS_ERROR;
                }
                if (push(vm, c / d) != RTS_OK) {
                    return RTS_STACK_OVERFLOW;
                }
                break;
            case I_PRINT_STR:
                if (token.str_value[0] != '\0') {
                    printf("%s\n", token.str_value);
                }

                break;
            case I_DROP:
                if (vm->stack_size == 0) {
                    return RTS_STACK_UNDERFLOW;
                }
                vm->stack_size--;
                break;
            case I_JMP:
                 if (token.int_value < 0 || token.int_value > program.size) {
                     return RTS_ERROR;
                 }
                 i = token.int_value;
                 continue;
             case I_JMP_IF: // JMP_IF DEST COND | JMP_IF 0 10 | JMP_IF 0 (!= 10)
                 if (vm->stack_size == 0) {
                     return RTS_STACK_UNDERFLOW;
                 }
                 if (vm->stack[vm->stack_size - 1] != token.jump_if_args.cond) {
                     if (token.jump_if_args.dest < 0 || token.jump_if_args.dest > program.size) {
                         return RTS_ERROR;
                     }
                     i = token.jump_if_args.dest;
                     continue;
                 }
                 break;
            case I_DUP:
                if (vm->stack_size == 0) {
                    return RTS_STACK_UNDERFLOW;
                }
                if (vm->stack_size >= STACK_CAPACITY) {
                    return RTS_STACK_OVERFLOW;
                }
                vm->stack[vm->stack_size] = vm->stack[vm->stack_size - 1];
                vm->stack_size++;
                break;
            case I_NOP:
                 break;
            case I_EXIT:
                exit(token.int_value);
            case I_INPUT:
                int64_t input;
                scanf("%ld", &input);
                 if (push(vm, input) != RTS_OK) {
                     return RTS_STACK_OVERFLOW;
                 }
                 break;
            case I_FILE_OPEN:
                FILE *file = fopen(token.str_value, "a+");
                if (file == NULL) {
                    return RTS_ERROR;
                }
                int64_t fd = (int64_t)file;
                if (push(vm, fd) != RTS_OK) {
                    fclose(file);
                    return RTS_STACK_OVERFLOW;
                }
                break;

            case I_FILE_WRITE:
                {
                    FILE *file1 = (FILE *)vm->stack[vm->stack_size - 1];
                    char *buffer = token.str_value;

                    if (file1 == NULL) {
                        return RTS_ERROR;
                    }

                    fwrite(buffer, sizeof(char), strlen(buffer), file1);
                    break;
                }

            case I_FILE_READ:
                {
                    FILE *file2 = (FILE *)vm->stack[vm->stack_size - 1];
                    if (file2 == NULL) {
                        return RTS_ERROR;
                    }

                    char buffer[1024] = {0};
                    fread(buffer, sizeof(char), 1023, file2);
                    buffer[strlen(buffer)] = '\0';

                    if (push(vm, (int64_t)buffer) != RTS_OK) {
                        return RTS_STACK_OVERFLOW;
                    }
                    break;
                }
            case I_CAST_AND_PRINT:
                {
                    char *buffer = (char *)vm->stack[vm->stack_size - 1];
                    if (buffer == NULL) {
                        return RTS_ERROR;
                    }

                    printf("%s\n", buffer);
                    break;
                }

            case I_SWAP:
                if (vm->stack_size < 2) {
                    return RTS_STACK_UNDERFLOW;
                }
                int64_t temp = vm->stack[vm->stack_size - 1];
                vm->stack[vm->stack_size - 1] = vm->stack[vm->stack_size - 2];
                vm->stack[vm->stack_size - 2] = temp;
                break;
            case I_EQ:
                if (vm->stack_size < 2) {
                    return RTS_STACK_UNDERFLOW;
                }
                int64_t a1 = vm->stack[vm->stack_size - 1];
                int64_t b1 = vm->stack[vm->stack_size - 2];
                int64_t result = (a1 == b1) ? 1 : 0;
                if (push(vm, result) != RTS_OK) {
                    return RTS_STACK_OVERFLOW;
                }
                break;
            case I_RANDOM_INT:
                {
                    srand(time(NULL));
                    int64_t min = token.random_int.min;
                    int64_t max = token.random_int.max;
                    int64_t random = min + rand() % (max - min + 1);
                    if (push(vm, random) != RTS_OK) {
                        return RTS_STACK_OVERFLOW;
                    }
                    break;
                }
            case I_COLOR:
                {
                    // TODO: Add more colors
                    char color[1024] = {0};
                    if (strcmp(token.str_value, "red") == 0) {
                        strcpy(color, "\033[31m");
                    } else if (strcmp(token.str_value, "green") == 0) {
                        strcpy(color, "\033[32m");
                    } else if (strcmp(token.str_value, "yellow") == 0) {
                        strcpy(color, "\033[33m");
                    } else if (strcmp(token.str_value, "blue") == 0) {
                        strcpy(color, "\033[34m");
                    } else if (strcmp(token.str_value, "magenta") == 0) {
                        strcpy(color, "\033[35m");
                    } else if (strcmp(token.str_value, "cyan") == 0) {
                        strcpy(color, "\033[36m");
                    } else if (strcmp(token.str_value, "white") == 0) {
                        strcpy(color, "\033[37m");
                    } else if (strcmp(token.str_value, "reset") == 0) {
                        strcpy(color, "\033[0m");
                    } else {
                        return RTS_ERROR;
                    }
                    printf("%s", color);
                    break;
                }
            case I_CLEAR:
                // TODO: Make it os independent
                system("clear");
                break;
            case I_SLEEP:
                struct timespec req;
                req.tv_sec = token.int_value / 1000;
                req.tv_nsec = (token.int_value % 1000) * 1000000;
                nanosleep(&req, NULL);
                break;

            case I_CLEAR_STACK:
                vm->stack_size = 0;
                break;
            case I_LT:
                if (vm->stack_size < 2) {
                    return RTS_STACK_UNDERFLOW;
                }
                int64_t a2 = vm->stack[vm->stack_size - 1];
                int64_t b2 = vm->stack[vm->stack_size - 2];
                int64_t result2 = (b2 < a2) ? 1 : 0;
                if (push(vm, result2) != RTS_OK) {
                    return RTS_STACK_OVERFLOW;
                }
                break;
            case I_GT:
                if (vm->stack_size < 2) {
                    return RTS_STACK_UNDERFLOW;
                }
                int64_t a3 = vm->stack[vm->stack_size - 1];
                int64_t b3 = vm->stack[vm->stack_size - 2];
                int64_t result3 = (b3 > a3) ? 1 : 0;
                if (push(vm, result3) != RTS_OK) {
                    return RTS_STACK_OVERFLOW;
                }
                break;
            case I_PUSH_STR:
                if (push_str(vm, token.str_value) != RTS_OK) {
                    return RTS_STACK_OVERFLOW;
                }
                break;

            case I_EXIT_IF:
                if (vm->stack_size == 0) {
                    return RTS_STACK_UNDERFLOW;
                }
                int64_t cond = token.exit_if.cond;
                int64_t exit_code = token.exit_if.exit_code;
                if (vm->stack[vm->stack_size - 1] == cond) {
                    exit(exit_code);
                }
                break;

            case I_STR_LEN:
                if (vm->stack_size == 0) {
                    return RTS_STACK_UNDERFLOW;
                }
                char *str = (char *)vm->stack[vm->stack_size - 1];
                int64_t len = strlen(str);

                if (push(vm, len) != RTS_OK) {
                    return RTS_STACK_OVERFLOW;
                }
                break;
            case I_INPUT_STR:
                char buffer[1024] = {0};
                scanf("%1023s", buffer);
                if (push_str(vm, buffer) != RTS_OK) {
                    return RTS_STACK_OVERFLOW;
                }
                break;

            case I_EXECUTE_CMD:
                system(token.str_value);
                break;
            case I_IGNORE:
                i += 2;
                break;
            case I_IGNORE_IF:
                if (vm->stack_size == 0) {
                    return RTS_STACK_UNDERFLOW;
                }
                int64_t cond1 = token.int_value;
                if (vm->stack[vm->stack_size - 1] == cond1) {
                    i += 2;
                }
                break;
            case I_GET:
                if (vm->stack_size == 0) {
                    return RTS_STACK_UNDERFLOW;
                } else if (token.int_value > vm->stack_size) {
                    return RTS_STACK_OVERFLOW;
                }
                int64_t index = token.int_value;
                if (push(vm, vm->stack[index]) != RTS_OK) {
                    return RTS_STACK_OVERFLOW;
                }
                break;
            default:
                return RTS_UNDEFINED_INSTRUCTION;
        }

    }
    return RTS_OK;
}

RTStatus OrtaVM_load_program_from_file(OrtaVM *vm, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return RTS_ERROR;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size % sizeof(Token) != 0) {
        fclose(file);
        return RTS_ERROR;
    }

    vm->program.size = file_size / sizeof(Token);
    fread(vm->program.tokens, sizeof(Token), vm->program.size, file);

    fclose(file);
    return RTS_OK;
}

RTStatus OrtaVM_save_program_to_file(OrtaVM *vm, char *filename) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        return RTS_ERROR;
    }

    size_t written = fwrite(vm->program.tokens,
                           sizeof(Token),
                           vm->program.size,
                           file);

    if (written != vm->program.size) {
        fclose(file);
        return RTS_ERROR;
    }

    fclose(file);
    return RTS_OK;
}


void OrtaVM_dump(OrtaVM *vm) {
    printf("Stack (top to bottom):\n");
    for (size_t i = vm->stack_size; i > 0; i--) {
        if (vm->stack[i - 1] == 0) continue;
        printf("%ld\n", vm->stack[i - 1]);
    }
}

RTStatus OrtaVM_parse_program(OrtaVM *vm, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "ERROR: Failed to open file %s\n", filename);
        return RTS_ERROR;
    }

    char line[256];
    size_t program_size = 0;
    Token tokens[1024];
    int line_count = 0;
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '\n' || line[0] == '#') continue;

        char instruction[32];
        char args[4][128] = {{0}};
        int matched_args = 0;

        int matched = sscanf(line, "%31s %127s %127s %127s %127s",
                             instruction, args[0], args[1], args[2], args[3]);
        if (matched < 1) {
            fprintf(stderr, "ERROR: Invalid syntax: %s", line);
            fclose(file);
            return RTS_ERROR;
        }

        line_count++;

        Token token = {0};
        token.inst = -1;

        if (strcmp(instruction, "PUSH") == 0) {
            token.inst = I_PUSH;
            if (matched < 2 || sscanf(args[0], "%ld", &token.int_value) != 1) {
                fprintf(stderr, "ERROR: PUSH expects an integer: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        }  else if (strcmp(instruction, "ADD") == 0) {
            token.inst = I_ADD;
        } else if (strcmp(instruction, "SUB") == 0) {
            token.inst = I_SUB;
        } else if (strcmp(instruction, "DIV") == 0) {
            token.inst = I_DIV;
        } else if (strcmp(instruction, "MUL") == 0) {
            token.inst = I_MUL;
        } else if (strcmp(instruction, "PRINT") == 0) {
            token.inst = I_PRINT;
        } else if (strcmp(instruction, "PRINT_STR") == 0) {
             token.inst = I_PRINT_STR;

             strcpy(token.str_value, extract_content(line));
             token.str_value[strlen(token.str_value) + 1] = '\0';
        } else if (strcmp(instruction, "DUP") == 0) {
            token.inst = I_DUP;
        } else if (strcmp(instruction, "JMP") == 0) {
            token.inst = I_JMP;
            if (matched < 2 || sscanf(args[0], "%ld", &token.int_value) != 1) {
                fprintf(stderr, "ERROR: JMP expects a line number: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        } else if (strcmp(instruction, "JMP_IF") == 0) {
            token.inst = I_JMP_IF;
            if (matched < 3 || sscanf(args[0], "%ld", &token.jump_if_args.dest) != 1 || sscanf(args[1], "%ld", &token.jump_if_args.cond) != 1) {
                fprintf(stderr, "ERROR: JMP_IF expects a line number: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        } else if (strcmp(instruction, "DROP") == 0) {
            token.inst = I_DROP;
        }
        else if (strcmp(instruction, "NOP") == 0) {
            token.inst = I_NOP;
        }
        else if (strcmp(instruction, "EXIT") == 0) {
            token.inst = I_EXIT;

            if (matched < 2 || sscanf(args[0], "%ld", &token.int_value) != 1) {
                fprintf(stderr, "ERROR: EXIT expects an exit code: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        }
        else if (strcmp(instruction, "INPUT") == 0) {
            token.inst = I_INPUT;
        }
        else if (strcmp(instruction, "FILE_OPEN") == 0) {
            token.inst = I_FILE_OPEN;
            strcpy(token.str_value, extract_content(line));
            token.str_value[strlen(token.str_value) + 1] = '\0';
        }
        else if (strcmp(instruction, "FILE_WRITE") == 0) {
            token.inst = I_FILE_WRITE;
            strcpy(token.str_value, extract_content(line));
            token.str_value[strlen(token.str_value) + 1] = '\0';
        }
        else if (strcmp(instruction, "FILE_READ") == 0) {
            token.inst = I_FILE_READ;
        } else if (strcmp(instruction, "CAST_AND_PRINT") == 0) {
            token.inst = I_CAST_AND_PRINT;
            insecure_function(line_count, "CAST_AND_PRINT");
        } else if (strcmp(instruction, "SWAP") == 0) {
            token.inst = I_SWAP;
        } else if (strcmp(instruction, "EQ") == 0) {
            token.inst = I_EQ;
        } else if (strcmp(instruction, "RANDOM_INT") == 0) {
            token.inst = I_RANDOM_INT;
            if (matched < 3 || sscanf(args[0], "%ld", &token.random_int.min) != 1 || sscanf(args[1], "%ld", &token.random_int.max) != 1) {
                fprintf(stderr, "ERROR: RANDOM_INT expects two integers: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        } else if (strcmp(instruction, "COLOR") == 0) {
            token.inst = I_COLOR;
            strcpy(token.str_value, extract_content(line));
            token.str_value[strlen(token.str_value) + 1] = '\0';
        } else if (strcmp(instruction, "CLEAR") == 0) {
            token.inst = I_CLEAR;
        }
        else if (strcmp(instruction, "SLEEP") == 0) {
            token.inst = I_SLEEP;

            if (matched < 2 || sscanf(args[0], "%ld", &token.int_value) != 1) {
                fprintf(stderr, "ERROR: Sleep expects ms: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        } else if (strcmp(instruction, "CLEAR_STACK") == 0) {
            token.inst = I_CLEAR_STACK;
        } else if (strcmp(instruction, "GT") == 0) {
            token.inst = I_GT; // Greater than
        } else if (strcmp(instruction, "LT") == 0) {
            token.inst = I_LT; // Less then
        } else if (strcmp(instruction, "PUSH_STR") == 0) {
            token.inst = I_PUSH_STR;
            strcpy(token.str_value, extract_content(line));
            token.str_value[strlen(token.str_value) + 1] = '\0';
        } else if (strcmp(instruction, "EXIT_IF") == 0) {
            token.inst = I_EXIT_IF;
            if (matched < 3 || sscanf(args[0], "%ld", &token.exit_if.exit_code) != 1 || sscanf(args[1], "%ld", &token.exit_if.cond) != 1) {
                fprintf(stderr, "ERROR: EXIT_IF expects an exit code and condition: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        } else if (strcmp(instruction, "STRLEN") == 0) {
            insecure_function(line_count, "STRLEN");
            token.inst = I_STR_LEN;
        } else if (strcmp(instruction, "INPUT_STR") == 0) {
            token.inst = I_INPUT_STR;
        } else if (strcmp(instruction, "EXECUTE_CMD") == 0) {
            token.inst = I_EXECUTE_CMD;
            strcpy(token.str_value, extract_content(line));
            token.str_value[strlen(token.str_value) + 1] = '\0';
        } else if (strcmp(instruction, "IGNORE") == 0) {
            token.inst = I_IGNORE;
        } else if (strcmp(instruction, "IGNORE_IF") == 0) {
            token.inst = I_IGNORE_IF;
            if (matched < 2 || sscanf(args[0], "%ld", &token.int_value) != 1) {
                fprintf(stderr, "ERROR: IGNORE_IF expects cond: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        } else if (strcmp(instruction, "GET") == 0) {
            token.inst = I_GET;
            if (matched < 2 || sscanf(args[0], "%ld", &token.int_value) != 1) {
                fprintf(stderr, "ERROR: GET expects a stack index: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        }



        if (token.inst == -1) {
            fprintf(stderr, "ERROR: Unknown instruction: %s\n", instruction);
            fclose(file);
            return RTS_ERROR;
        }

        if (program_size >= 1024) {
            fprintf(stderr, "ERROR: Program too large.\n");
            fclose(file);
            return RTS_ERROR;
        }
        tokens[program_size++] = token;
    }

    memcpy(vm->program.tokens, tokens, program_size * sizeof(Token));
    vm->program.size = program_size;

    fclose(file);
    return RTS_OK;
}


bool ends_with(const char *string, const char *suffix) {
    size_t string_len = strlen(string);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > string_len) return false;

    return strncmp(string + string_len - suffix_len, suffix, suffix_len) == 0;
}




#endif // ORTA_H



