#ifndef CDX_SOCKET_UTILS_H
#define CDX_SOCKET_UTILS_H

#define TIMEOUT_ONE_SECS 1000000L

#ifdef __cplusplus
extern "C"
{
#endif

int CdxSockSetBlocking(int sockfd, int blocking);

int CdxSockAsynConnect(int sockfd, const struct sockaddr *addr, socklen_t addrlen,
                        long timeoutUs/* microseconds */, int *pForceStop);


ssize_t CdxSockAsynSend(int sockfd, const void *buf, size_t len, 
                        long timeoutUs/* microseconds */, int *pForceStop);


ssize_t CdxSockAsynRecv(int sockfd, void *buf, size_t len, 
                        long timeoutUs/* microseconds */, int *pForceStop);

#ifdef __cplusplus
}
#endif

#endif
