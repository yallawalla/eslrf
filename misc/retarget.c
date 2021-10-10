
/*----------------------------------------------------------------------------
 * Name:    Retarget.c
 * Purpose: 'Retarget' layer for target-dependent low level functions
 * Note(s):
 *----------------------------------------------------------------------------
 * This file is part of the uVision/ARM development tools.
 * This software may only be used under the terms of a valid, current,
 * end user licence from KEIL for a compatible version of KEIL software
 * development tools. Nothing else gives you the right to use this software.
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 * Copyright (c) 2012 Keil - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "io.h"
#include "proc.h"
#include "ascii.h"
//_________________________________________________________________________________
FILE 		__stdout;
FILE 		__stdin;
FILE 		__stderr;
//__________________________________________________________________________________
int 		fputc(int c, FILE *f) {
				if(f==stdout) {
					if(f->io && f->io->put) {
						while(f->io->put(f->io->tx,c) == EOF) {
							_wait(2);
						}
					}
					return c;
				}
				return EOF;
}
//__________________________________________________________________________________
int 		_fgetc(FILE *f) {
int			c=EOF;
				if(f==stdin) {
					if(f->io && f->io->get) {
						c=f->io->get(f->io->rx);
					}
					return c;
				}
				return EOF;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
int		fgetc(FILE *f) {
int		i=_fgetc(f);
			if(f->io->esc == NULL)
				f->io->esc=calloc(1,sizeof(esc));
			if(i==__Esc) {
				f->io->esc->seq=i;
				f->io->esc->timeout=HAL_GetTick()+10;
			} else if(i==EOF) {
				if(f->io->esc->timeout && (HAL_GetTick() > f->io->esc->timeout)) {
					f->io->esc->timeout=0;
					return f->io->esc->seq;
				}
			} else if(f->io->esc->timeout) {
				f->io->esc->seq=((f->io->esc->seq) << 8) | i;
			} else {
				return i;
			}
			return EOF;
}
