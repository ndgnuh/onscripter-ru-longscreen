/**
 *  nscdec.cpp
 *  ONScripter-RU
 *
 *  Script decompression tool.
 *
 *  Consult LICENSE file for licensing terms and copyright holders.
 */

#include <zlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_IN 0x10000000
#define VERSION 110

struct header {
	int magic;
	int compressed;
	int decompressed;
	int version;
} hdr;

int main(int argc, char **argv) {
	char *in_filename = NULL, *out_filename = NULL;
	FILE *in_fp, *out_fp;
	bool use_stdout              = false;
	unsigned char key_table[256] = {
		0x37, 0x6a, 0x09, 0x5e, 0x7a, 0xaf, 0xf5, 0xa4, 0xba, 0x78, 0x84, 0x58, 0x35, 0x1e, 0x6b, 0x0c,
		0x49, 0xc6, 0xc3, 0x44, 0x40, 0x9e, 0x6f, 0x65, 0xe4, 0xf6, 0xfe, 0x22, 0xe2, 0x95, 0xc7, 0x38,
		0xf0, 0x1a, 0x82, 0xe0, 0x5b, 0x2a, 0xd8, 0xe5, 0xce, 0x2f, 0x74, 0x25, 0xec, 0x59, 0xc0, 0x45,
		0x4b, 0x64, 0x43, 0xdc, 0xb0, 0xb9, 0x30, 0x6d, 0x28, 0xd1, 0x16, 0xbb, 0x66, 0x98, 0x92, 0x90,
		0x2c, 0xa7, 0xf1, 0x80, 0xc1, 0xd4, 0x8b, 0xd6, 0xdf, 0x24, 0x2d, 0xf7, 0xfb, 0x88, 0x4d, 0x3c,
		0x72, 0xf3, 0xdb, 0x2b, 0x93, 0x73, 0xef, 0x85, 0x83, 0xee, 0xc2, 0x8d, 0x5c, 0xb2, 0x0b, 0x94,
		0x3d, 0xa8, 0x3f, 0x1c, 0x4c, 0x6e, 0x03, 0x7b, 0x1d, 0x5a, 0x51, 0xa1, 0x70, 0x41, 0xd0, 0xaa,
		0xa0, 0x7e, 0xcd, 0xd5, 0x15, 0xa9, 0x18, 0x76, 0xc9, 0x7d, 0x7f, 0x0e, 0x3a, 0x99, 0xbf, 0xab,
		0x3b, 0x14, 0x3e, 0x9a, 0x04, 0xda, 0x02, 0xfd, 0x63, 0xd9, 0xfa, 0x9f, 0x4e, 0xe3, 0x61, 0xbe,
		0x07, 0x11, 0xa6, 0x1b, 0x19, 0x55, 0x8e, 0x77, 0x0a, 0x47, 0xe6, 0xf8, 0x0d, 0xcf, 0xd7, 0x33,
		0x23, 0x1f, 0xbc, 0x62, 0xde, 0x9b, 0x29, 0x53, 0x68, 0xe8, 0x21, 0xb6, 0x34, 0x52, 0x87, 0xcb,
		0x08, 0x79, 0xf4, 0x67, 0x69, 0x54, 0xe7, 0x86, 0xea, 0xb4, 0x20, 0x71, 0x01, 0xbd, 0x06, 0x31,
		0x00, 0x50, 0xc8, 0xb8, 0xac, 0x5d, 0x57, 0x7c, 0x89, 0xeb, 0xb7, 0x36, 0x8f, 0xf2, 0xe1, 0x56,
		0x81, 0x4a, 0xd2, 0x8c, 0xf9, 0xad, 0x60, 0xa5, 0x42, 0x10, 0x5f, 0x12, 0xb3, 0xff, 0x4f, 0xdd,
		0x46, 0x26, 0xa2, 0x17, 0xc5, 0x75, 0x91, 0x27, 0xb5, 0x8a, 0xd3, 0x13, 0x2e, 0xc4, 0xe9, 0x9d,
		0x97, 0x39, 0x32, 0x05, 0x0f, 0xca, 0xcc, 0x48, 0xfc, 0xae, 0x96, 0xed, 0x6c, 0x9c, 0xb1, 0xa3
	};
	int i;

	if ((argc == 2) || (argc == 3)) {
#ifndef WIN32
		if (strcmp(argv[1], "-")) {
			// "-" arg means "use stdin"
#endif
			in_filename = argv[1];
#ifndef WIN32
		}
#endif
		if (argc == 3) {
#ifndef WIN32
			if (strcmp(argv[2], "-")) {
				// "-" arg means "use stdout"
#endif
				out_filename = argv[2];
#ifndef WIN32
			} else {
				use_stdout = true;
			}
#endif
		}
	} else {
		fprintf(stderr, "Usage: nscdec nsc_file [out_file]\n");
		fprintf(stderr, "	(out_file defaults to \"result.txt\")\n");
		exit(-1);
	}

	if (in_filename)
		in_fp = fopen(in_filename, "rb");
	else
		in_fp = stdin;
	if (!in_fp && in_filename) {
		fprintf(stderr, "Couldn't open '%s' for reading\n", in_filename);
		exit(1);
	}

	fseek(in_fp, 0, 0);
	fread(&hdr, sizeof(hdr), 1, in_fp);
	if (hdr.magic != 0x32534E4F || hdr.version != VERSION) {
		fprintf(stderr, "This file is corrupted or unsupported\n");
		exit(1);
	}

	unsigned char *in_buf  = (unsigned char *)malloc(hdr.compressed);
	unsigned char *out_buf = (unsigned char *)malloc(hdr.decompressed);

	if (!in_buf || !out_buf) {
		fprintf(stderr, "Memory allocation failure!");
		free(in_buf); free(out_buf);
		return 1;
	}

	fread(in_buf, hdr.compressed, 1, in_fp);
	if (in_filename)
		fclose(in_fp);

	for (i = 0; i < hdr.compressed; i++) {
		in_buf[i] = key_table[in_buf[i] ^ 0x86] ^ 0x23;
	}

	unsigned long decompressed = hdr.decompressed;

	int gz_result = uncompress(out_buf, &decompressed, in_buf, (unsigned long)hdr.compressed);

	if (gz_result != Z_OK) {
		fprintf(stderr, "Decompression error\n");
		free(in_buf); free(out_buf);
		return 1;
	}

	if (use_stdout)
		out_fp = stdout;
	else if (out_filename)
		out_fp = fopen(out_filename, "wb");
	else
		out_fp = fopen("result.txt", "wb");
	if (!out_fp && !use_stdout) {
		if (out_filename)
			fprintf(stderr, "Couldn't open '%s' for writing\n", out_filename);
		else
			fprintf(stderr, "Couldn't open '%s' for writing\n", "result.txt");
		free(in_buf); free(out_buf);
		return 1;
	}

	for (i = 0; i < hdr.decompressed; i++)
		fputc(key_table[out_buf[i] ^ 0x45] ^ 0x71, out_fp);

	if (!use_stdout)
		fclose(out_fp);

	free(in_buf); free(out_buf);

	return 0;
}