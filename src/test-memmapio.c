/*
 *****************************************************************

<GPL>

Copyright: © 2001-2012 Robert C Garvey

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
 .
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 .
 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
X-Comment: On Debian systems, the complete text of the GNU General Public
 License can be found in `/usr/share/common-licenses/GPL-3'.

</GPL>
*********************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>


#include <dqacc.h>
#include <cmdargs.h>
#include <mmfor.h>
#include <mmpool.h>

FILE* flog;

static int process_switch_help() {
	if (cmdarg_fetch_switch(NULL, "h")) {
		cmdarg_show_help(NULL);
		exit(1);
	}
	return 0;		// just to eliminate compiler whine
}

#define TA_NRECS 15
typedef struct _TESTA_REC {
	char sync[8];
	char data[9];
	unsigned long lsync;
	unsigned short index;
	char esync[8];
} TESTA_REC;

TESTA_REC* write_testa_rec(TESTA_REC* recp, char* text, unsigned short index) {
	recp->index = index;
	strcpy(recp->sync,"A1A1B2B2");
	strncpy(recp->data, text, sizeof(recp->data) - 1);
	recp->lsync = 0xFDFD;
	strcpy(recp->esync, "E1E1F2F2");
	return recp;
}

int cmp_testa_rec(TESTA_REC* rec1, TESTA_REC* rec2) {
	char* p1;
	char* p2;
	int equals = 1;
	int i;	
	for ( i = 0, p1 = (char*)rec1, p2 = (char*)rec2;
		i < sizeof(TESTA_REC); i++, p1++, p2++) {
		if (*p1 != !p2) {
			equals = 0;
			break;
		}
	}
	return equals;
}	
void print_testa_rec(TESTA_REC* recp) {
	if (recp->lsync != 0xFDFD) {
		fprintf(stdout, "TESTA_REC: lsync error\n");
		exit(1);
	}
	fprintf(stdout, "TESTA_REC: index [%d] text [%s]\n", 
		recp->index, recp->data);
}

static void rptline(char* fmtp, ...) {
	va_list ap;
	
	va_start(ap, fmtp);
	vfprintf(stdout, fmtp, ap);
	fprintf(stdout,"\n");
}

static int report_pool(BPOOL_HANDLE* bphp) {
	rptline("=========== BUFFER POOL REPORT =================");
	rptline("");
	rptline("Name: %s ID: %d Capacity: %d Req-Capacity %d",
		bphp->bpmf_recp->stats.name,
		bphp->bpmf_recp->stats.bp_id,
		bphp->bpmf_recp->stats.capacity,
		bphp->bpmf_recp->stats.rqst_capacity
	);
	rptline("In the pool: In: %d Capacity: %d Pct: %g",
		bphp->bpmf_recp->dq_inpool.dquse,
		bphp->bpmf_recp->dq_inpool.dqslots,
		(100.0 * bphp->bpmf_recp->dq_inpool.dquse)/ bphp->bpmf_recp->dq_inpool.dqslots
	);
	rptline("Out of pool: In: %d Capacity: %d Pct: %g",
		bphp->bpmf_recp->dq_outpool.dquse,
		bphp->bpmf_recp->dq_outpool.dqslots,
		(100.0 * bphp->bpmf_recp->dq_outpool.dquse)/ bphp->bpmf_recp->dq_outpool.dqslots
	);
	return 0;
}

static int process_switch_testa() {
	char fpathbuff[512];
	int status = 0;
	int recx = 0;
	char* strdir = NULL;
	MMFOR_HANDLE* mmfhp = NULL;
	TESTA_REC* recp = NULL;
	DQHEADER deque;
	
	status = 1;
	
	// Construct deque to hold TA_REC for readback test
	dq_init(TA_NRECS, sizeof(TESTA_REC), &deque);
	
	if (cmdarg_fetch_switch(NULL, "a")) {
		fprintf(stdout, "TEST-A: File of Records Test\n");
		strdir = cmdarg_fetch_string(NULL, "d");
		if (NULL == strdir) {
			fprintf(stdout, "TEST-A: Target directory not provided in cmd args\n");
			exit(1);
		}
		sprintf(fpathbuff,"%s/TEST-A.DAT",strdir);
		fprintf(stdout, "Writing file %s\n", fpathbuff);
		mmfhp = mmfor_create(fpathbuff, MMA_READ_WRITE, MMF_SHARED, 0660, 
			sizeof(TESTA_REC), TA_NRECS);
		for (recx = 0; recx < TA_NRECS; recx++) {
			char txtbuff[sizeof(recp->data)];
			
			recp = (TESTA_REC*)mmfor_x2p(mmfhp, recx);	
			if (NULL == recp) {
				fprintf(stdout, "TEST-A Fails! NULL pointer from mmfor_x2p at index %d\n",
					recx);
				exit(1);
			}
			sprintf(txtbuff, "REC-%d", recx);
			write_testa_rec(recp, txtbuff, recx);
			fprintf(stdout, "Data: [%s]\n", recp->data);
			dq_abd(&deque, recp);
		}
		fprintf(stdout, "\nWRITE TEST COMPLETE\n");
		fprintf(stdout, "Closing test file %s\n", fpathbuff);
		mmfor_close(mmfhp);
		fprintf(stdout, "Re-opening test file %s\n", fpathbuff);
		mmfhp = NULL;
		mmfhp = mmfor_open(fpathbuff, MMA_READ_WRITE, MMF_SHARED);
		if (NULL == mmfhp) {
			fprintf(stdout, "TEST-A: mmfor_open fails!\n");
			exit(1);
		}
		for (recx= 0; recx < TA_NRECS; recx++) {
			TESTA_REC xrec;
			
			dq_rtd(&deque, &xrec);
			recp = (TESTA_REC*)mmfor_x2p(mmfhp, recx);
			if (cmp_testa_rec(&xrec, recp)) {
				fprintf(stdout, "TEST-A: Readback compare fails at record %d\n", recx);
				exit(1);
			}
			fprintf(stdout, "Read Data: [%s]\n", recp->data);
		}
		dq_close(&deque);
		fprintf(stdout, "TEST-A: Completed for %d records\n", TA_NRECS);
		return status;
	}
	return status;
}

static int process_switch_testb() {
	char* pool_name = "pool1";
	int status = 0;
	BPOOL_HANDLE* bphp;
	BPMF_STATS* statsp;
	int i;
	char* logfilepath;
	FILE* logf;
	char* strdir;

	if (cmdarg_fetch_switch(NULL, "b")) {
		strdir = cmdarg_fetch_string(NULL, "d");
		if (NULL == strdir) {
			fprintf(stdout, "TEST-B: Target directory not provided in cmd args\n");
			exit(1);
		}
		setenv(MMPOOL_ENV_DATA_DIR, strdir, 1);

		flog = stdout;
		if (cmdarg_fetch_switch(NULL, "l")) {
			// Log file option
			logfilepath = cmdarg_fetch_string(NULL, "l");
			logf = fopen(logfilepath, "w");
			if (NULL != logf) {
				flog = logf;
			} else {
				flog = stdout;
				fprintf(stderr, "Unable to open log file %s --- default to stdout\n", logfilepath);
			}
		}
		pool_name = cmdarg_fetch_string(NULL, "p");
		bphp = mmpool_open(pool_name);
		if (NULL == bphp) {
			fprintf(stdout, "TEST-B Fails: Pool %s Not Found!\n", pool_name);
			exit(1);
		}
		fprintf(stdout, "TEST-B -- Allocation of buffers from pool.\n");
		report_pool(bphp);
		statsp = mmpool_getstats(bphp);
		
		// We got a pool. Now allocate all the buffers. TEST-C will deallocate
		// then,
		for (i=0; i < statsp->capacity; i++) {
			BPCF_BUFFER_REF* buff_refp;
			char* p;
			
			buff_refp = mmpool_getbuff(bphp);
			if (NULL == buff_refp) {
				fprintf(stdout, 
					"TEST-B Error: Null buffer reference returned from mmpool_getbuff after %d allocations\n", i);
				exit(1);
			}
			p = (char*)mmpool_buffer_data(buff_refp);
			sprintf(p, "%ld|test-memmapio TEST-B Allocated record %d pool index %ld", 
				buff_refp->bpindex, i, buff_refp->bpindex);
			fprintf(flog, "TEST-B: %ld\n", buff_refp->bpindex);
		}
		rptline("TEST-B Complete ... %d buffers allocated\n", i);
		report_pool(bphp);
	}
	return status;	
}
unsigned long parse_index(const char* s) {
	static char* delim = "|";
	char* buff;
	char *pToken;
	int val = 0;
	if (strstr(s, delim) == NULL) {
		val = 0;
	} else {
		buff = (char*)calloc(strlen(s) + 1, 1);
		strcpy(buff, s);
		pToken = strtok(buff, delim);
		val = atol(pToken);
		free(buff);
	}
	return val;
}

static int process_switch_testc() {
	char* pool_name = "pool1";
	int status = 0;
	BPOOL_HANDLE* bphp;
	BPMF_STATS* statsp;
	unsigned long  i;
	char* strdir;
	char* logfilepath;
	FILE* logf;

	if (cmdarg_fetch_switch(NULL, "c")) {
		strdir = cmdarg_fetch_string(NULL, "d");
		if (NULL == strdir) {
			fprintf(stdout, "TEST-C: Target directory not provided in cmd args\n");
			exit(1);
		}
		setenv(MMPOOL_ENV_DATA_DIR, strdir, 1);

		flog = stdout;
		if (cmdarg_fetch_switch(NULL, "l")) {
			// Log file option
			logfilepath = cmdarg_fetch_string(NULL, "l");
			logf = fopen(logfilepath, "w");
			if (NULL != logf) {
				flog = logf;
			} else {
				flog = stdout;
				fprintf(stderr, "Unable to open log file %s --- default to stdout\n", logfilepath);
			}
		}

		pool_name = cmdarg_fetch_string(NULL, "p");
		bphp = mmpool_open(pool_name);
		if (NULL == bphp) {
			fprintf(stdout, "TEST-C Fails: Pool %s Not Found!\n", pool_name);
			exit(1);
		}
		fprintf(stdout, "TEST-C -- Deallocation of buffers from pool.\n");
		report_pool(bphp);
		statsp = mmpool_getstats(bphp);
		// We got a pool. The assumption is TEST-B has allocated ALL the
		// buffers. 
		for (i=0; i < statsp->capacity; i++) {
			BPCF_BUFFER_REF* buff_refp;
			char *pdata;
			
			buff_refp = mmpool_buffx2refp(bphp, i);
			pdata = mmpool_buffer_data(buff_refp);
			if (i != parse_index(pdata)) {
				fprintf(stdout, "ERROR: Expected buffer index %ld pdata was %s\n",
					i, pdata);
				exit(1);
			} else {
				fprintf(flog, "Record: %d : %s\n", i, pdata);
			}
			mmpool_putbuff(bphp, buff_refp);
		}
		fprintf(stdout, "All buffers should now be deallocated!\n");
		report_pool(bphp);
	}
	return status;
}
static void register_args(int argc, char* argv[]) {
	
	cmdarg_init(argc, argv);
	cmdarg_register_option("a", "testa", CA_SWITCH,
		"Run Test a -- File of Records (mmfor)", NULL, NULL);
	cmdarg_register_option("b", "testb", CA_SWITCH,
		"Run Test b -- basic buffer allocation", NULL, NULL);
	cmdarg_register_option("c", "testc", CA_SWITCH,
		"Run Test c -- basic buffer read and deallocation", NULL, NULL); 
	cmdarg_register_option("h", "help", CA_SWITCH,
		"Print command help", NULL, NULL);

	// Log file option for testb -- buffer allocation
	cmdarg_register_option("l", "logfile", CA_OPTIONAL_ARG,
			"Logfile for testb -- buffer allocation", NULL, NULL);

	// Common options
	cmdarg_register_option("d", "directory", CA_DEFAULT_ARG,
		"Data directory", "/tmp/test-data", NULL);
	cmdarg_register_option("p", "pool", CA_DEFAULT_ARG,
		"Pool Name", "pool1", NULL);
	
}

int main(int argc, char* argv[]) {
	char* pargv[] = {"a", "b", "c"};
	static int switches[] = {'h', 'a', 'b', 'c', '\0'};
	static int (*process_func[])() = { 
		process_switch_help, 
		process_switch_testa,
		process_switch_testb,
		process_switch_testc,
		NULL
	};
	int status = 0;
	int i;
	
	flog = stdout;
	if (argc == 1) {
		// No arguments supplied ...presume
		// called from make check we want to execute all tests.
		register_args(4, pargv);
	} else {
		// We have command line arguments
		register_args(argc, argv);
	}
	
	status = cmdarg_parse(argc, argv);
	if (status) {
		cmdarg_show_help(NULL);
		exit(1);
	}
	
	// Parsed arguments successfully ... process. Handler routines
	// return > 0 if successful, 0 if they took no action, and < 0 
	// if they encountered an error.
	for (status = 0, i = 0; (status >= 0 && switches[i] != 0); i++) {
		status = (*process_func[i])();
	}
	return status;
}
