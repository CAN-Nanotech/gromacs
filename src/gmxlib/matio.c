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
 * GROtesk MACabre and Sinister
 */
#include "sysstuff.h"
#include "futil.h"
#include "string2.h"
#include "macros.h"
#include "smalloc.h"
#include "fatal.h"
#include "matio.h"
#include "statutil.h"

#define round(a) (int)(a+0.5)

char *read_quotestr(char *buf)
{
  int    i,j;
  static char bufje[STRLEN];
  
  for(i=0; (i<(int)strlen(buf)); i++)
    if (buf[i] =='"')
      break;
  for(j=0,i++; (i<(int)strlen(buf)); i++)
    if (buf[i] !='"')
      bufje[j++]=buf[i];
    else
      break;
  bufje[j]='\0';
  
  return bufje;
}

t_matrix *read_matrix(char *fn,int *nmat)
{
  FILE     *in;
  char     buf[2048],sss[2048],*test;
  t_matrix *mat=NULL,*mm;
  double   tt,yy0,uu0;
  int      nt,ny,nm=0,nline=1,i,quotes;
  
  in=ffopen(fn,"r");
  
  if (fgets2(buf,2048-1,in) == NULL)
    fatal_error(0,"Empty file %s",fn);
  do {
    nline++;
    srenew(mat,++nm);
    mm=&(mat[nm-1]);
    mm->axis_x=NULL;
    mm->axis_y=NULL;
    mm->label_x[0]='\0';
    mm->matrix=NULL;
    
    sscanf(buf,"%d",&ny);
    nt=0;
    if (fgets2(buf,2048-1,in) == NULL)
      fatal_error(0,"No y0 in file %s, line %d",fn,nline);
    if (buf[0] == '"') {
      strcpy(mm->label_x,read_quotestr(buf));
      quotes=0;
      i=-1;
      while (quotes <3) {
	i++;
	if (buf[i]=='"') quotes++;
      }      
      strcpy(mm->label_y,read_quotestr(buf+i));
      snew(mm->axis_y,ny);
      for(i=0;i<ny;i++) {
	fscanf(in,"%lf",&uu0);
	mm->axis_y[i] = uu0;
      }
      fgets2(buf,2048-1,in);
      mm->y0=0;
    }
    else {
      sscanf(buf,"%lf",&yy0);
      mm->y0=yy0;
      strcpy(mm->label_y,read_quotestr(buf));
    }
      
    nline++;
    while ((test=fgets2(buf,2048-1,in)) != NULL) {
      if (sscanf(buf,"%lf %s",&tt,sss) != 2)
	break;
      if (ny != strlen(sss)-2) 
	fatal_error(0,"Your input file '%s' sucks!\n"
		    "line %d contains %d residues while line 1 has %d\n",
		    fn,nline+1,strlen(sss)-2,ny);
      
      srenew(mm->axis_x,nt+1);
      mm->axis_x[nt]=tt;
      
      srenew(mm->matrix,nt+1);
      mm->matrix[nt]=strdup(read_quotestr(sss));
      
      nt++;
      nline++;
    }
    mm->ny=ny;
    mm->nx=nt;
  } while (test != NULL);
  fclose(in);
  
  *nmat=nm;
  return mat;
}

static char mapper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()-_=+{}|;:',<.>/?";
#define NMAP asize(mapper)

void do_wmap(FILE *out,int i0,int imax,
	     int nlevels,t_rgb rlo,t_rgb rhi,real lo,real hi)
{
  int  i,nlo;
  real r,g,b;
  
  for(i=0; (i<imax); i++) {
    nlo=nlevels-i;
    r=(nlo*rlo.r+i*rhi.r)/nlevels;
    g=(nlo*rlo.g+i*rhi.g)/nlevels;
    b=(nlo*rlo.b+i*rhi.b)/nlevels;
    fprintf(out,"%c %10.3g %10g  %10g  %10g\n",
	    mapper[i+i0],(nlo*lo+i*hi)/nlevels,r,g,b);
  }
}

char *fgetline(char **line, FILE *in)
{
  static char line0[2048];
  char *fg;

  fg=fgets(line0,2048,in);
  *line=line0;
  trim(*line);

  return fg;
}

void skipstr(char **line)
{
  ltrim(*line);
  while((*line[0] != ' ') && (*line[0] != '\0'))
    (*line)++;
}

char *line2string(char **line)
{
  int i;
  
  if (*line != NULL) {
    while (((*line)[0] != '\"' ) && ( (*line)[0] != '\0' ))
      (*line)++;
		       
    if ((*line)[0] != '\"')
      return NULL;
    (*line)++;
  
      i=0;
    while (( (*line)[i] != '\"' ) && ( (*line)[i] != '\0' ))
      i++;
    
    if ((*line)[i] != '\"')
      *line=NULL;
    else
      (*line)[i] = 0;
  }
    
  return *line;
}

void parsestring(char *line,char *label, char *string)
{
  if (strstr(line,label)) {
    if (strstr(line,label) < strchr(line,'\"')) {
      line2string(&line);
      strcpy(string,line);
    }
  }
}

void read_xpm_entry(FILE *in,t_matrix *mm)
{
  t_mapping *map;
  char *line=NULL,*str,buf[256];
  int i,m,col_len;
  long r,g,b;
  double u;
  char *fg;
  bool bGetOnWithIt;
  
  mm->title[0]=0;
  mm->legend[0]=0;
  mm->label_x[0]=0;
  mm->label_y[0]=0;
  mm->matrix=NULL;
  mm->axis_x=NULL;
  mm->axis_y=NULL;
  mm->bDiscrete=FALSE;

  while (fgetline(&line,in) && (strncmp(line,"static",6) != 0)) {
    parsestring(line,"title",(mm->title));
    parsestring(line,"legend",(mm->legend));
    parsestring(line,"x-label",(mm->label_x));
    parsestring(line,"y-label",(mm->label_y));
    parsestring(line,"type",buf);
  }
  if (buf[0] && (strcasecmp(buf,"Discrete")==0))
    mm->bDiscrete=TRUE;
   
  if (debug)
    printf("%s %s %s %s\n",mm->title,mm->legend,mm->label_x,mm->label_y);

  if  (strncmp(line,"static",6) != 0)
    fatal_error(0,"Invalid XPixMap\n");
  /* Read sizes */
  bGetOnWithIt=FALSE;
  while (!bGetOnWithIt && fgetline(&line,in)) {
    while (( line[0] != '\"' ) && ( line[0] != '\0' ))
      line++;

    if  ( line[0] == '\"' ) {
      line2string(&line);
      sscanf(line,"%d %d %d %d",&(mm->nx),&(mm->ny),&(mm->nmap),&i);
      if (i>1)
	fatal_error(0,"Sorry can only read xpm's with one caracter per pixel\n");
      bGetOnWithIt=TRUE;
    }
  }
  if (debug)
    printf("%d %d %d\n",mm->nx,mm->ny,mm->nmap);
  
  /* Read color map */
  snew(map,mm->nmap);
  m=0;
  while ((m < mm->nmap) && fgetline(&line,in)) {
    line=strchr(line,'\"');

    if  ( line == NULL ) 
      fatal_error(0,"Not enough color map entries");
    else {
      line++;
      /* Read xpm color map entry */
      str=strchr(line+2,'#')+1;
      col_len=0;
      while (isxdigit(str[col_len]))
	col_len++;
      if (col_len==6) {
	sscanf(line,"%c %*c #%2x%2x%2x",&(map[m].code),&r,&g,&b);
	map[m].rgb.r=r/255.0;
	map[m].rgb.g=g/255.0;
	map[m].rgb.b=b/255.0;
      }
      else if (col_len==12) {
	sscanf(line,"%c %*c #%4x%4x%4x",&(map[m].code),&r,&g,&b);
	map[m].rgb.r=r/65535.0;
	map[m].rgb.g=g/65535.0;
	map[m].rgb.b=b/65535.0;
      }
      else
	fatal_error(0,"Unsupported or invalid colormap in X PixMap\n");
      line=strchr(line,'\"');
      line++;
      line2string(&line);
      map[m].desc = strdup(line);
      m++;
    }
  }
  if (debug)
    for(m=0;m<mm->nmap;m++) 
      printf("%c %f %f %f %s\n",map[m].code,map[m].rgb.r,map[m].rgb.g,
	     map[m].rgb.b,map[m].desc);

  /* Read axes, if the are any */ 
  bGetOnWithIt=FALSE;
  do {
    if (strstr(line,"x-axis")) {
      line=strstr(line,"x-axis");
      skipstr(&line);
      if (debug)
	printf("%s\n",line);
      snew(mm->axis_x,mm->nx);
      for(i=0;i<mm->nx;i++) {
	sscanf(line,"%lf",&u);
	mm->axis_x[i] = u;
	skipstr(&line);
      }
    }
    else if (strstr(line,"y-axis")) {
      line=strstr(line,"y-axis");
      skipstr(&line);
      if (debug)
	printf("%s\n",line);
      snew(mm->axis_y,mm->ny);
      for(i=0;i<mm->ny;i++) {
	sscanf(line,"%lf",&u);
	   mm->axis_y[i] = u;
	   skipstr(&line);
      }
    }
  } while ((line[0] != '\"') && fgetline(&line,in));

  if (debug)
    for(i=0;i<mm->nx;i++)
      printf("%f %f\n",mm->axis_x[i],mm->axis_y[i]);

  /* Read matrix */
  snew(mm->matrix,mm->nx);
  for(i=0; i<mm->nx; i++)
    snew(mm->matrix[i],mm->ny);
  m=mm->ny-1;
  do {
    while ((line[0] != '\"') && (line[0] != '\0'))
      line++;
    if (line[0] != '\"')
      fatal_error(0,"Not enough caracters in row %d of the matrix\n",m+1);
    else {
      line++;
      for(i=0; i<mm->nx; i++)
	mm->matrix[i][m] = line[i];
      m--;
    }
  } while ((m>=0) && fgetline(&line,in));
  if (m>=0)
    fatal_error(0,"Not enough rows in the matrix\n");
  
  mm->map = map;
}

int read_xpm_matrix(char *fnm,t_matrix **matrix)
{
  FILE *in;
  char *line;
  int nmat;

  in=ffopen(fnm,"r");
  
  nmat=0;
  while (fgetline(&line,in)) {
    if (strstr(line,"/* XPM */")) {
      srenew(*matrix,nmat+1);
      read_xpm_entry(in,&(*matrix)[nmat]);
      nmat++;
    }
  }
  fclose(in);

  if (nmat==0)
    fatal_error(0,"Invalid XPixMap\n");

  return nmat;
}

void write_xpm_header(FILE *out,
		      char *title,char *legend,char *label_x,char *label_y,
		      bool bDiscrete)
{
  fprintf(out,  "/* XPM */\n");
  fprintf(out,  "/* This matrix is generated by %s. */\n",Program());
  fprintf(out,  "/* title:   \"%s\" */\n",title);
  fprintf(out,  "/* legend:  \"%s\" */\n",legend);
  fprintf(out,  "/* x-label: \"%s\" */\n",label_x);
  fprintf(out,  "/* y-label: \"%s\" */\n",label_y);
  if (bDiscrete) 
    fprintf(out,"/* type:    \"Discrete\" */\n");
  else
    fprintf(out,"/* type:    \"Continuous\" */\n");
}

void write_xpm_map3(FILE *out,int n_x,int n_y,int *nlevels,
		    real lo,real mid,real hi,
		    t_rgb rlo,t_rgb rmid,t_rgb rhi)
{
  int    i,nlo,nmid;
  real   r,g,b,clevels;

  fprintf(stderr,"THIS IS NEW 4");
    
  if (*nlevels > NMAP) {
    fprintf(stderr,"Warning, too many levels (%d) in file %s line %d, using %d only\n",*nlevels,__FILE__,__LINE__,NMAP);
    *nlevels=NMAP;
  }
  else if (*nlevels <= 1) {
    fprintf(stderr,"Warning, too few levels (%d) in file %s line %d, using 2 instead\n",*nlevels,__FILE__,__LINE__);
    *nlevels=2;
  }
  if (!((mid > lo) && (mid < hi)))
    fatal_error(0,"Lo: %f, Mid: %f, Hi: %f\n",lo,mid,hi);

  fprintf(out,"static char * gv_xpm[] = {\n");
  fprintf(out,"\"%d %d   %d %d\",\n",n_x,n_y,*nlevels+1,1);

  nmid = ((mid-lo)/(hi-lo))*(*nlevels);
  if (*nlevels > (2 * nmid))
    clevels = *nlevels-nmid;
  else
    clevels = nmid;
  for(i=0; (i<nmid); i++) {
    nlo=nmid-i;
    r=rmid.r+(nlo*(rlo.r-rmid.r)/clevels);
    g=rmid.g+(nlo*(rlo.g-rmid.g)/clevels);
    b=rmid.b+(nlo*(rlo.b-rmid.b)/clevels);
    fprintf(out,"\"%c  c #%02X%02X%02X \" /* \"%.3g\" */,\n",
	    mapper[i],round(255*r),round(255*g),round(255*b),
	    (nlo*lo+i*mid)/nmid);
     }
  for(i=0; (i<(*nlevels+1-nmid)); i++) {
    nlo=(*nlevels-nmid)-i;
    r=rmid.r+(i*(rhi.r-rmid.r)/clevels);
    g=rmid.g+(i*(rhi.g-rmid.g)/clevels);
    b=rmid.b+(i*(rhi.b-rmid.b)/clevels);
    fprintf(out,"\"%c  c #%02X%02X%02X \" /* \"%.3g\" */,\n",
	    mapper[i+nmid],round(255*r),round(255*g),round(255*b),
	    (nlo*mid+i*hi)/(*nlevels-nmid));
  }
}


void write_xpm_map(FILE *out,int n_x, int n_y,int *nlevels,real lo,real hi,
		   t_rgb rlo,t_rgb rhi)
{
  int    i,nlo;
  real   invlevel,r,g,b;
  
  if (*nlevels > NMAP) {
    fprintf(stderr,"Warning, too many levels (%d) in file %s line %d, using %d only\n",*nlevels,__FILE__,__LINE__,NMAP);
    *nlevels=NMAP;
  }
  else if (*nlevels <= 1) {
    fprintf(stderr,"Warning, too few levels (%d) in file %s line %d, using 2 instead\n",*nlevels,__FILE__,__LINE__);
    *nlevels=2;
  }

  fprintf(out,"static char * gv_xpm[] = {\n");
  fprintf(out,"\"%d %d   %d %d\",\n",n_x,n_y,*nlevels+1,1);

  invlevel=1.0/(*nlevels);
  for(i=0; (i<(*nlevels)+1); i++) {
    nlo=*nlevels-i;
    r=(nlo*rlo.r+i*rhi.r)*invlevel;
    g=(nlo*rlo.g+i*rhi.g)*invlevel;
    b=(nlo*rlo.b+i*rhi.b)*invlevel;
    fprintf(out,"\"%c  c #%02X%02X%02X \" /* \"%.3g\" */,\n",
	    mapper[i],round(255*r),round(255*g),round(255*b),
	    (nlo*lo+i*hi)*invlevel);
  }
}

void write_xpm_axis(FILE *out, char *axis, int n, real *label)
{
  int i;

  if (label) {
    fprintf(out,"/* %s-axis:  ",axis);
    for(i=0;i<n;i++) {
      fprintf(out,"%g ",label[i]);
    }
    fprintf(out,"*/\n");
  }
}

void write_xpm_data(FILE *out, int n_x, int n_y, real **matrix, 
		    real lo, real hi, int nlevels)
{
  int i,j,c;
  real invlevel;

  invlevel=nlevels/(hi-lo);
  for(j=n_y-1; (j>=0); j--) {
    fprintf(out,"\"");
    for(i=0; (i<n_x); i++) {
      c=round((matrix[i][j]-lo)*invlevel);
      if (c<0) c=0;
      if (c>nlevels) c=nlevels;
      fprintf(out,"%c",mapper[c]);
    }
    if (j > 0)
      fprintf(out,"\",\n");
    else
      fprintf(out,"\"\n");
  }
}

void write_xpm_m(FILE *out, t_matrix m)
{
  /* Writes a t_matrix struct to .xpm file */ 
     
  int i,j;
  

  write_xpm_header(out,m.title,m.legend,m.label_x,m.label_y,
		   m.bDiscrete);
  fprintf(out,"static char * gv_xpm[] = {\n");
  fprintf(out,"\"%d %d   %d %d\",\n",m.nx,m.ny,m.nmap,1);
  for(i=0; (i<m.nmap); i++) 
    fprintf(out,"\"%c  c #%02X%02X%02X \" /* \"%s\" */,\n",
	    m.map[i].code,round(m.map[i].rgb.r*255),
	                  round(m.map[i].rgb.g*255),
	                  round(m.map[i].rgb.b*255),m.map[i].desc);
  write_xpm_axis(out,"x",m.nx,m.axis_x);
  write_xpm_axis(out,"y",m.ny,m.axis_y);
  for(j=m.ny-1; (j>=0); j--) {
    fprintf(out,"\"");
    for(i=0; (i<m.nx); i++) 
      fprintf(out,"%c",m.matrix[i][j]);
    if (j > 0)
      fprintf(out,"\",\n");
    else
      fprintf(out,"\"\n");
  }
}

void write_xpm3(FILE *out,
		char *title,char *legend,char *label_x,char *label_y,
		int n_x,int n_y,real axis_x[],real axis_y[],
		real *matrix[],real lo,real mid,real hi,
		t_rgb rlo,t_rgb rmid,t_rgb rhi,int *nlevels)
{
  /* See write_xpm.
   * Writes a colormap varying as rlo -> rmid -> rhi.
   */

  if (hi <= lo) 
    fatal_error(0,"hi (%g) <= lo (%g)",hi,lo);

  write_xpm_header(out,title,legend,label_x,label_y,FALSE);
  write_xpm_map3(out,n_x,n_y,nlevels,lo,mid,hi,rlo,rmid,rhi);
  write_xpm_axis(out,"x",n_x,axis_x);
  write_xpm_axis(out,"y",n_y,axis_y);
  write_xpm_data(out,n_x,n_y,matrix,lo,hi,*nlevels);
}

void write_xpm(FILE *out,
	       char *title,char *legend,char *label_x,char *label_y,
	       int n_x,int n_y,real axis_x[],real axis_y[],
	       real *matrix[],real lo,real hi,
	       t_rgb rlo,t_rgb rhi,int *nlevels)
{
  /* out        xpm file
   * title      matrix title
   * legend     label for the continuous legend
   * label_x    label for the x-axis
   * label_y    label for the y-axis
   * n_x, n_y   size of the matrix
   * axis_x[]   the x-ticklabels
   * axis_y[]   the y-ticklables
   * *matrix[]  element x,y is matrix[x][y]
   * lo         output lower than lo is set to lo
   * hi         output higher than hi is set to hi
   * rlo        rgb value for level lo
   * rhi        rgb value for level hi
   * nlevels    number of levels for the output minus one
   */

  if (hi <= lo) 
    fatal_error(0,"hi (%g) <= lo (%g)",hi,lo);

  write_xpm_header(out,title,legend,label_x,label_y,FALSE);
  write_xpm_map(out,n_x,n_y,nlevels,lo,hi,rlo,rhi);
  write_xpm_axis(out,"x",n_x,axis_x);
  write_xpm_axis(out,"y",n_y,axis_y);
  write_xpm_data(out,n_x,n_y,matrix,lo,hi,*nlevels);
}

