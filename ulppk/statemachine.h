/*
 *****************************************************************

<GPL>

Copyright: © 2001-2015 Robert C Garvey

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

#ifndef STATEMACHINE_H_
#define STATEMACHINE_H_

#include "llacc.h"
#include "dqacc.h"

/**
 *
 * 
 * @file statemachine.h
 * 
 * @brief This package provides definitions and a framework for implementing
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
 */
 
#define MAX_SM_NAME_LEN 32
#define MAX_EVENT_NAME_LEN MAX_SM_NAME_LEN
#define MAX_ACTION_NAME_LEN MAX_SM_NAME_LEN
#define MAX_STATE_NAME_LEN MAX_SM_NAME_LEN
#define MAX_TRANSITION_NAME_LEN ((2 * MAX_STATE_NAME_LEN) + MAX_EVENT_NAME_LEN + 2)
#define MAX_TRANSITIONS_PER_STATE 32
#define MAX_STATES_PER_MACHINE 128
#define MAX_EVENTS_PER_MACHINE 256
#define MAX_ACTION_LISTS_PER_MACHINE 256

#define MACHINE_EVENT_STACK_LEN MAX_TRANSITIONS_PER_STATE
#define MACHINE_ERROR_QUEUE_LEN 16

// Object handle definitions

#define SM_NULL_HANDLE -1

typedef int SM_EVENT_HANDLE;
typedef int SM_ACTION_LIST_HANDLE;
typedef int SM_STATE_HANDLE;
typedef int SM_TRANSITION_HANDLE;
typedef int SM_ACTION_HANDLE;

// Pre-defined event handles 
#define EV_NULL "EVnull"
#define EV_ABORT "EVabort"
#define EV_INIT "EVinit"
#define EV_CLOCK "EVClock"

// Pre-defined state names
#define STATE_INIT "initial-state"

/** State machine error codes
 *
 */
typedef enum {
	SM_NO_ERROR = 0,				///< No error
	SM_REGISTER_DUPLICATE,			///< Attempt to register object of duplicate type and name
	SM_OBJECT_NOT_FOUND,			///< Could not find the name object in the tables
	SM_PARENT_OBJECT_NOT_FOUND,		///< The allege parent object could not be found
	SM_TOO_MANY_OBJECTS,			///< Registered too many objects ... increase limits and recompile
	SM_MACHINE_ABORT,				///< The state machine encountered an abort condition
	SM_MACHINE_DEF_NOT_COMPLETE,	///< The state machine definition is not complete and a start was attempted
	SM_LAST_ERR						// add new error codes above here and text to errfmt
} SM_ERROR;

// State machine objects

/**
 * State machine event structure definition.
 */
typedef struct _event_struct {
	SM_EVENT_HANDLE eventh;
	char event_name[MAX_EVENT_NAME_LEN];
} SM_EVENT;

/**
 * State machine action list structure definition
 */
typedef struct _action_list {
	SM_ACTION_LIST_HANDLE alh;
	char al_name[MAX_ACTION_NAME_LEN];
	LL_HEAD list;						///< A linked list of action handler definitions.
} SM_ACTION_LIST;
 
/**
 * State machine transition structure.
 */
typedef struct _transition_struct {
	char transition_name[MAX_TRANSITION_NAME_LEN];
	SM_EVENT_HANDLE eventh;
	SM_ACTION_LIST_HANDLE alisth;
	SM_STATE_HANDLE s1;
	SM_STATE_HANDLE s2;
} SM_TRANSITION;

/**
 * State machine state definition.
 */
typedef struct _state_struct {
	SM_STATE_HANDLE stateh;							///< state handle
	char state_name[MAX_STATE_NAME_LEN];			///< name of  the state
	int next_txh;							///< next open slot in transition table
	SM_TRANSITION transitions[MAX_TRANSITIONS_PER_STATE];	///< list of transitions out of this state
} SM_STATE;

/**
 * State machine transition error.
 */
typedef struct {
	SM_ERROR smerror;
	char state_name[MAX_STATE_NAME_LEN];
	char event_name[MAX_EVENT_NAME_LEN];
} SM_TX_ERROR;

/**
 * Distinguish between the state table defintions, given in structure
 * SM_STATE_TABLE_DEF, and the state machine that operates from it.
 * This is  because multiple statemachines (representing the states of
 * different object instances) can be running at the same time.
 * Note that the statedef_complete flag must be set before it is safe
 * to run the machine(s) by calling sm_transition. Function
 * sm_set_definition_complete must be called after all state table
 * objects (states, events, action lists, actions, and transitions)
 * ave been registered.
 *
 * Another way of looking at this: A SM_STATE_TABLE_DEF defines a schema.
 * A SM_MACHINE structure defines a process instance executing that schema.
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
 * machine2p = sm_new_machine(NULL, &mydefs, "mymachine2"
 *
 */
typedef struct {
	char name[MAX_SM_NAME_LEN];
	unsigned int statedef_complete;				///< state definition complete flag
	SM_STATE_HANDLE next_stateh;				///< next open slot in state table
	SM_EVENT_HANDLE next_eventh;				///< next open slot in event table
	SM_ACTION_LIST_HANDLE next_alx;				///< next open slot in action list table
	SM_ACTION_HANDLE next_acth;					///< next action handle
	SM_STATE states[MAX_STATES_PER_MACHINE];
	SM_EVENT events[MAX_EVENTS_PER_MACHINE];
	SM_ACTION_LIST actlists[MAX_ACTION_LISTS_PER_MACHINE];
} SM_STATE_TABLE_DEF;

/**
 * Definition of a state machine.
 */
typedef struct _state_table {
	char name[MAX_SM_NAME_LEN];
	SM_STATE_HANDLE curr_state;					///< handle of current state
	SM_STATE_HANDLE prev_state;					///< handle of previous state
	SM_EVENT_HANDLE last_event;					///< event that triggered prev-to-curr transition
	SM_STATE_TABLE_DEF* stdefp;					///< pointer to state definition table
	DQHEADER event_queue;						///< A queue of events produced by a transition
	DQHEADER txerrors;							///< A queue of errors produced by a transition
	SM_ERROR smerrno;							///< error number produced by last state machine op
	char errtext[1024];							///< error text
	char object_name[MAX_TRANSITION_NAME_LEN];	///< name of object associated with error
} SM_MACHINE;

typedef SM_EVENT_HANDLE SM_ACTION_HANDLER(SM_MACHINE* machinep, void* datap);

/**
 * An action handler structure. These are added to the linked list
 * member of SM_ACTION_LIST structures. (Field list)
 */
typedef struct _action_struct {
	SM_ACTION_HANDLE actionh;
	char action_name[MAX_ACTION_NAME_LEN];
	// SM_EVENT_HANDLE (*action_func) (void* user_datap);
	SM_ACTION_HANDLER* handlerp;
} SM_ACTION;

#ifdef __cplusplus
extern "C" {
#endif

// State machine object registration. First, define the events. Then the 
// action lists. Then the actions that are members of the action lists.
// Finally define the states and then the transitions.

SM_EVENT_HANDLE sm_register_event(SM_MACHINE *machinep, const char *name);
SM_ACTION_LIST_HANDLE sm_register_action_list(SM_MACHINE* machinep, const char *name);
SM_ACTION_HANDLE sm_register_action(SM_MACHINE *machinep, const char *actlist_name,  
	const char* name, SM_ACTION_HANDLER* handlerp); 
SM_STATE_HANDLE sm_register_state(SM_MACHINE *machinep, const char *name);
SM_TRANSITION_HANDLE sm_register_transition(
	SM_MACHINE *machinep,			// state machine
	const char *s1name,					// name of the initial state
	const char *s2name,					// name of the final state
	const char* evname,					// name of the triggering event
	const char* actlist_name				// name of the action list to execute
);

// Global transitions are useful for common error handling.
// For example, presume that all the states on the state/transition
// graph must handle the event EV_MYERROR, and that after handling
// the event the machine should land in the state MY_ERROR STATE.
// A common action list can be composed for this error event,
// Rather than register the event and action list for each 
// defined state, call sm_register_global_transition to apply the 
// transition to all the defined states. Restrictions:
// 1) ALL states must be required to handle the triggering event.
// 2) The same end state must be reached in all cases.
// 3) The same action list must be applied to the triggering event.
// 4) No other transition must be triggered by the event that triggers
//		the global transition.
//
// Returns 0 if successful and non-zero otherwise.
//
int sm_register_global_transition(
	SM_MACHINE *machinep,		// state machine
	const char *s2name,			// name of the final state
	const char* evname,			// name of the triggering event
	const char* actlist_name	// name of the action list to execute
);

// Register predefined events and other convenience objects
void sm_register_stock_defs(SM_MACHINE* machinep);

// Prepare a fresh state machine object. If machinep is NULL, storage is
// allocated from the heap. 
SM_MACHINE* sm_new_machine(SM_MACHINE* machinep, SM_STATE_TABLE_DEF* stdefp, const char* machine_name);

// Set the state definition complete flag for the machine's state definition 
// table. Note that this will affect ALL machines that share a particular 
// state definition table.
void sm_set_definition_complete(SM_MACHINE* machinep);

// Get the state table definition complete flag for a particular machine
int sm_get_definition_complete(SM_MACHINE* machinep);

// Reset a state machine object. This is like sm_new_machine, except you
// don't have to re-establish the states, events, actions and such.
void sm_reset_machine(SM_MACHINE* machinep);

// The main transition processor
SM_ERROR sm_transition(SM_MACHINE* machinep, const char* event_name, void* datap);

// Given a state machine and a event name string, retrieve the event
// handle. Returns SM_NULL_HANDLE if not found.
SM_EVENT_HANDLE sm_event_handle(SM_MACHINE* machinep, const char* evname);

// Retrieves the name string of the current machine state
char* sm_curr_state(SM_MACHINE* machinep);

SM_ERROR sm_errno(SM_MACHINE* machinep);

// Called to retrieve error string for simple errors.
char* sm_strerror(SM_ERROR smerrno);

// Called to retrieve error string for posted diagnostics. Call this when
// sm_errrno returns something other than SM_NO_ERROR
char* sm_strdiag(SM_MACHINE* machinep);

// Called to retrieve errors stacked on a state machine's error collection.
// Useful for diagnosing problems with the state transition table.
char* sm_pop_tx_error(SM_MACHINE* machinep);

// Closes the state machine. After this, machinep MUST be reset using
// sm_reset_machine if you want to re-use the machine structure
void sm_close_machine(SM_MACHINE* machinep);

#ifdef __cplusplus
}
#endif

// Helper macros
#define SMDEFNAME(a) const char* a = #a;

#define SMDECLARE_NAME(a) extern const char* a;

// Used when you want to refer to a pre-registered event (EV_NULL, EV_ABORT, EV_CLOCK, etc.)
// by another name. 
#define SMALIAS_NAME(a,b) const char* a = b;

// Convenience macros for referring to stock events. They presume
// that machinep points to the state machine structure.

#define EV_NULL_HANDLE sm_event_handle(machinep, EV_NULL)
#define EV_ABORT_HANDLE sm_event_handle(machinep, EV_ABORT)
#define EV_CLOCK_HANDLE sm_event_handle(machinep, EV_CLOCK)
#define EV_INIT_HANDLE sm_event_handle(machinep, EV_INIT)

#endif /*STATEMACHINE_H_*/
