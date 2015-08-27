#include <signal.h>

#include "signalkit.h"

SIGNAL_FUNCTION* set_signal(int signo, SIGNAL_FUNCTION sigfunc) {

	struct sigaction act;
	struct sigaction oact;
	
	act.sa_handler = sigfunc;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (SIGALRM == signo) {
#ifdef SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;	
#endif
	} else {
#ifdef SA_RESTART
		act.sa_flags |= SA_RESTART;
#endif
	}
	if (sigaction(signo, &act, &oact) < 0) {
		return SIG_ERR;
	} else {
		return oact.sa_handler;
	}
}
