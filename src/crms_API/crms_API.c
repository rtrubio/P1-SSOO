#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <byteswap.h>
#include <stdint.h>
#include "crms_API.h"

char MEMORY_PATH[256];
size_t PCB_ENTRY_SIZE = 256;
size_t PCB_ENTRIES = 16;
size_t PAGE_TABLE_SIZE = 32;
size_t FILE_DATA_ENTRY_SIZE = 21; // 1+12+4+4=validez+nombre+tamaño+dir
size_t FILE_DATA_ENTRIES = 10;

void cr_mount(char* memory_path) {
	strcpy(MEMORY_PATH, memory_path);
}

void cr_ls_processes() {

	unsigned char valid_byte[1];
	unsigned char pid[16][1];
	unsigned char pname[16][13];

	FILE *fptr = fopen(MEMORY_PATH, "rb");
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
		printf("Procesos activos:\n\n");
		printf(" %4s   %-12s \n\n", "PID", "NAME");
		for (int i = 0; i < n; i++) {
			printf(" %4i   %-12s \n", (int)(pid[i][0]), (char*)(pname[i]));
		}
		printf("\n");
	}
}

int cr_exists(int process_id, char* file_name) {

	unsigned char valid_byte[1];
	unsigned char valid_file_byte[1];
	unsigned char pid[1];
	char filename[13]; // probar con 12
	filename[12] = '\0';

	FILE *fptr = fopen(MEMORY_PATH, "rb");
	int start = 0;
	int start_file = 0;
	int n = 0;
	int exists = 0;

	while (start < PCB_ENTRY_SIZE * PCB_ENTRIES) {
		fseek(fptr, start, SEEK_SET);
		fread(valid_byte, 1, 1, fptr);

		if (valid_byte[0]) { // **revisar si nos interesan los activos o todos

			fread(pid, 1, 1, fptr);

			if ((int)(pid[0]) == process_id) {
				start += 14;
				// estamos en la data del proceso buscado
				// hay 10 sub entradas con data de archivos, cada una de 21 bytes
				while(start_file < FILE_DATA_ENTRY_SIZE * FILE_DATA_ENTRIES) {
				  // 1 byte para validez de la entrada
				  fseek(fptr, start + start_file, SEEK_SET);
				  fread(valid_file_byte, 1, 1, fptr);

				  if (valid_file_byte[0]) {
					  fread(filename, 1, 12, fptr);

					  if(!strcmp(filename, file_name)){
						  exists = 1;
					  }
				  }
					start_file += FILE_DATA_ENTRY_SIZE;
				}
			}
			n++;
		}

		start += PCB_ENTRY_SIZE;
	}
	fclose(fptr);

	if (exists) {
		return 1;
	} else {
		return 0;
	}
}

void cr_ls_files(int process_id) {

	unsigned char valid_byte[1];
	unsigned char valid_file_byte[1];
	unsigned char pid[1];
	unsigned char pname[13];
	unsigned char filename[10][13]; // probar con 12

	FILE *fptr = fopen(MEMORY_PATH, "rb");
	int start = 0;
	int start_file = 0;
	int n = 0;

	while (start < PCB_ENTRY_SIZE * PCB_ENTRIES) {
		fseek(fptr, start, SEEK_SET);
		fread(valid_byte, 1, 1, fptr);

		if (valid_byte[0]) { // **revisar si nos interesan los activos o todos
			fread(pid, 1, 1, fptr);
			if((int)(pid[0]) == process_id) {
				fread(pname, 1, 12, fptr);
				pname[12] = '\0';

				start += 14;
				// estamos en la data del proceso buscado
				// hay 10 sub entradas con data de archivos, cada una de 21 bytes
				while(start_file < FILE_DATA_ENTRY_SIZE * FILE_DATA_ENTRIES){
				  // 1 byte para validez de la entrada
				  fseek(fptr, start+start_file, SEEK_SET);
				  fread(valid_file_byte, 1, 1, fptr);
				  if(valid_file_byte[0]){
					  fread(filename[n], 1, 12, fptr);
					  filename[n][12] = '\0';
					  n++;
				  }
					start_file += FILE_DATA_ENTRY_SIZE;
				}
			}
		}

		start += PCB_ENTRY_SIZE;
	}
	fclose(fptr);

	if (!n)
		printf("No hay archivos para el proceso %i actualmente.\n\n", process_id);
	else {
		printf("\n");
		printf("Archivos de %s [ID %i]:\n\n", pname, process_id);
		for (int i = 0; i < n; i++) {
			printf("%s%s\n", "  ", (char*)(filename[i]));
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
		position = 256 * empty_space;

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

void cr_finish_process(int process_id) {

	unsigned char valid_byte[1];
	unsigned char valid_file_byte[1];
	unsigned char pid[1];
	unsigned char pname[13];
	//unsigned char filename[10][13]; // probar con 12

	FILE *fptr = fopen(MEMORY_PATH, "rb");
	int start = 0;
	int start_actual = 0;
	int start_file = 0;
	int n = 0;

	while (start < PCB_ENTRY_SIZE * PCB_ENTRIES) {
		fseek(fptr, start, SEEK_SET);
		fread(valid_byte, 1, 1, fptr);

		if (valid_byte[0]) { // iteramos en los procesos que sean validos
			fread(pid, 1, 1, fptr);
			if((int)(pid[0]) == process_id) { // buscamos el proceso que tenga id = process_id
				fread(pname, 1, 12, fptr);
				pname[12] = '\0';

				start += 14;
	
				while(start_file < FILE_DATA_ENTRY_SIZE * FILE_DATA_ENTRIES){ // iteramos en los archivos del proceso para "invalidarlos"
				  fseek(fptr, start+start_file, SEEK_SET);
				  fread(valid_file_byte, 1, 1, fptr);
				  if(valid_file_byte[0]){

					  // CAMBIAR VALOR DE BYTE DE VALIDEZ DE CADA ARCHIVO
					  // se cambian con fwrite u otra manera??

					  n++;
				  }
				}
				start == start_actual; // seria lo mismo que restar 14 al start
				fseek(fptr, start, SEEK_SET); // queremos volver a pararnos en el byte de validez del proceso con id = process_id

				// CAMBIAR VALOR DE VALIDEZ DE BYTE DEL PROCESO
				// se cambia con fwrite u otra manera??
			}
		}

		start += PCB_ENTRY_SIZE;
		start_actual += PCB_ENTRY_SIZE;
	}
	fclose(fptr);
}

int find_process(int process_id) {

	FILE *fptr = fopen(MEMORY_PATH, "rb");
	unsigned char valid_byte[1];
	unsigned char pid[1];

	for (int i = 0; i < PCB_ENTRIES; i++) {
		fseek(fptr, PCB_ENTRY_SIZE * i, SEEK_SET);
		fread(valid_byte, 1, 1, fptr);

		if (valid_byte[0]) {
			fread(pid, 1, 1, fptr);
			if ((int)(pid[0]) == process_id) {
				fclose(fptr);
				return i;
			}
		}
	}

	fclose(fptr);
	return -1;
}

int find_file(int pcb_position, char* file_name) {

	FILE *fptr = fopen(MEMORY_PATH, "rb");
	char valid_byte[1];
	char fname_bytes[13];
	fname_bytes[12] = '\0';

	int position = PCB_ENTRY_SIZE * pcb_position + 14;

	fseek(fptr, position, SEEK_SET);
	for (int i = 0; i < 10; i++) {
		fseek(fptr, position + i * 21, SEEK_SET);
		fread(valid_byte, 1, 1, fptr);
		if (valid_byte[0]) {
			fread(fname_bytes, 1, 12, fptr);
			if (!strcmp(fname_bytes, file_name)) {
				fclose(fptr);
				return i;
			}
		}	
	}

	fclose(fptr);
	return -1;
}

unsigned long in_big_endian(unsigned char* bytes) {

	unsigned int x = 1;
	int y = (int)(((char*)&x)[0]);

	if (y) {
		uint32_t buffer;
		memcpy(&buffer, bytes, 4);
		return (unsigned long)(bswap_32(buffer));
	}

	else
		return (unsigned long)(bytes);
}

CrmsFile* cr_open(int process_id, char* file_name, char mode) {
	
	if (mode == 'r') {
		int pcb_position = find_process(process_id);
		if (pcb_position == -1) {
			printf("No existe un proceso con ID %i.\n", process_id);
			return 0;
		}

		int file_position = find_file(pcb_position, file_name);
		if (file_position == -1) {
			printf("El proceso de ID %i no posee un archivo de nombre %s.\n",
				process_id, file_name);
			return 0;
		}

		FILE *fptr = fopen(MEMORY_PATH, "rb");

		fseek(fptr,
			PCB_ENTRY_SIZE * pcb_position + 14 + 21 * file_position + 1,
			SEEK_SET);

		unsigned char fsize_bytes[4];
		unsigned char vdir_bytes[4];
		char fname_bytes[13];
		fname_bytes[12] = '\0';

		fread(fname_bytes, 1, 12, fptr);
		fread(fsize_bytes, 1, 4, fptr);
		fread(vdir_bytes, 1, 4, fptr);
		
		unsigned long virtual_dir = in_big_endian(vdir_bytes);
		unsigned long file_size = in_big_endian(fsize_bytes);

		unsigned long mask;
		mask = (1 << 23) - 1;
		unsigned long offset = mask & virtual_dir;
		mask = ((1 << 5) - 1) << 23;
		unsigned long vpn = (mask & virtual_dir) >> 23;

		CrmsFile *file = malloc(sizeof(CrmsFile));
		file->pid = process_id;
		file->filename = file_name;
		file->filesize = file_size;
		file->vpn = vpn;
		file->offset = offset;

		return file;
	}

	else
		return 0;
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