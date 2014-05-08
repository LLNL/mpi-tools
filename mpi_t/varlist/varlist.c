//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, Lawrence Livermore National Security,
// LLC. Produced at the Lawrence Livermore National Laboratory. Written
// by Martin Schulz (schulzm@llnl.gov). CODE-LLNL-CODE-647221. All rights
// reserved. This file is part of mpi_T-tools. For details, see
// https://computation-rnd.llnl.gov/mpi_t/varList.php. Please also read
// this file - ./FULL-LICENSE.txt.
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License (as published by
// the Free Software Foundation) version 2.1 dated February 1999.
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
// conditions of the GNU General Public License for more details. You
// should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

#include <mpi.h>

#define SCREENLEN 78

#ifndef MPI_COUNT
#define MPI_COUNT MPI_INT
typedef int MPI_Count;
#endif

#ifndef MPI_T_CVAR_HANDLE_NULL
#define MPI_T_CVAR_HANDLE_NULL NULL
#endif

#define CHECKERR(errstr,err) if (err!=MPI_SUCCESS) { printf("ERROR: %s: MPI error code %i\n",errstr,err); usage(1); } 

int list_pvar,list_cvar,longlist,verbosity,runmpi; 

#define RUNMPI 1

/* Usage */

void usage(int e)
{
	printf("Usage: varlist [-c] [-p] [-v <VL>] [-l] [-m]\n");
	printf("    -c = List only Control Variables\n");
	printf("    -p = List only Performance Variables\n");
	printf("    -v = List up to verbosity level <VL> (1=U/B to 9=D/D)\n");
	printf("    -l = Long list with all information, incl. descriptions\n");
	printf("    -m = Do not call MPI_Init before listing variables\n");
	printf("    -h = This help text\n");
	exit(e);
}


/* Print a filled string */

void print_filled(char *s, int len, char c)
{
	int i;
	printf("%s",s);
	for (i=strlen(s); i<len; i++)
		printf("%c",c);
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


/* Print a CVAR/PVAR bind */

void print_bind(int b)
{
	switch (b) {
	case MPI_T_BIND_NO_OBJECT:
		printf("n/a     ");
		break;
	case MPI_T_BIND_MPI_COMM:
		printf("COMM    ");
		break;
	case MPI_T_BIND_MPI_DATATYPE:
		printf("DATATYPE");
		break;
	case MPI_T_BIND_MPI_ERRHANDLER:
		printf("ERRHAND ");
		break;
	case MPI_T_BIND_MPI_FILE:
		printf("FILE    ");
		break;
	case MPI_T_BIND_MPI_GROUP:
		printf("GROUP   ");
		break;
	case MPI_T_BIND_MPI_OP:
		printf("OP      ");
		break;
	case MPI_T_BIND_MPI_REQUEST:
		printf("REQUEST ");
		break;
	case MPI_T_BIND_MPI_WIN:
		printf("WINDOW  ");
		break;
	case MPI_T_BIND_MPI_MESSAGE:
		printf("MESSAGE ");
		break;
	case MPI_T_BIND_MPI_INFO:
		printf("INFO    ");
		break;
	default:
		printf("UNKNOWN ");
		break;
	}
}


/* Print a CVAR/PVAR type */

void print_type(MPI_Datatype t)
{
	if (t==MPI_INT)
	{
		printf("INT   ");
	}
	else if (t==MPI_UNSIGNED)
	{
		printf("UINT  ");
	}
	else if (t==MPI_UNSIGNED_LONG)
	{
		printf("ULONG ");
	}
	else if (t==MPI_UNSIGNED_LONG_LONG)
	{
		printf("ULLONG");
	}
	else if (t==MPI_COUNT)
	{
		printf("COUNT ");
	}
	else if (t==MPI_CHAR)
	{
		printf("CHAR  ");
	}
	else if (t==MPI_DOUBLE)
	{
		printf("DOUBLE");
	}
	else
	{
		printf("UNKNOW");
	}
}


/* Print CVAR Scope */

void print_scope(int s)
{
	switch (s) {
#if 0
	case MPI_T_SCOPE_CONSTANT:
		printf("CONST   ");
		break;
#endif
	case MPI_T_SCOPE_READONLY:
		printf("READONLY");
		break;
	case MPI_T_SCOPE_LOCAL:
		printf("LOCAL   ");
		break;
	case MPI_T_SCOPE_GROUP:
		printf("GROUP   ");
		break;
	case MPI_T_SCOPE_GROUP_EQ:
		printf("GROUP-EQ");
		break;
	case MPI_T_SCOPE_ALL:
		printf("ALL-EQ  ");
		break;
	case MPI_T_SCOPE_ALL_EQ:
		printf("ALL     ");
		break;
	default:
		printf("UNKNOWN ");
		break;
	}
}


/* Print a Yes/No class */

void print_yesno(int yesno)
{
	if (yesno)
		printf("YES");
	else
		printf(" NO");
}


/* Print verbosity level */

void print_verbosity(int verbos)
{
	switch (verbos) {
	case MPI_T_VERBOSITY_USER_BASIC:
		printf("User/Basic = 1");
		break;
	case MPI_T_VERBOSITY_USER_DETAIL:
		printf("User/Detail = 2");
		break;
	case MPI_T_VERBOSITY_USER_ALL:
		printf("User/All = 3");
		break;
	case MPI_T_VERBOSITY_TUNER_BASIC:
		printf("Tuner/Basic = 4");
		break;
	case MPI_T_VERBOSITY_TUNER_DETAIL:
		printf("Tuner/Detail = 5");
		break;
	case MPI_T_VERBOSITY_TUNER_ALL:
		printf("Tuner/All = 6");
		break;
	case MPI_T_VERBOSITY_MPIDEV_BASIC:
		printf("Developer/Basic = 7");
		break;
	case MPI_T_VERBOSITY_MPIDEV_DETAIL:
		printf("Developer/Detail = 8");
		break;
	case MPI_T_VERBOSITY_MPIDEV_ALL:
		printf("Developer/All = 9");
		break;
	default:
		printf("UNKNOWN");
		break;
	}
}


void print_verbosity_short(int verbos)
{
	switch (verbos) {
	case MPI_T_VERBOSITY_USER_BASIC:
		printf("U/B-1");
		break;
	case MPI_T_VERBOSITY_USER_DETAIL:
		printf("U/D-2");
		break;
	case MPI_T_VERBOSITY_USER_ALL:
		printf("U/A-3");
		break;
	case MPI_T_VERBOSITY_TUNER_BASIC:
		printf("T/B-4");
		break;
	case MPI_T_VERBOSITY_TUNER_DETAIL:
		printf("T/D-5");
		break;
	case MPI_T_VERBOSITY_TUNER_ALL:
		printf("T/A-6");
		break;
	case MPI_T_VERBOSITY_MPIDEV_BASIC:
		printf("D/B-7");
		break;
	case MPI_T_VERBOSITY_MPIDEV_DETAIL:
		printf("D/D-8");
		break;
	case MPI_T_VERBOSITY_MPIDEV_ALL:
		printf("D/A-9");
		break;
	default:
		printf("?????");
		break;
	}
}


/* Print all Performance Variables */

void list_pvars()
{
	int num,err,i,numvars;
	char *name, *desc;
	int bind,vc,verbos,ro,ct,at;
	MPI_Datatype dt;
	MPI_T_enum et;
	int maxnamelen=strlen("Variable");
	int maxdesclen=0;
	int prtlen;
	int namelen,desclen;


	/* Get number of variables */

	err=MPI_T_pvar_get_num(&num);
	CHECKERR("PVARNUM",err);
	printf("Found %i performance variables\n",num);


	/* Find string sizes */

	numvars=0;

	for (i=0; i<num; i++)
	{
		int namelen=0;
		int desclen=0;
		char fname[5];
		char fdesc[5];
		err=MPI_T_pvar_get_info(i,fname,&namelen,&verbos,&vc,&dt,&et,fdesc,&desclen,&bind,&ro,&ct,&at);
		if (namelen>maxnamelen) maxnamelen=namelen;
		if (desclen>maxdesclen) maxdesclen=desclen;
		if (verbos<=verbosity) numvars++;
	}

	printf("Found %i performance variables with verbosity <= ",numvars);
	print_verbosity_short(verbosity);
	printf("\n\n");


	/* Allocate string buffers */

	name=(char*)malloc(sizeof(char)*maxnamelen);
	CHECKERR("Malloc Name",name==NULL);
	desc=(char*)malloc(sizeof(char)*maxdesclen);
	CHECKERR("Malloc Desc",desc==NULL);

	/* Print header */

	prtlen=0;
	if (!longlist)
	{
		print_filled("Variable",maxnamelen,' ');
		printf(" ");
		prtlen=maxnamelen+1;
		printf("VRB  ");
		printf(" ");
		prtlen+=5+1;
		printf("Class  ");
		printf(" ");
		prtlen+=7+1;
		printf("Type  ");
		printf(" ");
		prtlen+=6+1;
		printf("Bind    ");
		printf(" ");
		prtlen+=8+1;
		printf("R/O");
		printf(" ");
		prtlen+=3+1;
		printf("CNT");
		printf(" ");
		prtlen+=3+1;
		printf("ATM");
		printf("\n");
		prtlen+=3;
		print_filled("",prtlen,'-');printf("\n");
	}


	/* Loop and print */

	for (i=0; i<num; i++)
	{
		namelen=maxnamelen;
		desclen=maxdesclen;
		err=MPI_T_pvar_get_info(i,name,&namelen,&verbos,&vc,&dt,&et,desc,&desclen,&bind,&ro,&ct,&at);
		CHECKERR("PVARINFO",err);
		if (verbos<=verbosity)
		{
			if (!longlist)
			{
				print_filled(name,maxnamelen,' ');
				printf(" ");
				print_verbosity_short(verbos);
				printf(" ");
				print_class(vc);
				printf(" ");
				print_type(dt);
				printf(" ");
				print_bind(bind);
				printf(" ");
				print_yesno(ro);
				printf(" ");
				print_yesno(ct);
				printf(" ");
				print_yesno(at);
				printf("\n");
			}
			else
			{
				print_filled("",SCREENLEN,'-');printf("\n");
				printf("Name: %s (",name); print_verbosity(verbos);printf(")\n");
				printf("Class: "); print_class(vc); printf("\n");
				printf("Type:  "); print_type(dt); printf("\n");
				printf("Bind:  "); print_bind(bind); printf("\n");
				printf("Attr.: ");
				printf("Readonly:");print_yesno(ro);printf(" ");
				printf("Cont.:");print_yesno(ct);printf(" ");
				printf("Atomic:");print_yesno(at);printf("\n\n");
				if (desc!=NULL)
					printf("%s\n\n",desc);
			}
		}
	}

	if (numvars>0)
	{
		if (!longlist)
		{
			print_filled("",prtlen,'-');printf("\n");
		}
		else
		{
			print_filled("",SCREENLEN,'-');printf("\n");
		}
	}


	/* free buffers */

	free(name);
	free(desc);
}


/* Print all Control Variables */

void list_cvars()
{
	int num,err,i,numvars;
	char *name, *desc;
	int bind,verbos,scope;
	MPI_Datatype dt;
	MPI_T_enum et;
	int maxnamelen=strlen("Variable");
	int maxdesclen=0;
	int prtlen;
	int namelen,desclen;

	int v_int;
	unsigned int v_uint;
	unsigned long v_ulong;
	unsigned long long v_ullong;
	MPI_Count v_count;
	char v_char[4097];
	double v_double;
	char value[257];
	int value_sup;
	MPI_T_cvar_handle handle=MPI_T_CVAR_HANDLE_NULL;
	int count;

	/* Get number of variables */

	err=MPI_T_cvar_get_num(&num);
	CHECKERR("CVARNUM",err);
	printf("Found %i control variables\n",num);


	/* Find string sizes */

	numvars=0;

	for (i=0; i<num; i++)
	{
		int namelen=0;
		int desclen=0;
		char fname[5];
		char fdesc[5];
		err=MPI_T_cvar_get_info(i,fname,&namelen,&verbos,&dt,&et,fdesc,&desclen,&bind,&scope);
		if (namelen>maxnamelen) maxnamelen=namelen;
		if (desclen>maxdesclen) maxdesclen=desclen;
		if (verbos<=verbosity) numvars++;
	}

	printf("Found %i control variables with verbosity <= ",numvars);
	print_verbosity_short(verbosity);
	printf("\n\n");

	/* Allocate string buffers */

	name=(char*)malloc(sizeof(char)*maxnamelen);
	CHECKERR("Malloc Name",name==NULL);
	desc=(char*)malloc(sizeof(char)*maxdesclen);
	CHECKERR("Malloc Desc",desc==NULL);


	/* Print header */

	prtlen=0;
	if (!longlist)
	{
		print_filled("Variable",maxnamelen,' ');
		printf(" ");
		prtlen=maxnamelen+1;
		printf("VRB  ");
		printf(" ");
		prtlen+=5+1;
		printf("Type  ");
		printf(" ");
		prtlen+=6+1;
		printf("Bind    ");
		printf(" ");
		prtlen+=8+1;
		printf("Scope   ");
		printf(" ");
		prtlen+=8+1;
		printf("Value");
		printf("\n");
		prtlen+=12;
		print_filled("",prtlen,'-');printf("\n");
	}


	/* Loop and print */

	for (i=0; i<num; i++)
	{
		namelen=maxnamelen;
		desclen=maxdesclen;
		err=MPI_T_cvar_get_info(i,name,&namelen,&verbos,&dt,&et,desc,&desclen,&bind,&scope);
		if (MPI_T_ERR_INVALID_INDEX == err)// { NOTE TZI: This variable is not recognized by Mvapich. It is OpenMPI specific.
			continue;

		CHECKERR("CVARINFO",err);

		value_sup=1;
		if (bind==MPI_T_BIND_NO_OBJECT)
		{
			err=MPI_T_cvar_handle_alloc(i,NULL,&handle,&count);
			CHECKERR("CVAR-ALLOC",err);
		}
		else if (bind==MPI_T_BIND_MPI_COMM)
		{
			MPI_Comm comm=MPI_COMM_WORLD;
			err=MPI_T_cvar_handle_alloc(i,&comm,&handle,&count);
			CHECKERR("CVAR-ALLOC",err);
		}
		else
		{
			value_sup=0;
			sprintf(value,"unsupported");
		}

		if (value_sup)
		{
			if (count==1 || dt==MPI_CHAR)
			{
				if (dt==MPI_INT)
				{
					err=MPI_T_cvar_read(handle,&v_int);
					CHECKERR("CVARREAD",err);
					if (et==MPI_T_ENUM_NULL)
					{
						sprintf(value,"%i",v_int);
					}
					else
					{
						int i,etnum;
						char etname[20];
						int etlen=20;
						int done=0;
						int newval;
						err=MPI_T_enum_get_info(et,&etnum,etname,&etlen);
						for (i=0; i<etnum; i++)
						{
							etlen=12;
							err=MPI_T_enum_get_item(et,i,&newval,etname,&etlen);
							if (newval==v_int)
							{
								sprintf(value, "%s",etname);
								done=1;
							}
						}
						if (!done)
						{
							sprintf(value, "unknown");
						}
					}
				}
				else if (dt==MPI_UNSIGNED)
				{
					err=MPI_T_cvar_read(handle,&v_uint);
					CHECKERR("CVARREAD",err);
					sprintf(value,"%u",v_uint);
				}
				else if (dt==MPI_UNSIGNED_LONG)
				{
					err=MPI_T_cvar_read(handle,&v_ulong);
					CHECKERR("CVARREAD",err);
					sprintf(value,"%lu",v_ulong);
				}
				else if (dt==MPI_UNSIGNED_LONG_LONG)
				{
					err=MPI_T_cvar_read(handle,&v_ullong);
					CHECKERR("CVARREAD",err);
					sprintf(value,"%llu",v_ullong);
				}
				else if (dt==MPI_COUNT)
				{
					err=MPI_T_cvar_read(handle,&v_count);
					CHECKERR("CVARREAD",err);
					sprintf(value,"%lu",v_count);
				}
				else if (dt==MPI_CHAR)
				{
					err=MPI_T_cvar_read(handle,v_char);
					CHECKERR("CVARREAD",err);
					sprintf(value,"%s",v_char);
				}
				else if (dt==MPI_DOUBLE)
				{
					err=MPI_T_cvar_read(handle,&v_double);
					CHECKERR("CVARREAD",err);
					sprintf(value,"%d",v_double);
				}
				else
				{
					value_sup=0;
					sprintf(value,"unsupported");
				}
			}
			else
			{
				value_sup=0;
				sprintf(value,"unsupported");
			}
		}

		if (handle==MPI_T_CVAR_HANDLE_NULL)
		{
			MPI_T_cvar_handle_free(&handle);
			CHECKERR("CVAR-FREE",err);
		}

		if (verbos<=verbosity)
		{
			if (!longlist)
			{
				print_filled(name,maxnamelen,' ');
				printf(" ");
				print_verbosity_short(verbos);
				printf(" ");
				print_type(dt);
				printf(" ");
				print_bind(bind);
				printf(" ");
				print_scope(scope);
				printf(" ");
				printf("%s",value);
				printf("\n");
			}
			else
			{
				print_filled("",SCREENLEN,'-');printf("\n");
				printf("Name: %s (",name); print_verbosity(verbos);printf(")\n");
				printf("Type:  "); print_type(dt); printf("\n");
				printf("Bind:  "); print_bind(bind); printf("\n");
				printf("Scope: ");print_scope(scope);printf("\n");
				printf("Value: %s\n\n",value);
				if (desc!=NULL)
					printf("%s\n\n",desc);
			}
		}
	}

	if (numvars>0)
	{
		if (!longlist)
		{
			print_filled("",prtlen,'-');printf("\n");
		}
		else
		{
			print_filled("",SCREENLEN,'-');printf("\n");
		}
	}


	/* free buffers */

	free(name);
	free(desc);
}


/* Main */

int main(int argc, char *argv[])
{
	int err,errarg;
	int threadsupport,threadsupport_t;
	int rank;
	int opt,erropt;
	int reqthread=MPI_THREAD_MULTIPLE;

	/* Read options */

	verbosity=MPI_T_VERBOSITY_MPIDEV_ALL;
	list_pvar=1;
	list_cvar=1;
	longlist=0;
	runmpi=1;
	errarg=0;

	while ((opt=getopt(argc,argv, "hv:pclim")) != -1 ) {
		switch (opt) {
		case 'h':
			errarg=-1;
			break;
		case 'v':
			switch (atoi(optarg)) {
			case 1: verbosity=MPI_T_VERBOSITY_USER_BASIC; break;
			case 2: verbosity=MPI_T_VERBOSITY_USER_DETAIL; break;
			case 3: verbosity=MPI_T_VERBOSITY_USER_ALL; break;
			case 4: verbosity=MPI_T_VERBOSITY_TUNER_BASIC; break;
			case 5: verbosity=MPI_T_VERBOSITY_TUNER_DETAIL; break;
			case 6: verbosity=MPI_T_VERBOSITY_TUNER_ALL; break;
			case 7: verbosity=MPI_T_VERBOSITY_MPIDEV_BASIC; break;
			case 8: verbosity=MPI_T_VERBOSITY_MPIDEV_DETAIL; break;
			case 9: verbosity=MPI_T_VERBOSITY_MPIDEV_ALL; break;
			}
			break;
			case 'p':
				list_pvar=1;
				list_cvar=0;
				break;
			case 'c':
				list_cvar=1;
				list_pvar=0;
				break;
			case 'l':
				longlist=1;
				break;
			case 'm':
				runmpi=0;
				break;
			default:
				errarg=1;
				erropt=opt;
				break;
		}
	}

	/* Initialize */

	if (runmpi)
	{
		err=MPI_Init_thread(&argc,&argv,reqthread,&threadsupport);
		CHECKERR("Init",err);

		err=MPI_Comm_rank(MPI_COMM_WORLD,&rank);
		CHECKERR("Rank",err);
	}
	else
		rank=0;


	/* ONLY FOR RANK 0 */

	if (rank==0)
	{
		err=MPI_T_init_thread(reqthread, &threadsupport_t);
		CHECKERR("T_Init",err);

		if (errarg)
		{
			if (errarg>0)
				printf("Argument error: %c\n",erropt);
			usage(errarg!=-1);
		}


		/* Header */

		printf("MPI_T Variable List\n");


		if (runmpi)
		{
			/* Print thread support for MPI */

			printf("  MPI Thread support: ");
			switch (threadsupport) {
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
				printf("unknown (%i)\n",threadsupport);
			}
		}

		/* Print thread support for MPI_T */

		printf("  MPI_T Thread support: ");
		switch (threadsupport_t) {
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
			printf("unknown (%i)\n",threadsupport_t);
		}

		/* Start MPI_T */


		if (list_cvar)
		{
			printf("\n===============================\n");
			printf("Control Variables");
			printf("\n===============================\n\n");
			list_cvars();
			printf("\n");
		}

		if (list_pvar)
		{
			printf("\n===============================\n");
			printf("Performance Variables");
			printf("\n===============================\n\n");
			list_pvars();
			printf("\n");
		}
	}

	/* Clean up */

	if (runmpi)
	{
		err=MPI_Barrier(MPI_COMM_WORLD);
		CHECKERR("Barrier",err);
	}

	if (rank==0)
		MPI_T_finalize();

	if (runmpi)
		MPI_Finalize();

	if (rank==0)
		printf("Done.\n");

	return 0;
}

