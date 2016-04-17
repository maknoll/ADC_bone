/*
 ============================================================================
 Name        : ADC_bone.c
 Author      : 
 Version     :
 Copyright   : 
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <libserialport.h>

FILE * logfile;
struct sigaction old_action;
int total_bytes = 0;
extern int errno;

void int_handler(int sig)
{
	printf("\nflushing buffers...\n");
	fflush(stdout);
	fflush(logfile);
	exit(0);
}

int main(int argc, char * argv[])
{

	char buffer[1024];
	struct sp_port * port;

	sp_get_port_by_name(argv[1], &port);
	if (sp_open(port,SP_MODE_READ) != SP_OK) {
		fprintf(stderr,"Error opening Port\n");
		return EXIT_FAILURE;
	}
	struct sp_port_config * config;
	sp_new_config(&config);
	sp_set_config_baudrate(config,3000000);
	sp_set_config_bits(config,8);
	sp_set_config_parity(config,SP_PARITY_NONE);
	sp_set_config_stopbits(config,1);
	if (sp_set_config(port,config) != SP_OK) {
		fprintf(stderr,"Error configuring Port\n");
		return EXIT_FAILURE;
	}
	sp_free_config(config);

	logfile = fopen("logfile","w");

	if (logfile == NULL) {
		fprintf(stderr, "Value of errno: %d\n", errno);
		perror("Error printed by perror");
		fprintf(stderr, "Error opening file: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	while (1) {
		int count = sp_blocking_read(port,buffer,8,100);
		fwrite(buffer, sizeof(char), count, logfile);
		total_bytes += count;
		if (!(total_bytes & 0xFF)) {
			printf("\rTotal bytes: %d",total_bytes);
			fflush(stdout);
		}
	}

	fclose(logfile);
	sp_close(port);
	sp_free_port(port);

	return EXIT_SUCCESS;
}
