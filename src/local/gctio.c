/*
 *       @(#) gctio.c 1.8 2/16/97
 *
 *       This source code is part of
 *
 *        G   R   O   M   A   C   S
 *
 * GROningen MAchine for Chemical Simulations
 *
 *            VERSION 1.51
 * 
 * Copyright (c) 1990-1996,
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
 * GROwing Monsters And Cloning Shrimps
 */
#include "typedefs.h"
#include "do_gct.h"
#include "block_tx.h"
#include "futil.h"
#include "xvgr.h"
#include "macros.h"
#include "physics.h"
#include "network.h"
#include "smalloc.h"
#include "string2.h"
#include "readinp.h"
#include "filenm.h"

char *eoNames[eoNR] = { "Pres", "Epot" };

static int Name2eo(char *s)
{
  int i,res;
  
  res=-1;
  
  for(i=0; (i<eoNR); i++)
    if (strcasecmp(s,eoNames[i]) == 0) {
      res=i;
      fprintf(stderr,"Coupling to observable %d (%s)\n",res,eoNames[res]);
      break;
    }
  
  return res;
}

static char *NoYes[] = { "No", "Yes" };
	
static void send_tcr(int dest,t_coupl_rec *tcr)
{
  blocktx(dest,tcr->pres0);
  blocktx(dest,tcr->epot0);
  blocktx(dest,tcr->nLJ);
  nblocktx(dest,tcr->nLJ,tcr->tcLJ);
  blocktx(dest,tcr->nBU);
  nblocktx(dest,tcr->nBU,tcr->tcBU);
  blocktx(dest,tcr->nQ);
  nblocktx(dest,tcr->nQ,tcr->tcQ);
}

static void rec_tcr(int src,t_coupl_rec *tcr)
{
  blockrx(src,tcr->pres0);
  blockrx(src,tcr->epot0);
  
  blockrx(src,tcr->nLJ);
  snew(tcr->tcLJ,tcr->nLJ);
  nblockrx(src,tcr->nLJ,tcr->tcLJ);
  
  blockrx(src,tcr->nBU);
  snew(tcr->tcBU,tcr->nBU);
  nblockrx(src,tcr->nBU,tcr->tcBU);
  
  blockrx(src,tcr->nQ);
  snew(tcr->tcQ,tcr->nQ);
  nblockrx(src,tcr->nQ,tcr->tcQ);
}

void comm_tcr(FILE *log,t_commrec *cr,t_coupl_rec **tcr)
{
  t_coupl_rec shit;

  if (cr->pid == 0) {
    send_tcr(cr->left,*tcr);
    rec_tcr(cr->right,&shit);
  }
  else {
    snew(*tcr,1);
    rec_tcr(cr->right,*tcr);
    send_tcr(cr->left,*tcr);
  }
} 

static void clear_lj(t_coupl_LJ *tc)
{
  tc->at_i=tc->at_j=0;
  tc->eObs=-1;
  tc->bPrint=TRUE;
  tc->c6=tc->c12=0.0;
  tc->xi_6=tc->xi_12=0.0;
}

static void clear_bu(t_coupl_BU *tc)
{
  tc->at_i=tc->at_j=0;
  tc->eObs=-1;
  tc->bPrint=TRUE;
  tc->a=tc->b=tc->c=0.0;
  tc->xi_a=tc->xi_b=tc->xi_c=0.0;
}

static void clear_q(t_coupl_Q *tc)
{
  tc->at_i=0;
  tc->eObs=-1;
  tc->bPrint=TRUE;
  tc->Q=0.0;
  tc->xi_Q=0.0;
}

void copy_ff(t_coupl_rec *tcr,t_forcerec *fr,t_mdatoms *md,
	     t_idef *idef)
{
  int        i,j,ati,atj,type;
  t_coupl_LJ *tclj;
  t_coupl_BU *tcbu;
  t_coupl_Q  *tcq;
  
  /* Save values for printing */
  for(i=0; (i<tcr->nLJ); i++) {
    tclj=&(tcr->tcLJ[i]);
    
    ati=tclj->at_i;
    atj=tclj->at_j;
    if (atj == -1)
      atj=ati;
    tclj->c6  = C6(fr->nbfp,ati,atj);
    tclj->c12 = C12(fr->nbfp,ati,atj);
  }
  
  for(i=0; (i<tcr->nBU); i++) {
    tcbu=&(tcr->tcBU[i]);
    
    ati=tcbu->at_i;
    atj=tcbu->at_j;
    if (atj == -1)
      atj=ati;
    tcbu->a = (fr->nbfp)[(ati)][3*(atj)];
    tcbu->b = (fr->nbfp)[(ati)][3*(atj)+1];
    tcbu->c = (fr->nbfp)[(ati)][3*(atj)+2];
  }
  
  for(i=0; (i<tcr->nQ); i++) {
    tcq=&(tcr->tcQ[i]);
    for(j=0; (j<md->nr); j++) {
      if (md->typeA[j] == tcq->at_i) {
	tcr->tcQ[i].Q=md->chargeA[j];
	break;
      }
    }
  }
  for(i=0; (i<tcr->nIP); i++) {
    /* Let's just copy the whole struct !*/
    type=tcr->tIP[i].type;
    tcr->tIP[i].iprint=idef->iparams[type];
  }
}

void write_gct(char *fn,t_coupl_rec *tcr,t_idef *idef)
{
  FILE *fp;
  int  i,ftype;
  
  fp=ffopen(fn,"w");
  nice_header(fp,fn);
  fprintf(fp,"%-12s = %12g  ; Reference pressure for coupling\n",
	  eoNames[eoPres],tcr->pres0);
  fprintf(fp,"%-12s = %12g  ; Reference potential energy\n",
	  eoNames[eoEpot],tcr->epot0);
  fprintf(fp,"\n; Q-Coupling   %6s  %12s\n","type","xi");
  for(i=0; (i<tcr->nQ); i++) {
    fprintf(fp,"%-8s = %8s  %6d  %12g\n",
	    "Q",eoNames[tcr->tcQ[i].eObs],tcr->tcQ[i].at_i,tcr->tcQ[i].xi_Q);
  }
  
  fprintf(fp,"\n; %8s %8s  %6s  %6s  %12s  %12s\n","Couple","To",
	  "i-type","j-type","xi-c6","xi-c12");
  fprintf(fp,"; j-type == -1 means mixing rules will be applied!\n");
  for(i=0; (i<tcr->nLJ); i++) {
    fprintf(fp,"%-8s = %8s  %6d  %6d  %12g  %12g\n",
	    "LJ",eoNames[tcr->tcLJ[i].eObs],
	    tcr->tcLJ[i].at_i,tcr->tcLJ[i].at_j,
	    tcr->tcLJ[i].xi_6,tcr->tcLJ[i].xi_12);
  }
  
  fprintf(fp,"\n; %8s %8s  %6s  %6s  %12s  %12s  %12s\n","Couple","To",
	  "i-type","j-type","xi-A","xi-B","xi-C");
  fprintf(fp,"; j-type == -1 means mixing rules will be applied!\n");
  for(i=0; (i<tcr->nBU); i++) {
    fprintf(fp,"%-8s = %8s  %6d  %6d  %12g  %12g  %12g\n",
	    "BU",eoNames[tcr->tcBU[i].eObs],
	    tcr->tcBU[i].at_i,tcr->tcBU[i].at_j,
	    tcr->tcBU[i].xi_a,tcr->tcBU[i].xi_b,tcr->tcBU[i].xi_c);
  }
  
  fprintf(fp,"\n; More Coupling\n");
  for(i=0; (i<tcr->nIP); i++) {
    ftype=idef->functype[tcr->tIP[i].type];
    switch (ftype) {
    case F_BONDS:
      fprintf(fp,"%-12s = %-8s  %4d  %12g  %12g\n",
	      "Bonds",eoNames[tcr->tIP[i].eObs],tcr->tIP[i].type,
	      tcr->tIP[i].xi.harmonic.krA,
	      tcr->tIP[i].xi.harmonic.rA);
      break;
    default:
      fprintf(stderr,"ftype %s not supported (yet)\n",
	      interaction_function[ftype].longname);
    }
  }
  fclose(fp);
}

static bool add_lj(int *nLJ,t_coupl_LJ **tcLJ,char *s)
{
  int       j,ati,atj,eo;
  char      buf[256];
  double    xi6,xi12;
  
  if (sscanf(s,"%s%d%d%lf%lf",buf,&ati,&atj,&xi6,&xi12) != 5) 
    return TRUE;
  if ((eo=Name2eo(buf)) == -1)
    fatal_error(0,"Invalid observable for LJ coupling: %s",buf);
  
  for(j=0; (j<*nLJ); j++) {
    if ((((*tcLJ)[j].at_i == ati) && ((*tcLJ)[j].at_j == atj)) &&
	((*tcLJ)[j].xi_6 || (*tcLJ)[j].xi_12) &&
	((*tcLJ)[j].eObs == eo))
      break;
  }
  if (j == *nLJ) {
    ++(*nLJ);
    srenew((*tcLJ),*nLJ);
  }
  else
    fprintf(stderr,"\n*** WARNING: overwriting entry for LJ coupling '%s'\n",s);
  
  clear_lj(&((*tcLJ)[j]));
  if (((*tcLJ)[j].eObs = eo) == -1) {
    fatal_error(0,"Invalid observable for LJ coupling: %s",buf);
  }
  (*tcLJ)[j].at_i   = ati;
    (*tcLJ)[j].at_j   = atj;
    (*tcLJ)[j].xi_6   = xi6;
    (*tcLJ)[j].xi_12  = xi12;
  
    return FALSE;
}

static bool add_bu(int *nBU,t_coupl_BU **tcBU,char *s)
{
  int       j,ati,atj,eo;
  char      buf[256];
  double    xia,xib,xic;
  
  if (sscanf(s,"%s%d%d%lf%lf%lf",buf,&ati,&atj,&xia,&xib,&xic) != 6) 
    return TRUE;
  if ((eo=Name2eo(buf)) == -1)
    fatal_error(0,"Invalid observable for BU coupling: %s",buf);
  
  for(j=0; (j<*nBU); j++) {
    if ((((*tcBU)[j].at_i == ati) && ((*tcBU)[j].at_j == atj)) &&
	((*tcBU)[j].xi_a || (*tcBU)[j].xi_b || (*tcBU)[j].xi_c ) &&
	((*tcBU)[j].eObs == eo))
      break;
  }
  if (j == *nBU) {
    ++(*nBU);
    srenew((*tcBU),*nBU);
  }
  else
    fprintf(stderr,"\n*** WARNING: overwriting entry for BU coupling '%s'\n",s);
  
  clear_bu(&((*tcBU)[j]));
  if (((*tcBU)[j].eObs = eo) == -1) {
    fatal_error(0,"Invalid observable for BU coupling: %s",buf);
  }
  (*tcBU)[j].at_i   = ati;
    (*tcBU)[j].at_j   = atj;
    (*tcBU)[j].xi_a   = xia;
    (*tcBU)[j].xi_b   = xib;
    (*tcBU)[j].xi_c   = xic;
  
    return FALSE;
}

static bool add_ip(int *nIP,t_coupl_iparams **tIP,char *s,int ftype)
{
  int    i,eo,type;
  char   buf[256];
  double kb,b0;
  
  switch (ftype) {
  case F_BONDS:
    /* Pick out the type */
    if (sscanf(s,"%s%d",buf,&type) != 2)
      return TRUE;
    if ((eo=Name2eo(buf)) == -1)
      fatal_error(0,"Invalid observable for IP coupling: %s",buf);
      
    /* Check whether this entry is there already */
    for(i=0; (i<*nIP); i++) {
      if ((*tIP)[i].type == type)
	break;
    }
    if (i < *nIP) {
      fprintf(stderr,"*** WARNING: overwriting entry for type %d\n",type);
    }
    else {
      i=*nIP;
      srenew((*tIP),i+1);
      (*nIP)++;
    }
    if (sscanf(s,"%s%d%lf%lf",buf,&type,&kb,&b0) != 4)
      return TRUE;
    (*tIP)[i].type=type;
    (*tIP)[i].eObs=eo;
    (*tIP)[i].xi.harmonic.krA = kb;
    (*tIP)[i].xi.harmonic.rA  = b0;
    break;
  default:
    fprintf(stderr,"ftype %s not supported (yet)\n",
	    interaction_function[ftype].longname);
    return TRUE;
  }
  return FALSE;
}

static bool add_q(int *nQ,t_coupl_Q **tcQ,char *s)
{
  int       j,ati;
  char      buf[256];
  double    xiQ;
  
  if (sscanf(s,"%s%d%lf",buf,&ati,&xiQ) != 3) 
    return TRUE;
  
  for(j=0; (j<*nQ); j++) {
    if ((*tcQ)[j].at_i == ati)
      break;
  }
  if (j == *nQ) {
    ++(*nQ);
    srenew((*tcQ),*nQ);
  }
  else
    fprintf(stderr,"\n*** WARNING: overwriting entry for Q coupling '%s'\n",s);
  
  clear_q(&((*tcQ)[j]));
  (*tcQ)[j].eObs = Name2eo(buf);
  if ((*tcQ)[j].eObs == -1) {
    fatal_error(0,"Invalid observable for Q coupling: %s",buf);
  }
  (*tcQ)[j].at_i   = ati;
  (*tcQ)[j].xi_Q  = xiQ;
  
  return FALSE;
}

void read_gct(char *fn,t_coupl_rec *tcr)
{
  t_inpfile *inp;
  int       i,j,ninp,nQ,nLJ,nBU,nIP;
  bool      bWrong;
  
  inp=read_inpfile(fn,&ninp);
  RTYPE (eoNames[eoPres],	tcr->pres0,	1.0);
  RTYPE (eoNames[eoEpot],	tcr->epot0,	0.0);
  
  tcr->tcLJ=NULL;
  tcr->tcBU=NULL;
  tcr->tcQ=NULL;
  tcr->tIP=NULL;
  nQ=nLJ=nBU=nIP=0;
  
  for(i=0; (i<ninp); i++) {
    bWrong=FALSE;
    if (strcasecmp(inp[i].name,"LJ") == 0) 
      bWrong=add_lj(&nLJ,&(tcr->tcLJ),inp[i].value);
    else if (strcasecmp(inp[i].name,"BU") == 0) 
      bWrong=add_bu(&nBU,&(tcr->tcBU),inp[i].value);
    else if (strcasecmp(inp[i].name,"Q") == 0) 
      bWrong=add_q(&nQ,&(tcr->tcQ),inp[i].value);
    else if (strcasecmp(inp[i].name,"Bonds") == 0)
      bWrong=add_ip(&nIP,&(tcr->tIP),inp[i].value,F_BONDS);
      
    if (bWrong)
      fprintf(stderr,"Wrong line in %s: '%s = %s'\n",
	      fn,inp[i].name,inp[i].value);
    /*sfree(inp[i].name);
    sfree(inp[i].value);*/
  }
  /* Check which ones have to be printed */
  for(i=1; (i<nQ); i++)
    for(j=0; (j<i); j++) {
      if (tcr->tcQ[i].at_i == tcr->tcQ[j].at_i)
	tcr->tcQ[j].bPrint=FALSE;
    }
  for(i=1; (i<nLJ); i++)
    for(j=0; (j<i); j++) {
      if (((tcr->tcLJ[i].at_i == tcr->tcLJ[j].at_i) &&
	   (tcr->tcLJ[i].at_j == tcr->tcLJ[j].at_j)) ||
	  ((tcr->tcLJ[i].at_i == tcr->tcLJ[j].at_j) &&
	   (tcr->tcLJ[i].at_j == tcr->tcLJ[j].at_i))) 
	tcr->tcLJ[j].bPrint=FALSE;
    }
  
  for(i=1; (i<nBU); i++)
    for(j=0; (j<i); j++) {
      if (((tcr->tcBU[i].at_i == tcr->tcBU[j].at_i) &&
	   (tcr->tcBU[i].at_j == tcr->tcBU[j].at_j)) ||
	  ((tcr->tcBU[i].at_i == tcr->tcBU[j].at_j) &&
	   (tcr->tcBU[i].at_j == tcr->tcBU[j].at_i))) 
	tcr->tcBU[j].bPrint=FALSE;
    }
  
  tcr->nQ  = nQ;
  tcr->nLJ = nLJ;
  tcr->nBU = nBU;
  tcr->nIP = nIP;
  
  sfree(inp);
}

