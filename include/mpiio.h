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
 * Gromacs Runs On Most of All Computer Systems
 */
#ifndef _mpiio_h
#define _mpiio_h

#include <mpi.h>
#include "typedefs.h"

extern void mpiio_tx(int pid,void *buf,int bufsize);
extern void mpiio_tx_wait(int pid);
extern void mpiio_txs(int pid,void *buf,int bufsize);
extern void mpiio_rx(int pid,void *buf,int bufsize);
extern void mpiio_rx_wait(int pid);
extern void mpiio_rxs(int pid,void *buf,int bufsize);
extern int  mpiio_setup(char *argv[],int *nprocs);
/* If this is the master, spawn some kids. Nprocs is set to the
 * number of processors.
 * Return pid */

extern void mpiio_stat(FILE *fp,char *msg);
extern int  mpinodenumber(void);
extern int  mpinodecount(void);
extern int  mpi_idle_send(void);
extern int  mpi_idle_rec(void);
extern void mpi_left_right(int nprocs,int pid,int *left,int *right);
extern void mpiio_tx_rx(int send_pid,void *send_buf,int send_bufsize,
			int rec_pid,void *rec_buf,int rec_bufsize);
extern void mpiio_wait(int left,int right);
extern char *mpi_error(int errorno);
extern void mpiio_sync_ring(int pid,int nprocs,int left,int right);
extern void mpi_reset_idle();

#define gmx_tx       	mpiio_tx
#define gmx_tx_wait  	mpiio_tx_wait
#define gmx_txs      	mpiio_txs
#define gmx_rx       	mpiio_rx
#define gmx_rx_wait  	mpiio_rx_wait
#define gmx_rxs      	mpiio_rxs
#define gmx_stat     	mpiio_stat
#define gmx_wait       	mpiio_wait
#define gmx_cpu_num    	mpinodecount
#define gmx_cpu_id     	mpinodenumber
#define gmx_left_right 	mpi_left_right
#define gmx_idle_send   mpi_idle_send
#define gmx_idle_rec  	mpi_idle_rec
#define gmx_reset_idle  mpi_reset_idle


#endif 
