#include <stdio.h>

extern char MEMORY_PATH[256];
extern size_t PCB_ENTRY_SIZE;
extern size_t PCB_ENTRIES;
extern size_t PAGE_TABLE_SIZE;

typedef struct CrmsFile {
	int pid;
	char *filename;
	unsigned long filesize;
	unsigned long vpn;
	unsigned long offset;
	char mode;
	int allocated;
} CrmsFile;

void cr_mount(char*);
void cr_ls_processes();
void cr_start_process(int, char*);
int cr_write_file(CrmsFile*, void*, int);
CrmsFile* cr_open(int, char*, char);
void cr_ls_files(int);
int cr_exists(int, char*);
void cr_finish_process(int);
void cr_delete_file(CrmsFile*);