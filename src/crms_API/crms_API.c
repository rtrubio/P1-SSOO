#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crms_API.h"

char *MEMORY_PATH = 0;
size_t PCB_ENTRY_SIZE = 256;
size_t PCB_ENTRIES = 16;

void cr_clean() {

	if (MEMORY_PATH)
		free(MEMORY_PATH);
}

void cr_mount(char* memory_path) {

	if (MEMORY_PATH)
		free(MEMORY_PATH);

	int str_size = strlen(memory_path) + 1;
	MEMORY_PATH = malloc(str_size * sizeof(char));
	strcpy(MEMORY_PATH, memory_path);
}

void cr_ls_processes() {

	unsigned char valid_byte[1];
	unsigned char pid[1];
	unsigned char pname[12];

	FILE *fptr = fopen(MEMORY_PATH, "r");
	int start = 0;
	int n = 0;

	while (start < PCB_ENTRY_SIZE * PCB_ENTRIES) {
		fseek(fptr, start, SEEK_SET);
		fread(valid_byte, 1, 1, fptr);

		if (valid_byte[0]) {
			fread(pid, 1, 1, fptr);
			fread(pname, 1, 12, fptr);

			if (!n) {
				n++;
				printf("%3s | %12s\n", "PID", "NAME");
			}

			printf("%3i | %12s\n", (int)pid[0], (char*)pname);
		}

		start += PCB_ENTRY_SIZE;
	}
	fclose(fptr);

	if (!n)
		printf("No hay procesos ejecutando actualmente.\n");
}

