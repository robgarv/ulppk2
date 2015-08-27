#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include <ulppk_log.h>

ssize_t sio_readn(int fd, void *vptr, size_t n) {
	size_t nleft;
	ssize_t nread;
	char* ptr;
	
	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			switch (errno) {
			case EWOULDBLOCK:
				return -2;
			case EINTR:
				nread = 0;
				break;
			default:
				return -1;
			}
		} else if (nread == 0) {
			break;			// EOF
		}
		nleft -=nread;
		ptr += nread;
	}
	return (n - nleft);		// return >= 0 
}

ssize_t sio_writen(int fd, void* vptr, size_t n) {
	size_t nleft;
	ssize_t nwritten;
	const char* ptr;
	
	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ((nwritten = write(fd, ptr, nleft)) <= 0) {
			if (errno == EINTR) {
				nwritten = 0; 			// and call write again
			} else {
				return -1;				// error
			}
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return n;	
}
ssize_t sio_readline(int fd, void* vptr, size_t maxlen) {
	ssize_t n, rc;
	char c, *ptr;
	
	ptr = vptr;
	for (n = 1; n < maxlen; n++) {
		again:
			if ((rc = read(fd, &c, 1)) == 1) {
				*ptr++ = c;
				if (c == '\n') {
					break;				// new line is stored, like fgets
				}
			} else if (rc == 0) {
				if (n ==  1) {
					return 0;			// EOF ... no data read
				}
			} else {
				if (errno == EINTR) {
					goto again;
				}
				return -1;				// Error ... errno set by read
			}
	}	
	*ptr = 0;							// null terminate like fgets
	return n;
}

struct sockaddr_in* sio_getpeername(int fd) {
	static struct sockaddr_in addr;
	struct sockaddr* addrp = NULL;
	socklen_t addrsize;

	addrp = (struct sockaddr*)&addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addrsize = (socklen_t)sizeof(addr);
	if (getpeername(fd, addrp, &addrsize)) {
		addrp = NULL;
	}
	return (struct sockaddr_in*)addrp;
}

int sio_connect(char* host, int port) {
	static struct sockaddr_in server_addr;
	static struct sockaddr* server_addrp = (struct sockaddr*)&server_addr;
	int sockfd;

	sockfd = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (sockfd != -1) {
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(port);
		server_addr.sin_addr.s_addr = inet_addr(host);
		if (connect(sockfd,server_addrp,sizeof(struct sockaddr_in))) {
			close(sockfd);
			sockfd = -1;
		}
	}
	return sockfd;
}

int sio_connectbyhostname(char* hostname, int port) {
	int sockfd;
	struct addrinfo hints;
	struct addrinfo *res;
	struct addrinfo *p;
	int status;
	char ipstr6[INET6_ADDRSTRLEN];
	char ipstr4[INET_ADDRSTRLEN];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(hostname, NULL, &hints, &res)) != 0) {
		ULPPK_LOG(ULPPK_LOG_ERROR, "getaddrinfo fails: hostname: [%s] error: [%s]", hostname, gai_strerror(status));
		return -1;
	}

	// hostname lookup successful ... perform connection attempt using
	// first IP in the list. Note: At this point, socketio.c is not IPv6
	// compatible. (We need to fix that fast.) So we'll scan the list
	// and use the first IPv4 IP we find.

	for (p = res; p != NULL; p = p->ai_next) {
		void* addr;
		if (p->ai_family == AF_INET) {
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);

			/* convert the IP to a string and print it: */
			inet_ntop(p->ai_family, addr, ipstr4, sizeof ipstr4);
			sockfd = sio_connect(ipstr4, port);
			freeaddrinfo(res);
			return sockfd;
		}
	}
	freeaddrinfo(res);

	// Error if we get here. Did not find a IPv4 IP
	ULPPK_LOG(ULPPK_LOG_ERROR, "Hostname not found or does not resolve to IPv4 IP: hostname: [%s] port: [%d] error: [%s]", hostname, port, strerror(errno));
	return -1;
}
