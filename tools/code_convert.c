/*
 * code convert - to convert binary ARM instructions into series of
 * ARM instructions so that those insturctions can be formed and to
 * be copied without referencing to other location.
 *
 * This code is needed due to the NAND boot limitation that it
 * can only execute instructions sequencially until it is ready
 * to transfer the execution to DDR.
 *
 * Usage: code_convert input_binary output_arm_assembly_program
 *
 * The output file is to be included in Boot Stage 1 code
 *
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int errno;

int main(int argc, char *argv[])
{
	FILE* ifd;
	FILE* ofd;
	unsigned int value;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s binary_file output_file\n", argv[0]);	}
	else {
		if ((ifd = fopen(argv[1], "rb")) == NULL) {
			fprintf(stderr, "Can't open input file %s: %s\n", argv[1], strerror(errno));
			return -1;
		};
		if ((ofd = fopen(argv[2], "w")) == NULL) {
			fprintf(stderr, "Can't open output file %s: %s\n", argv[1], strerror(errno));
			return -1;
		};
		/* start the inittial information */
		fprintf(ofd, "/* do not modify this file content, it is generated automatically */\n");
		while (fread (&value, sizeof (int), 1, ifd)) {
			fprintf(ofd, "	mov	r2, #0x%x\n",value & 0xff);
			fprintf(ofd, "	add	r2, r2, #0x%x\n",value & 0xff00);
			fprintf(ofd, "	add	r2, r2, #0x%x\n",value & 0xff0000);
			fprintf(ofd, "	add	r2, r2, #0x%x\n",value & 0xff000000);
			fprintf(ofd, "	str	r2, [r1], #4\n");
		}
		fclose(ifd);
		fclose(ofd);
	}
	return 0;
}
