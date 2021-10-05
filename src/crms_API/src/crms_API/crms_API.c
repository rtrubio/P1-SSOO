#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crms_API.h"

char *MEMORY_PATH = 0;
size_t PCB_ENTRY_SIZE = 256;
size_t PCB_ENTRIES = 16;
size_t PAGE_TABLE_SIZE = 32;

/*typedef struct CrmsFile {
	int pid;
	char *filename;
	size_t fsize;
	int vpn;
	unsigned int offset;
	int entry_id;
} CrmsFile;*/

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
	unsigned char pid[16][1];
	unsigned char pname[16][13];

	FILE *fptr = fopen(MEMORY_PATH, "r+b");
	int start = 0;
	int n = 0;

	while (start < PCB_ENTRY_SIZE * PCB_ENTRIES) {
		fseek(fptr, start, SEEK_SET);
		fread(valid_byte, 1, 1, fptr);

		if (valid_byte[0]) {
			fread(pid[n], 1, 1, fptr);
			fread(pname[n], 1, 12, fptr);
			pname[n][12] = '\0';
			n++;
		}

		start += PCB_ENTRY_SIZE;
	}
	fclose(fptr);

	if (!n)
		printf("No hay procesos ejecutando actualmente.\n");
	else {
		printf("\n");
		printf("%3s | %12s\n", "PID", "NAME");
		printf("----|-------------\n");
		for (int i = 0; i < n; i++) {
			printf("%3i | %12s\n", (int)(pid[i][0]), (char*)(pname[i]));
		}
		printf("\n");
	}
}

void cr_start_process(int process_id, char* process_name) {

	FILE *fptr = fopen(MEMORY_PATH, "r+b");
	unsigned char valid_byte[1];
	unsigned char pid[1];
	int empty_space = 0;
	int position = 0;

	/* Busca un espacio vacío,
	verifica que no existan procesos con el mismo ID.*/

	for (int i = 0; i < 16; i++) {
		fseek(fptr, position, SEEK_SET);
		fread(valid_byte, 1, 1, fptr);
		if (!valid_byte[0] && !empty_space)
			empty_space = i;
		else if (valid_byte[0]) {
			fread(pid, 1, 1, fptr);
			if ((int)(pid[0]) == process_id) {
				printf("Ya existe un proceso con ID %i.\n", process_id);
				empty_space = -1;
				break;
			}
		}
		position += 256;
	}

	if (!empty_space)
		printf("No hay espacio para más procesos.\n");

	/* Si se encuentra un espacio vacío y el ID es válido,
	se crea la entrada en la tabla de PCB */

	else if (empty_space != -1) {
		position = 256*empty_space;

		fseek(fptr, position, SEEK_SET);
		unsigned char byte_buffer = 1;
		fwrite(&byte_buffer, 1, 1, fptr);

		fseek(fptr, position + 1, SEEK_SET);
		byte_buffer = process_id;
		fwrite(&byte_buffer, 1, 1, fptr);

		/* Si el nombre del proceso es de más de 12 caracteres,
		se consideran solo los primeros 12. */

		fseek(fptr, position + 2, SEEK_SET);
		char *pname = malloc(sizeof(char) * (strlen(process_name) + 1));
		strcpy(pname, process_name);
		if (strlen(process_name) > 12)
			pname[12] = '\0';
		fwrite(pname, strlen(pname), 1, fptr);
		free(pname);
	}

	fclose(fptr);
}

/*
int cr_write_file(CrmsFile* file_desc, void* buffer, int n_bytes) {

	unsigned char valid_byte[1];
	unsigned char pid[1];

	FILE *fptr = fopen(MEMORY_PATH, "r+b");
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