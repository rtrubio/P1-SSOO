#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crms_API.h"

char *MEMORY_PATH = 0;
size_t PCB_ENTRY_SIZE = 256;
size_t PCB_ENTRIES = 16;
size_t PAGE_TABLE_SIZE = 32;

struct CrmsFile {
	int pid;
	char *filename;
	size_t fsize;
	int vpn;
	unsigned int offset;
	int entry_id;
};

void cr_mount(char* memory_path) {
	while (1) break;
}

void cr_ls_processes() {

	unsigned char valid_byte[1];
	unsigned char pid[1];
	unsigned char pname[12];

	FILE *fptr = fopen(MEMORY_PATH, "r+b");
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

/*

int cr_write_file(CrmsFile* file_desc, void* buffer, int n_bytes) {

	unsigned char valid_byte[1];
	unsigned char pid[1];

	FILE *fptr = fopen(MEMORY_PATH, "r");
	int start = 0;
	int p_found = 0;

	while (start < PCB_ENTRY_SIZE * PCB_ENTRIES) {
		fseek(fptr, start, SEEK_SET);
		fread(valid_byte, 1, 1, fptr);

		if (valid_byte[0]) {
			fread(pid, 1, 1, fptr);

			if ((int)pid[0] == file_desc->pid) {
				p_found = 1;
				fseek(fptr, start + 14, SEEK_SET);
				break;
			}
		}
		start += PCB_ENTRY_SIZE;
	}
	 

	if (p_found) {

		Se busca un espacio vacío y se guarda la dirección
		del archivo anterior y del archivo siguiente, si existen.

		int empty_space = -1;
		unsigned char prev_address[4];
		unsigned char next_address[4];

		for (int i = 0; i < 10; i++) {
			fread(valid_byte, 1, 1, fptr);

			if (!valid_byte[0] && empty_space == -1) {
				empty_space = i;
				if (0 < i) {
					fseek(fptr, -5, SEEK_CUR);
					fread(prev_address, 1, 4, fptr);
					fseek(fptr, 1, 21, SEEK_CUR);
				}
				fseek(fptr, 1, 20, SEEK_CUR);
			}
			else if (valid_byte[0] && empty_space != -1) {
				fseek(fptr, 16, SEEK_CUR);
				fread(next_address, 1, 4, fptr);
				break;
			}
			else {
				fseek(fptr, 1, 20, SEEK_CUR);
			}
		}

		if (empty_space == -1) {
			printf("No hay espacio disponible para la entrada de un nuevo archivo.\n");
		}
		else {
			
		}

	}
	else {
		printf("No existe proceso válido asociado al archivo.\n");
	}

	fclose(fptr);
} */
