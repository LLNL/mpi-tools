//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, Lawrence Livermore National Security,
//  LLC. Produced at the Lawrence Livermore National Laboratory. Written
//  by Tanzima Z. Islam (islam3@llnl.gov).
// CODE-LLNL-CODE-647221. All rights reserved.
// This file is part of mpi_T-tools. For details, see
//  https://computation-rnd.llnl.gov/mpi_t/gyan.php.
// Please also read this file - FULL-LICENSE.txt.
// This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License (as published by
//  the Free Software Foundation) version 2.1 dated February 1999.
// This program is distributed in the hope that it will be useful, but
//  WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms
//  and conditions of the GNU General Public License for more
//  details. You should have received a copy of the GNU Lesser General
//  Public License along with this program; if not, write to the Free
//  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
//  02111-1307 USA
//////////////////////////////////////////////////////////////////////////////
/*
 * utility.h
 *
 *  Created on: Oct 22, 2013
 *      Author: islam3
 */
#include <mpi.h> /* Adds MPI_T definiKons as well */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef UTILITY_H_
#define UTILITY_H_

char* get_pvars_name_list();
char* get_pvar_class(int c);
void print_filled(char *s, int len, char c);
void print_variable(
		MPI_T_pvar_session sess,
		MPI_T_pvar_handle handle2,
		char *name,
		MPI_Datatype datatype,
		int count,
		unsigned long long int **value_buffer);
void print_class(int c);
#endif /* UTILITY_H_ */
