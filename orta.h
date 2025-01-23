// OrtaVM | Informations:
// Current Version: 1.0
// Next Version Release Date: February 20 2025
// License: NovaLicense(https://github.com/zhrxxgroup/files/blob/main/NovaLicense.md)

// Goals of OrtaVM:
// - High Performance             | Fast execution speed                                                      | DONE
// - Small Size                   | We wanna make the binaries very small                                     | Expected to be fixed in 1.4
// - Cross Platform               | You can run the code on MacOS, Linux, Windows and others                  | DONE
// - Good Error Handling          | Informative Error messages                                                | DONE

//---------------------------------------------------------------------------------------------------------------------------------

// GLOBAL TODOS:
// TODO: Add more functions
// TODO: Add support for push value evaluation (stack::push 1+1-1)

// SOMETIME TODO:
// TODO: Add Graphics and Audio libraries
// TODO: Renew deovm
// TODO: Add a syscall inst, there for i need a abstraction layer: int syscall(int syscall, int arg1, int arg2);
// TODO: Implement STR support for dump

// DONE:
// DONE: Add magic code to the binaries(.ovm)                               | OVM1 (OVM_MAGIC_CODE)
// DONE: Escape strings (PUSH_STR, PRINT_STR etc)                           | Currently only \n
// DONE: Separate buffers for int64_t and char *                            | I decided to make a Word union and use it as seperate buffers (Word)
// DONE: Make the instructions look prettier (like "push" or other style)   | I decided to allow both Uppercase(add flag --support=uppercase) and lowercase
// DONE: Add some time functions
// DONE: Optimize the OVM files (some compression or other idk i tried every thing (compression not)) | Read code

// IDEAS:
// IDEA: I think it were be cool to add support to open .so or .a files and call a function

//---------------------------------------------------------------------------------------------------------------------------------

// Changelog:


//---------------------------------------------------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------------------------------------------------

#define O_STACK_CAPACITY 200000
#define O_DEFAULT_BUFFER_CAPACITY 1024
#define O_DEFAULT_SIZE 256
#define O_COMMENT_SYMBOL '#'
//---
#define OVM_MAGIC_NUMBER "OVM1"
#define OVM_VERSION 1
//---
#define PREPROC_SYMBOL '#'

//---------------------------------------------------------------------------------------------------------------------------------

typedef enum {
    RTS_OK,
    RTS_STACK_OVERFLOW,
    RTS_STACK_UNDERFLOW,
    RTS_ERROR,
    RTS_UNDEFINED_INSTRUCTION,
    RTS_INVALID_TYPE,
} RTStatus;

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

    I_GET_DATE,        // Pushes current date to stack (%d:%m:%y)
} Instruction;

typedef enum {
    T_INT,
    T_STR,
    T_PTR,
} Type;

typedef union {
    int64_t as_i64;
    char *as_str;
    void *as_ptr;
} Word;

typedef struct {
    Word dest;
    Word cond;
} JumpIfArgs;
typedef struct {
    Word min;
    Word max;
} RandomInt;
typedef struct {
    Word exit_code;
    Word cond;
} ExitIf;

typedef struct {
    Instruction inst;
    union {
        Word word;
        JumpIfArgs jump_if_args;
        RandomInt random_int;
        ExitIf exit_if;
    };
    int pos;
} Token;

typedef struct {
    Token tokens[O_DEFAULT_BUFFER_CAPACITY];
    size_t current_token;
    size_t size;
} Program;

typedef struct OrtaVM OrtaVM;
typedef void (*BFunc)(OrtaVM*);

struct OrtaVM {
    Word stack[O_STACK_CAPACITY];
    size_t stack_size;

    // TODO: Use it somewhere
    Word orta_stack[8]; // stack of runtime values (only accessible by OrtaVM) (like registers)
    size_t orta_stack_size;

    BFunc bfunctions[O_DEFAULT_SIZE];
    size_t bfunctions_size;

    Program program;
    char *file_path;
};

//---------------------------------------------------------------------------------------------------------------------------------

// Flags
size_t level = 0;
size_t limit = 1000;
size_t support_uppercase = 0;

// Variables
char variables[O_DEFAULT_BUFFER_CAPACITY][2][O_DEFAULT_BUFFER_CAPACITY] = {0};
size_t variable_count = 0;
static char include_paths[10][O_DEFAULT_SIZE] = {".", "~/.orta"};
static size_t include_paths_count = 2;
static char macros[O_DEFAULT_SIZE][2][O_DEFAULT_SIZE];
static size_t macros_count = 0;

//---------------------------------------------------------------------------------------------------------------------------------

void push_bfunc(OrtaVM *vm, BFunc func) {
    if (vm->bfunctions_size >= O_DEFAULT_SIZE) {
        return;
    }
    vm->bfunctions[vm->bfunctions_size++] = func;
}
void insecure_function(int line_count, char *func, char *reason) {
    if (level == 1) {
        printf("\033[33mWarning: Usage of insecure function at token %d (Reason: %s): %s\033[0m\n", line_count, reason, func);
    }
}
void error_occurred(char *file, int line_count, char *token, char *reason) {
    fprintf(stderr, "\033[31m%s:%d: Token <%s>: %s\033[0m\n", file, line_count, token, reason);
    exit(0);
}

void OrtaVM_dump(OrtaVM *vm) {
    if (vm == NULL) {
        printf("Error: VM is NULL.\n");
        return;
    }
    if (vm->stack_size == 0) {
        printf("Stack is empty.\n");
        return;
    }

    printf("| STACK DUMP |\n");
    printf("| INT64      | PTR         |\n");
    for (size_t i = vm->stack_size; i > 0; i--) {
        Word word = vm->stack[i - 1];
        printf("| %ld | %p |\n", word.as_i64, word.as_ptr);
    }
}


void no_win_support() {
    #ifdef _WIN32
    printf("Sorry, but this Function is not supported on Windows.\n");
    exit(1);
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
void process_and_print_string(const char *str) {
    while (*str) {
        if (*str == '\\' && *(str + 1) == 'n') {
            putchar('\n');
            str += 2;
        } else if (*str == '\\' && *(str + 1) == 'c') {
            printf("\033[%sm", str + 2);
            while (*str && *str != '\\') {
                str++;
            }
            if (*str == '\\') str--;
        } else {
            putchar(*str);
            str++;
        }
    }
}
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
void toLowercase(char *str) {
    while (*str) {
        *str = tolower((unsigned char)*str);
        str++;
    }
}
static int is_preprocessor_defined(char preprocessors[O_DEFAULT_SIZE][2][O_DEFAULT_SIZE], size_t preprocessors_count,const char *name) {
    for (size_t i = 0; i < preprocessors_count; i++) {
        if (strcmp(preprocessors[i][0], name) == 0) {
            return 1;
        }
    }
    return 0;
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
void OrtaVM_add_include_path(const char *path) {
    if (include_paths_count < 10) {
        strncpy(include_paths[include_paths_count], path, O_DEFAULT_SIZE);
        include_paths_count++;
    } else {
        fprintf(stderr, "ERROR: Maximum number of include paths reached\n");
    }
}
char* trim_all(char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    return str;
}
char* trim_left(char* str) {
    while (isspace((unsigned char)*str)) {
        str++;
    }
    return str;
}
char* trim_right(char* str) {
    if (str == NULL || *str == '\0') {
        return str;
    }
    char* end = str + strlen(str) - 1;
    while (end >= str && isspace((unsigned char)*end)) {
        end--;
    }
    *(end + 1) = '\0';
    return str;
}
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

RTStatus push(OrtaVM *vm, Word value) {
    if (vm->stack_size >= O_STACK_CAPACITY) {
        return RTS_STACK_OVERFLOW;
    }
    Word temp;
    temp.as_i64 = value.as_i64;
    vm->stack[vm->stack_size] = temp;
    vm->stack_size++;
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
    Word temp;
    temp.as_str = strdup(dynamic_str);

    vm->stack[vm->stack_size++] = temp;
    return RTS_OK;
}
RTStatus add(OrtaVM *vm) {
    if (vm->stack_size < 2) {
        return RTS_STACK_UNDERFLOW;
    }
    Word a = vm->stack[--vm->stack_size];
    Word b = vm->stack[--vm->stack_size];
    return push(vm, (Word)(a.as_i64 + b.as_i64));
}
RTStatus sub(OrtaVM *vm) {
    if (vm->stack_size < 2) {
        return RTS_STACK_UNDERFLOW;
    }
    Word a = vm->stack[--vm->stack_size];
    Word b = vm->stack[--vm->stack_size];
    return push(vm, (Word)(b.as_i64 - a.as_i64));
}
RTStatus print(OrtaVM *vm) {
    if (vm->stack_size == 0) {
        error_occurred(vm->file_path, vm->program.tokens[vm->program.current_token].pos,"print", "Stack Underflow");
    }
    printf("%ld\n", vm->stack[vm->stack_size - 1].as_i64);
    return RTS_OK;
}
Word pop(OrtaVM *vm) {
    if (vm->stack_size == 0) {
        return (Word){0};
    }
    return vm->stack[--vm->stack_size];
}

#ifndef _WIN32
// TODO: Make it os independent
void asocket(OrtaVM *vm) {
    no_win_support();
    Word result;
    result.as_i64= socket(AF_INET, SOCK_STREAM, 0);
    if (push(vm, result) != RTS_OK) {
        error_occurred(vm->file_path, vm->program.tokens[vm->program.current_token].pos,"asocket", "Stack Overflow");
    }
}
void abind(OrtaVM *vm) {
    no_win_support();
    if (vm->stack_size < 2) {
        error_occurred(vm->file_path, vm->program.tokens[vm->program.current_token].pos,"bind", "Stack Underflow");
    }
    Word sockfd = vm->stack[vm->stack_size - 1];
    if (sockfd.as_i64 == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    Word port = vm->stack[vm->stack_size - 2];
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port.as_i64);

    if (bind(sockfd.as_i64, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd.as_i64);
        exit(EXIT_FAILURE);
    }
}
void alisten(OrtaVM *vm) {
    no_win_support();
    if (vm->stack_size < 2) {
        error_occurred(vm->file_path, vm->program.tokens[vm->program.current_token].pos,"listen", "Stack Underflow");
    }
    Word sockfd = vm->stack[--vm->stack_size];
    Word backlog = vm->stack[--vm->stack_size];

    Word result;
    result.as_i64 = listen(sockfd.as_i64, backlog.as_i64);
}
void aaccept(OrtaVM *vm) {
    no_win_support();
    if (vm->stack_size < 1) {
        error_occurred(vm->file_path, vm->program.tokens[vm->program.current_token].pos,"accept", "Stack Underflow");
    }
    Word sockfd = vm->stack[vm->stack_size - 1];

    Word result;
    result.as_i64 = accept(sockfd.as_i64, NULL, NULL);
    if (push(vm, result) != RTS_OK) {
        error_occurred(vm->file_path, vm->program.tokens[vm->program.current_token].pos,"aaccept", "Stack Overflow");
    }
}
void aconnect(OrtaVM *vm) {
    no_win_support();
    if (vm->stack_size < 2) {
        error_occurred(vm->file_path, vm->program.tokens[vm->program.current_token].pos,"connect", "Stack Underflow");
    }

    Word sockfd = vm->stack[vm->stack_size - 1];
    Word port = vm->stack[vm->stack_size - 2];

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port.as_i64);

    if (connect(sockfd.as_i64, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed");
        close(sockfd.as_i64);
        exit(EXIT_FAILURE);
    }
}
void asend(OrtaVM *vm) {
    no_win_support();

    if (vm->stack_size < 2) {
        error_occurred(vm->file_path, vm->program.tokens[vm->program.current_token].pos,"send", "Stack Underflow");
    }

    Word sockfd = vm->stack[vm->stack_size - 1];

    char *data = vm->stack[vm->stack_size - 2].as_str;

    if (data == NULL) {
        error_occurred(vm->file_path, vm->program.tokens[vm->program.current_token].pos,"send", "Invalid String data");
    }

    if (sockfd.as_i64 < 0) {
        error_occurred(vm->file_path, vm->program.tokens[vm->program.current_token].pos,"send", "Invalid Socket Descriptor");
    }

    ssize_t result = send(sockfd.as_i64, data, strlen(data), 0);
    if (result < 0) {
        perror("Send failed");
        close(sockfd.as_i64);
        exit(EXIT_FAILURE);
    }

    vm->stack_size -= 2;
}
void arecv(OrtaVM *vm) {
    no_win_support();
    if (vm->stack_size < 1) {
        error_occurred(vm->file_path, vm->program.tokens[vm->program.current_token].pos,"recv", "Stack Underflow");
    }

    Word sockfd = vm->stack[vm->stack_size - 1];
    char buffer[O_DEFAULT_BUFFER_CAPACITY];
    ssize_t result = recv(sockfd.as_i64, buffer, sizeof(buffer), 0);
    if (result < 0) {
        perror("Receive failed");
        close(sockfd.as_i64);
        exit(EXIT_FAILURE);
    }

    buffer[result] = '\0';
    push_str(vm, buffer);
}
void aclose(OrtaVM *vm) {
    no_win_support();
    if (vm->stack_size < 1) {
        error_occurred(vm->file_path, vm->program.tokens[vm->program.current_token].pos,"close", "Stack Underflow");
    }

    Word sockfd = vm->stack[vm->stack_size - 1];
    if (close(sockfd.as_i64) < 0) {
        perror("Close failed");
        exit(EXIT_FAILURE);
    }
}
void aset_opt(OrtaVM *vm) {
    no_win_support();
    if (vm->stack_size < 3) {
        error_occurred(vm->file_path,vm->program.tokens[vm->program.current_token].pos,"set_opt", "Stack Underflow");
    }

    Word sockfd = vm->stack[vm->stack_size - 1];
    Word option_name = vm->stack[vm->stack_size - 2];
    Word option_value = vm->stack[vm->stack_size - 3];

    if (setsockopt(sockfd.as_i64, SOL_SOCKET, option_name.as_i64, &option_value, sizeof(option_value)) < 0) {
        perror("Set socket option failed");
        close(sockfd.as_i64);
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

//---------------------------------------------------------------------------------------------------------------------------------

#define CHECK_STACK(vm, count, token, op, reason) \
        if (vm->stack_size < count) { \
            error_occurred(vm->file_path, token.pos, op, reason); \
            return RTS_ERROR; \
        }

RTStatus OrtaVM_execute(OrtaVM *vm) {
    init_bfuncs(vm);
    Program program = vm->program;
    size_t iterations = 0;
    for (size_t i = 0; i < program.size && iterations < limit*4; i++) {
        iterations++;
        Token token = program.tokens[i];
        vm->program.current_token = i;
        switch (token.inst) {
            case I_PUSH:
                if (push(vm, token.word) != RTS_OK) {
                    error_occurred(vm->file_path, token.pos,"push", "Stack Overflow");
                }
                break;
            case I_ADD:
                if (add(vm) != RTS_OK) {
                    error_occurred(vm->file_path, token.pos,"add", "Stack Underflow");
                }
                break;
            case I_SUB:
                if (sub(vm) != RTS_OK) {
                    error_occurred(vm->file_path, token.pos,"sub", "Stack Underflow");
                }
                break;
            case I_PRINT:
                if (print(vm) != RTS_OK) {
                    error_occurred(vm->file_path, token.pos,"print", "Stack Underflow");
                }
                break;
            case I_MUL:
                CHECK_STACK(vm, 2, token, "mul", "Stack Underflow");

                Word a = pop(vm);
                Word b = pop(vm);
                if (push(vm, (Word)(a.as_i64 * b.as_i64)) != RTS_OK) {
                    error_occurred(vm->file_path, token.pos,"mul", "Stack Overflow");
                }
                break;
            case I_DIV:
                CHECK_STACK(vm, 2, token, "div", "Stack Underflow");
                Word c = pop(vm);
                Word d = pop(vm);
                if (d.as_i64 == 0) {
                    return RTS_ERROR;
                }
                if (push(vm, (Word)(c.as_i64 / d.as_i64)) != RTS_OK) {
                    error_occurred(vm->file_path, token.pos,"div", "Stack Overflow");
                }
                break;
            case I_PRINT_STR:
                if (strcmp(token.word.as_str, "$1") == 0 && vm->stack_size > 0) {
                    strcpy(token.word.as_str, vm->stack[vm->stack_size - 1].as_str);
                } else if (strcmp(token.word.as_str, "$2") == 0 && vm->stack_size > 1) {
                    strcpy(token.word.as_str, vm->stack[vm->stack_size - 2].as_str);
                } else if (strcmp(token.word.as_str, "$3") == 0 && vm->stack_size > 2) {
                    strcpy(token.word.as_str, vm->stack[vm->stack_size - 3].as_str);
                }
                if (token.word.as_str[0] != '\0') {
                    process_and_print_string(token.word.as_str);
                }
                break;
            case I_DROP:
                if (vm->stack_size == 0) {
                    error_occurred(vm->file_path, token.pos,"drop", "Stack Underflow");
                }
                vm->stack_size--;
                break;
            case I_JMP:
                 if (token.word.as_i64 < 0 || token.word.as_i64 > program.size) {
                     error_occurred(vm->file_path, token.pos,"jmp", "Stack Underflow");
                 }
                 i = token.word.as_i64;
                 continue;
            case I_JMP_IFN: // JMP_IFB DEST COND | JMP_IFN 0 10 | JMP_IFN 0 (!= 10)
                 if (vm->stack_size == 0) {
                     error_occurred(vm->file_path, token.pos,"jmp_ifn", "Stack Underflow");
                 }
                 if (vm->stack[vm->stack_size - 1].as_i64 != token.jump_if_args.cond.as_i64) {
                     if (token.jump_if_args.dest.as_i64 < 0 || token.jump_if_args.dest.as_i64 > program.size) {
                         error_occurred(vm->file_path, token.pos,"jmp_ifn", "Stack Underflow");
                     }
                     i = token.jump_if_args.dest.as_i64;
                     continue;
                 }
                 break;
            case I_JMP_IF: // JMP_IF DEST COND | JMP_IF 0 10 | JMP_IF 0 (== 10)
                 if (vm->stack_size == 0) {
                     error_occurred(vm->file_path, token.pos,"jmp_if", "Stack Underflow");
                 }
                 if (vm->stack[vm->stack_size - 1].as_i64 == token.jump_if_args.cond.as_i64) {
                     if (token.jump_if_args.dest.as_i64 < 0 || token.jump_if_args.dest.as_i64 > program.size) {
                         error_occurred(vm->file_path, token.pos,"jmp_if", "Stack Underflow");
                     }
                     i = token.jump_if_args.dest.as_i64;
                     continue;
                 }
                 break;
            case I_DUP:
                if (vm->stack_size == 0) {
                    error_occurred(vm->file_path, token.pos,"dup", "Stack Underflow");
                }
                if (vm->stack_size >= O_STACK_CAPACITY) {
                    error_occurred(vm->file_path, token.pos,"dup", "Stack Overflow");
                }
                vm->stack[vm->stack_size].as_i64 = vm->stack[vm->stack_size - 1].as_i64;
                vm->stack_size++;
                break;
            case I_NOP:
                 break;
            case I_EXIT:
                exit(token.word.as_i64);
            case I_INPUT:
                Word input;
                scanf("%ld", &input);
                 if (push(vm, input) != RTS_OK) {
                     error_occurred(vm->file_path, token.pos,"input", "Stack Overflow");
                 }
                 break;
            case I_FILE_OPEN:
                FILE *file = fopen(token.word.as_str, "a+");
                if (file == NULL) {
                    error_occurred(vm->file_path, token.pos,"file_open", "File allocation failed");
                }
                Word fd;
                fd.as_i64 = (int64_t)file;
                if (push(vm, fd) != RTS_OK) {
                    fclose(file);
                    error_occurred(vm->file_path, token.pos,"file_open", "Stack Overflow");
                }
                break;

            case I_FILE_WRITE:
                {
                    FILE *file1 = (FILE *)vm->stack[vm->stack_size - 1].as_i64;
                    char *buffer = token.word.as_str;
                    if (file1 == NULL) {
                        error_occurred(vm->file_path, token.pos,"file_write", "File allocation failed");
                    }
                    fwrite(buffer, sizeof(char), strlen(buffer), file1);
                    break;
                }

            case I_FILE_READ:
                {
                    FILE *file2 = (FILE *)vm->stack[vm->stack_size - 1].as_i64;
                    if (file2 == NULL) {
                        error_occurred(vm->file_path, token.pos,"file_read", "File allocation failed");
                    }
                    char buffer[O_DEFAULT_BUFFER_CAPACITY] = {0};
                    fread(buffer, sizeof(char), 1023, file2);
                    buffer[strlen(buffer)] = '\0';

                    if (push(vm, (Word)buffer) != RTS_OK) {
                        error_occurred(vm->file_path, token.pos,"file_read", "Stack Overflow");
                    }
                    break;
                }
            case I_CAST_AND_PRINT:
                {
                    char *buffer = vm->stack[vm->stack_size - 1].as_str;
                    if (buffer == NULL) {
                        error_occurred(vm->file_path, token.pos,"cast_and_print", "Cast failed");
                    }
                    process_and_print_string(buffer);
                    break;
                }

            case I_SWAP:
                CHECK_STACK(vm, 2, token, "swap", "Stack Underflow");

                Word temp = vm->stack[vm->stack_size - 1];
                vm->stack[vm->stack_size - 1] = vm->stack[vm->stack_size - 2];
                vm->stack[vm->stack_size - 2] = temp;
                break;
            case I_EQ:
                CHECK_STACK(vm, 2, token, "eq", "Stack Underflow");

                Word a1 = vm->stack[vm->stack_size - 1];
                Word b1 = vm->stack[vm->stack_size - 2];
                Word result;
                result.as_i64 = (a1.as_i64 == b1.as_i64) ? 1 : 0;
                if (push(vm, result) != RTS_OK) {
                    error_occurred(vm->file_path, token.pos,"eq", "Stack Overflow");
                }
                break;
            case I_RANDOM_INT:
                {
                    srand(time(NULL));
                    int64_t min = token.random_int.min.as_i64;
                    int64_t max = token.random_int.max.as_i64;
                    Word random;
                    random.as_i64 = min + rand() % (max - min + 1);
                    if (push(vm, random) != RTS_OK) {
                        error_occurred(vm->file_path, token.pos,"random_int", "Stack Overflow");
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
                    if (strcmp(token.word.as_str, "red") == 0) {
                        strcpy(color, "\033[31m");
                    } else if (strcmp(token.word.as_str, "green") == 0) {
                        strcpy(color, "\033[32m");
                    } else if (strcmp(token.word.as_str, "yellow") == 0) {
                        strcpy(color, "\033[33m");
                    } else if (strcmp(token.word.as_str, "blue") == 0) {
                        strcpy(color, "\033[34m");
                    } else if (strcmp(token.word.as_str, "magenta") == 0) {
                        strcpy(color, "\033[35m");
                    } else if (strcmp(token.word.as_str, "cyan") == 0) {
                        strcpy(color, "\033[36m");
                    } else if (strcmp(token.word.as_str, "white") == 0) {
                        strcpy(color, "\033[37m");
                    } else if (strcmp(token.word.as_str, "black") == 0) {
                        strcpy(color, "\033[30m");
                    } else if (strcmp(token.word.as_str, "gray") == 0) {
                        strcpy(color, "\033[90m");
                    } else if (strcmp(token.word.as_str, "light_red") == 0) {
                        strcpy(color, "\033[91m");
                    } else if (strcmp(token.word.as_str, "light_green") == 0) {
                        strcpy(color, "\033[92m");
                    } else if (strcmp(token.word.as_str, "light_yellow") == 0) {
                        strcpy(color, "\033[93m");
                    } else if (strcmp(token.word.as_str, "light_blue") == 0) {
                        strcpy(color, "\033[94m");
                    } else if (strcmp(token.word.as_str, "light_magenta") == 0) {
                        strcpy(color, "\033[95m");
                    } else if (strcmp(token.word.as_str, "light_cyan") == 0) {
                        strcpy(color, "\033[96m");
                    } else if (strcmp(token.word.as_str, "light_white") == 0) {
                        strcpy(color, "\033[97m");
                    } else if (strcmp(token.word.as_str, "reset") == 0) {
                        strcpy(color, "\033[0m");
                    } else {
                        error_occurred(vm->file_path, token.pos,"color", "Invalid Color");
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
                req.tv_sec = token.word.as_i64 / 1000;
                req.tv_nsec = (token.word.as_i64 % 1000) * 1000000;
                nanosleep(&req, NULL);
                break;

            case I_CLEAR_STACK:
                vm->stack_size = 0;
                break;
            case I_LT:
                CHECK_STACK(vm, 2, token, "lt", "Stack Underflow");

                Word a2 = vm->stack[vm->stack_size - 1];
                Word b2 = vm->stack[vm->stack_size - 2];
                Word result2;
                result2.as_i64 = (b2.as_i64 < a2.as_i64) ? 1 : 0;
                if (push(vm, result2) != RTS_OK) {
                    error_occurred(vm->file_path, token.pos,"lt", "Stack Overflow");
                }
                break;
            case I_GT:
                CHECK_STACK(vm, 2, token, "gt", "Stack Underflow");

                Word a3 = vm->stack[vm->stack_size - 1];
                Word b3 = vm->stack[vm->stack_size - 2];
                Word result3;
                result3.as_i64 = (b3.as_i64 > a3.as_i64) ? 1 : 0;
                if (push(vm, result3) != RTS_OK) {
                    error_occurred(vm->file_path, token.pos,"gt", "Stack Overflow");
                }
                break;
            case I_PUSH_STR:
                if (push_str(vm, token.word.as_str) != RTS_OK) {
                    error_occurred(vm->file_path, token.pos,"push", "Stack Overflow");
                }
                break;
            case I_EXIT_IF:
                if (vm->stack_size == 0) {
                    error_occurred(vm->file_path, token.pos,"exit_if", "Stack Underflow");
                }
                Word cond = token.exit_if.cond;
                Word exit_code = token.exit_if.exit_code;
                if (vm->stack[vm->stack_size - 1].as_i64 == cond.as_i64) {
                    exit(exit_code.as_i64);
                }
                break;

            case I_STR_LEN:
                if (vm->stack_size == 0) {
                    error_occurred(vm->file_path, token.pos,"strlen", "Stack Underflow");
                }
                char *str = vm->stack[vm->stack_size - 1].as_str;
                Word len;
                len.as_i64 = strlen(str);

                if (push(vm, len) != RTS_OK) {
                    error_occurred(vm->file_path, token.pos,"strlen", "Stack Overflow");
                }
                break;
            case I_INPUT_STR:
                char buffer[O_DEFAULT_BUFFER_CAPACITY] = {0};
                scanf("%1023s", buffer);
                if (push_str(vm, buffer) != RTS_OK) {
                    error_occurred(vm->file_path, token.pos,"input_str", "Stack Overflow");
                }
                break;

            case I_EXECUTE_CMD:
                system(token.word.as_str);
                break;
            case I_IGNORE:
                i += 2;
                break;
            case I_IGNORE_IF:
                if (vm->stack_size == 0) {
                    error_occurred(vm->file_path, token.pos,"ignore_if", "Stack Underflow");
                }
                Word cond1 = token.word;
                if (vm->stack[vm->stack_size - 1].as_i64 == cond1.as_i64) {
                    i += 2;
                }
                break;
            case I_GET:
                if (vm->stack_size == 0) {
                    error_occurred(vm->file_path, token.pos,"get", "Stack Underflow");
                } else if (token.word.as_i64 > vm->stack_size) {
                    error_occurred(vm->file_path, token.pos,"get", "Stack Overflow");
                }
                Word index;
                index.as_i64 = token.word.as_i64;
                if (push(vm, vm->stack[index.as_i64]) != RTS_OK) {
                    error_occurred(vm->file_path, token.pos,"get", "Stack Overflow");
                }
                break;
            case I_STR_CMP:
                if (vm->stack_size == 0) {
                    error_occurred(vm->file_path, token.pos,"strcmp", "Stack Underflow");
                }

                char *str1 = vm->stack[vm->stack_size - 1].as_str;
                char *str2 = token.word.as_str;

                Word strcmpres;
                strcmpres.as_i64 = strcmp(str1, str2);
                Word result4;
                result4.as_i64 = 0;
                if (strcmpres.as_str == 0) result4.as_i64 = 1;
                if (push(vm, result4) != RTS_OK) {
                    error_occurred(vm->file_path, token.pos,"strcmp", "Stack Overflow");
                }
                break;

            case I_PUSH_CURRENT_INST:
                Word itp;
                itp.as_i64 = i;
                if (push(vm, itp) != RTS_OK) {
                    error_occurred(vm->file_path, token.pos,"push_current", "Stack Overflow");
                }
                break;
            case I_JMP_FS:
                if (vm->stack_size == 0) {
                    error_occurred(vm->file_path, token.pos,"jmp_fs", "Stack Underflow");
                }
                i = vm->stack[vm->stack_size].as_i64;
                continue;
            case I_BFUNC:
                if (vm->bfunctions_size < token.word.as_i64) {
                    error_occurred(vm->file_path, token.pos,"bfunc", "Invalid Function index");
                }

                vm->bfunctions[token.word.as_i64](vm);
                break;
            case I_CAST:
               if (vm->stack_size == 0) {
                   error_occurred(vm->file_path, token.pos,"cast", "Stack Underflow");
               }
               Word value = pop(vm);
               if (strcmp(token.word.as_str, "int") == 0) {
                   if (push(vm, value) != RTS_OK) {
                       error_occurred(vm->file_path, token.pos,"cast", "Stack Overflow");
                   }
               } else if (strcmp(token.word.as_str, "str") == 0) {
                   char *str = value.as_str;
                   if (push_str(vm, str) != RTS_OK) {
                       error_occurred(vm->file_path, token.pos,"cast", "Stack Overflow");
                   }
               } else {
                   error_occurred(vm->file_path, token.pos,"cast", "Invalid Type");
               }

            case I_SEND:
                #ifdef _WIN32
                no_win_support();
                #else
                if (vm->stack_size < 1) {
                   error_occurred(vm->file_path, token.pos,"send", "Stack Underflow");
                }
                Word sockfd = vm->stack[vm->stack_size - 1];
                char *data = token.word.as_str;
                if (data == NULL) {
                    error_occurred(vm->file_path, token.pos,"send", "Invalid String Data");
                }
                if (sockfd.as_i64 < 0) {
                    error_occurred(vm->file_path, token.pos,"send", "Invalid Socket Descriptor");
                }
                ssize_t result6 = send(sockfd.as_i64, data, strlen(data), 0);
                if (result6 < 0) {
                    perror("Send failed");
                    close(sockfd.as_i64);
                    exit(EXIT_FAILURE);
                }

                vm->stack_size -= 1;
                break;
                #endif
            case I_GET_DATE:
                char date[9];
                time_t t = time(NULL);
                struct tm tm = *localtime(&t);

                strftime(date, sizeof(date), "%d:%m:%y", &tm);

                push_str(vm, date);
                break;
            default:
                error_occurred(vm->file_path, token.pos,"InstError", "Unknown Instruction");
        }

    }
    return RTS_OK;
}


// Ovm File description
// | 16 bytes | ...bytes |
//    ^
//    Magic Number(4byte), OvmFile metadata (12byte)
RTStatus OrtaVM_load_program_from_file(OrtaVM *vm, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return RTS_ERROR;
    }

    vm->file_path = strdup(filename);

    char magic_number[5] = {0};
    char meta[12] = {0};
    int version = 0;

    if (fread(magic_number, sizeof(char), 4, file) != 4 ||
        strcmp(magic_number, OVM_MAGIC_NUMBER) != 0) {
        fclose(file);
        return RTS_ERROR;
    }

    if (fread(meta, sizeof(char), 12, file) != 12) {
        fclose(file);
        return RTS_ERROR;
    }

    memcpy(&version, meta, sizeof(int));
    if (version != OVM_VERSION) {
        fclose(file);
        return RTS_ERROR;
    }

    size_t program_size = 0;
    while (!feof(file)) {
        Token token = {0};
        if (fread(&token.inst, sizeof(token.inst), 1, file) != 1) {
            break;
        }

        // TODO(#1): Add more instruction handlings (TOGO STRING_HANDLING)
        if (token.inst == I_PUSH_STR || token.inst == I_PRINT_STR) {
            size_t str_len;
            if (fread(&str_len, sizeof(size_t), 1, file) != 1) {
                fclose(file);
                return RTS_ERROR;
            }
            token.word.as_str = malloc(str_len);
            if (fread(token.word.as_str, sizeof(char), str_len, file) != str_len) {
                free(token.word.as_str);
                fclose(file);
                return RTS_ERROR;
            }
        } else {
            if (fread(&token.word, sizeof(token.word), 1, file) != 1) {
                fclose(file);
                return RTS_ERROR;
            }
        }

        if (program_size >= O_DEFAULT_BUFFER_CAPACITY) {
            fclose(file);
            return RTS_ERROR;
        }
        vm->program.tokens[program_size++] = token;
    }

    vm->program.size = program_size;
    fclose(file);
    return RTS_OK;
}


RTStatus OrtaVM_save_program_to_file(OrtaVM *vm, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        return RTS_ERROR;
    }

    const char magic_number[] = OVM_MAGIC_NUMBER;
    if (fwrite(magic_number, sizeof(char), 4, file) != 4) {
        fclose(file);
        return RTS_ERROR;
    }

    char meta[12] = {0};
    int version = OVM_VERSION;
    memcpy(meta, &version, sizeof(int));
    if (fwrite(meta, sizeof(char), 12, file) != 12) {
        fclose(file);
        return RTS_ERROR;
    }

    for (size_t i = 0; i < vm->program.size; i++) {
        Token *token = &vm->program.tokens[i];
        if (fwrite(&token->inst, sizeof(token->inst), 1, file) != 1) {
            fclose(file);
            return RTS_ERROR;
        }
        // TODO(#2): Add more instruction handlings (TOGO STRING_HANDLING)
        if (token->inst == I_PUSH_STR || token->inst == I_PRINT_STR) {
            size_t str_len = strlen(token->word.as_str) + 1;
            if (fwrite(&str_len, sizeof(size_t), 1, file) != 1 ||
                fwrite(token->word.as_str, sizeof(char), str_len, file) != str_len) {
                fclose(file);
                return RTS_ERROR;
            }
        } else {
            if (fwrite(&token->word, sizeof(token->word), 1, file) != 1) {
                fclose(file);
                return RTS_ERROR;
            }
        }
    }

    fclose(file);
    return RTS_OK;
}


RTStatus OrtaVM_parse_program(OrtaVM *vm, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "ERROR: Failed to open file %s\n", filename);
        return RTS_ERROR;
    }

    if (!ends_with(filename, ".pdd")) {
        char *new_filename = strdup(filename);
        new_filename[strlen(new_filename) - 4] = '\0';
        vm->file_path = strdup(new_filename);
    } else {
        vm->file_path = strdup(filename);
    }

    char line[O_DEFAULT_SIZE];
    size_t program_size = 0;
    Token tokens[O_DEFAULT_BUFFER_CAPACITY];
    int line_count = 0;
    int token_count = 0;

    init_variables();
    while (fgets(line, sizeof(line), file)) {
        line_count++;
        if (line[0] == '\n' || line[0] == O_COMMENT_SYMBOL) continue;

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

        if (support_uppercase) toLowercase(instruction);
        token_count++;
        Token token = {0};
        token.inst = -1;
        token.word.as_str = malloc(O_DEFAULT_SIZE);
        token.pos = line_count;
        if (strcmp(instruction, "stack::push") == 0) {
            strcpy(token.word.as_str, extract_content(line));
            token.word.as_str[strlen(token.word.as_str)] = '\0';
            if (strcmp(token.word.as_str, "No text provided") != 0) {
                token.inst = I_PUSH_STR;
            } else {
                token.inst = I_PUSH;
                if (sscanf(args[0], "%ld", &token.word) != 1) {
                    fprintf(stderr, "ERROR: push expects an integer or string: %s\n", line);
                    fclose(file);
                    return RTS_ERROR;
                }
            }
        } else if (strcmp(instruction, "math::add") == 0) {
            token.inst = I_ADD;
        } else if (strcmp(instruction, "math::sub") == 0) {
            token.inst = I_SUB;
        } else if (strcmp(instruction, "math::div") == 0) {
            token.inst = I_DIV;
        } else if (strcmp(instruction, "math::mul") == 0) {
            token.inst = I_MUL;
        } else if (strcmp(instruction, "io::print") == 0) {
            strcpy(token.word.as_str, extract_content(line));
            token.word.as_str[strlen(token.word.as_str)] = '\0';
            if (strcmp(token.word.as_str, "No text provided") != 0) {
                token.inst = I_PRINT_STR;
            } else {
                token.inst = I_PRINT;
            }
        } else if (strcmp(instruction, "stack::dup") == 0) {
            token.inst = I_DUP;
        } else if (strcmp(instruction, "inst::jmp") == 0) {
            token.inst = I_JMP;
            if (matched < 2 || sscanf(args[0], "%ld", &token.word) != 1) {
                fprintf(stderr, "ERROR: jmp expects a line number: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        } else if (strcmp(instruction, "inst::jmp_ifn") == 0) {
            token.inst = I_JMP_IFN;
            if (matched < 3 || sscanf(args[0], "%ld", &token.jump_if_args.dest) != 1 || sscanf(args[1], "%ld", &token.jump_if_args.cond) != 1) {
                fprintf(stderr, "ERROR: jmp_ifn expects a line number: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        } else if (strcmp(instruction, "inst::jmp_if") == 0) {
            token.inst = I_JMP_IF;
            if (matched < 3 || sscanf(args[0], "%ld", &token.jump_if_args.dest) != 1 || sscanf(args[1], "%ld", &token.jump_if_args.cond) != 1) {
                fprintf(stderr, "ERROR: jmp_if expects a line number: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        } else if (strcmp(instruction, "stack::drop") == 0) {
            token.inst = I_DROP;
        }
        else if (strcmp(instruction, "nop") == 0) {
            token.inst = I_NOP;
        }
        else if (strcmp(instruction, "exit") == 0) {
            token.inst = I_EXIT;

            if (matched < 2 || sscanf(args[0], "%ld", &token.word) != 1) {
                fprintf(stderr, "ERROR: exit expects an exit code: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        }
        else if (strcmp(instruction, "io::input") == 0) {
            token.inst = I_INPUT;
        }
        else if (strcmp(instruction, "file::open") == 0) {
            token.inst = I_FILE_OPEN;
            strcpy(token.word.as_str, extract_content(line));
            token.word.as_str[strlen(token.word.as_str) + 1] = '\0';
        }
        else if (strcmp(instruction, "file::write") == 0) {
            token.inst = I_FILE_WRITE;
            strcpy(token.word.as_str, extract_content(line));
            token.word.as_str[strlen(token.word.as_str) + 1] = '\0';
        }
        else if (strcmp(instruction, "file::read") == 0) {
            token.inst = I_FILE_READ;
        } else if (strcmp(instruction, "io::cprint") == 0) {
            token.inst = I_CAST_AND_PRINT;
            insecure_function(line_count, "cast_and_print", "The latest stack element is casting to char *");
        } else if (strcmp(instruction, "stack::swap") == 0) {
            token.inst = I_SWAP;
        } else if (strcmp(instruction, "stack::eq") == 0) {
            token.inst = I_EQ;
        } else if (strcmp(instruction, "math::random_int") == 0) {
            token.inst = I_RANDOM_INT;
            if (matched < 3 || sscanf(args[0], "%ld", &token.random_int.min) != 1 || sscanf(args[1], "%ld", &token.random_int.max) != 1) {
                fprintf(stderr, "ERROR: random_int expects two integers: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        } else if (strcmp(instruction, "io::color") == 0) {
            token.inst = I_COLOR;
            strcpy(token.word.as_str, extract_content(line));
            token.word.as_str[strlen(token.word.as_str) + 1] = '\0';
        } else if (strcmp(instruction, "io::clear") == 0) {
            token.inst = I_CLEAR;
        }
        else if (strcmp(instruction, "io::sleep") == 0) {
            token.inst = I_SLEEP;

            if (matched < 2 || sscanf(args[0], "%ld", &token.word) != 1) {
                fprintf(stderr, "ERROR: sleep expects ms: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        } else if (strcmp(instruction, "stack::clear") == 0) {
            token.inst = I_CLEAR_STACK;
        } else if (strcmp(instruction, "stack::gt") == 0) {
            token.inst = I_GT; // Greater than
        } else if (strcmp(instruction, "stack::lt") == 0) {
            token.inst = I_LT; // Less then
        } else if (strcmp(instruction, "exit_if") == 0) {
            token.inst = I_EXIT_IF;
            if (matched < 3 || sscanf(args[0], "%ld", &token.exit_if.exit_code) != 1 || sscanf(args[1], "%ld", &token.exit_if.cond) != 1) {
                fprintf(stderr, "ERROR: exit_if expects an exit code and condition: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        } else if (strcmp(instruction, "stack::strlen") == 0) {
            insecure_function(line_count, "strlen", "if the latest element is not a char * it will throw an segfault");
            token.inst = I_STR_LEN;
        } else if (strcmp(instruction, "io::input_str") == 0) {
            token.inst = I_INPUT_STR;
        } else if (strcmp(instruction, "io::execute") == 0) {
            token.inst = I_EXECUTE_CMD;
            strcpy(token.word.as_str, extract_content(line));
            token.word.as_str[strlen(token.word.as_str) + 1] = '\0';
        } else if (strcmp(instruction, "inst::ignore") == 0) {
            token.inst = I_IGNORE;
        } else if (strcmp(instruction, "inst::ignore_if") == 0) {
            token.inst = I_IGNORE_IF;
            if (matched < 2 || sscanf(args[0], "%ld", &token.word) != 1) {
                fprintf(stderr, "ERROR: ignore_if expects cond: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        } else if (strcmp(instruction, "stack::get") == 0) {
            token.inst = I_GET;

            if (matched < 2 || sscanf(args[0], "%ld", &token.word) != 1) {
                fprintf(stderr, "ERROR: get expects a stack index: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        } else if (strcmp(instruction, "stack::strcmp") == 0) {
            token.inst = I_STR_CMP;
            strcpy(token.word.as_str, extract_content(line));
            token.word.as_str[strlen(token.word.as_str) + 1] = '\0';
        } else if (strcmp(instruction, "inst::current") == 0) {
            token.inst = I_PUSH_CURRENT_INST;
        } else if (strcmp(instruction, "inst::jmp_fs") == 0) {
            token.inst = I_JMP_FS;
        } else if (strcmp(instruction, "orta::var") == 0) {
            token.inst = I_PUSH_STR;
            char variable[O_DEFAULT_BUFFER_CAPACITY];
            strcpy(variable, extract_content(line));
            variable[strlen(variable) + 1] = '\0';
            strcpy(token.word.as_str, get_variable(variable));
        } else if (strcmp(instruction, "c::bfunc") == 0) {
            token.inst = I_BFUNC;

            if (matched < 2 || sscanf(args[0], "%ld", &token.word) != 1) {
                fprintf(stderr, "ERROR: bfunc expects a function: %s", line);
                fclose(file);
                return RTS_ERROR;
            }
        } else if (strcmp(instruction, "stack::cast") == 0) {
            token.inst = I_CAST;
            strcpy(token.word.as_str, extract_content(line));
            token.word.as_str[strlen(token.word.as_str) + 1] = '\0';
        } else if (strcmp(instruction, "net::send") == 0) {
            token.inst = I_SEND;
            strcpy(token.word.as_str, extract_content(line));
            token.word.as_str[strlen(token.word.as_str) + 1] = '\0';
        } else if (strcmp(instruction, "time::today") == 0) {
            token.inst = I_GET_DATE;
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

// DONE: Make it os independent
RTStatus OrtaVM_preprocess_file(char *filename, int argc, char argv[20][256], int target) {
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
    if (target == 0) {
        #ifdef _WIN32
            strncpy(preprocessors[preprocessors_count][0], "OS_WINDOWS", O_DEFAULT_SIZE);
            strncpy(preprocessors[preprocessors_count][1], "1", O_DEFAULT_SIZE);
            preprocessors_count++;
        #else
            strncpy(preprocessors[preprocessors_count][0], "OS_POSIX", O_DEFAULT_SIZE);
            strncpy(preprocessors[preprocessors_count][1], "1", O_DEFAULT_SIZE);
            preprocessors_count++;
        #endif
    } else if (target == 1) {
        strncpy(preprocessors[preprocessors_count][0], "OS_WINDOWS", O_DEFAULT_SIZE);
        strncpy(preprocessors[preprocessors_count][1], "1", O_DEFAULT_SIZE);
        preprocessors_count++;
    } else if (target == 2) {
        strncpy(preprocessors[preprocessors_count][0], "OS_POSIX", O_DEFAULT_SIZE);
        strncpy(preprocessors[preprocessors_count][1], "1", O_DEFAULT_SIZE);
        preprocessors_count++;
    } else {
        fprintf(stderr, "ERROR: Invalid target\n");
        return RTS_ERROR;
    }

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
                    char *strext = extract_content(line+8);
                    if (strcmp(strext, "No text provided") == 0) {
                        strncpy(preprocessors[preprocessors_count][1], value, O_DEFAULT_SIZE);
                    } else {
                        strncpy(preprocessors[preprocessors_count][1], strext, O_DEFAULT_SIZE);
                    }
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
            // REMOVED #var because it dont works if you run the program as .ovm
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

                    if (OrtaVM_preprocess_file(include_path, 0, NULL, target) != RTS_OK) {
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



#endif // ORTA_H



