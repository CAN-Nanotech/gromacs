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
#include <math.h>
#include "typedefs.h"
#include "random.h"
#include "mcprop.h"
#include "smalloc.h"
#include "vec.h"
#include "futil.h"

void normalise_vec(int nx,real x[])
{
  real   fac,nnorm;
  int    j;

  /* Normalise vector */
  nnorm=0.0;
  for(j=0; (j<nx); j++)
    nnorm+=x[j]*x[j];
  fac=1.0/sqrt(nnorm);
  for(j=0; (j<nx); j++)
    x[j]*=fac;
}

static real do_step(int nx,real x[],int i,int *ig,real step,bool bPlus)
{
  static real   r=0;
  
  /* Modify a coordinate */
  if (bPlus) 
    r = rando(ig)*step;
  else
    r = -r;
  x[i] += r;

  normalise_vec(nx,x);
  
  return r;
}

real find_min(real x0,real x1,real x2,real y0,real y1,real y2)
{
  matrix X,X_1;
  rvec   y;
  rvec   abc;
  real   a2_1,wortel,xx0,yy0,XXX;
  
  X[0][0]=x0*x0, X[0][1]=x0, X[0][2]=1;
  X[1][0]=x1*x1, X[1][1]=x1, X[1][2]=1;
  X[2][2]=x2*x2, X[2][1]=x2, X[2][2]=1;
  y[0]=y0, y[1]=y1, y[2]=y2;
  
  m_inv(X,X_1);
  mvmul(X_1,y,abc);
  
  if (abc[0] > 0) {
    /* There IS a minimum */
    xx0 = -abc[1]/(2.0*abc[0]);
    if ((x0 < xx0) && (xx0 < x1))
      XXX = xx0;
    else {
      /* The minimum is not on our interval */
      if (xx0 < x0)
	XXX = x0;
      else
	XXX = x2;
    }
  }
  else if (abc[0] < 0) {
    if (y0 < y2)
      XXX = x0;
    else
      XXX = x2;
  }
  else
    XXX = x1;
    
  return XXX;
}

void do_mc(FILE *fp,int nx,real x[],real step,real v0,real tol,
	   int maxsteps,t_propfunc *func)
{
  FILE   *ffp[2];
  FILE   *ftrj;
  int    i,j,k,m,f,n,ig,cur=0;
  bool   bConv,bUp;
  real   vtol,r,bmf,*rx[2],valmin,vplusmin[2],stepsize;
  double dv,val[2];
#define next (1-cur)

  snew(rx[cur], nx);
  snew(rx[next],nx);
  ffp[0]=fp;
  ffp[1]=stderr;

  ftrj=ffopen("ftrj.out","w");
    
  for(j=0; (j<nx); j++) 
    rx[cur][j]=x[j];

  /* Random seed */
  ig       = 1993;
  
  /* Initial value */
  val[cur] = func(nx,x);
  vtol     = tol*v0;
  
  for(f=0; (f<2); f++) {
    fprintf(ffp[f],"Starting MC in property space, YES!\n\n");
    fprintf(ffp[f],"Initial value: %10.3e\n",val[cur]);
    fprintf(ffp[f],"Going to do %d steps in %dD space\n",maxsteps,nx);
  }
  bConv=FALSE;
  valmin=val[cur];
  for(n=0; (n<maxsteps) && !bConv; ) {
    for(i=0; (i<nx)  && !bConv; i++,n++) {
      
      for(m=0; (m<2); m++) {
	for(j=0; (j<nx); j++) 
	  rx[next][j]=rx[cur][j];
	stepsize=do_step(nx,rx[next],i,&ig,step,1-m);
	vplusmin[m]=func(nx,rx[next]);
      }
      for(j=0; (j<nx); j++) 
	rx[next][j]=rx[cur][j];
      rx[next][i]=find_min(rx[cur][i]+stepsize,rx[cur][i],rx[cur][i]-stepsize,
			   vplusmin[0],val[cur],vplusmin[1]);
      normalise_vec(nx,rx[next]);
      val[next]=func(nx,rx[next]);

      bmf=0;
      bUp=FALSE;
      dv=val[next]-val[cur];
      if (dv < 0) {
	cur=next;
	if (val[cur] < valmin)
	  valmin=val[cur];
	for(k=0; (k<nx); k++) {
	  x[k]=rx[cur][k];
	  fprintf(ftrj,"%6.3f  ",x[k]);
	}
	fprintf(ftrj,"\n");
      }
      if ((fabs(dv) < vtol) && (val[cur]<=valmin))
	bConv=TRUE;
      else if ((dv >= 0) && (v0 > 0)) {
	r=rando(&ig);
	bmf=exp(-dv/v0);
	if (bmf < r) {
	  cur=next;
	  bUp=TRUE;
	}
      }
      for(f=0; (f<2); f++)
	fprintf(ffp[f],"Step %5d, Min: %10.3e,  Cur: %10.3e,  BMF %6.3f %s\n",
		n,valmin,val[cur],bmf,bUp ? "+" : "");
    }
  }
  if (bConv) {
    fprintf(fp,"Converged !\n");
    fprintf(stderr,"Converged !\n");
  }
  fclose(ftrj);
}
 
