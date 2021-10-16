#include <stdio.h>

extern char MEMORY_PATH[1024];
extern int CR_ERROR;

typedef struct CrmsFile {
	int pid;
	char *filename;
	unsigned long filesize;
	unsigned long vpn;
	unsigned long offset;
	char mode;
	int allocated;
} CrmsFile;

enum {
	ALREADY_EXISTS,
	NOT_FOUND,
	OUT_OF_MEMORY,
	NO_AVAILABLE_ENTRY,
	MODE_ERROR
};

void cr_mount(char*);
void cr_ls_processes();
void cr_start_process(int, char*);
int cr_write_file(CrmsFile*, void*, int);
CrmsFile* cr_open(int, char*, char);
void cr_ls_files(int);
int cr_exists(int, char*);
void cr_finish_process(int);
void cr_delete_file(CrmsFile*);
int cr_read(CrmsFile*, void*, int);
void cr_close(CrmsFile*);
void cr_strerror(int);