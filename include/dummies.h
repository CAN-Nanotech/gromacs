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
 * GROningen MAchine for Chemical Simulation
 */
#ifndef _dummies_h
#define _dummies_h

#include <stdio.h>
#include "typedefs.h"

extern void construct_dummies(FILE *log,rvec x[],t_nrnb *nrnb,
			      real dt,rvec v[],t_idef *idef);
/* Create positions of dummy atoms based on surrounding atoms.
 */
 
extern void spread_dummy_f(FILE *log,rvec x[],rvec f[],
			   t_nrnb *nrnb,t_idef *idef);
/* Spread the force operating on the dummy atoms on the surrounding atoms.
 */

#endif
