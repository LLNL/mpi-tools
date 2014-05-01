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
 * gyan.c

 *
 *  Created on: Jul 11, 2013
 *      Author: Tanzima Z. Islam, (islam3@llnl.gov)
 */

#include "utility.h"

#define THRESHOLD 0
#define NOT_FOUND -1
#define FALSE 0
#define TRUE 1
#define STR_SZ 100
#define DEBUG 0
#define NUM_PERF_VAR_SUPPORTED 50
#define NEG_INF -10000000
#define POS_INF 10000000
/* Global variables for tool */
static MPI_T_pvar_session session;
static MPI_T_pvar_handle *pvar_handles;
static int *pvar_index;
static int *pvar_count;
static unsigned long long int **pvar_value_buffer; // cumulative values
static void *read_value_buffer; // values are read into this buffer.
static int pvar_num_watched;
static int total_num_of_var;
static int max_num_of_state_per_pvar = -1; //  max_num_of_state_per_pvar = max(pvar_count[performance_variable]) for all performance_variables
static int num_mpi_tasks;

typedef struct{
	int pvar_index;
	char *name;
	int name_len;
	int verbosity;
	int var_class;
	MPI_Datatype datatype;
	MPI_T_enum enumtype;
	char *desc;
	int desc_len;
	int binding;
	int readonly;
	int continuous;
	int atomic;
}PERF_VAR;
static PERF_VAR *perf_var_all;

typedef struct{
	int max_rank, min_rank; // max_rank : rank that resulted in this max value, min_rank: rank that resulted in this min value
	unsigned long long int max, min; // the actual max and min values
	unsigned long long int total; // summation of values across all MPI ranks for this particular variable
	//	unsigned long long int freq; // how many ranks had this value?
}STATISTICS;
static STATISTICS **pvar_stat;

typedef struct {
	double value;
	int rank;
}mpi_data;

static char *env_var_name;
static int tool_enabled = FALSE;
static int num_send, num_isend, num_recv, num_irecv;
static int rank = 0;

static void stop_watching(){
	int i;
	for(i = 0; i < pvar_num_watched; i++){
		MPI_T_pvar_stop(session, pvar_handles[i]);
		MPI_T_pvar_handle_free(session, &pvar_handles[i]);
	}
	MPI_T_pvar_session_free(&session);
}

static int get_watched_var_index_with_class( char *var_name, char *var_class_name ){
	int i;
	int var_class;
	if( (strcmp(var_class_name, "level") == 0) || (strcmp(var_class_name, "LEVEL") == 0))
		var_class = MPI_T_PVAR_CLASS_LEVEL;
	else if( (strcmp(var_class_name, "highwat") == 0) || (strcmp(var_class_name, "HIGHWAT") == 0))
		var_class = MPI_T_PVAR_CLASS_HIGHWATERMARK;
	else if( (strcmp(var_class_name, "lowwat") == 0) || (strcmp(var_class_name, "LOWWAT") == 0))
		var_class = MPI_T_PVAR_CLASS_LOWWATERMARK;
	else if( (strcmp(var_class_name, "counter") == 0) || (strcmp(var_class_name, "COUNTER") == 0))
		var_class = MPI_T_PVAR_CLASS_COUNTER;
	else if( (strcmp(var_class_name, "state") == 0) || (strcmp(var_class_name, "STATE") == 0))
		var_class = MPI_T_PVAR_CLASS_STATE;
	else if( (strcmp(var_class_name, "size") == 0) || (strcmp(var_class_name, "SIZE") == 0))
		var_class = MPI_T_PVAR_CLASS_SIZE;
	else if( (strcmp(var_class_name, "percent") == 0) || (strcmp(var_class_name, "PERCENT") == 0))
		var_class = MPI_T_PVAR_CLASS_PERCENTAGE;
	else if( (strcmp(var_class_name, "aggr") == 0) || (strcmp(var_class_name, "AGGR") == 0))
		var_class = MPI_T_PVAR_CLASS_AGGREGATE;
	else if( (strcmp(var_class_name, "timer") == 0) || (strcmp(var_class_name, "TIMER") == 0))
		var_class = MPI_T_PVAR_CLASS_TIMER;
	else if( (strcmp(var_class_name, "generic") == 0) || (strcmp(var_class_name, "GENERIC") == 0))
		var_class = MPI_T_PVAR_CLASS_GENERIC;

	for(i = 0; i < total_num_of_var; i++){
		if( (var_class == perf_var_all[i].var_class) &&
				( strcmp(var_name, perf_var_all[i].name) == 0 ))
			return i;
	}
	return NOT_FOUND;
}

static int get_watched_var_index( char *var_name){
	int i;
	char *pch = strchr(var_name, ':');
	if(pch != NULL){
		// If the class is specified
		(*pch) = 0;
		return get_watched_var_index_with_class(var_name, (pch+1));
	}
	// no class was specified
	for(i = 0; i < total_num_of_var; i++){
		if( ( strcmp(var_name, perf_var_all[i].name) == 0 ) )
			return i;
	}
	return NOT_FOUND;
}

/**
 * This function prints statistics of all performance variables monitored
 * @param none
 * @return none
 */

static void print_pvar_buffer_all(){
	int i;
	int j;
	int index;

	printf("%-40s\tType   ", "Variable Name");
	printf(" Minimum(Max_Rank)    Maximum(Min_Rank)       Average\n");
	print_filled("",88,'-');
	for(i = 0; i < pvar_num_watched; i++){
		index = pvar_index[i];
		if(perf_var_all[index].var_class != MPI_T_PVAR_CLASS_TIMER){
			for(j = 0; j < pvar_count[i]; j++){ // asuuming that pvar_count[i] on all processes was the same
				printf("%-40s\t", perf_var_all[index].name);
				print_class(perf_var_all[index].var_class);
				printf("\t%8llu(%3d)  %8llu(%3d)  %12.2lf\n", pvar_stat[i][j].min, pvar_stat[i][j].min_rank,
						pvar_stat[i][j].max, pvar_stat[i][j].max_rank,
						(pvar_stat[i][j].total / (float)num_mpi_tasks));
			}
		}
	}
}

static void pvar_read_all(){
	int i, j;
	int size;
	unsigned long long int zero = 0;
	for(i = 0; i < pvar_num_watched; i++){
		MPI_T_pvar_read(session, pvar_handles[i], read_value_buffer);
		MPI_Type_size(perf_var_all[i].datatype, &size);
		for(j = 0; j < pvar_count[j]; j++){
			pvar_value_buffer[i][j] = 0;
			//memcpy(&(pvar_value_buffer[i][j]), &zero, size);
			memcpy(&(pvar_value_buffer[i][j]), read_value_buffer, size);
		}
	}
	return;
}


static void clean_up_perf_var_all(int num_of_perf_var){
	int i;
	for(i = 0; i < num_of_perf_var; i++){
		free(perf_var_all[i].name);
		free(perf_var_all[i].desc);
	}
	free(perf_var_all);
}

static void clean_up_pvar_value_buffer(int num_of_var_watched){
	int i;
	for(i = 0; i < num_of_var_watched; i++){
		free(pvar_value_buffer[i]);
	}
	free(pvar_value_buffer);
}

static void clean_up_the_rest(){
	free(read_value_buffer);
	free(pvar_handles);
	free(pvar_index);
	free(pvar_count);
}

static void collect_sum_from_all_ranks(MPI_Op op){
	int i;
	int j;
	int root = 0;

	unsigned long long int *value_in;
	unsigned long long int *value_out;
	value_in = (unsigned long long int*)malloc(sizeof(unsigned long long int) * ( max_num_of_state_per_pvar + 1));
	if(rank == root)
		value_out = (unsigned long long int*)malloc(sizeof(unsigned long long int) * ( max_num_of_state_per_pvar + 1));

	for(i = 0; i < pvar_num_watched; i++){
		for(j = 0; j < pvar_count[i]; j++){
			value_in[j] = pvar_value_buffer[i][j];
		}
		PMPI_Reduce(value_in, value_out, pvar_count[i] /*number_of_elements*/, MPI_UNSIGNED_LONG_LONG, op, root, MPI_COMM_WORLD);
		if(root == rank){
			/**
			 * Populate these values into the pvar_stat array of rank 0.
			 */
			for(j = 0; j < pvar_count[i]; j++){
				pvar_stat[i][j].total = value_out[j];
			}
		}// root is done extracting information from "out" to pvar_stat
	}
	free(value_in);
	if(root == rank)
		free(value_out);
}

static void collect_range_with_loc_from_all_ranks(MPI_Op op){
	int i;
	int j;
	int root = 0;
	mpi_data *in;
	mpi_data *out;

	/**
	 * Pack all variables into a buffer
	 */
	/**
	 * MPI_Reduce on each element. Each element here is a performance variable
	 */
	in = (mpi_data*)malloc(sizeof(mpi_data) * ( max_num_of_state_per_pvar + 1));
	if(rank == root)
		out = (mpi_data*)malloc(sizeof(mpi_data) * (max_num_of_state_per_pvar + 1));
	for(i = 0; i < pvar_num_watched; i++){
		for(j = 0; j < pvar_count[i]; j++){
			in[j].value = 0;
			in[j].value = (double)(pvar_value_buffer[i][j]);
			in[j].rank = rank;
		}
		PMPI_Reduce(in, out, pvar_count[i] /*number_of_elements*/, MPI_DOUBLE_INT, op, root, MPI_COMM_WORLD);
		//printout
		if(root == rank){
			for(j = 0; j < pvar_count[i]; j++){
				if(op == MPI_MINLOC){
					pvar_stat[i][j].min = (unsigned long long int)(out[j].value);
					pvar_stat[i][j].min_rank = out[j].rank;
				}
				else if(op == MPI_MAXLOC){
					pvar_stat[i][j].max = (unsigned long long int)(out[j].value);
					pvar_stat[i][j].max_rank = out[j].rank;
				}
			}
		}
	}
	free(in);
	if(root == rank)
		free(out);
}

int MPI_Init(int *argc, char ***argv){

	if(DEBUG)printf("********** Interception starts **********\n");
	int err, num, i, namelen, verb, varclass, bind, threadsup;
	int index;
	int readonly, continuous, atomic;
	char name[STR_SZ + 1] = "";
	int desc_len;
	char desc[STR_SZ + 1] = "";
	int mpi_init_return;
	MPI_Datatype datatype;
	MPI_T_enum enumtype;

	/* Run MPI Initialization */
	mpi_init_return = PMPI_Init(argc, argv);
	if (mpi_init_return != MPI_SUCCESS)
		return mpi_init_return;

	/* get global rank */
	PMPI_Comm_rank(MPI_COMM_WORLD, &rank);

	/* get number of tasks*/
	PMPI_Comm_size(MPI_COMM_WORLD, &num_mpi_tasks);

	/* Run MPI_T Initialization */
	err = MPI_T_init_thread(MPI_THREAD_SINGLE, &threadsup);
	if (err != MPI_SUCCESS)
		return mpi_init_return;

	/* Print thread support for MPI */
	if(!rank) {
		print_filled("",88,'-');
		printf("MPI_T Thread support: ");
		switch (threadsup) {
		case MPI_THREAD_SINGLE:
			printf("MPI_THREAD_SINGLE\n");
			break;
		case MPI_THREAD_FUNNELED:
			printf("MPI_THREAD_FUNNELED\n");
			break;
		case MPI_THREAD_SERIALIZED:
			printf("MPI_THREAD_SERIALIZED\n");
			break;
		case MPI_THREAD_MULTIPLE:
			printf("MPI_THREAD_MULTIPLE\n");
			break;
		default:
			printf("unknown (%i)\n",threadsup);
		}
	}


	/* Create a session */
	err = MPI_T_pvar_session_create(&session);
	if (err != MPI_SUCCESS)
		return mpi_init_return;

	/* get number of variables */
	err = MPI_T_pvar_get_num(&num);
	if(!rank) {
		printf("%d performance variables exposed by this MPI library\n",num);
		print_filled("",88,'-');
	}

	if (err != MPI_SUCCESS)
		return mpi_init_return;
	total_num_of_var = num;
	// Get the name of the environment variable to look for
	env_var_name = getenv("MPIT_VAR_TO_TRACE");
	int set_default = 0;
	if( (env_var_name != NULL )){
		if(DEBUG)printf("Environment variable set: %s\n", env_var_name);
		if(strlen(env_var_name) == 0){
			set_default = 1;
		}
	}
	else{
		set_default = 1;
	}

	/* Allocate handles for all performance variables*/
	pvar_handles = (MPI_T_pvar_handle*)malloc(sizeof(MPI_T_pvar_handle) * (num + 1));
	pvar_index = (int*)malloc(sizeof(int) * (num + 1));
	pvar_count = (int*)malloc(sizeof(int) * (num + 1));
	memset(pvar_count, 0, sizeof(int) * (num + 1));
	perf_var_all = (PERF_VAR*)malloc(sizeof(PERF_VAR) * (num + 1));
	pvar_stat = (STATISTICS**)malloc(sizeof(STATISTICS*) * (num + 1));
	int total_length_pvar_name = 0;
	for(i = 0; i < num; i++){
		namelen = desc_len = STR_SZ;
		err = MPI_T_pvar_get_info(i/*IN*/,
				name /*OUT*/,
				&namelen /*INOUT*/,
				&verb /*OUT*/,
				&varclass /*OUT*/,
				&datatype /*OUT*/,
				&enumtype /*OUT*/,
				desc /*desc: OUT*/,
				&desc_len /*desc_len: INOUT*/,
				&bind /*OUT*/,
				&readonly /*OUT*/,
				&continuous /*OUT*/,
				&atomic/*OUT*/);
		perf_var_all[i].pvar_index = -1; // gets setup later
		perf_var_all[i].name_len = namelen;
		perf_var_all[i].name = (char*)malloc(sizeof(char) * (namelen + 1));
		strcpy(perf_var_all[i].name, name);
		total_length_pvar_name += namelen;

		perf_var_all[i].verbosity = verb;
		perf_var_all[i].var_class = varclass;
		perf_var_all[i].datatype = datatype;
		perf_var_all[i].enumtype = enumtype;
		perf_var_all[i].desc_len = desc_len;
		perf_var_all[i].desc = (char*)malloc(sizeof(char) * (desc_len + 1));
		strcpy(perf_var_all[i].desc, desc);
		perf_var_all[i].binding = bind;
		perf_var_all[i].readonly = readonly;
		perf_var_all[i].continuous = continuous;
		perf_var_all[i].atomic = atomic;
	}

	if(set_default == 1){
		/*By default, watch all variables in the list.*/
		//env_var_name = get_pvars_name_list();
		/* Allocate string buffers */

		env_var_name = (char*)malloc(sizeof(char)* (total_length_pvar_name + num * 8 /*strlen(:CLASS_NAME)*/ + num /*delimiter*/ + 1));
		int index = 0;
		char *class_name;
		for (i = 0; i < num; i++)
		{
			memcpy((env_var_name + index), perf_var_all[i].name, strlen(perf_var_all[i].name));
			index += (strlen(perf_var_all[i].name) );
			memcpy((env_var_name + index), ":", strlen(":"));
			index += (strlen(":"));
			class_name = get_pvar_class(perf_var_all[i].var_class);
			memcpy((env_var_name + index), class_name, strlen(class_name));
			index += (strlen(class_name));
			memcpy((env_var_name + index), ";", strlen(";"));
			index += (strlen(";"));
		}
		env_var_name[index] = 0;
	}

	/* Now, start session for those variables in the watchlist*/
	pvar_num_watched = 0;
	char *p = strtok(env_var_name, ";");
	pvar_value_buffer = (unsigned long long int**)malloc(sizeof(unsigned long long int*) * (num + 1));
	int max_count = -1;
	int k;
	while(p != NULL){
		index = get_watched_var_index(p);
		if(index != NOT_FOUND){
			pvar_index[pvar_num_watched] = index;
			perf_var_all[index].pvar_index = pvar_num_watched;
			err = MPI_T_pvar_handle_alloc(session, index, NULL, &pvar_handles[pvar_num_watched], &pvar_count[pvar_num_watched]);
			if (err != MPI_SUCCESS)
				return mpi_init_return;
			// If err == MPI_SUCCESS
			pvar_value_buffer[pvar_num_watched] = (unsigned long long int*)malloc(sizeof(unsigned long long int) * (pvar_count[pvar_num_watched] + 1));
			pvar_stat[pvar_num_watched] = (STATISTICS*)malloc(sizeof(STATISTICS) * (pvar_count[pvar_num_watched] + 1));
			memset(pvar_value_buffer[pvar_num_watched], 0, sizeof(unsigned long long int) * pvar_count[pvar_num_watched]);
			for(k = 0; k < pvar_count[pvar_num_watched]; k++){
				pvar_value_buffer[pvar_num_watched][k] = 0;
				pvar_stat[pvar_num_watched][k].max = NEG_INF;
				pvar_stat[pvar_num_watched][k].min = POS_INF;
				pvar_stat[pvar_num_watched][k].total = 0;

			}
			if(max_count < pvar_count[pvar_num_watched])
				max_count = pvar_count[pvar_num_watched];

			if(perf_var_all[index].continuous == 0){
				err = MPI_T_pvar_start(session, pvar_handles[pvar_num_watched]);
			}
			pvar_num_watched++;
		}
		p = strtok(NULL, ";");
	}
	read_value_buffer = (void*)malloc(sizeof(unsigned long long int) * (max_count + 1));
	max_num_of_state_per_pvar = max_count;

	assert(num >= pvar_num_watched);
	assert(pvar_value_buffer != NULL);
	/* iterate unit variable is found */
	tool_enabled = TRUE;
	num_send = 0;
	num_isend = 0;
	num_recv = 0;
	num_irecv = 0;
	return mpi_init_return;
}

int MPI_Finalize(void)
{
	pvar_read_all();
	/**
	 * Collect statistics from all ranks onto root
	 */
	collect_range_with_loc_from_all_ranks(MPI_MINLOC);
	collect_range_with_loc_from_all_ranks(MPI_MAXLOC);
	collect_sum_from_all_ranks(MPI_SUM);

	if(rank == 0){
		print_filled("",88,'-');
		printf("Performance profiling for the complete MPI job:\n");
		print_filled("",88,'-');
		print_pvar_buffer_all();
		print_filled("",88,'-');
	}
	stop_watching();
	clean_up_perf_var_all(total_num_of_var);
	clean_up_pvar_value_buffer(pvar_num_watched);
	clean_up_the_rest();
	PMPI_Barrier(MPI_COMM_WORLD);
	MPI_T_finalize();
	return PMPI_Finalize();
}
