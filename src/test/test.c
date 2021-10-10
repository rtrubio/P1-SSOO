#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "../crms_API/crms_API.h"

void read_file(char* file_name) {

	printf("\n");
	printf("Leyendo archivo %s...\n\n", file_name);

	CrmsFile *file = cr_open(0, file_name, 'r');


	printf("%16s %i\n", "PARENT PROCESS", file->pid);
	printf("%16s %s\n", "FILE NAME", file->filename);
	printf("%16s %lu\n", "FILE SIZE", file->filesize);
	printf("%16s %lu\n", "VPN", file->vpn);
	printf("%16s %lu\n", "OFFSET", file->offset);
	
	printf("\n");

	free(file);
}

int main(int argc, char *argv[]) {

	if (argc == 1) {
		printf("No se ha especificado un archivo de memoria.\n");
		exit(0);
	}

	if (access(argv[1], F_OK)) {
		printf("El archivo especificado no existe.\n");
		exit(0);
	}

	cr_mount(argv[1]);

	cr_ls_processes();

	read_file("message.txt");
	read_file("secret.txt");
	read_file("test.jpg");
	read_file("im_a_mp3.bin");
}