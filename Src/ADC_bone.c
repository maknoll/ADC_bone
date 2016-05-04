
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
const unsigned char me_decode_tab[256] = {
0x0, 0x0, 0x1, 0x1, 0x0, 0x0, 0x1, 0x1, 0x2,
0x2, 0x3, 0x3, 0x2, 0x2, 0x3, 0x3, 0x0, 0x0,
0x1, 0x1, 0x0, 0x0, 0x1, 0x1, 0x2, 0x2, 0x3,
0x3, 0x2, 0x2, 0x3, 0x3, 0x4, 0x4, 0x5, 0x5,
0x4, 0x4, 0x5, 0x5, 0x6, 0x6, 0x7, 0x7, 0x6,
0x6, 0x7, 0x7, 0x4, 0x4, 0x5, 0x5, 0x4, 0x4,
0x5, 0x5, 0x6, 0x6, 0x7, 0x7, 0x6, 0x6, 0x7,
0x7, 0x0, 0x0, 0x1, 0x1, 0x0, 0x0, 0x1, 0x1,
0x2, 0x2, 0x3, 0x3, 0x2, 0x2, 0x3, 0x3, 0x0,
0x0, 0x1, 0x1, 0x0, 0x0, 0x1, 0x1, 0x2, 0x2,
0x3, 0x3, 0x2, 0x2, 0x3, 0x3, 0x4, 0x4, 0x5,
0x5, 0x4, 0x4, 0x5, 0x5, 0x6, 0x6, 0x7, 0x7,
0x6, 0x6, 0x7, 0x7, 0x4, 0x4, 0x5, 0x5, 0x4,
0x4, 0x5, 0x5, 0x6, 0x6, 0x7, 0x7, 0x6, 0x6,
0x7, 0x7, 0x8, 0x8, 0x9, 0x9, 0x8, 0x8, 0x9,
0x9, 0xa, 0xa, 0xb, 0xb, 0xa, 0xa, 0xb, 0xb,
0x8, 0x8, 0x9, 0x9, 0x8, 0x8, 0x9, 0x9, 0xa,
0xa, 0xb, 0xb, 0xa, 0xa, 0xb, 0xb, 0xc, 0xc,
0xd, 0xd, 0xc, 0xc, 0xd, 0xd, 0xe, 0xe, 0xf,
0xf, 0xe, 0xe, 0xf, 0xf, 0xc, 0xc, 0xd, 0xd,
0xc, 0xc, 0xd, 0xd, 0xe, 0xe, 0xf, 0xf, 0xe,
0xe, 0xf, 0xf, 0x8, 0x8, 0x9, 0x9, 0x8, 0x8,
0x9, 0x9, 0xa, 0xa, 0xb, 0xb, 0xa, 0xa, 0xb,
0xb, 0x8, 0x8, 0x9, 0x9, 0x8, 0x8, 0x9, 0x9,
0xa, 0xa, 0xb, 0xb, 0xa, 0xa, 0xb, 0xb, 0xc,
0xc, 0xd, 0xd, 0xc, 0xc, 0xd, 0xd, 0xe, 0xe,
0xf, 0xf, 0xe, 0xe, 0xf, 0xf, 0xc, 0xc, 0xd,
0xd, 0xc, 0xc, 0xd, 0xd, 0xe, 0xe, 0xf, 0xf,
0xe, 0xe, 0xf, 0xf, };

static int exit_requested = 0;
static void sigint_handler(int signum)
{
    exit_requested = 1;
}

void me_decode(uint16_t * encoded, uint8_t * decoded, uint16_t lenght)
{
	for (int i = 0; i < lenght; i++) {
		decoded[i] = me_decode_tab[encoded[i] & 0x00FF] | ((me_decode_tab[(encoded[i] & 0xFF00) >> 8]) << 4) ;
	}
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
        	break;
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
    ftdi_read_data_set_chunksize(ftdi, 1<<14);
    uint32_t chunksize = 0;
    ftdi_read_data_get_chunksize(ftdi,&chunksize);
    printf("%i\n\r",chunksize);

	int f = 0;
    f = ftdi_usb_open(ftdi, vid, pid);
    if (f < 0)
    {
    	fprintf(stderr, "unable to open ftdi device: %d (%s)\n", f, ftdi_get_error_string(ftdi));
    	exit(-1);
    }

    f = ftdi_set_baudrate(ftdi, baudrate);
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
    	snprintf(filename, sizeof(filename), "%s-%i.bin", filenameprefix, i+1);
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
#define BUFFERLENGTH (uint16_t)2000
	uint8_t buffer[BUFFERLENGTH];
	uint8_t decoded_buffer[BUFFERLENGTH/2];
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
			case DATA:
				count = ftdi_read_data(ftdi, buffer+samples, sizeof(buffer)-samples);
				samples += count;
				total_bytes += count;
				if (BUFFERLENGTH <= samples) {
					samples = 0;
					state = IDLE;
					//me_decode((uint16_t*)buffer,decoded_buffer,BUFFERLENGTH/2);
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
