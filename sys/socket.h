#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "types.h"

typedef Uint32 in_addr_t;
typedef Uint32 socklen_t;
typedef Uint32 size_t;
typedef Sint32 ssize_t;

typedef struct Socket
{
    Uint16 afinet;     /* DC H'2'    address family*/
    Uint16 port;       /* DC H'3000' port to connect to or listen at*/
    in_addr_t ipaddr;  /* DC F'0'    INET address (netid)*/
    Uint8 reserved[8]; /* DC XL8'00' Reserved area not used*/
} Socket;

extern void ez_begin(void);
extern void ez_end(void);

/* Convert Internet host address from numbers-and-dots notation in CP
   into binary data in network byte order.  */
extern in_addr_t inet_addr(const char *srcaddr);
extern Sint32 ez_pton(const char *srcaddr, in_addr_t *dstaddr, Sint32 *errno);

/* Convert Internet number in IN to ASCII representation.  The return value
   is a pointer to an internal array containing the string.  */
extern char *inet_ntoa(in_addr_t srcaddr);
extern Sint32 ez_ntop(Uint32 *srcaddr, char *dstaddr, Sint32 *errno);

/* Create a new socket of type TYPE in domain DOMAIN, using
   protocol PROTOCOL.  If PROTOCOL is zero, one is chosen automatically.
   Returns a file descriptor for the new socket, or -1 for errors.  
   domain = AF_INET 
   type = SOCK_STREAM | SOCK_DGRAM
   protocol = 0
*/
extern int socket(Sint32 domain, Sint32 type, Sint32 protocol);
extern Sint32 ez_socket(Sint32 *errno);

extern int connect(Sint16 fd, Socket *socket, socklen_t len);
extern Sint32 ez_conn(Sint16 *desc, Socket *socket, Sint32 *errno);

extern Sint32 ez_close(Sint16 *desc, Sint32 *errno);

/* Send N bytes of BUF to socket FD.  Returns the number sent or -1. */
extern ssize_t send(Sint16 fd, const void *buf, size_t nbyte, int flags);
extern Sint32 ez_send(Sint16 *desc, Uint32 nbyte, void *buf, Sint32 *errno);

/* Read N bytes into BUF from socket FD.
   Returns the number read or -1 for errors. */
extern ssize_t recv(int fd, void *buf, size_t nbyte, int flags);
extern Sint32 ez_recv(Sint16 *desc, Uint32 nbyte, void *buf, Sint32 *errno);

/* Return entry from host data base which address match ADDR with
length LEN and type TYPE.

 */
struct hostent *gethostbyaddr(const void *addr, socklen_t len, int type);
extern Sint32 ez_getha(char *hostaddr, char *hostent, Sint32 *errno);

/* Return entry from host data base for host with NAME. */
struct hostent *gethostbyname(const char *name);
extern Sint32 ez_gethn(char *name, char *hostent, Sint32 *errno);

/* Put the address of the peer connected to socket FD into *ADDR
(which is *LEN bytes long), and its actual length into *LEN. */
extern Sint32 getpeername(Sint16 fd, Socket *socket, socklen_t *len);
extern Sint32 ez_getpn(Sint16 *desc, Socket *socket, Sint32 *errno);

#endif
