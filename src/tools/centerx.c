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
 * Green Red Orange Magenta Azure Cyan Skyblue
 */
#include "typedefs.h"
#include "confio.h"
#include "string2.h"
#include "vec.h"
#include "txtdump.h"
#include "statutil.h"
#include "copyrite.h"
#include "smalloc.h"
#include "vec.h"
#include "macros.h"

int main(int argc,char *argv[])
{
  static char *desc[] = {
    "centerx reads a .gro file, centers the coordinates in the middle",
    "of the box, irrespective of PBC."
  };
  t_manual man = { asize(desc),desc,0,NULL,NULL,0,NULL};
  
  FILE    *out;
  char    title[STRLEN];
  int     natoms,i;
  rvec    *x,*v;
  matrix  box;
  rvec    xcm,dx;
  t_atoms atoms;
  t_filenm fnm[] = {
    { efGRO, "-f", NULL,      ffREAD },
    { efGRO, "-o", "confout", ffWRITE }
  };
#define NFILE asize(fnm)

  CopyRight(stderr,argv[0]);
  parse_common_args(&argc,argv,0,NFILE,fnm,FALSE,&man);

  atoms.nr=0;
  atoms.nres=0;
  get_coordnum(fnm[0].fn,&natoms);
  snew(atoms.atomname,natoms);
  snew(atoms.resname,natoms);
  snew(atoms.atom,natoms);
  snew(x,natoms);
  snew(v,natoms);
  read_whole_conf(fnm[0].fn,title,&atoms,x,v,box);

  clear_rvec(xcm);
  for(i=0; (i<natoms); i++)  
    rvec_inc(xcm,x[i]);
  for(i=0; (i<DIM); i++) {
    xcm[i]=xcm[i]/natoms;
    dx[i]=box[i][i]*0.5-xcm[i];
  }
  pr_rvec(stdout,0,"xcm",xcm,DIM);
  for(i=0; (i<natoms); i++)  
    rvec_inc(x[i],dx);
    
  write_conf(opt2fn("-o",NFILE,fnm),title,&atoms,x,v,box);
  
  thanx(stdout);
  
  return 0;
}
