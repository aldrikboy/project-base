#include <projectbase/project_base.h>

#define error_if(e) if (e) { printf("       --! ERROR AT: %s\n", #e); return 1; };
#define success return 0;

#define CONFIG_DIRECTORY "test_program_config"

#include "string_utils.c"
#include "threads.c"
#include "window.c"
#include "array.c"

bool failure = false;
void print_h1(char *str) {
    printf("# %s\n", str);
}

void print_result(char *str, s32 result) {
    if (!result) {
        printf("    %s\n", str);
    }
    else {
        printf("    %s - FAILURE\n", str);
        failure = true;
    }
}

int main(int argc, char** argv) {

    print_h1("String utils");
    print_result("String to number", string_to_number());

    print_h1("Threads");
    print_result("Detached thread", detached_thread());
    print_result("Joined thread", joined_thread());

    print_h1("Platform");
    print_result("Open window", open_window(argc, argv));
    
    print_h1("Array");
    print_result("Write", array_write());
    print_result("Read", array_read());
    print_result("Delete", array_delete());
    print_result("Swap", array_swap_());

    if (failure) exit(EXIT_FAILURE);
    return 0;
}