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

	unsigned char valid_byte[1] = {0};
	unsigned char pid[16][1] = {0};
	unsigned char pname[16][13] = {0};

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

void cr_start_process(int process_id, char* process_name) {

	FILE *fptr = fopen(MEMORY_PATH, "r+b");
	unsigned char valid_byte[1] = {0};
	unsigned char pid[1] = {0};
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
		char pname[12] = {0};
		strcpy(pname, process_name);
		fwrite(pname, 12, 1, fptr);
	}

	fclose(fptr);
}

int find_process(int process_id) {

	FILE *fptr = fopen(MEMORY_PATH, "rb");
	unsigned char valid_byte[1] = {0};
	unsigned char pid[1] = {0};

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
	char fname_bytes[13] = {0};
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

	int pcb_position = find_process(process_id);
	if (pcb_position == -1) {
		printf("No existe un proceso con ID %i.\n", process_id);
		return 0;
	}

	int file_position = find_file(pcb_position, file_name);
	
	if (mode == 'r' && file_position != -1) {
		FILE *fptr = fopen(MEMORY_PATH, "rb");

		fseek(fptr,
			PCB_ENTRY_SIZE * pcb_position + 14 + 21 * file_position + 1,
			SEEK_SET);

		unsigned char fsize_bytes[4] = {0};
		unsigned char vdir_bytes[4] = {0};
		char fname_bytes[13] = {0};
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
		file->mode = 'r';
		file->allocated = 1;

		fclose(fptr);
		return file;
	}

	if (mode == 'r' && file_position == -1) {
		printf("El proceso de ID %i no posee un archivo de nombre %s.\n",
			process_id, file_name);
		return 0;
	}

	if (mode == 'w' && file_position == -1) {

		CrmsFile *file = malloc(sizeof(CrmsFile));
		file->pid = process_id;
		file->filename = file_name;
		file->mode = 'w';
		file->allocated = 0;
		return file;
	}

	if (mode == 'w' && file_position != -1) {

		printf("El archivo ya existe y no se puede escribir.\n");
		return 0;
	}
}

int cr_exists(int process_id, char* file_name) {

	unsigned char valid_byte[1] = {0};
	unsigned char valid_file_byte[1] = {0};
	unsigned char pid[1] = {0};
	char filename[13] = {0}; // probar con 12
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
	unsigned char valid_file_byte[1] = {0};
	unsigned char pid[1] = {0};
	unsigned char pname[13] = {0};
	char filename[10][13] = {0};
	unsigned long fsize[10] = {0};
	unsigned long offset[10] = {0};
	unsigned long vpn[10] = {0};

	CrmsFile *file;

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

					file = cr_open(process_id, filename[n], 'r');
					offset[n] = file->offset;
					vpn[n] = file->vpn;
					fsize[n] = file->filesize;

					free(file);

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
		printf("%12s | %-10s %-10s %-10s\n", "NAME", "VPN", "OFFSET", "SIZE");
		for (int i = 0; i < n; i++) {
			printf("%12s | %-10lu %-10lu %-10lu\n", (char*)(filename[i]),
				vpn[i], offset[i], fsize[i]);
		}
		printf("\n");
	}

}

int get_fpn(unsigned char byte) {

	return (int)((unsigned char)(byte << 1) >> 1);
}

int valid_page_entry(unsigned char byte) {

	return (int)(byte >> 7);
}

void min_offset(int pid, int vpn, unsigned long offset, unsigned long* w_offset) {

	FILE *fptr = fopen(MEMORY_PATH, "r+b");

	unsigned long pos = PCB_ENTRY_SIZE * find_process(pid) + 14;
	unsigned long min_offset = (1 << 23);
	unsigned char valid_byte[1];
	unsigned char vdir[10][4] = {0};
	int n = 0;

	for (int i = 0; i < 10; i++) {
		fseek(fptr, pos + i * 21, SEEK_SET);
		fread(valid_byte, 1, 1, fptr);

		if (valid_byte[0]) {
			fseek(fptr, 16, SEEK_CUR);
			fread(vdir[n], 1, 4, fptr);
			n++;
		}
	}

	unsigned long cur_vdir, cur_vpn, cur_offset;
	unsigned long mask;
	
	for (int i = 0; i < n; i++) {
		cur_vdir = in_big_endian(vdir[i]);
		mask = ((1 << 5) - 1) << 23;
		cur_vpn = (mask & cur_vdir) >> 23;

		if (cur_vpn == vpn) {
			mask = (1 << 23) - 1;
			cur_offset = mask & cur_vdir;

			if (cur_offset < min_offset && offset <= cur_offset) {
				min_offset = cur_offset;
			}
		}
	}

	(*w_offset) = min_offset;
	fclose(fptr);
}	

void max_offset(int pid, int vpn, unsigned long* offset) {

	FILE *fptr = fopen(MEMORY_PATH, "r+b");

	unsigned long pos = PCB_ENTRY_SIZE * find_process(pid) + 14;
	unsigned long max_offset = 0;
	int found = 0;
	unsigned char valid_byte[1];
	unsigned char vdir[10][4] = {0};
	unsigned char size[10][4] = {0};
	unsigned long size_ = 0;
	int n = 0;

	for (int i = 0; i < 10; i++) {
		fseek(fptr, pos + i * 21, SEEK_SET);
		fread(valid_byte, 1, 1, fptr);

		if (valid_byte[0]) {
			fseek(fptr, 12, SEEK_CUR);
			fread(size[n], 1, 4, fptr);
			fread(vdir[n], 1, 4, fptr);
			n++;
		}
	}

	unsigned long cur_vdir, cur_vpn, cur_offset;
	unsigned long mask;
	
	for (int i = 0; i < n; i++) {
		cur_vdir = in_big_endian(vdir[i]);
		mask = ((1 << 5) - 1) << 23;
		cur_vpn = (mask & cur_vdir) >> 23;

		if (cur_vpn == vpn) {
			mask = (1 << 23) - 1;
			cur_offset = mask & cur_vdir;

			if (cur_offset > max_offset || !found) {
				found = 1;
				max_offset = cur_offset;
				size_ = in_big_endian(size[i]);
			}
		}
	}

	(*offset) = max_offset + size_;
	fclose(fptr);
}


int find_available_frame() {

	int save = -1;

	FILE *fptr = fopen(MEMORY_PATH, "r+b");

	unsigned int x = 1;
	int y = (int)(((char*)&x)[0]);

	unsigned char byte[1];
	unsigned char temp[1];

	for (int i = 0; i < 16; i++) {
		fseek(fptr, (1 << 12) + i, SEEK_SET);
		fread(byte, 1, 1, fptr);
		memcpy(temp, byte, 1);

		printf("Frame %i-%i: %u\n", i*8, (i + 1)*8 - 1, byte[0]);

		if (y) {
			for (int k = 0; k < 8; k++) {
				if (!((unsigned char)(temp[0] & (1 << (7 - k))) >> (7 - k)) && save == -1) {
					printf("Frame available: %i\n", i * 8 + k);
					save = i * 8 + k;
				}
			}
		}
	}

	fclose(fptr);
	return save;
}

void frame_page_association(int pid, int vpn, int fpn) {

	FILE *fptr = fopen(MEMORY_PATH, "r+b");
	unsigned char byte[1];

	fseek(fptr,
		PCB_ENTRY_SIZE * (find_process(pid) + 1) - PAGE_TABLE_SIZE,
		SEEK_SET);

	printf("I'm going to associate vpn %i with fpn %i\n", vpn, fpn);

	fseek(fptr, vpn, SEEK_CUR);
	byte[0] = (unsigned char)(fpn) | (1 << 7);
	fwrite(byte, 1, 1, fptr);

	fseek(fptr, (1 << 12) + fpn / 8, SEEK_SET);
	fread(byte, 1, 1, fptr);
	byte[0] = byte[0] | (1 << (7 - (fpn % 8)));

	fseek(fptr, (1 << 12) + fpn / 8, SEEK_SET);
	fwrite(byte, 1, 1, fptr);

	fclose(fptr);
}

void write_file_entry(CrmsFile* file, int vpn, unsigned long offset) {

	FILE *fptr = fopen(MEMORY_PATH, "r+b");

	int pentry = find_process(file->pid);
	unsigned char valid_byte[1];
	char fname[13] = {0};
	unsigned long fsize_bytes;
	unsigned char vdir[4] = {0};
	unsigned char fsize[4] = {0};

	if (!(file->allocated)) {
		file->allocated = 1;
		for (int i = 0; i < 10; i++) {

			fseek(fptr,
				PCB_ENTRY_SIZE * pentry + 14 + 21 * i,
				SEEK_SET);

			fread(valid_byte, 1, 1, fptr);
			if (!(valid_byte[0])) {

				vdir[0] = (int)(vpn & (((1 << 4) - 1) << 1)) >> 1;
				vdir[1] = (int)((unsigned long)(offset & (((1 << 7) - 1) << 16)) >> 16);
				vdir[1] = (vpn & 1) << 7 | vdir[1];
				vdir[2] = (unsigned long)(offset & (((1 << 8) - 1) << 8)) >> 8;
				vdir[3] = offset & ((1 << 8) - 1);

				memcpy(fsize, &(file->filesize), 4);
				fsize_bytes = in_big_endian(fsize);
				memcpy(fsize, &fsize_bytes, 4);

				strcpy(fname, file->filename);

				fseek(fptr,
					PCB_ENTRY_SIZE * pentry + 14 + 21 * i,
					SEEK_SET);
				unsigned char byte[1] = {1};
				fwrite(byte, 1, 1, fptr);

				fseek(fptr,
					PCB_ENTRY_SIZE * pentry + 14 + 21 * i + 1,
					SEEK_SET);
				fwrite(fname, 12, 1, fptr);

				fseek(fptr,
					PCB_ENTRY_SIZE * pentry + 14 + 21 * i + 13,
					SEEK_SET);
				fwrite(fsize, 4, 1, fptr);

				fseek(fptr,
					PCB_ENTRY_SIZE * pentry + 14 + 21 * i + 17,
					SEEK_SET);
				fwrite(vdir, 4, 1, fptr);

				printf("Offset %lu, VPN %i\n", offset, vpn);

				break;
			}
		}
	} else {

		int position = find_file(find_process(file->pid), file->filename);

		if (position == -1) {
			printf("El archivo ya no existe.\n");
		}

		fseek(fptr,
				PCB_ENTRY_SIZE * pentry + 14 + 21 * position,
				SEEK_SET);

		fread(valid_byte, 1, 1, fptr);

		if (valid_byte[0]) {
			fseek(fptr,
				PCB_ENTRY_SIZE * pentry + 14 + 21 * position + 13,
				SEEK_SET);
			memcpy(fsize, &(file->filesize), 4);
			fsize_bytes = in_big_endian(fsize);
			memcpy(fsize, &fsize_bytes, 4);

			fwrite(fsize, 4, 1, fptr);
		} else {
			printf("El archivo ya no existe.\n");
		}

	}

	fclose(fptr);
}

int cr_write_file(CrmsFile* file_desc, void* buffer, int n_bytes) {

	if (file_desc->mode != 'w') {
		printf("El archivo %s es de solo lectura.\n", file_desc->filename);
		return 0;
	}

	int pentry = find_process(file_desc->pid);

	if (pentry == -1) {
		printf("El proceso de ID %i ya no se encuentra activo.\n", file_desc->pid);
		return 0;
	}

	if (find_file(pentry, file_desc->filename) == -1 && file_desc->allocated) {
		printf("El archivo ya no existe.\n");
		file_desc->allocated = 0;
		return 0;
	} 

	if (find_file(pentry, file_desc->filename) != -1 && file_desc->allocated == 0) {
		printf("Ya existe un archivo de nombre %s.\n", file_desc->filename);
		return 0;
	}

	FILE *fptr = fopen(MEMORY_PATH, "r+b");

	int position = PCB_ENTRY_SIZE * (pentry + 1) - PAGE_TABLE_SIZE;
	fseek(fptr, position, SEEK_SET);

	unsigned char byte[1];
	int first_empty_vp = -1;
	unsigned long start_offset = 0;
	int vpn = 0;
	unsigned char vpn_byte[1];
	int fpn;

	if (!(file_desc->allocated)) {
		for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
			fseek(fptr, position + i, SEEK_SET);
			fread(byte, 1, 1, fptr);

			printf("Página número %i libre:", i);
			
			if (!valid_page_entry(byte[0])) {
				printf(" sí.\n");
				first_empty_vp = i;
				break;
			} else printf(" no.\n");
		}

		if (first_empty_vp != 0) {

			if (first_empty_vp == -1)
				first_empty_vp = 32;

			vpn = first_empty_vp - 1;

			max_offset(file_desc->pid, vpn, &start_offset);

			int k = 1;
			while (!start_offset && k <= vpn) {
				max_offset(file_desc->pid, vpn - k, &start_offset);
				k++;
				if (start_offset) {
					start_offset = start_offset % (1 << 23);
					break;
				}
			}

			if (start_offset == (1 << 23)) {
				start_offset = 0;
				vpn++;
			}

		}

		if (vpn >= 32) {
			printf("No queda memoria virtual.\n");
			return 0;
		}

		fseek(fptr,
			PCB_ENTRY_SIZE * (pentry + 1) - PAGE_TABLE_SIZE + vpn,
			SEEK_SET);


		fread(vpn_byte, 1, 1, fptr);

		if (valid_page_entry(vpn_byte[0]))
			fpn = get_fpn(vpn_byte[0]);
		else {
			fpn = find_available_frame();
			if (fpn == -1) {
				printf("No quedan frames disponibles.\n");
				fclose(fptr);
				return 0;
			} else {
				frame_page_association(file_desc->pid, vpn, fpn);
			}
		}
	}
		
	unsigned long abs_dir = (1 << 12) + (1 << 4) + ((1 << 23) * fpn);
	unsigned long limit;
	int written = 0;
	int pos, start_vpn;

	if (file_desc->allocated) {
		min_offset(file_desc->pid,
			file_desc->vpn, file_desc->offset, &limit);

		if (limit == file_desc->offset) {
			printf("No queda espacio contiguo en la memoria.");
			fclose(fptr);
			return 0;
		}

		pos = file_desc->offset;
		start_vpn = file_desc->vpn;
	} else {
		limit = 1 << 23;
		file_desc->vpn = vpn;
		file_desc->offset = start_offset;
		file_desc->filesize = 0;
		start_vpn = vpn;
		pos = start_offset;
	}

	while (1) {

		fseek(fptr, abs_dir + pos, SEEK_SET);
		fwrite(buffer + written, 1, 1, fptr);
		file_desc->filesize += 1;
		file_desc->offset += 1;
		written++;
		pos++;

		if (written == n_bytes) {
			write_file_entry(file_desc, start_vpn, start_offset);
			file_desc->allocated = 1;
			fclose(fptr);
			return written;
		}

		if (start_offset + pos == limit) {
			vpn++;

			if (limit < (1 << 23) || vpn >= 32) {
				printf("No queda memoria contigua.\n");
				write_file_entry(file_desc, start_vpn, start_offset);
				file_desc->allocated = 1;
				fclose(fptr);
				return written;
			}

			file_desc->vpn = vpn;
			file_desc->offset = 0;

			fseek(fptr,
				PCB_ENTRY_SIZE * (pentry + 1) - PAGE_TABLE_SIZE + vpn,
				SEEK_SET);

			fread(vpn_byte, 1, 1, fptr);
			if (valid_page_entry(vpn_byte[0])) {
				fpn = get_fpn(vpn_byte[0]);
				abs_dir = (1 << 12) + (1 << 4) + ((1 << 23) * fpn);
				pos = 0;
				min_offset(file_desc->pid, vpn, 0, &limit);

				if (limit == 0) {
					write_file_entry(file_desc, start_vpn, start_offset);
					file_desc->allocated = 1;
					printf("No queda memoria contigua.\n");
					fclose(fptr);
					return written;
				}

			} else {
				fpn = find_available_frame();

				if (fpn == -1) {
					write_file_entry(file_desc, start_vpn, start_offset);
					file_desc->allocated = 1;
					printf("No quedan frames disponibles.\n");
					fclose(fptr);
					return written;
				}

				frame_page_association(file_desc->pid, vpn, fpn);
				abs_dir = (1 << 12) + (1 << 4) + ((1 << 23) * fpn);
				pos = 0;
				limit = (1 << 23);
			}
		}

	}
}

void cr_finish_process(int process_id) {

	FILE *fptr = fopen(MEMORY_PATH, "r+b");

	int pentry = find_process(process_id);
	unsigned char byte[1];
	int fpn;
	unsigned long position;
	unsigned char mask;

	if (pentry == -1) {
		printf("No existe un proceso con ID %i\n", process_id);
		return;
	}

	for (int i = 0; i < 32; i++) {
		position = PCB_ENTRY_SIZE * (pentry + 1) - PAGE_TABLE_SIZE + i;
			
		fseek(fptr, position, SEEK_SET);
		fread(byte, 1, 1, fptr);

		if (valid_page_entry(byte[0])) {

			fpn = get_fpn(byte[0]);
			byte[0] = 0;
			fseek(fptr, position, SEEK_SET);
			fwrite(byte, 1, 1, fptr);

			fseek(fptr, (1 << 12) + fpn / 8, SEEK_SET);
			fread(byte, 1, 1, fptr);
			mask = (1 << (7 - (fpn % 8)));
			byte[0] = byte[0] ^ mask;

			fseek(fptr, (1 << 12) + fpn / 8, SEEK_SET);
			fwrite(byte, 1, 1, fptr);
		}
	}


	byte[0] = 0;
	for (int i = 0; i < 10; i++) {
		position = PCB_ENTRY_SIZE * pentry + 14 + i * 21;
		fseek(fptr, position, SEEK_SET);
		fwrite(byte, 1, 1, fptr);
	}

	fseek(fptr, PCB_ENTRY_SIZE * pentry, SEEK_SET);
	byte[0] = 0;
	fwrite(byte, 1, 1, fptr);

	fclose(fptr);
}

void invalidate_frame(int fpn) {

	FILE *fptr = fopen(MEMORY_PATH, "r+b");
	unsigned char byte[1] = {0};
	unsigned char mask;

	fseek(fptr, (1 << 12) + fpn / 8, SEEK_SET);
	fread(byte, 1, 1, fptr);
	mask = (1 << (7 - (fpn % 8)));
	byte[0] = byte[0] ^ mask;
	fseek(fptr, (1 << 12) + fpn / 8, SEEK_SET);
	fwrite(byte, 1, 1, fptr);

	fclose(fptr);
}

void cr_delete_file(CrmsFile* file_desc) {

	// Restricciones
	/*
	if (file_desc->mode != 'w') {
		printf("El archivo %s [PID = %i] está abierto en solo lectura.\n",
			file_desc->filename, file_desc->pid);
		return;
	}*/

	if (!(file_desc->allocated)) {
		printf("El archivo %s [PID = %i] no se ha escrito en memoria aún.\n",
			file_desc->filename, file_desc->pid);
	}

	FILE *fptr = fopen(MEMORY_PATH, "r+b");

	// Encontrar el número de entrada en el PCB del proceso asociado al archivo.
	// Ídem para el número de entrada del archivo dentro del proceso.
	int pentry = find_process(file_desc->pid);
	int fentry = find_file(pentry, file_desc->filename);	

	printf("pentry %i, fentry %i\n", pentry, fentry);


	// Se invalida la entrada del archivo en el proceso.
	fseek(fptr,
		PCB_ENTRY_SIZE * pentry + 14 + 21 * fentry, SEEK_SET);
	unsigned char byte[1] = {0};
	fwrite(byte, 1, 1, fptr);


	// Todo esto es para conseguir el VPN, el offset y el tamaño del archivo.
	fseek(fptr,
		PCB_ENTRY_SIZE * pentry + 14 + 21 * fentry + 13, SEEK_SET);

	unsigned char fsize_bytes[4] = {0};
	unsigned char vdir_bytes[4] = {0};
	fread(fsize_bytes, 1, 4, fptr);
	fread(vdir_bytes, 1, 4, fptr);
		
	unsigned long virtual_dir = in_big_endian(vdir_bytes);
	unsigned long file_size = in_big_endian(fsize_bytes);

	unsigned long mask;
	mask = (1 << 23) - 1;
	unsigned long offset = mask & virtual_dir;

	mask = ((1 << 5) - 1) << 23;
	unsigned long vpn = (mask & virtual_dir) >> 23;

	// Hay que sumar el offset + el tamaño de la página,
	// y ver cuántas páginas se extiende hacia arriba el archivo actual.
	// Hay que revisar la máxima página en la que el archivo está escrito,
	// en caso de que quede vacía al borrarlo.
	// (además de todas las páginas que el archivo ocupa por completo,
	// si existen)

	int occupying = (offset + file_size) / (1 << 23); // (1 << 23) = 2^23 = 8MiB
	unsigned long residue = (offset + file_size) % (1 << 23);
	unsigned long upper_bound, lower_bound;
	int temp_fpn;

	if (occupying) {

		for (int i = 1; i < occupying; i++) {
			// Estas son las páginas intermedias,
			// se invalidan porque el archivo las ocupa completas.
			fseek(fptr,
				PCB_ENTRY_SIZE * (pentry + 1) - PAGE_TABLE_SIZE + vpn + i,
				SEEK_SET);
			fread(byte, 1, 1, fptr);

			temp_fpn = get_fpn(byte[0]);
			invalidate_frame(temp_fpn);

			printf("FPN %i invalidado.\n", temp_fpn);
			
			fseek(fptr,
				PCB_ENTRY_SIZE * (pentry + 1) - PAGE_TABLE_SIZE + vpn + i,
				SEEK_SET);
			byte[0] = 0;
			fwrite(byte, 1, 1, fptr);
		}

		// Ahora revisamos la última, con VPN = vpn + occupying
		upper_bound = 0;
		min_offset(file_desc->pid, vpn + occupying, 0, &upper_bound);

		// min_offset deja upper_bound en 1 << 23 = 2^23 si no hay nada arriba.
		// Si no hay nada, hay que invalidar la página y el frame.
		if (upper_bound == (1 << 23)) {
			fseek(fptr,
				PCB_ENTRY_SIZE * (pentry + 1) - PAGE_TABLE_SIZE + vpn + occupying,
				SEEK_SET);
			fread(byte, 1, 1, fptr);

			temp_fpn = get_fpn(byte[0]);
			invalidate_frame(temp_fpn);

			printf("FPN %i invalidado.\n", temp_fpn);

			fseek(fptr,
				PCB_ENTRY_SIZE * (pentry + 1) - PAGE_TABLE_SIZE + vpn + occupying,
				SEEK_SET);
			byte[0] = 0;
			fwrite(byte, 1, 1, fptr);
		}


	} else {
	// Ahora revisamos si hay un archivo EN LA MISMA PÁGINA,
	// arriba del que queremos borrar.
		upper_bound = 0;
		min_offset(file_desc->pid, vpn, 0, &upper_bound);

		if (upper_bound < (1 << 23)) {
			printf("There still are files in VPN %i\n", vpn);
			return;
		}
	}

	// Revisamos si hay un archivo abajo en la misma página.
	// Puede ser que no comienze en esta página, así que hay que buscar
	// en las páginas de abajo.

	lower_bound = 0;
	max_offset(file_desc->pid, vpn, &lower_bound);
	int k = 1;
	while (!lower_bound && k <= vpn) {
		max_offset(file_desc->pid, vpn - k, &lower_bound);
		k++;
	}

	if (lower_bound) {
		int cur_vpn = vpn - (k - 1);
		if (lower_bound > (vpn - cur_vpn) * (1 << 23)) {
			printf("There still are files in VPN %i\n", vpn);
			return;
		}
	}
	printf("There are no more files in VPN %i.\n", vpn);


	// Si nada de esto pasa, hay que invalidar el frame y la página.
	fseek(fptr,
		PCB_ENTRY_SIZE * (pentry + 1) - PAGE_TABLE_SIZE + vpn,
		SEEK_SET);
	fread(byte, 1, 1, fptr);

	// Se consigue el FPN de la página antes de invalidarla.
	int fpn = get_fpn(byte[0]);
	
	// Se invalida la página.
	fseek(fptr,
		PCB_ENTRY_SIZE * (pentry + 1) - PAGE_TABLE_SIZE + vpn,
		SEEK_SET);
	byte[0] = 0;
	fwrite(byte, 1, 1, fptr);


	// Se invalida el frame.
	invalidate_frame(fpn);

	printf("FPN %i invalidado.\n", fpn);

	fclose(fptr);

}

int os_read(CrmsFile* file_desc, void* buffer, int n_bytes){
	
	if (file_desc->mode != 'r') {
		printf("El archivo %s es de solo lectura.\n", file_desc->filename);
		return 0;
	}

	int pentry = find_process(file_desc->pid);

	if (pentry == -1) {
		printf("El proceso de ID %i ya no se encuentra activo.\n", file_desc->pid);
		return 0;
	}

    int fentry = find_file(pentry, file_desc->filename); 

	if (fentry == -1 && file_desc->allocated) {
		printf("El archivo ya no existe.\n");
		file_desc->allocated = 0;
		return 0;
	} 

	FILE *fptr = fopen(MEMORY_PATH, "r+b");
	CrmsFile* file=  cr_open(file_desc->pid, file_desc->filename, "r");

	get_fpn(unsigned char byte)
	valid_page_entry(unsigned char byte)
	
}

