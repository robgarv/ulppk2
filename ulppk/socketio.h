#ifndef SOCKETIO_H_
#define SOCKETIO_H_

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

/*
 * Simple utilities for socket I/O. Borrowed/apdated from Stevens
 */
#ifdef __cplusplus
extern "C" {
#endif

ssize_t sio_readn(int fd, void *vptr, size_t n);
ssize_t sio_writen(int fd, void* vptr, size_t n);
ssize_t sio_readline(int fd, void* vptr, size_t maxlen);
struct sockaddr_in* sio_getpeername(int fd);
int sio_connect(char* host, int port);
int sio_connectbyhostname(char* hostname, int port);

#ifdef __cplusplus
}
#endif
#endif /*SOCKETIO_H_*/
