#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "libs/xhex.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <input.xbin> <output>\n", argv[0]);
        return 1;
    }
    
    xhex_options_t options;
    xhex_init_options(&options);
    options.c_style_output = true;
    options.var_name = "bytecode";
    options.input = fopen(argv[1], "rb");
    if (!options.input) {
        fprintf(stderr, "Error opening input file\n");
        return 1;
    }
    
    options.output = fopen(argv[2], "w");
    if (!options.output) {
        fprintf(stderr, "Error opening output file\n");
        fclose(options.input);
        return 1;
    }
    
     
    fprintf(options.output, "#include <orta.h>\n");
    
    xhex_print_hex_dump(&options);
    
    fprintf(options.output, "\n\nint main() {\n");
    fprintf(options.output, "    OrtaVM vm = ortavm_create(\"%s\");\n", argv[1]);
    fprintf(options.output, "    load_bytecode_from_memory(&vm, bytecode, bytecode_size);\n");
    fprintf(options.output, "    execute_program(&vm);\n");
    fprintf(options.output, "    ortavm_free(&vm);\n");
    fprintf(options.output, "    return 0;\n");
    fprintf(options.output, "}\n");
    
    if (options.input != stdin) fclose(options.input);
    if (options.output != stdout) fclose(options.output);
    return 0;
}
