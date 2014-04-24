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
 * utility.c
 *
 *  Created on: Oct 22, 2013
 *      Author: islam3
 */

#include "utility.h"


#define SCREENLEN 78

#ifndef MPI_COUNT
#define MPI_COUNT MPI_INT
typedef int MPI_Count;
#endif

#ifndef MPI_T_CVAR_HANDLE_NULL
#define MPI_T_CVAR_HANDLE_NULL NULL
#endif

#define CHECKERR(errstr,err) if (err!=MPI_SUCCESS) { printf("ERROR: %s: MPI error code %i\n",errstr,err); }


char* get_pvars_name_list()
{
	int num, i;
	char *name;
	char *class_name;
	int bind,vc,verbos,ro,ct,at;
	MPI_Datatype dt;
	MPI_T_enum et;
	int namelen, desclen;
	int total_length_of_pvar_names;
	char fname[105];
	char fdesc[105];
	int index;
	/* Get number of variables */

	MPI_T_pvar_get_num(&num);

	/* Find string sizes */
	total_length_of_pvar_names = 0;
	for (i = 0; i<num; i++)
	{
		int namelen = 0;
		int desclen = 0;
		MPI_T_pvar_get_info(i,fname,&namelen,&verbos,&vc,&dt,&et,fdesc,&desclen,&bind,&ro,&ct,&at);
		total_length_of_pvar_names += namelen;
	}

	/* Allocate string buffers */
	name = (char*)malloc(sizeof(char)* (total_length_of_pvar_names + num * 8 /*strlen(:CLASS_NAME)*/ + num /*delimiter*/ + 1));
	CHECKERR("Malloc Name",(name==NULL));
	index = 0;
	for (i = 0; i < num; i++)
	{
		namelen = 0;
		desclen = 0;
		MPI_T_pvar_get_info(i,fname,&namelen,&verbos,&vc,&dt,&et,fdesc,&desclen,&bind,&ro,&ct,&at);
		memcpy((name + index), fname, namelen);
		index += namelen;
		memcpy((name + index), ":", 1);
		index += 1;
		class_name = get_pvar_class(vc);
		memcpy((name+index), class_name, strlen(class_name));
		index += strlen(class_name);
		memcpy((name + index), ";", 1);
		index += 1;
	}
	name[index] = 0;
	return name;
}

/* Print a PVAR class */

char* get_pvar_class(int c)
{
	switch(c) {
	case MPI_T_PVAR_CLASS_STATE:
		return ("STATE");
	case MPI_T_PVAR_CLASS_LEVEL:
		return("LEVEL");
	case MPI_T_PVAR_CLASS_SIZE:
		return("SIZE");
	case MPI_T_PVAR_CLASS_PERCENTAGE:
		return("PERCENT");
	case MPI_T_PVAR_CLASS_HIGHWATERMARK:
		return("HIGHWAT");
	case MPI_T_PVAR_CLASS_LOWWATERMARK:
		return("LOWWAT");
	case MPI_T_PVAR_CLASS_COUNTER:
		return("COUNTER");
	case MPI_T_PVAR_CLASS_AGGREGATE:
		return("AGGR");
	case MPI_T_PVAR_CLASS_TIMER:
		return("TIMER");
	case MPI_T_PVAR_CLASS_GENERIC:
		return("GENERIC");
	default:
		return("UNKNOWN");
	}
}

/* Print a PVAR class */

void print_class(int c)
{
	switch(c) {
	case MPI_T_PVAR_CLASS_STATE:
		printf("STATE  ");
		break;
	case MPI_T_PVAR_CLASS_LEVEL:
		printf("LEVEL  ");
		break;
	case MPI_T_PVAR_CLASS_SIZE:
		printf("SIZE   ");
		break;
	case MPI_T_PVAR_CLASS_PERCENTAGE:
		printf("PERCENT");
		break;
	case MPI_T_PVAR_CLASS_HIGHWATERMARK:
		printf("HIGHWAT");
		break;
	case MPI_T_PVAR_CLASS_LOWWATERMARK:
		printf("LOWWAT ");
		break;
	case MPI_T_PVAR_CLASS_COUNTER:
		printf("COUNTER");
		break;
	case MPI_T_PVAR_CLASS_AGGREGATE:
		printf("AGGR   ");
		break;
	case MPI_T_PVAR_CLASS_TIMER:
		printf("TIMER  ");
		break;
	case MPI_T_PVAR_CLASS_GENERIC:
		printf("GENERIC");
		break;
	default:
		printf("UNKNOWN");
		break;
	}
}

/* Print a filled string */
void print_filled(char *s, int len, char c)
{
	int i;
	printf("%s",s);
	for (i=strlen(s); i<len; i++)
		printf("%c",c);
	printf("\n");
}

void print_variable(
		MPI_T_pvar_session sess,
		MPI_T_pvar_handle handle2,
		char *name,
		MPI_Datatype datatype,
		int count,
		unsigned long long int **value_buffer){
	int j;
	void *readbuf = NULL;

	if(datatype == MPI_INT){
		readbuf = (void*)malloc(sizeof(int) * (count + 1));
		MPI_T_pvar_read( sess, handle2, readbuf);
		for(j = 0; j < count; j++)
			printf("(I) %s: %d\n", name, ((int*)readbuf)[j]);
	}
	else if(datatype == MPI_UNSIGNED){
		readbuf = (void*)malloc(sizeof(unsigned int) * (count + 1));
		MPI_T_pvar_read( sess, handle2, readbuf);
		for(j = 0; j < count; j++)
			printf("(UI) %s: %u\n", name, ((unsigned int*)readbuf)[j]);
	}
	else if(datatype == MPI_UNSIGNED_LONG){
		readbuf = (void*)malloc(sizeof(unsigned long) * (count + 1));
		MPI_T_pvar_read( sess, handle2, readbuf);
		for(j = 0; j < count; j++)
			printf("(UL) %s: %lu\n", name, ((unsigned long*)readbuf)[j]);
	}
	else if(datatype == MPI_UNSIGNED_LONG_LONG){
		readbuf = (void*)malloc(sizeof(unsigned long long) * (count + 1));
		MPI_T_pvar_read( sess, handle2, readbuf);
		for(j = 0; j < count; j++)
			printf("(ULL) %s: %llu\n", name, ((unsigned long long*)readbuf)[j]);
	}
	else if(datatype == MPI_CHAR){
		readbuf = (void*)malloc(sizeof(char) * (count + 1));
		MPI_T_pvar_read( sess, handle2, readbuf);
		for(j = 0; j < count; j++)
			printf("(C) %s: %c\n", name, ((char*)readbuf)[j]);
	}
	else if(datatype == MPI_DOUBLE){
		readbuf = (void*)malloc(sizeof(double) * (count + 1));
		MPI_T_pvar_read( sess, handle2, readbuf);
		for(j = 0; j < count; j++)
			printf("(D) %s: %lf\n", name, ((double*)readbuf)[j]);
	}
	else if(datatype == MPI_COUNT){
		readbuf = (void*)malloc(sizeof(MPI_Count) * (count + 1));
		MPI_T_pvar_read( sess, handle2, readbuf);
		for(j = 0; j < count; j++)
			printf("(MPI_Count) %s: %llu\n", name, ((MPI_Count*)readbuf)[j]);
	}
	else{
		printf("<ERROR> UNKNOWN DATATYPE: %s\n", name);
	}
	if(readbuf != NULL)
		free(readbuf);
}


