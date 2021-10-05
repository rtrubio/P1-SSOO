#include <stdio.h>

extern char *MEMORY_PATH;
extern size_t PCB_ENTRY_SIZE;
extern size_t PCB_ENTRIES;
extern size_t PAGE_TABLE_SIZE;

struct CrmsFile;

void cr_mount(char*);
void cr_ls_processes();
void cr_start_process(int, char*);

/*int cr_write_file(CrmsFile*);*/