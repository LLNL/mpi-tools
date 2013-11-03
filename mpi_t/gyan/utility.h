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
