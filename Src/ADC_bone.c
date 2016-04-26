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

FILE * files[4] = {0};
int total_bytes = 0;
extern int errno;

static int exit_requested = 0;
static void sigint_handler(int signum)
{
    exit_requested = 1;
}

int main(int argc, char * argv[])
{
	uint32_t baudrate = 8000000;
	char filenameprefix[64] = "channel";
    int vid = 0x403;
    int pid = 0x6010;

	int optint;
    while ((optint = getopt(argc, argv, "b:f::")) != -1)
    {
        switch (optint)
        {
        case 'b':
        	baudrate = strtoul(optarg, NULL, 0);
        	break;
        case 'f':
        	snprintf(filenameprefix,sizeof(filenameprefix),"%s",optarg);
        default:
            fprintf(stderr, "usage: %s [-b baudrate] [-f filename-prefix]\n", *argv);
            exit(-1);
        }
    }

	struct ftdi_context *ftdi;

	signal(SIGINT, sigint_handler);

    if ((ftdi = ftdi_new()) == 0)
    {
        fprintf(stderr, "ftdi_new failed\n");
        return EXIT_FAILURE;
    }

    ftdi_set_interface(ftdi, 1);

	int f = 0;
    f = ftdi_usb_open(ftdi, vid, pid);
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

    for (int i=0; i<4; i++) {
    	char filename[128];
    	snprintf(filename, sizeof(filename), "%s-%i.bin", filenameprefix, i);
    	files[i] = fopen(filename,"w");
    	if (files[i] == NULL) {
    		fprintf(stderr, "Value of errno: %d\n", errno);
    		perror("Error printed by perror");
    		fprintf(stderr, "Error opening file: %s\n", strerror(errno));
    		return EXIT_FAILURE;
    	}
    }

	enum decoder_state {
		IDLE,
		PREAMBLE,
		CHANNEL,
		DATA
	};

	enum decoder_state state = IDLE;
	uint16_t active_channel = 1;
	uint16_t samples = 0;
	uint8_t buffer[2000];
	uint16_t count = 0;
	while (!exit_requested) {
		switch (state) {
			case IDLE:
				count = ftdi_read_data(ftdi, buffer, 1);
				if (count && (0xFF == buffer[0])) {
					state = PREAMBLE;
				}
				break;
			case PREAMBLE:
				count = ftdi_read_data(ftdi, buffer, 1);
				if (count && (0xFF == buffer[0])) {
					state = CHANNEL;
				}
				if (count && (0xFF != buffer[0])) {
					state = IDLE;
				}
				break;
			case CHANNEL:
				count = ftdi_read_data(ftdi, buffer, 2);
				if (count) {
					active_channel = (uint16_t)buffer[0];
					if ((0 < active_channel) && (active_channel <= 4)) {
						state = DATA;
					} else {
						state = IDLE;
					}
				}
				break;
			case DATA:
				count = ftdi_read_data(ftdi, buffer, sizeof(buffer)-samples);
				samples += count;
				total_bytes += count;
				if (2000 <= samples) {
					samples = 0;
					state = IDLE;
					fwrite(buffer, sizeof(char), sizeof(buffer), files[active_channel-1]);
				}
				break;
		}
		printf("\rTotal bytes: %d",total_bytes);
		fflush(stdout);
	}
	printf("\n\rCleaning up...\n\r");

	for (int i=0; i<4; i++) {
		fflush(files[i]);
		fclose(files[i]);
	}

    signal(SIGINT, SIG_DFL);

    ftdi_usb_close(ftdi);
    ftdi_free(ftdi);

	return EXIT_SUCCESS;
}
