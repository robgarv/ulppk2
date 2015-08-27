#ifndef SIGNALKIT_H_
#define SIGNALKIT_H_

// A typedef for signal handler functions
typedef void SIGNAL_FUNCTION(int);

#ifdef __cplusplus
extern "C" {
#endif

SIGNAL_FUNCTION* set_signal(int signo, SIGNAL_FUNCTION sigfunc);

#ifdef __cplusplus
}
#endif

#endif /*SIGNALKIT_H_*/
