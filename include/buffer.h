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
 * GROup of MAchos and Cynical Suckers
 */

#ifndef	_buffer_h
#define	_buffer_h

#ifdef HAVE_IDENT
#ident	"@(#) buffer.h 1.8 11/23/92"
#endif /* HAVE_IDENT */

#include <stdio.h>

extern int mask(int i);

extern void clear_buff(int data[],int items);

extern void fill_buff(int data[],int items);

extern int check_buff(FILE *fp,char *title,int data[],int items,int verbose);

#endif	/* _buffer_h */
