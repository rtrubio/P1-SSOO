#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../crms_API/crms_API.h"

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

	printf("Iniciando procesos...\n");

	cr_start_process(12, "PROCESS12");
	cr_start_process(99, "PROCESS99");
	cr_start_process(100, "PROCESS100");

	cr_ls_processes();

	cr_clean();
}