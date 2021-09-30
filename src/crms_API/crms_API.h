#include <stdio.h>

extern char *MEMORY_PATH;
extern size_t PCB_ENTRY_SIZE;
extern size_t PCB_ENTRIES;

void cr_mount(char*);
void cr_ls_processes();
void cr_start_process(int, char*);

void cr_clean();