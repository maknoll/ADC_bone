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
#include <signal.h>
#include <libftdi1/ftdi.h>

FILE * logfile;
int total_bytes = 0;
extern int errno;

static int exit_requested = 0;
static void sigint_handler(int signum)
{
    exit_requested = 1;
}

int main(int argc, char * argv[])
{

	struct ftdi_context *ftdi;
	uint8_t buffer[1024];

	signal(SIGINT, sigint_handler);

    if ((ftdi = ftdi_new()) == 0)
    {
        fprintf(stderr, "ftdi_new failed\n");
        return EXIT_FAILURE;
    }

    ftdi_set_interface(ftdi, 1);

	int f = 0;
    f = ftdi_usb_open(ftdi, 0x403, 0x6010);
    if (f < 0)
    {
    	fprintf(stderr, "unable to open ftdi device: %d (%s)\n", f, ftdi_get_error_string(ftdi));
    	exit(-1);
    }

    f = ftdi_set_baudrate(ftdi, 8000000);
    if (f < 0)
    {
        fprintf(stderr, "unable to set baudrate: %d (%s)\n", f, ftdi_get_error_string(ftdi));
        exit(-1);
    }

    f = ftdi_set_line_property(ftdi, 8, STOP_BIT_1, NONE);
    if (f < 0)
    {
        fprintf(stderr, "unable to set line parameters: %d (%s)\n", f, ftdi_get_error_string(ftdi));
        exit(-1);
    }

	logfile = fopen("logfile","w");

	if (logfile == NULL) {
		fprintf(stderr, "Value of errno: %d\n", errno);
		perror("Error printed by perror");
		fprintf(stderr, "Error opening file: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	while (!exit_requested) {
		int count = ftdi_read_data(ftdi, buffer, sizeof(buffer));;
		fwrite(buffer, sizeof(char), count, logfile);
		total_bytes += count;
		printf("\rTotal bytes: %d",total_bytes);
		fflush(stdout);
	}
	printf("\n\rCleaning up...\n\r");
	fflush(logfile);
	fclose(logfile);
    signal(SIGINT, SIG_DFL);

    ftdi_usb_close(ftdi);
    ftdi_free(ftdi);

	return EXIT_SUCCESS;
}
