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
 * Giant Rising Ordinary Mutants for A Clerical Setup
 */
#include <stdio.h>
#include <math.h>
#include "typedefs.h"
#include "statutil.h"
#include "fatal.h"
#include "pdbio.h"
#include "macros.h"
#include "smalloc.h"
#include "vec.h"
#include "physics.h"
#include "txtdump.h"
#include "symtab.h"
#include "confio.h"

#define ODIST 0.274
#define HDIST 0.1
#define TET   109.47

void unitcell(t_pdbatom pdba[],rvec box)
{
#define cx  0.81649658
#define cy  0.47140452
#define cy2 0.94280904
#define cz  0.33333333
  
  rvec xx[24] = {
    { 0,   0,         0 }, /* O1 */
    { 0,   0,         1 }, /* H relative to Oxygen */
    { cx, cy,       -cz },
    { cx, cy,       -cz }, /* O2 */
    { 0, 0,       -1    }, /* H relative to Oxygen */
    { cx,-cy,       +cz },
    { cx, cy+cy2,     0 }, /* O3 */
    { -cx, cy,    -cz   }, /* H relative to Oxygen */
    { 0,   -cy2,    -cz },
    { 0,  2*cy+cy2, -cz }, /* O4 */
    {-cx,-cy,       +cz }, /* H relative to Oxygen */
    { 0 , cy2,      +cz },
    { 0,   0,         1 }, /* O5 */
    {-cx, cy,       +cz }, /* H relative to Oxygen */
    { 0 , -cy2,     +cz },
    { cx, cy,      1+cz }, /* O6 */
    { -cx, -cy,     -cz }, /* H relative to Oxygen */
    { 0,   cy2,     -cz },
    { cx, cy+cy2,     1 }, /* O7 */
    { 0,  0,       -1   }, /* H relative to Oxygen */
    { cx, cy,       +cz },
    { 0,  2*cy+cy2,1+cz }, /* O8 */
    { 0,   0,         1 }, /* H relative to Oxygen */
    { cx,   -cy,    -cz }
  };
  int  i,i3,j;
  rvec tmp,t2,dip;
  
  clear_rvec(dip);
  for(i=0; (i<8); i++) {
    i3 = 3*i;
    svmul(ODIST,xx[i3],pdba[i3].x);
    svmul(-0.82,pdba[i3].x,t2);
    rvec_inc(dip,t2);
    for(j=1; (j<=2); j++) {
      svmul(HDIST,xx[i3+j],tmp);
      rvec_add(pdba[i3].x,tmp,pdba[i3+j].x);
      svmul(0.41,pdba[i3+j].x,t2);
      rvec_inc(dip,t2);
    }
  }
  printf("Dipole: %10.5f%10.5f%10.5f (e nm)\n",dip[XX],dip[YY],dip[ZZ]);
  
  box[XX] = 2*cx;
  box[YY] = 2*(cy2+cy);
  box[ZZ] = 2*(1+cz);
  for(i=0; (i<DIM); i++)
    box[i] *= ODIST;
}

int main(int argc,char *argv[])
{
  static int nx=1,ny=1,nz=1;
  t_pargs pa[] = {
    { "-nx", FALSE, etINT, &nx, "nx" },
    { "-ny", FALSE, etINT, &ny, "ny" },
    { "-nz", FALSE, etINT, &nz, "nz" },
  };
  t_filenm fnm[] = {
    { efPDB, "-p", "ice", FALSE },
    { efTRJ, "-o", "ice", FALSE }
  };
#define NFILE asize(fnm)

  FILE      *fp;
  int       i,j,k,n,nmax,m;
  t_pdbatom *pdba;
  t_atoms   atoms;
  t_symtab  symtab;
  rvec      box,tmp,*xx,*vv;
  matrix    boxje;
  
  CopyRight(stdout,argv[0]);
  parse_common_args(&argc,argv,0,FALSE,NFILE,fnm,asize(pa),pa,0,NULL,0,NULL);

  nmax = 24*nx*ny*nz;
  snew(vv,nmax);
  snew(pdba,nmax);
  for(n=0; (n<nmax); n++) {
    pdba[n].pdbtp = epdbATOM;
    pdba[n].atomnr= n;
    pdba[n].resnr = n/3;
    switch (n % 3) {
    case 0:
      strcpy(pdba[n].atomnm,"OW");
      break;
    case 1:
      strcpy(pdba[n].atomnm,"HW1");
      break;
    case 2:
      strcpy(pdba[n].atomnm,"HW2");
      break;
    }
    strcpy(pdba[n].resnm,"SOL");
    sprintf(pdba[n].pdbresnr,"%d",n);
    pdba[n].chain = ' ';
  }
  unitcell(pdba,box);
  n=0;
  for(i=0; (i<nx); i++) {
    tmp[XX] = i*box[XX];
    for(j=0; (j<ny); j++) {
      tmp[YY] = j*box[YY];
      for(k=0; (k<nz); k++) {
	tmp[ZZ] = k*box[ZZ];
	for(m=0; (m<24); m++,n++) {
	  rvec_add(pdba[n % 24].x,tmp,pdba[n].x);
	}
      }
    }
  }
  
  printf("box = (%10.5f%10.5f%10.5f)\n",box[XX],box[YY],box[ZZ]);
  /*calc_vecs3(nx,ny,nz,pdba,bReset);*/
  
  fp=ftp2FILE(efPDB,NFILE,fnm,"w");
  print_pdbatoms(fp,nmax,pdba,box);
  fclose(fp);

  clear_mat(boxje);
  boxje[XX][XX] = box[XX]*nx;
  boxje[YY][YY] = box[YY]*ny;
  boxje[ZZ][ZZ] = box[ZZ]*nz;
  atoms.nr=0;
  open_symtab(&symtab);
  pdb2atoms(nmax,pdba,&atoms,&xx,&symtab);  
  write_status(ftp2fn(efTRJ,NFILE,fnm),0,0,0,NULL,boxje,NULL,NULL,
	       nmax,xx,vv,NULL,0,NULL,NULL);
	       
  return 0;
}

