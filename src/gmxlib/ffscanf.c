/*
 *       @(#) copyrgt.c 1.12 9/30/97
 *
 *       This source code is part of
 *
 *        G   R   O   M   A   C   S
 *
 * GROningen MAchine for Chemical Simulations
 *
 *            VERSION 2.0b
 * 
 * Copyright (c) 1990-1997,
 * BIOSON Research Institute, Dept. of Biophysical Chemistry,
 * University of Groningen, The Netherlands
 *
 * Please refer to:
 * GROMACS: A message-passing parallel molecular dynamics implementation
 * H.J.C. Berendsen, D. van der Spoel and R. van Drunen
 * Comp. Phys. Comm. 91, 43-56 (1995)
 *
 * Also check out our WWW page:
 * http://rugmd0.chem.rug.nl/~gmx
 * or e-mail to:
 * gromacs@chem.rug.nl
 *
 * And Hey:
 * GRowing Old MAkes el Chrono Sweat
 */
#include <stdarg.h>
#include <ctype.h>
#include "typedefs.h"
#include "string2.h"
#include "ffscanf.h"

static int getfld(char **p)
{
  int fld;
  
  fld=0;
  while (isdigit(**p)) fld=(fld*10)+((*((*p)++))-'0');
  return fld;
}

void ffscanf(FILE *in,char *fmt, ...)
{
  va_list ap;
  char *p;
  char buf[STRLEN];
  int i,fld;
  double dval;

  va_start(ap,fmt);
  for (p=fmt; *p; p++) {
    if (*p == '%') {
      p++;
      fld=getfld(&p);
      for(i=0; (i<fld); ) {
	buf[i]=fgetc(in);
	if (buf[i] != '\n') i++;
      }
      buf[fld]='\0';
      switch(*p) {
      case 'd':
	sscanf(buf,"%d",va_arg(ap,int *));
	break;
      case 'f':
	sscanf(buf,"%f",va_arg(ap,float *));
	break;
      case 'F':
	sscanf(buf,"%lf",va_arg(ap,double *));
	break;
      case 'r':
	sscanf(buf,"%lf",&dval);
	*(va_arg(ap,real *)) = dval;
	break;
      default:
	break;
      }
    }
    else {
      fprintf(stderr,"ffscanf format error\n");
      exit(1);
    }
  }
  va_end(ap);
}

