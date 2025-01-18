// GLOBAL TODOS:
// TODO: Add a syscall inst there for i need a abstraction layer: int syscall(int syscall, int arg1, int arg2);


#ifndef ORTA_H
#define ORTA_H
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#define O_STACK_CAPACITY 200000
#define PREPROC_SYMBOL '#'
#define O_DEFAULT_BUFFER_CAPACITY 1024
#define O_DEFAULT_SIZE 256

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
    I_JMP_IF,         // JMP to given index while == cond
    I_JMP_IFN,         // JMP to given index while != cond
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
    I_STR_CMP,         // Compare 2 strings (one out stack, one out stack)

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

    // Some functions
    I_RANDOM_INT,     // Pushes an random int to stack
    I_PUSH_CURRENT_INST,
    I_JMP_FS,          // JMP to given index from the stack
    I_BFUNC,           // Builtin Functions (C functions)
    I_CAST,
    I_SEND,            // Send a message to a socket
} Instruction;

// Flags
size_t level = 0;
size_t limit = 1000;

void insecure_function(int line_count, char *func, char *reason) {
    if (level == 1) {
        printf("\033[33mWarning: Usage of insecure function at token %d (Reason: %s): %s\033[0m\n", line_count, reason, func);
    }
}

void error_occurred(char *token, char *reason) {
    if (level == 1) {
        printf("\033[31mError: %s (Reason: %s)\033[0m\n", token, reason);
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
        char str_value[O_DEFAULT_BUFFER_CAPACITY];
        JumpIfArgs jump_if_args;
        RandomInt random_int;
        ExitIf exit_if;
};
} Token;

typedef struct {
    Token tokens[O_DEFAULT_BUFFER_CAPACITY];
    size_t size;
} Program;

typedef struct OrtaVM OrtaVM;
typedef void (*BFunc)(OrtaVM*);

struct OrtaVM {
    int64_t stack[O_STACK_CAPACITY];
    size_t stack_size;

    // TODO: Use it somewhere
    int64_t orta_stack[8]; // stack of runtime values (only accessible by OrtaVM) (like registers)
    size_t orta_stack_size;

    BFunc bfunctions[O_DEFAULT_SIZE];
    size_t bfunctions_size;

    Program program;
};

void push_bfunc(OrtaVM *vm, BFunc func) {
    if (vm->bfunctions_size >= O_DEFAULT_SIZE) {
        return;
    }
    vm->bfunctions[vm->bfunctions_size++] = func;
}

void OrtaVM_dump(OrtaVM *vm) {
    printf("Stack (top to bottom):\n");
    for (size_t i = vm->stack_size; i > 0; i--) {
        if (vm->stack[i - 1] == 0) continue;
        printf("%ld\n", vm->stack[i - 1]);
    }
}

RTStatus push(OrtaVM *vm, int64_t value) {
    if (vm->stack_size >= O_STACK_CAPACITY) {
        return RTS_STACK_OVERFLOW;
    }
    vm->stack[vm->stack_size++] = value;
    return RTS_OK;
}

RTStatus push_str(OrtaVM *vm, const char *str) {
    if (vm->stack_size >= O_STACK_CAPACITY) {
        return RTS_STACK_OVERFLOW;
    }
    char *dynamic_str = strdup(str);
    if (!dynamic_str) {
        return RTS_ERROR;
    }
    vm->stack[vm->stack_size++] = (int64_t)dynamic_str;
    return RTS_OK;
}

void no_win_support() {
    #ifdef _WIN32
    printf("Sorry, but this Function is not supported on Windows.\n");
    exit(1);
    #endif
}

#ifndef _WIN32
// TODO: Make it os independent
void asocket(OrtaVM *vm) {
    no_win_support();
    int64_t result = socket(AF_INET, SOCK_STREAM, 0);
    if (push(vm, result) != RTS_OK) {
        error_occurred("asocket", "Stack Overflow");
    }
}

void abind(OrtaVM *vm) {
    no_win_support();
    if (vm->stack_size < 2) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }
    int64_t sockfd = vm->stack[vm->stack_size - 1];
    if (sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    int64_t port = vm->stack[vm->stack_size - 2];
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}


void alisten(OrtaVM *vm) {
    no_win_support();
    if (vm->stack_size < 2) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }
    int64_t sockfd = vm->stack[--vm->stack_size];
    int64_t backlog = vm->stack[--vm->stack_size];

    int64_t result = listen(sockfd, backlog);
}

void aaccept(OrtaVM *vm) {
    no_win_support();
    if (vm->stack_size < 1) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }
    int64_t sockfd = vm->stack[vm->stack_size - 1];

    int64_t result = accept(sockfd, NULL, NULL);
    if (push(vm, result) != RTS_OK) {
        error_occurred("aaccept", "Stack Overflow");
    }
}

void aconnect(OrtaVM *vm) {
    no_win_support();
    if (vm->stack_size < 2) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }

    int64_t sockfd = vm->stack[vm->stack_size - 1];
    int64_t port = vm->stack[vm->stack_size - 2];

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}


void asend(OrtaVM *vm) {
    no_win_support();

    if (vm->stack_size < 2) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }

    int64_t sockfd = vm->stack[vm->stack_size - 1];

    char *data = (char *)vm->stack[vm->stack_size - 2];

    if (data == NULL) {
        fprintf(stderr, "Invalid string data\n");
        exit(1);
    }

    if (sockfd < 0) {
        fprintf(stderr, "Invalid socket descriptor\n");
        exit(1);
    }

    ssize_t result = send(sockfd, data, strlen(data), 0);
    if (result < 0) {
        perror("Send failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    vm->stack_size -= 2;
}


void arecv(OrtaVM *vm) {
    no_win_support();
    if (vm->stack_size < 1) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }

    int64_t sockfd = vm->stack[vm->stack_size - 1];
    char buffer[O_DEFAULT_BUFFER_CAPACITY];
    ssize_t result = recv(sockfd, buffer, sizeof(buffer), 0);
    if (result < 0) {
        perror("Receive failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    buffer[result] = '\0';
    push_str(vm, buffer);
}

void aclose(OrtaVM *vm) {
    no_win_support();
    if (vm->stack_size < 1) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }

    int64_t sockfd = vm->stack[vm->stack_size - 1];
    if (close(sockfd) < 0) {
        perror("Close failed");
        exit(EXIT_FAILURE);
    }
}

void aset_opt(OrtaVM *vm) {
    no_win_support();
    if (vm->stack_size < 3) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }

    int64_t sockfd = vm->stack[vm->stack_size - 1];
    int64_t option_name = vm->stack[vm->stack_size - 2];
    int64_t option_value = vm->stack[vm->stack_size - 3];

    if (setsockopt(sockfd, SOL_SOCKET, option_name, &option_value, sizeof(option_value)) < 0) {
        perror("Set socket option failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

void init_socket_bfuncs(OrtaVM *vm) {
    BFunc sockett = asocket;
    BFunc bindd = abind;
    BFunc listenn = alisten;
    BFunc acceptt = aaccept;
    BFunc connectt = aconnect;
    BFunc sendt = asend;
    BFunc recvt = arecv;
    BFunc closet = aclose;
    BFunc setoptt = aset_opt;

    push_bfunc(vm, sockett);
    push_bfunc(vm, bindd);
    push_bfunc(vm, listenn);
    push_bfunc(vm, acceptt);
    push_bfunc(vm, connectt);
    push_bfunc(vm, sendt);
    push_bfunc(vm, recvt);
    push_bfunc(vm, closet);
    push_bfunc(vm, setoptt);
}

#endif
void init_bfuncs(OrtaVM *vm) {
    #ifndef _WIN32
    init_socket_bfuncs(vm);
    #endif
}

char *extract_content(const char *line) {
    const char *start = strchr(line, '"');
    const char *end = strrchr(line, '"');
    if (!start || !end || start == end) {
        return "No text provided";
    }
    size_t length = end - start - 1;
    static char content[O_DEFAULT_BUFFER_CAPACITY];
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
    init_bfuncs(vm);
    Program program = vm->program;
    size_t iterations = 0;
    for (size_t i = 0; i < program.size && iterations < limit*4; i++) {
        iterations++;
        Token token = program.tokens[i];
        switch (token.inst) {
            case I_PUSH:
                if (push(vm, token.int_value) != RTS_OK) {
                    error_occurred("PUSH", "Stack Overflow");
                }
                break;
            case I_ADD:
                if (add(vm) != RTS_OK) {
                    error_occurred("ADD", "Stack Underflow");

                }
                break;
            case I_SUB:
                if (sub(vm) != RTS_OK) {
                    error_occurred("SUB", "Stack Underflow");
                }
                break;
            case I_PRINT:
                if (print(vm) != RTS_OK) {
                    error_occurred("PRINT", "Stack Underflow");
                }
                break;
            case I_MUL:
                if (vm->stack_size < 2) {
                    error_occurred("MUL", "Stack Underflow");
                }
                int64_t a = vm->stack[--vm->stack_size];
                int64_t b = vm->stack[--vm->stack_size];
                if (push(vm, a * b) != RTS_OK) {
                    error_occurred("MUL", "Stack Overflow");
                }
                break;
            case I_DIV:
                if (vm->stack_size < 2) {
                    error_occurred("DIV", "Stack Underflow");
                }
                int64_t c = vm->stack[--vm->stack_size];
                int64_t d = vm->stack[--vm->stack_size];
                if (d == 0) {
                    return RTS_ERROR;
                }
                if (push(vm, c / d) != RTS_OK) {
                    error_occurred("DIV", "Stack Overflow");
                }
                break;
            case I_PRINT_STR:
                if (token.str_value[0] != '\0') {
                    printf("%s\n", token.str_value);
                }

                break;
            case I_DROP:
                if (vm->stack_size == 0) {
                    error_occurred("DROP", "Stack Underflow");
                }
                vm->stack_size--;
                break;
            case I_JMP:
                 if (token.int_value < 0 || token.int_value > program.size) {
                     error_occurred("JMP", "Stack Underflow");
                 }
                 i = token.int_value;
                 continue;
            case I_JMP_IFN: // JMP_IFB DEST COND | JMP_IFN 0 10 | JMP_IFN 0 (!= 10)
                 if (vm->stack_size == 0) {
                     error_occurred("JMP_IF", "Stack Underflow");
                 }
                 if (vm->stack[vm->stack_size - 1] != token.jump_if_args.cond) {
                     if (token.jump_if_args.dest < 0 || token.jump_if_args.dest > program.size) {
                         error_occurred("JMP_IFN", "Stack Underflow");
                     }
                     i = token.jump_if_args.dest;
                     continue;
                 }
                 break;
            case I_JMP_IF: // JMP_IF DEST COND | JMP_IF 0 10 | JMP_IF 0 (== 10)
                 if (vm->stack_size == 0) {
                     error_occurred("JMP_IF", "Stack Underflow");
                 }
                 if (vm->stack[vm->stack_size - 1] == token.jump_if_args.cond) {
                     if (token.jump_if_args.dest < 0 || token.jump_if_args.dest > program.size) {
                         error_occurred("JMP_IF", "Stack Underflow");
                     }
                     i = token.jump_if_args.dest;
                     continue;
                 }
                 break;
            case I_DUP:
                if (vm->stack_size == 0) {
                    error_occurred("DUP", "Stack Underflow");
                }
                if (vm->stack_size >= O_STACK_CAPACITY) {
                    error_occurred("DUP", "Stack Overflow");
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
                     error_occurred("INPUT", "Stack Overflow");
                 }
                 break;
            case I_FILE_OPEN:
                FILE *file = fopen(token.str_value, "a+");
                if (file == NULL) {
                    error_occurred("FILE_OPEN", "File allocation failed");
                }
                int64_t fd = (int64_t)file;
                if (push(vm, fd) != RTS_OK) {
                    fclose(file);
                    error_occurred("FILE_OPEN", "Stack Overflow");
                }
                break;

            case I_FILE_WRITE:
                {
                    FILE *file1 = (FILE *)vm->stack[vm->stack_size - 1];
                    char *buffer = token.str_value;

                    if (file1 == NULL) {
                        error_occurred("FILE_WRITE", "File allocation failed");
                    }

                    fwrite(buffer, sizeof(char), strlen(buffer), file1);
                    break;
                }

            case I_FILE_READ:
                {
                    FILE *file2 = (FILE *)vm->stack[vm->stack_size - 1];
                    if (file2 == NULL) {
                        error_occurred("FILE_READ", "File allocation failed");
                    }

                    char buffer[O_DEFAULT_BUFFER_CAPACITY] = {0};
                    fread(buffer, sizeof(char), 1023, file2);
                    buffer[strlen(buffer)] = '\0';

                    if (push(vm, (int64_t)buffer) != RTS_OK) {
                        error_occurred("FILE_READ", "Stack Overflow");
                    }
                    break;
                }
            case I_CAST_AND_PRINT:
                {
                    char *buffer = (char *)vm->stack[vm->stack_size - 1];
                    if (buffer == NULL) {
                        error_occurred("CAST_AND_PRINT", "Cast failed");
                    }

                    printf("%s\n", buffer);
                    break;
                }

            case I_SWAP:
                if (vm->stack_size < 2) {
                    error_occurred("SWAP", "Stack Underflow");
                }
                int64_t temp = vm->stack[vm->stack_size - 1];
                vm->stack[vm->stack_size - 1] = vm->stack[vm->stack_size - 2];
                vm->stack[vm->stack_size - 2] = temp;
                break;
            case I_EQ:
                if (vm->stack_size < 2) {
                    error_occurred("EQ", "Stack Underflow");
                }
                int64_t a1 = vm->stack[vm->stack_size - 1];
                int64_t b1 = vm->stack[vm->stack_size - 2];
                int64_t result = (a1 == b1) ? 1 : 0;
                if (push(vm, result) != RTS_OK) {
                    error_occurred("EQ", "Stack Overflow");
                }
                break;
            case I_RANDOM_INT:
                {
                    srand(time(NULL));
                    int64_t min = token.random_int.min;
                    int64_t max = token.random_int.max;
                    int64_t random = min + rand() % (max - min + 1);
                    if (push(vm, random) != RTS_OK) {
                        error_occurred("RANDOM_INT", "Stack Overflow");
                    }
                    break;
                }
            case I_COLOR:
                {
                    // DONE: Add more colors
                    #ifdef _WIN32
                    DWORD dwMode = 0;
                    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
                    GetConsoleMode(hStdout, &dwMode);
                    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                    SetConsoleMode(hStdout, dwMode);
                    #endif

                    char color[O_DEFAULT_BUFFER_CAPACITY] = {0};
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
                    } else if (strcmp(token.str_value, "black") == 0) {
                        strcpy(color, "\033[30m");
                    } else if (strcmp(token.str_value, "gray") == 0) {
                        strcpy(color, "\033[90m");
                    } else if (strcmp(token.str_value, "light_red") == 0) {
                        strcpy(color, "\033[91m");
                    } else if (strcmp(token.str_value, "light_green") == 0) {
                        strcpy(color, "\033[92m");
                    } else if (strcmp(token.str_value, "light_yellow") == 0) {
                        strcpy(color, "\033[93m");
                    } else if (strcmp(token.str_value, "light_blue") == 0) {
                        strcpy(color, "\033[94m");
                    } else if (strcmp(token.str_value, "light_magenta") == 0) {
                        strcpy(color, "\033[95m");
                    } else if (strcmp(token.str_value, "light_cyan") == 0) {
                        strcpy(color, "\033[96m");
                    } else if (strcmp(token.str_value, "light_white") == 0) {
                        strcpy(color, "\033[97m");
                    } else if (strcmp(token.str_value, "reset") == 0) {
                        strcpy(color, "\033[0m");
                    } else {
                        error_occurred("COLOR", "Invalid Color");
                    }

                    printf("%s", color);
                    break;
                }
            case I_CLEAR:
                // DONE: Make it os independent
                #if defined(_WIN32) || defined(_WIN64)
                    system("cls");
                #else
                   system("clear");
                #endif
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
                    error_occurred("LT", "Stack Underflow");
                }
                int64_t a2 = vm->stack[vm->stack_size - 1];
                int64_t b2 = vm->stack[vm->stack_size - 2];
                int64_t result2 = (b2 < a2) ? 1 : 0;
                if (push(vm, result2) != RTS_OK) {
                    error_occurred("LT", "Stack Overflow");
                }
                break;
            case I_GT:
                if (vm->stack_size < 2) {
                    error_occurred("GT", "Stack Underflow");
                }
                int64_t a3 = vm->stack[vm->stack_size - 1];
                int64_t b3 = vm->stack[vm->stack_size - 2];
                int64_t result3 = (b3 > a3) ? 1 : 0;
                if (push(vm, result3) != RTS_OK) {
                    error_occurred("GT", "Stack Overflow");
                }
                break;
            case I_PUSH_STR:
                if (push_str(vm, token.str_value) != RTS_OK) {
                    error_occurred("PUSH_STR", "Stack Overflow");
                }
                break;

            case I_EXIT_IF:
                if (vm->stack_size == 0) {
                    error_occurred("EXIT_IF", "Stack Underflow");
                }
                int64_t cond = token.exit_if.cond;
                int64_t exit_code = token.exit_if.exit_code;
                if (vm->stack[vm->stack_size - 1] == cond) {
                    exit(exit_code);
                }
                break;

            case I_STR_LEN:
                if (vm->stack_size == 0) {
                    error_occurred("STRLEN", "Stack Underflow");
                }
                char *str = (char *)vm->stack[vm->stack_size - 1];
                int64_t len = strlen(str);

                if (push(vm, len) != RTS_OK) {
                    error_occurred("STRLEN", "Stack Overflow");
                }
                break;
            case I_INPUT_STR:
                char buffer[O_DEFAULT_BUFFER_CAPACITY] = {0};
                scanf("%1023s", buffer);
                if (push_str(vm, buffer) != RTS_OK) {
                    error_occurred("INPUT_STR", "Stack Overflow");
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
                    error_occurred("IGNORE_IF", "Stack Underflow");
                }
                int64_t cond1 = token.int_value;
                if (vm->stack[vm->stack_size - 1] == cond1) {
                    i += 2;
                }
                break;
            case I_GET:
                if (vm->stack_size == 0) {
                    error_occurred("GET", "Stack Underflow");
                } else if (token.int_value > vm->stack_size) {
                    error_occurred("GET", "Stack Overflow");
                }
                int64_t index = token.int_value;
                if (push(vm, vm->stack[index]) != RTS_OK) {
                    error_occurred("GET", "Stack Overflow");
                }
                break;
            case I_STR_CMP:
                if (vm->stack_size == 0) {
                    error_occurred("STRCMP", "Stack Underflow");
                }

                char *str1 = (char *)vm->stack[vm->stack_size - 1];
                char *str2 = token.str_value;

                int64_t strcmpres = strcmp(str1, str2);
                int64_t result4 = 0;
                if (strcmpres == 0) result4 = 1;
                if (push(vm, result4) != RTS_OK) {
                    error_occurred("STRCMP", "Stack Overflow");
                }
                break;

            case I_PUSH_CURRENT_INST:
                if (push(vm, i) != RTS_OK) {
                    error_occurred("PUSH_CURRENT", "Stack Overflow");
                }
                break;
            case I_JMP_FS:
                if (vm->stack_size == 0) {
                    error_occurred("JMP_FS", "Stack Underflow");
                }
                i = vm->stack[vm->stack_size];
                continue;
            case I_BFUNC:
                if (vm->bfunctions_size < token.int_value) {
                    error_occurred("BFUNC", "Invalid Function index");
                }

                vm->bfunctions[token.int_value](vm);
                break;
            case I_CAST:
               if (vm->stack_size == 0) {
                   error_occurred("CAST", "Stack Underflow");
               }
               int64_t value = vm->stack[vm->stack_size - 1];
               vm->stack_size--;
               if (strcmp(token.str_value, "int") == 0) {
                   if (push(vm, value) != RTS_OK) {
                       error_occurred("CAST", "Stack Overflow");
                   }
               } else if (strcmp(token.str_value, "str") == 0) {
                   char *str = (char *)value;
                   if (push_str(vm, str) != RTS_OK) {
                       error_occurred("CAST", "Stack Overflow");
                   }
               } else {
                   error_occurred("CAST", "Invalid Type");
               }
            case I_SEND:
                no_win_support();

                if (vm->stack_size < 1) {
                    fprintf(stderr, "Stack underflow\n");
                    exit(1);
                }

                int64_t sockfd = vm->stack[vm->stack_size - 1];

                char *data = token.str_value;

                if (data == NULL) {
                    fprintf(stderr, "Invalid string data\n");
                    exit(1);
                }

                if (sockfd < 0) {
                    fprintf(stderr, "Invalid socket descriptor\n");
                    exit(1);
                }

                ssize_t result6 = send(sockfd, data, strlen(data), 0);
                if (result < 0) {
                    perror("Send failed");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }

                vm->stack_size -= 1;
                break;
            default:
                error_occurred("Unknown Instruction", "");
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

char variables[O_DEFAULT_BUFFER_CAPACITY][2][O_DEFAULT_BUFFER_CAPACITY] = {0};
size_t variable_count = 0;

void push_variable(const char *name, const char *value) {
    if (variable_count >= O_DEFAULT_BUFFER_CAPACITY) {
        printf("Error: Variable storage full.\n");
        return;
    }

    strncpy(variables[variable_count][0], name, O_DEFAULT_BUFFER_CAPACITY - 1);
    variables[variable_count][0][O_DEFAULT_BUFFER_CAPACITY - 1] = '\0';

    strncpy(variables[variable_count][1], value, O_DEFAULT_BUFFER_CAPACITY - 1);
    variables[variable_count][1][O_DEFAULT_BUFFER_CAPACITY - 1] = '\0';

    variable_count++;
}

char *get_variable(char *name) {
    for (size_t i = 0; i < variable_count; i++) {
        if (strcmp(variables[i][0], name) == 0) {
            return variables[i][1];
        }
    }
    return "unknown";
}

void init_variables() {
    push_variable("orta_version", "1.0"); // TODO: Dont forgot to update
    #ifdef _WIN32
        push_variable("os", "windows");
    #else
        push_variable("os", "linux");
    #endif
}

RTStatus OrtaVM_parse_program(OrtaVM *vm, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "ERROR: Failed to open file %s\n", filename);
        return RTS_ERROR;
    }

    char line[O_DEFAULT_SIZE];
    size_t program_size = 0;
    Token tokens[O_DEFAULT_BUFFER_CAPACITY];
    int line_count = 0;
    init_variables();
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
        } else if (strcmp(instruction, "JMP_IFN") == 0) {
            token.inst = I_JMP_IFN;
            if (matched < 3 || sscanf(args[0], "%ld", &token.jump_if_args.dest) != 1 || sscanf(args[1], "%ld", &token.jump_if_args.cond) != 1) {
                fprintf(stderr, "ERROR: JMP_IFN expects a line number: %s", line);
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
            insecure_function(line_count, "CAST_AND_PRINT", "The latest stack element is casting to char *");
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
            insecure_function(line_count, "STRLEN", "if the latest element is not a char * it will throw an segfault");
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
        } else if (strcmp(instruction, "STRCMP") == 0) {
            token.inst = I_STR_CMP;
            strcpy(token.str_value, extract_content(line));
            token.str_value[strlen(token.str_value) + 1] = '\0';
        } else if (strcmp(instruction, "PUSH_CURRENT") == 0) {
            token.inst = I_PUSH_CURRENT_INST;
        } else if (strcmp(instruction, "JMP_FS") == 0) {
            token.inst = I_JMP_FS;
        } else if (strcmp(instruction, "VAR") == 0) {
            token.inst = I_PUSH_STR;
            char variable[O_DEFAULT_BUFFER_CAPACITY];
            strcpy(variable, extract_content(line));
            variable[strlen(variable) + 1] = '\0';
            strcpy(token.str_value, get_variable(variable));
        } else if (strcmp(instruction, "BFUNC") == 0) {
            token.inst = I_BFUNC;

            if (matched < 2 || sscanf(args[0], "%ld", &token.int_value) != 1) {
                fprintf(stderr, "ERROR: BFUNC expects a function: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        } else if (strcmp(instruction, "CAST") == 0) {
            token.inst = I_CAST;
            strcpy(token.str_value, extract_content(line));
            token.str_value[strlen(token.str_value) + 1] = '\0';
        } else if (strcmp(instruction, "SEND") == 0) {
            token.inst = I_SEND;
            strcpy(token.str_value, extract_content(line));
            token.str_value[strlen(token.str_value) + 1] = '\0';
        }

        if (token.inst == -1) {
            fprintf(stderr, "ERROR: Unknown instruction: %s\n", instruction);
            fclose(file);
            return RTS_ERROR;
        }

        if (program_size >= O_DEFAULT_BUFFER_CAPACITY) {
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

void expand_tilde(const char *path, char *expanded_path, size_t max_length) {
    if (path[0] == '~') {
        #ifdef _WIN32
            const char *home = getenv("USERPROFILE");
        #else
            const char *home = getenv("HOME");
        #endif
        if (home) {
            snprintf(expanded_path, max_length, "%s%s", home, path + 1);
        } else {
            fprintf(stderr, "ERROR: HOME environment variable is not set\n");
            strncpy(expanded_path, path, max_length);
        }
    } else {
        strncpy(expanded_path, path, max_length);
    }
}

// DONE: Make it os independent
static char include_paths[10][O_DEFAULT_SIZE] = {".", "~/.orta"};
static size_t include_paths_count = 2;
static char macros[O_DEFAULT_SIZE][2][O_DEFAULT_SIZE];
static size_t macros_count = 0;

static int is_preprocessor_defined(char preprocessors[O_DEFAULT_SIZE][2][O_DEFAULT_SIZE], size_t preprocessors_count,const char *name) {
    for (size_t i = 0; i < preprocessors_count; i++) {
        if (strcmp(preprocessors[i][0], name) == 0) {
            return 1;
        }
    }
    return 0;
}

RTStatus OrtaVM_preprocess_file(char *filename, int argc, char argv[20][256]) {
    char preprocessed_filename[O_DEFAULT_SIZE];
    static char preprocessors[O_DEFAULT_SIZE][2][O_DEFAULT_SIZE];
    static size_t preprocessors_count = 0;

    for (int i = 0; i < argc; i++) {
        char arg_preproc[O_DEFAULT_SIZE];
        snprintf(arg_preproc, sizeof(arg_preproc), "$%d", i);
        strncpy(preprocessors[preprocessors_count][0], arg_preproc, O_DEFAULT_SIZE);
        strncpy(preprocessors[preprocessors_count][1], argv[i], O_DEFAULT_SIZE);
        preprocessors_count++;
    }
    #ifdef _WIN32
        strncpy(preprocessors[preprocessors_count][0], "OS_WINDOWS", O_DEFAULT_SIZE);
        strncpy(preprocessors[preprocessors_count][1], "1", O_DEFAULT_SIZE);
        preprocessors_count++;
    #else
        strncpy(preprocessors[preprocessors_count][0], "OS_POSIX", O_DEFAULT_SIZE);
        strncpy(preprocessors[preprocessors_count][1], "1", O_DEFAULT_SIZE);
        preprocessors_count++;
    #endif

    snprintf(preprocessed_filename, sizeof(preprocessed_filename), "%s.ppd", filename);

    FILE *input_file = fopen(filename, "r");
    if (input_file == NULL) {
        fprintf(stderr, "ERROR: Failed to open file %s\n", filename);
        return RTS_ERROR;
    }

    FILE *output_file = fopen(preprocessed_filename, "w");
    if (output_file == NULL) {
        fprintf(stderr, "ERROR: Failed to open file %s\n", preprocessed_filename);
        fclose(input_file);
        return RTS_ERROR;
    }

    char line[O_DEFAULT_BUFFER_CAPACITY];
    int skip_section = 0;
    int in_else = 0;
    while (fgets(line, sizeof(line), input_file)) {
        if (line[0] == '\n' || line[0] == PREPROC_SYMBOL) {
            if (strncmp(line + 1, "define", 6) == 0) {
                char name[O_DEFAULT_SIZE], value[O_DEFAULT_SIZE];
                if (sscanf(line + 7, "%s %s", name, value) == 2) {
                    strncpy(preprocessors[preprocessors_count][0], name, O_DEFAULT_SIZE);
                    strncpy(preprocessors[preprocessors_count][1], value, O_DEFAULT_SIZE);
                    preprocessors_count++;
                } else if (sscanf(line + 7, "%s", name) == 1) {
                    strncpy(preprocessors[preprocessors_count][0], name, O_DEFAULT_SIZE);
                    preprocessors[preprocessors_count][1][0] = '1';
                    preprocessors[preprocessors_count][1][1] = '\0';
                    preprocessors_count++;
                } else {
                    fprintf(stderr, "WARNING: Malformed #define in line: %s\n", line);
                }
            }

            if (strncmp(line + 1, "ifdef", 5) == 0) {
                char name[O_DEFAULT_SIZE];
                if (sscanf(line + 6, "%s", name) == 1) {
                    skip_section = !is_preprocessor_defined(preprocessors, preprocessors_count, name);
                    in_else = 0;
                } else {
                    fprintf(stderr, "WARNING: Malformed #ifdef in line: %s\n", line);
                }
            }

            if (strncmp(line + 1, "ifndef", 6) == 0) {
                char name[O_DEFAULT_SIZE];
                if (sscanf(line + 7, "%s", name) == 1) {
                    skip_section = is_preprocessor_defined(preprocessors, preprocessors_count, name);
                    in_else = 0;
                } else {
                    fprintf(stderr, "WARNING: Malformed #ifndef in line: %s\n", line);
                }
            }
            if (strncmp(line + 1, "else", 4) == 0) {
                if (skip_section == 0) {
                    skip_section = 1;
                } else if (in_else == 0){
                    in_else = 1;
                    skip_section = 0;
                }
            }
            if (strncmp(line + 1, "endif", 5) == 0) {
                skip_section = 0;
                continue;
            }

            if (strncmp(line + 1, "var", 3) == 0) {
                char name[O_DEFAULT_SIZE], value[O_DEFAULT_SIZE];
                if (sscanf(line + 4, "%s %s", name, value) == 2) {
                    push_variable(name, value);
                } else {
                    fprintf(stderr, "WARNING: Malformed #var in line: %s\n", line);
                }
            }
            if (strncmp(line + 1, "macro", 5) == 0) {
                char name[O_DEFAULT_SIZE];
                char value[O_DEFAULT_BUFFER_CAPACITY] = "";
                if (sscanf(line + 6, "%s {", name) == 1) {
                    while (fgets(line, sizeof(line), input_file)) {
                        char *end_brace = strchr(line, '}');
                        if (end_brace) {
                            *end_brace = '\0';
                            strncat(value, line, sizeof(value) - strlen(value) - 1);
                            break;
                        } else {
                            strncat(value, line, sizeof(value) - strlen(value) - 1);
                        }
                    }
                    for (size_t i = 0; i < preprocessors_count; i++) {
                        char *pos;
                        while ((pos = strstr(value, preprocessors[i][0])) != NULL) {
                            char temp[O_DEFAULT_BUFFER_CAPACITY];
                            size_t before = pos - value;
                            snprintf(temp, before + 1, "%s", value);
                            snprintf(temp + before, sizeof(temp) - before, "%s%s",
                                     preprocessors[i][1], pos + strlen(preprocessors[i][0]));
                            strncpy(value, temp, sizeof(value));
                        }
                    }

                    strncpy(macros[macros_count][0], name, O_DEFAULT_SIZE);
                    strncpy(macros[macros_count][1], value, O_DEFAULT_SIZE);
                    macros_count++;
                } else {
                    fprintf(stderr, "WARNING: Malformed #macro in line: %s\n", line);
                }
                continue;
            }
            if (strncmp(line + 1, "include", 7) == 0) {
                char include_file[O_DEFAULT_SIZE];
                if (sscanf(line + 8, "%s", include_file) == 1) {
                    size_t len = strlen(include_file);
                    if (len > 1 && include_file[0] == '<' && include_file[len - 1] == '>') {
                        include_file[len - 1] = '\0';
                        memmove(include_file, include_file + 1, len - 1);
                    }
                    if (!ends_with(include_file, ".horta") && strcmp(filename, "build.orta") != 0) {
                        fprintf(stderr, "ERROR: .orta files are not supported for including (Only .horta)\n");
                        exit(1);
                    }
                    if (strcmp(include_file, filename) == 0) {
                        fprintf(stderr, "WARNING: Circular include detected: %s\n", include_file);
                        return RTS_ERROR;
                    }
                    char include_path[O_DEFAULT_SIZE];
                    int found = 0;
                    for (size_t i = 0; i < include_paths_count; i++) {

                        char expanded_path[O_DEFAULT_SIZE];
                        expand_tilde(include_paths[i], expanded_path, sizeof(expanded_path));

                        snprintf(include_path, sizeof(include_path), "%s/%s", expanded_path, include_file);

                        FILE *test_file = fopen(include_path, "r");
                        if (test_file != NULL) {
                            fclose(test_file);
                            found = 1;
                            break;
                        }
                    }

                    if (!found) {
                        fprintf(stderr, "ERROR: Include file %s not found\n", include_file);
                        fclose(input_file);
                        fclose(output_file);
                        return RTS_ERROR;
                    }

                    if (OrtaVM_preprocess_file(include_path, 0, NULL) != RTS_OK) {
                        fprintf(stderr, "ERROR: Failed to preprocess include file %s\n", include_path);
                        fclose(input_file);
                        fclose(output_file);
                        return RTS_ERROR;
                    }

                    char include_preprocessed[O_DEFAULT_SIZE];
                    snprintf(include_preprocessed, sizeof(include_preprocessed), "%s.ppd", include_path);
                    FILE *include_processed_file = fopen(include_preprocessed, "r");
                    if (include_processed_file == NULL) {
                        fprintf(stderr, "ERROR: Failed to open preprocessed include file %s\n", include_preprocessed);
                        fclose(input_file);
                        fclose(output_file);
                        return RTS_ERROR;
                    }

                    char include_line[O_DEFAULT_BUFFER_CAPACITY];
                    while (fgets(include_line, sizeof(include_line), include_processed_file)) {
                        fputs(include_line, output_file);
                    }

                    fclose(include_processed_file);
                    continue;
                } else {
                    fprintf(stderr, "WARNING: Malformed #include in line: %s\n", line);
                }
            }
            continue;
        }
        if (skip_section) {
            continue;
        }

        for (size_t i = 0; i < preprocessors_count; i++) {
            char *pos;
            while ((pos = strstr(line, preprocessors[i][0])) != NULL) {
                char temp[O_DEFAULT_BUFFER_CAPACITY];
                size_t before = pos - line;
                snprintf(temp, before + 1, "%s", line);
                snprintf(temp + before, sizeof(temp) - before, "%s%s", preprocessors[i][1], pos + strlen(preprocessors[i][0]));
                strncpy(line, temp, sizeof(line));
            }
        }

        for (size_t i = 0; i < macros_count; i++) {
            char *pos;
            while ((pos = strstr(line, macros[i][0])) != NULL) {
                char temp[O_DEFAULT_BUFFER_CAPACITY];
                size_t before = pos - line;
                snprintf(temp, before + 1, "%s", line);
                snprintf(temp + before, sizeof(temp) - before, "%s%s", macros[i][1], pos + strlen(macros[i][0]));
                strncpy(line, temp, sizeof(line));

                for (size_t j = 0; j < preprocessors_count; j++) {
                    while ((pos = strstr(line, preprocessors[j][0])) != NULL) {
                        char nested_temp[O_DEFAULT_BUFFER_CAPACITY];
                        size_t nested_before = pos - line;
                        snprintf(nested_temp, nested_before + 1, "%s", line);
                        snprintf(nested_temp + nested_before, sizeof(nested_temp) - nested_before, "%s%s",
                                 preprocessors[j][1], pos + strlen(preprocessors[j][0]));
                        strncpy(line, nested_temp, sizeof(line));
                    }
                }
            }
        }

        fputs(line, output_file);
    }

    fclose(input_file);
    fclose(output_file);

    return RTS_OK;
}

void OrtaVM_add_include_path(const char *path) {
    if (include_paths_count < 10) {
        strncpy(include_paths[include_paths_count], path, O_DEFAULT_SIZE);
        include_paths_count++;
    } else {
        fprintf(stderr, "ERROR: Maximum number of include paths reached\n");
    }
}



#endif // ORTA_H



