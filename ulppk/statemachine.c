/*
 *****************************************************************

<GPL>

Copyright: © 2001-2009 Robert C Garvey

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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "statemachine.h"
#include <pathinfo.h>
#include <appenv.h>
#include <sysconfig.h>

/**
 * @file statemachine.c
 *
 * @brief Implementation of the statemachine mechanism.
 *
 * This package provides definitions and a framework for implementing
 * classic Ward/Mellor state machines. A state machine is described by a graph consisting
 * of
 *
 * <ol>
 * <li>A set of states.</li>
 * <li>A set of transitions connecting the states</li>
 * </ol>
 *
 * Transitions are triggered by events and implemented by one or more actions. The
 * sequence of actions is expressed in a list. The individual actions are performed
 * in the order they are given on the list.
 *
 * The triggering event, action list, initial state and final state are considered
 * properties of the transitions.  The semantics could be expressed: When in state S1,
 * when event E1 occurs, perform action list AL1 and transition to State S2.
 *
 * Nomenclature:
 * <ul>
 * 	<li>tx => transition</li>
 *  <li>ev => event</li>
 *  <li>al => action list</li>
 *  <li>ax => action (member of action list)</li>
 *  <li>st => state</li>
 * </ul>
 *
 * WARNING: This package is designed under the guiding philosophy of "Strong
 * typing is for weak minds." Stick to it. Or else. :D
 *
 * <h3>NOTES ON MULTIPLE MACHINES USING THE SAME DEFINITIONS</h3>
 *
 * To create a machine. (This machine is allocated off the heap.)
 *
 * static * SM_STATE_TABLE_DEF mydefs;
 *
 *
 * machine1p = sm_new_machine(NULL, &mydefs, "mymachine1");
 *
 * Now register events, action lists, Then create a second machine reusing
 * the definitions table:
 *
 * machine2p = sm_new_machine(NULL, &mydefs, "mymachine2")
 *
 * <h3>NOTES ON LOGGING:</h3>
 *
 * If you want log statemachine operations to the system
 * logger, then uncomment the define below or configure
 * with -D_STATEMACHINE_USE_SYSLOG. e.g.
 *
 * env CFLAGS="-g -O0 -D_STATEMACHINE_USE_SYSLOG_" ./configure
 *
 * The default is to log to [logdir]/[appname].statemachine.log
 * where
 *	[logdir] is the value of environment variable SYSCONFIG_LOG_DIR
 *	[appname] is the value of environment variable SYSCONFIG_APPNAME
 *
 * If SYSLOG is used for logging, then the package is counting
 * on the calling program to properly call openlog and set up use
 * of the system logger. (That is normally done using ulppk_log functions.)
 */

#if 0
static SM_ERROR smerrno = SM_NO_ERROR;
static char errtext[1024];
static char object_name[MAX_TRANSITION_NAME_LEN];
#endif

// Note: This needs to be kept consistent with the SM_ERROR enum definition
// in statemachine.h
static char* errfmt[] = {
	"No error",
	"Attempt to register duplicate object name: [%s]",
	"Named object not found: [%s]",
	"Parent object not found. Name: [%s]",
	"Too many objects already registered: [%s]",
	"State machine aborted! [%s]",
	"Attempt to transition when state machine definition not complete! [%s]"
};

static int logtimestamp(char* buff, size_t bufflen) {
	char* timefmt="%Y-%m-%d %H:%M:%S";
	struct tm *tmp;
	time_t t;

	t = time(NULL);
	tmp = localtime(&t);
	return strftime(buff, bufflen - 1, timefmt, tmp);
}

// If you want log statemachine operations to the system
// logger, then uncomment the #define below or configure
// with -D_STATEMACHINE_USE_SYSLOG. e.g.

// env CFLAGS="-g -O0 -D_STATEMACHINE_USE_SYSLOG_" ./configure
//
// The default is to log to <logdir>/<appname>.statemachine.log
// where
//	<logdir> is the value of environment variable SYSCONFIG_LOG_DIR
//	<appname> is the value of environment variable SYSCONFIG_APPNAME
// #define _STATEMACHINE_USE_SYSLOG

#ifndef _STATEMACHINE_USE_SYSLOG
static FILE* flog = NULL;
#endif

#define _SM_LOGGING
#ifdef _SM_LOGGING

#define SMLOG(...) smlog( __VA_ARGS__ );
#define SMOPENLOG(a) smopenlog(a);
#define SMCLOSELOG smclose_log();

#else

#define SMLOG(...) 
#define SMOPENLOG
#define SMCLOSELOG

#endif

// Given a pointer to a state machine, retrieve a
// pointer to its state definition table structure.
static SM_STATE_TABLE_DEF* get_stdefp(SM_MACHINE* machinep) {
	return machinep->stdefp;
}

#ifndef _STATEMACHINE_USE_SYSLOG
static void smopenlog(SM_MACHINE* machinep) {
	if (NULL == flog ) {
		char* log_dir;
		char* appname;
		char* filename;
		char* filepath;
		char timestr[32];

		logtimestamp(timestr, sizeof(timestr));
		log_dir = appenv_read_env_var(SYSCONFIG_LOG_DIR);
		appname = appenv_read_env_var(SYSCONFIG_APPNAME);
		filename = (char*)calloc(strlen(appname) + strlen(".statemachine.log") + 3, sizeof(char));
		strcpy(filename, appname);
		strcat(filename, ".statemachine.log");
		filepath = pathinfo_append2path(log_dir, filename);
		flog = fopen(filepath, "a");
		fprintf(flog, "%s | State Machine Started: %s\n",
			timestr, machinep->name);
		fflush(flog);
		free(filepath);
		free(filename);
	}
}

static void smlog(SM_MACHINE* machinep, char* fmtp, ...) {
	va_list ap;
	char timestr[32];

	logtimestamp(timestr, sizeof(timestr));
	va_start(ap, fmtp);
	fprintf(flog, "%s | State Machine: %s|", timestr, machinep->name);
	vfprintf(flog, fmtp, ap);
	fprintf(flog, "\n");
	fflush(flog);
}

static void smclose_log() {
	fclose(flog);
}

#else

static void smopenlog(SM_MACHINE* machinep) {
	// Don't actually do this. It will just make ULPPK_LOG
	// output go to the wrong place!
	// openlog(machinep->name, LOG_PID, LOG_LOCAL1);
	return;
}

static void smlog(SM_MACHINE* machinep, char* fmtp, ...) {
	va_list		ap;

	char buff[1024];
	int prefix_len;

	va_start(ap, fmtp);
 	prefix_len = sprintf(buff, "%s|", machinep->name);
 	vsnprintf(buff + prefix_len, sizeof(buff)-prefix_len, fmtp, ap);
	// vsprintf(buff, fmtp, ap);
	va_end(ap);
	syslog(LOG_INFO, "%s", buff);
}

static void smclose_log() {
	closelog();
}

#endif
static void set_diag(SM_MACHINE* machinep, SM_ERROR err, const char* objname) {
	machinep->smerrno = err;
	memset(machinep->object_name, 0, sizeof(machinep->object_name));
	strncpy(machinep->object_name, objname, sizeof(machinep->object_name)-1);
}

static void push_txdiag(SM_MACHINE* machinep, SM_ERROR err, SM_EVENT_HANDLE evh) {
	SM_TX_ERROR txerror;
	SM_STATE_TABLE_DEF* stdefp;
	
	stdefp = get_stdefp(machinep);
	strncpy(txerror.event_name, stdefp->events[evh].event_name, 
		sizeof(txerror.event_name) -1);
	strncpy(txerror.state_name, stdefp->states[machinep->curr_state].state_name,
		sizeof(txerror.state_name) -1);
	txerror.smerror = err;
	
	// If the deque fills up, just let last errors fall off.
	dq_abd(&machinep->txerrors, &txerror);
}
	
static void clear_err(SM_MACHINE* machinep) {
	machinep->smerrno = SM_NO_ERROR;
	machinep->object_name[0] = 0;
	machinep->errtext[0] = 0;
}


// Determine if the event at handle evh is named by the parameter name.
static int is_named_event(SM_MACHINE* machinep, SM_EVENT_HANDLE evh, char* name) {
	int flag = 0;
	if (evh == SM_NULL_HANDLE) {
		return flag;
	}
	if (strcmp(get_stdefp(machinep)->events[evh].event_name, name) == 0) {
		flag = 1;
	}
	return flag;
}

static int is_null_event(SM_MACHINE* machinep, SM_EVENT_HANDLE evh) {
	return is_named_event(machinep, evh, EV_NULL);
}

static int is_abort_event(SM_MACHINE* machinep, SM_EVENT_HANDLE evh) {
	return is_named_event(machinep, evh, EV_ABORT);
}

static char* make_transition_name(char* buff, const char* s1name, 
	const char* evname, const char* s2name) {
	static char internalbuff[MAX_TRANSITION_NAME_LEN];
	char* buffp;
	char s1buff[MAX_STATE_NAME_LEN];
	char s2buff[MAX_STATE_NAME_LEN];
	char evbuff[MAX_EVENT_NAME_LEN];
	
	strncpy(s1buff, s1name, sizeof(s1buff) -1);
	strncpy(s2buff, s2name, sizeof(s2buff) -1);
	strncpy(evbuff, evname, sizeof(evbuff) -1);
	
	if (buff == NULL) {
		buffp = internalbuff;
	} else {
		buffp = buff;
	}
	sprintf(buffp, "%s-%s-%s", s1buff, evbuff, s2buff);
	return buffp;
}

static SM_ACTION_LIST_HANDLE find_action_list(SM_MACHINE *machinep, const char *actlist_name) {
	SM_ACTION_LIST_HANDLE alh = SM_NULL_HANDLE;
	SM_STATE_TABLE_DEF* stdefp;
	int i;

	stdefp = get_stdefp(machinep);
	for (i = 0; i < MAX_ACTION_LISTS_PER_MACHINE; i++) {
		if (strcmp(actlist_name, stdefp->actlists[i].al_name) == 0) {
			alh = stdefp->actlists[i].alh;
			break;
		}
	}
	return alh;
}

static SM_EVENT_HANDLE find_event(SM_MACHINE *machinep, const char *eventname) {
	SM_EVENT_HANDLE evh = SM_NULL_HANDLE;
	SM_STATE_TABLE_DEF* stdefp;
	int i;

	stdefp = get_stdefp(machinep);
	
	for (i = 0; i < MAX_EVENTS_PER_MACHINE; i++) {
		if (strcmp(eventname, stdefp->events[i].event_name) == 0) {
			evh = stdefp->events[i].eventh;
			break;
		}
	}
	return evh;
}
static SM_STATE_HANDLE find_state(SM_MACHINE *machinep, const char *statename) {
	SM_STATE_HANDLE sth = SM_NULL_HANDLE;
	SM_STATE_TABLE_DEF* stdefp;
	int i;

	stdefp = get_stdefp(machinep);
	
	for (i = 0; i < MAX_STATES_PER_MACHINE; i++) {
		if (strcmp(statename, stdefp->states[i].state_name) == 0) {
			sth = stdefp->states[i].stateh;
			break;
		}
	}
	return sth;
}

static unsigned char match_actname(void* p, LL_NODE* nodep) {
	SM_ACTION* actp;
	char* matchname;
	unsigned char match = 0;
	matchname = (char*)p;
	actp = (SM_ACTION*)nodep->data;
	if (strcmp(actp->action_name, matchname) == 0) {
		match = 1;
	}
	return match;
}

static SM_ACTION* find_action(SM_MACHINE *machinep, const char * actlist_name, 
	const char *action_name) {
	SM_ACTION_LIST_HANDLE alh = SM_NULL_HANDLE;
	SM_STATE_TABLE_DEF* stdefp;
	LL_HEAD* llhp;
	SM_ACTION* actp = NULL;
	LL_NODE* nodep;

	stdefp = get_stdefp(machinep);

	// Find the action list named by the caller.
	alh = find_action_list(machinep, actlist_name);
	if (SM_NULL_HANDLE == alh) {
		return NULL;			// action list not found!
	}
	llhp = &stdefp->actlists[alh].list;
	nodep = ll_search(llhp, (char*)action_name, match_actname);
	if (nodep != NULL) {
		actp = (SM_ACTION*)nodep->data;
	}
	return actp;
}

/**
 * @brief State machine event registration.
 *
 * Define your events first using this function.
 * @param machinep Pointer to SM_MACHINE structure for the statemachine
 * @param name Name of the event
 * @return SM_EVENT_HANDLE of the created event (an integer)
 */
int sm_register_event(SM_MACHINE *machinep, const char *name) {
	SM_EVENT_HANDLE h = SM_NULL_HANDLE;
	SM_STATE_TABLE_DEF* stdefp;
	clear_err(machinep);
	
	stdefp = get_stdefp(machinep);

	// Determine if event of this name is already registered.
	if (find_event(machinep, name) == SM_NULL_HANDLE) {
		if (stdefp->next_eventh < MAX_EVENTS_PER_MACHINE) {
			h = stdefp->next_eventh;
			stdefp->events[h].eventh = h;
			strncpy(stdefp->events[h].event_name, name, 
				sizeof(stdefp->events[h].event_name) - 1);
			stdefp->next_eventh++;
		}
	} else {
		set_diag(machinep, SM_REGISTER_DUPLICATE, name);
	}
	return h;
}

/**
 * @brief Register a state machine action list.
 *
 * Action list registration normally follows event registration.
 *
 * @param machinep Pointer to SM_MACHINE structure for the statemachine
 * @param name Name of the action list
 * @return SM_ACTION_LIST_HANDLE of the created action list (an integer)
 */
SM_ACTION_LIST_HANDLE sm_register_action_list(SM_MACHINE* machinep, const char* name) {
	SM_ACTION_LIST_HANDLE h = SM_NULL_HANDLE;
	SM_STATE_TABLE_DEF* stdefp;

	clear_err(machinep);
	stdefp = get_stdefp(machinep);

	// Determine if action list of this name is already registered
	if (find_action_list(machinep, name) == SM_NULL_HANDLE) {
		if (stdefp->next_alx < MAX_ACTION_LISTS_PER_MACHINE) {
			h = stdefp->next_alx;
			stdefp->actlists[h].alh = h;
			strncpy(stdefp->actlists[h].al_name, name, 
				sizeof(stdefp->actlists[h].al_name) - 1);
			ll_init(&stdefp->actlists[h].list);
			stdefp->next_alx++;
		}
	} else {
		set_diag(machinep, SM_REGISTER_DUPLICATE, name);
	}
	return h;
}

/**
 * @brief Register a state machine action by adding it to an action list.
 *
 * After action lists have been registered, register the actual
 * action handlers.
 *
 * @param machinep Pointer to SM_MACHINE structure for the statemachine
 * @param actlist_name Name of the action list to which the handler must be added.
 * @param name Name of the action handler
 * @param handlerp Pointer to the actual action handler function.
 * @return SM_ACTION_HANDLE of the created action handler (an integer)
 */
SM_ACTION_HANDLE sm_register_action(SM_MACHINE* machinep, const char* actlist_name, 
	const char* name, SM_ACTION_HANDLER* handlerp) {
	
	SM_ACTION_LIST_HANDLE alh;
	SM_STATE_TABLE_DEF* stdefp;
	LL_HEAD *llhp;
	SM_ACTION act;
	LL_NODE *nodep;
	SM_ACTION_HANDLE acth = SM_NULL_HANDLE;
	
	clear_err(machinep);
	
	stdefp = get_stdefp(machinep);

	alh = find_action_list(machinep, actlist_name);
	if (SM_NULL_HANDLE == alh) {
		set_diag(machinep, SM_PARENT_OBJECT_NOT_FOUND, actlist_name);
		return SM_NULL_HANDLE;
	}
	llhp = &stdefp->actlists[alh].list;
	
	// Determine if an action of this name is already registered
	// to this list. Actions can be registered to more than one list,
	// but not to the same list more than once.
	
	if (find_action(machinep, actlist_name, name) == NULL) {
		strncpy(act.action_name, name, sizeof(act.action_name) - 1);
		act.actionh = stdefp->next_acth++;
		act.handlerp = handlerp;
		
		// ll_new_node makes a copy of the passed buffer (act) and installs
		// it in a fresh ll_node ready for list insertion.
		nodep = ll_new_node(&act, sizeof(SM_ACTION));
		ll_addback(llhp, nodep);
		acth = act.actionh;
	} else {
		set_diag(machinep, SM_REGISTER_DUPLICATE, name);
	}
	return acth;
}

/*
 * @brief Register state machine states.
 *
 * States are registered after events, action lists, and action handlers.
 *
 * @param machinep Pointer to SM_MACHINE structure for the statemachine
 * @param name Name of the state
 * @return SM_STATE_HANDLE of the created event (an integer)
 */
SM_STATE_HANDLE sm_register_state(SM_MACHINE* machinep, const char* name) {
	SM_STATE_HANDLE sth = SM_NULL_HANDLE;
	SM_STATE* statep;
	SM_STATE_TABLE_DEF* stdefp;

	stdefp = get_stdefp(machinep);

	// Determine if a state by this name is already registered
	sth = find_state(machinep, name);
	if (SM_NULL_HANDLE == sth) {
		// Register the state and increment next_statex
		sth = stdefp->next_stateh++;
		statep = &stdefp->states[sth];
		statep->next_txh = 0;
		statep->stateh = sth;
		strncpy(statep->state_name, name, sizeof(statep->state_name) -1);
	} else {
		set_diag(machinep, SM_REGISTER_DUPLICATE, name);
	}
	return sth;
}

/**
 * @brief Register state transitions.
 *
 * Once events, action lists, action handlers, and states have been registered,
 * transitions between states can be registered.
 *
 * @param machinep Pointer to SM_MACHINE structure for the statemachine
 * @param s1name Name of the initial state
 * @param s2name Name of the final state
 * @param evname Name of the triggereding event
 * @param actlist_name Name of the action list to execute on receipt of the triggering event.
 * @return SM_TRANSITION_HANDLE of the created transition (an integer)
 */
SM_TRANSITION_HANDLE sm_register_transition(
	SM_MACHINE *machinep,		// state machine
	const char *s1name,			// name of the initial state
	const char *s2name,			// name of the final state
	const char* evname,			// name of the triggering event
	const char* actlist_name	// name of the action list to execute
) {
	SM_STATE_HANDLE st1h;
	SM_STATE_HANDLE st2h;
	SM_EVENT_HANDLE evh;
	SM_ACTION_LIST_HANDLE alh;
	SM_TRANSITION_HANDLE stxh;
	SM_TRANSITION* txp;
	SM_STATE_TABLE_DEF* stdefp;

	stdefp = get_stdefp(machinep);

	// Find initial and final states
	st1h = find_state(machinep, s1name);
	st2h = find_state(machinep, s2name);
	
	// Find triggering event
	evh = find_event(machinep, evname);

	// Find the action list
	alh = find_action_list(machinep, actlist_name);
	
	if (st1h == SM_NULL_HANDLE) {
		set_diag(machinep, SM_OBJECT_NOT_FOUND, s1name);
		return SM_NULL_HANDLE;
	}
	if (st2h == SM_NULL_HANDLE) {
		set_diag(machinep, SM_OBJECT_NOT_FOUND, s2name);
		return SM_NULL_HANDLE;
	}
	if (evh == SM_NULL_HANDLE) {
		set_diag(machinep, SM_OBJECT_NOT_FOUND, evname);
		return SM_NULL_HANDLE;
	}
	if (alh == SM_NULL_HANDLE) {
		set_diag(machinep, SM_OBJECT_NOT_FOUND, actlist_name);
		return SM_NULL_HANDLE;
	}
	
	// If we get here, all the named objects where found. Register the
	// transition if we have room.
	if (stdefp->states[st1h].next_txh >= MAX_TRANSITIONS_PER_STATE) {
		set_diag(machinep, SM_TOO_MANY_OBJECTS, 
			make_transition_name(NULL, s1name, evname, s2name));
		return SM_NULL_HANDLE;
	}
	stxh = stdefp->states[st1h].next_txh;
	stdefp->states[st1h].next_txh++;
	txp = &stdefp->states[st1h].transitions[stxh];
	txp->alisth = alh;
	txp->eventh = evh;
	txp->s1 = st1h;
	txp->s2 = st2h;
	make_transition_name(txp->transition_name, s1name, evname, s2name);
	return stxh;
}

/**
 * @brief Register a global transition.
 *
 * Global transitions are useful for common error handling.
 * For example, presume that all the states on the state/transition
 * graph must handle the event EV_MYERROR, and that after handling
 * the event the machine should land in the state MY_ERROR STATE.
 * A common action list can be composed for this error event,
 * Rather than register the event and action list for each
 * defined state, call sm_register_global_transition to apply the
 * transition to all the defined states. Restrictions:
 * 1) ALL states must be required to handle the triggering event.
 * 2) The same end state must be reached in all cases.
 * 3) The same action list must be applied to the triggering event.
 * 4) No other transition must be triggered by the event that triggers
 *		the global transition.
 *
 * @param machinep Pointer to SM_MACHINE structure for the statemachine
 * @param s2name Name of the final state
 * @param evname Name of the trigger event
 * @param actlist_name Name of the action list to be executed on receipt of the triggering event.
 * @ return 0 if successful and non-zero otherwise.
 */
int sm_register_global_transition(
	SM_MACHINE *machinep,		// state machine
	const char *s2name,			// name of the final state
	const char* evname,			// name of the triggering event
	const char* actlist_name	// name of the action list to execute
) {
	SM_STATE_HANDLE st1h;
	SM_STATE_TABLE_DEF* stdefp;
	SM_TRANSITION_HANDLE stxh;
	int retval = 0;

	stdefp = get_stdefp(machinep);
	for (st1h = 0; st1h < MAX_STATES_PER_MACHINE; st1h++) {
		stxh = sm_register_transition(
			machinep,
			stdefp->states[st1h].state_name,
			s2name,
			evname,
			actlist_name
		);
		if (SM_NULL_HANDLE == stxh) {
			retval = 1;
			break;
		}
	}
	return retval;
}

/**
 *
 * Register predefined events and other convenience objects
 * @param machinep Pointer to SM_MACHINE structure for the statemachine
 */
void sm_register_stock_defs(SM_MACHINE* machinep) {
	// Register pre-defined events
	// Init state is unnecessary: sm_register_state(sm_state_defp(machinep), STATE_INIT);
	
	sm_register_event(machinep, EV_NULL);
	sm_register_event(machinep, EV_INIT);
	sm_register_event(machinep, EV_ABORT);
	sm_register_event(machinep, EV_CLOCK);
}

/*
 * Prepare a fresh state machine object. If machinep is NULL, then
 * storage is allocated from the heap.
 *
 * @param machinep Pointer to SM_MACHINE structure for the statemachine
 * @param SM_STATE_TABLE_DEF*
 * @param machine_name Name of the state machine
 * @return SM_EVENT_HANDLE of the created event (an integer)
 */
SM_MACHINE* sm_new_machine(SM_MACHINE* machinep, SM_STATE_TABLE_DEF* stdefp, const char* machine_name) {
	if (machinep == NULL) {
		machinep = (SM_MACHINE*)calloc(1, sizeof(SM_MACHINE));
	} else {
		memset(machinep, 0, sizeof(SM_MACHINE));
	}
	machinep->stdefp = stdefp;
	strncpy(machinep->name, machine_name, sizeof(machinep->name) -1);
	dq_init(MACHINE_EVENT_STACK_LEN, sizeof(SM_EVENT_HANDLE), &machinep->event_queue);
	dq_init(MACHINE_ERROR_QUEUE_LEN, sizeof(SM_TX_ERROR), &machinep->txerrors);

	machinep->curr_state = 0;
	machinep->prev_state = machinep->curr_state;
	machinep->last_event = find_event(machinep, EV_INIT);
	
	SMOPENLOG(machinep);
	return machinep;
}

/**
 * @brief Reset a state machine object. This is like sm_new_machine, except you
 * don't have to re-establish the states, events, actions and such.
 * @param machinep Pointer to SM_MACHINE structure for the statemachine
 *
 */
void sm_reset_machine(SM_MACHINE* machinep) {
	if (machinep != NULL) {
		// machinep->curr_state = find_state(machinep, STATE_INIT);
		machinep->curr_state = 0;
		machinep->prev_state = machinep->curr_state;
		machinep->last_event = find_event(machinep, EV_INIT);
		dq_init(MACHINE_EVENT_STACK_LEN, sizeof(SM_EVENT_HANDLE), &machinep->event_queue);
		dq_init(MACHINE_ERROR_QUEUE_LEN, sizeof(SM_TX_ERROR), &machinep->txerrors);
		clear_err(machinep);
	}
}

/**
 *
 * @brief The main transition processor. Should not be called until machine has been
 * started by calling sm_set_definition_complete to mark the definition of the
 * state/transition tables to be complete. The machine is currently in state s1.
 * The incoming event named by event_name selects a transition. If no
 * transition matching the incoming event is found, no transition occurs
 * but the function and sm_errno(machinep) return non-zero. The datap is provided
 * to the action handlers which are members of the transition's action list.
 * Any action handler can return the events EVH_NULL, meaning no event, or EV_ABORT
 * indicating the machine is in a bad state. Action handlers may also return
 * custom events. These will get pushed onto the event queue. After the transition
 * is complete, the event queue is serviced, which may trigger additional transitions.
 *
 * @param machinep Pointer to SM_MACHINE structure for the statemachine
 * @param event_name Name of the incoming event
 * @param datap Application defined data to be passed to action handler functions during transition
 * @return an SM_ERROR code (SM_NO_ERROR = 0 indicates success)
 */
SM_ERROR sm_transition(SM_MACHINE* machinep, const char* event_name, void* datap) {
	SM_EVENT_HANDLE evh = SM_NULL_HANDLE;
	SM_EVENT_HANDLE queue_evh = SM_NULL_HANDLE;
	SM_TRANSITION* txp;
	SM_STATE_TABLE_DEF* stdefp;
	SM_TRANSITION_HANDLE txh;
	SM_STATE* state1p;
	LL_NODE* nodep;
	LL_HEAD* llhp;
	SM_ERROR status = SM_NO_ERROR;
	
	// Find the event
	evh = find_event(machinep, event_name);
	if (SM_NULL_HANDLE == evh) {
		set_diag(machinep, SM_OBJECT_NOT_FOUND, event_name);
		return SM_OBJECT_NOT_FOUND;
	}
	
	if (is_null_event(machinep, evh)) {
		return SM_NO_ERROR;
	}
	if (is_abort_event(machinep, evh)) {
		SMLOG(machinep, "Abort Event Encountered");
		return SM_MACHINE_ABORT;
	}
	
	stdefp = get_stdefp(machinep);
	if (!sm_get_definition_complete(machinep)) {
		SMLOG(machinep, "State table definition not complete! Machine name [%s]", machinep->name);
		set_diag(machinep, SM_MACHINE_DEF_NOT_COMPLETE, machinep->name);
		return SM_MACHINE_DEF_NOT_COMPLETE;
	}

	state1p = &stdefp->states[machinep->curr_state]; 	// point to current state
		
	// Now select transition
	for (txh = 0, txp = NULL; txh < state1p->next_txh; txh++) {
		if (state1p->transitions[txh].eventh == evh) {
			// Found the matching state transition ...
			// Execute the action list
			txp = &state1p->transitions[txh];
			llhp = &stdefp->actlists[txp->alisth].list;	// get ptr to linked list 
			nodep = llhp->ll_fptr;				// point to first node on list
			if (!is_named_event(machinep, evh, EV_CLOCK)) {
				// Don't log clock pulses
				SMLOG(machinep, "S1 [%s] E [%s] S2 [%s] AL [%s]",
					state1p->state_name, event_name,
					stdefp->states[txp->s2].state_name, 
					&stdefp->actlists[txp->alisth].al_name);
			}
			while (nodep != NULL) {
				SM_ACTION* actp;
				SM_EVENT_HANDLE action_evh = SM_NULL_HANDLE;
				
				actp = (SM_ACTION*)nodep->data;
				action_evh = actp->handlerp(machinep, datap);
				if (!is_named_event(machinep, evh, EV_CLOCK)) {
					// Don't log clock pulse actions .. 
					SMLOG(machinep, "\tACTION: %s ==> Event %s",
						actp->action_name, stdefp->events[action_evh].event_name);
				}
				dq_abd(&machinep->event_queue, &action_evh);
				nodep = ll_iterate(nodep);
			}
			// Now set new machine state
			machinep->prev_state = machinep->curr_state;
			machinep->curr_state = txp->s2;
			machinep->last_event = txp->eventh;
			break;			// break out of the for
		}
	}
		
	// Process any events stacked up while processing the action list
	// This necessitates a recursive call to sm_transition_machine!
	while ((status == SM_NO_ERROR) && (!dq_rtd(&machinep->event_queue, &queue_evh))) {
		status = sm_transition(machinep, 
			stdefp->events[queue_evh].event_name, datap);
	}
	if (SM_NO_ERROR != status) {
		set_diag(machinep, status, machinep->name);
		push_txdiag(machinep, status, queue_evh);
	}
	return status;
}

/**
 * @brief Given a state machine and a event name string, retrieve the event
 * handle.
 *
 * @param machinep Pointer to SM_MACHINE structure for the statemachine
 * @param evname Name of the event of interest.
 * @returns SM_NULL_HANDLE if not found. Otherwise, event handle.
 *
 */
SM_EVENT_HANDLE sm_event_handle(SM_MACHINE* machinep, const char* evname) {
	return find_event(machinep, evname);
}

/**
 *
 * @brief Retrieves the name string of the current machine state
 *
 * @param machinep Pointer to SM_MACHINE structure for the statemachine
 * @return Name of the current machine state/
 */
char* sm_curr_state(SM_MACHINE* machinep) {
	return machinep->stdefp->states[machinep->curr_state].state_name;
}

/**
 * @brief Return the last machine error posted on the machine.
 *
 * @param machinep Pointer to SM_MACHINE structure for the statemachine
 * @return an SM_ERROR code (SM_NO_ERROR = 0 indicates success)
 */
SM_ERROR sm_errno(SM_MACHINE* machinep) {
	return machinep->smerrno;
}

/**
 * @brief Convenience function for getting string interpretation of an error code
 *
 * @param errcode Numeric error code (see statemachine.h type SM_ERROR)
 * @return Error text message string.
 */
char* sm_strerror(SM_ERROR errcode) {
	static char errtext[1024];
	char* object_name = "NILL";
	switch (errcode) {
		case SM_NO_ERROR:
			sprintf(errtext, "%s", errfmt[SM_NO_ERROR]);
			break;
		default:
			if (errcode < SM_LAST_ERR) {
				sprintf(errtext, errfmt[errcode], object_name);
			} else {
				sprintf(errtext, "Unknown error code [%d] object name [%s]",
					errcode, object_name);
			}
			break;
	}
	return errtext;
}

/**
 * @brief Format error text. Updates the errtext field of the state machine structure.
 * @param machinep Pointer to SM_MACHINE structure for the statemachine
 * @return Pointer to formatted text string.
 */
char* sm_strdiag(SM_MACHINE* machinep) {
	switch (machinep->smerrno) {
		case SM_NO_ERROR:
			sprintf(machinep->errtext, "%s", errfmt[SM_NO_ERROR]);
			break;
		default:
			if (machinep->smerrno < SM_LAST_ERR) {
				sprintf(machinep->errtext, errfmt[machinep->smerrno], machinep->object_name);
			} else {
				sprintf(machinep->errtext, "Unknown error code [%d] object name [%s]",
					machinep->smerrno, machinep->object_name);
			}
			break;
	}
	return machinep->errtext;
}

/**
 * Called to retrieve errors stacked on a state machine's error collection.
 * Useful for diagnosing problems with the state transition table.
 * @param machinep Pointer to SM_MACHINE structure for the statemachine
 * @return Pointer to transition diagnostic text.
 */
char* sm_pop_tx_error(SM_MACHINE* machinep) {
	static char buff[1024];
	char* buffp = NULL;
	SM_TX_ERROR txerror;
	char save_object_name[MAX_TRANSITION_NAME_LEN];
	
	
	if (!dq_rtd(&machinep->txerrors, &txerror)) {
		// Popped off an error
		buffp = buff;
		strcpy(save_object_name, machinep->object_name);
		strncpy(machinep->object_name, txerror.event_name, sizeof(machinep->object_name) -1);
		sprintf(buff, "TX Error from State: %s Event: %s Errno %d ErrString %s",
			txerror.state_name, txerror.event_name, txerror.smerror, 
			sm_strerror(txerror.smerror));
		strcpy(machinep->object_name, save_object_name);
	}
	return buffp;
}
/**
 * @brief Set the state definition complete flag for the machine's state definition
 * table. Note that this will affect ALL machines that share a particular
 * state definition table.
 * @param machinep Pointer to SM_MACHINE structure for the statemachine
 */
void sm_set_definition_complete(SM_MACHINE* machinep) {
	machinep->stdefp->statedef_complete = 1;
}

/**
 *
 * @brief Get the state table definition complete flag for a particular machine
 * @param machinep Pointer to SM_MACHINE structure for the statemachine
 * @return 1 if statemachine definition has been posted as being compelte, 0 otherwise.
 */
int sm_get_definition_complete(SM_MACHINE* machinep) {
	return machinep->stdefp->statedef_complete;
}

/** @brief
 *
 * Closes the state machine. After this, machinep MUST be reset using
 * sm_reset_machine if you want to re-use the machine structure
 * @param machinep Pointer to SM_MACHINE structure for the statemachine
 */
void sm_close_machine(SM_MACHINE* machinep) {
	SMCLOSELOG
}

