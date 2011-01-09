#ifndef READPPM_H
#define READPPM_H

typedef enum {PNM_NATIVE,
	      PNM_RGB,
	      PNM_GREY,
	      PNM_UNKNOWN} PnmPixformat;

int
read_pnm (const char *filename,
	  unsigned char **bytes,
	  int *width, int *height,
	  PnmPixformat *pixformat);

#endif
