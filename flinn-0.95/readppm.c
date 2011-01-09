
#include <stdlib.h>
#include <stdio.h>

#include "abs-file.h"

#include "readppm.h"


int
read_pnm (const char *filename,
	  unsigned char **bytes,
	  int *width, int *height,
	  PnmPixformat *pixformat)
{
  MY_FILETYPE *fp;
  int rtn = 0;
  char str1[201];
  PnmPixformat actual_format = PNM_UNKNOWN;
  char format_ident[3];
  int num_bytes;


	
  if (filename == NULL)
    return -1;
  
  fp = MY_OPEN_FOR_READ(filename);
  
  if (fp == NULL)
    return -2;
  
  if (MY_READ(format_ident, 3, 1, fp) != 1 ||
      format_ident[0]!='P' || format_ident[2]!='\n')
    {
      rtn = -3;
      goto close_file;
    }

  switch (format_ident[1])
    {
    case '5': actual_format = PNM_GREY; break;
    case '6': actual_format = PNM_RGB; break;
    default:
      rtn = -3;
      goto close_file;
    }

  do
    {
      if (NULL == MY_GETS(str1, 200, fp))
	{
	  rtn = -4;
	  goto close_file;
	}
    }
  while (str1[0] == '#');

  if (sscanf(str1, "%d %d\n", width, height) != 2 ||
      *width <= 0 ||
      *height <= 0)
    {
      rtn = -5;
      goto close_file;
    }

  /* fprintf(stderr, ">> %dx%d <<\n", *width, *height); */

  if (MY_GETC(fp)!='2' || MY_GETC(fp)!='5' || MY_GETC(fp)!='5' ||
      MY_GETC(fp)!='\n')
    {
      rtn = -6;
      goto close_file;
    }

  switch (actual_format)
    {
    case PNM_RGB:
      num_bytes = *width * *height * 3;
      break;
    case PNM_GREY:
      num_bytes = *width * *height;
      break;
    default:
      rtn = -9;
      goto close_file;
    }
  
  *bytes = (unsigned char *)malloc(num_bytes);

  if (*bytes == NULL)
    {
      rtn = -7;
      goto close_file;
    }

  if ((MY_READ(*bytes, num_bytes, 1, fp)) != 1)
    {
      free(*bytes);
      *bytes = NULL;
      rtn = -8;
      goto close_file;
    }

  /* if requested format differs from actual format, convert */
  if (*pixformat != PNM_NATIVE &&
      *pixformat != actual_format)
    {
      switch (*pixformat)
	{
	case PNM_RGB: switch (actual_format)
	  {
	  case PNM_GREY:
	    {
	      unsigned char *temp = (unsigned char*)malloc(*width * *height * 3);
	      int i = *width * *height;
	      if (!temp)
		{
		  rtn = -11;
		  free (*bytes);
		  goto close_file;
		}
	      while (i--)
		{
		  temp[i*3] = temp[i*3 + 1] = temp[i*3 + 2] = (*bytes)[i];
		}
	      free (*bytes);
	      *bytes = temp;
	    }
	    break;
	  default:
	    rtn = -10;
	    free (*bytes);
	    goto close_file;
	  }
	break;
	case PNM_GREY: switch (actual_format)
	  {
	  case PNM_RGB:
	    {
	      unsigned char *temp = (unsigned char*)malloc(*width * *height);
	      int i = *width * *height;
	      if (!temp)
		{
		  rtn = -11;
		  goto close_file;
		}
	      while (i--)
		{
		  temp[i] = ((*bytes)[i*3] + ((*bytes)[i*3+1]<<1) +
			     (*bytes)[i*3+2]) >> 2;
		}
	      free (*bytes);
	      *bytes = temp;
	    }
	    break;
	  default:
	    rtn = -10;
	    goto close_file;
	  }
	break;
	default:
	  rtn = -10;
	  goto close_file;	  
	}
    }

  *pixformat = actual_format;

 close_file:  
  MY_CLOSE(fp);

  return rtn;
}
