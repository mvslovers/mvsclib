#ifndef __EZASMI_H__
#define __EZASMI_H__

typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef unsigned int Uint32;

typedef char Sint8;
typedef short Sint16;
typedef int Sint32;

typedef struct Socket
{
    Uint16 afinet;     /* DC H'2'    address family*/
    Uint16 port;       /* DC H'3000' port to connect to or listen at*/
    Uint32 ipaddr;     /* DC F'0'    INET address (netid)*/
    Uint8 reserved[8]; /* DC XL8'00' Reserved area not used*/
} Socket;

void ez_begin(void);
void ez_end(void);

Sint32 ez_socket(Sint32 *errno);

Sint32 ez_conn(Sint16 *desc, Socket *socket, Sint32 *errno);
Sint32 ez_close(Sint16 *desc, Sint32 *errno);

Sint32 ez_send(Sint16 *desc, Uint32 nbyte, void *buf, Sint32 *errno);
Sint32 ez_recv(Sint16 *desc, Uint32 nbyte, void *buf, Sint32 *errno);

Sint32 ez_pton(char *srcaddr, Uint32 *dstaddr, Sint32 *errno);
Sint32 ez_ntop(Uint32 *srcaddr, char *dstaddr, Sint32 *errno);

Sint32 ez_getha(char *hostaddr, char *hostent, Sint32 *errno);
Sint32 ez_gethn(char *name, char *hostent, Sint32 *errno);

Sint32 ez_getpn(Sint16 *desc, Socket *socket, Sint32 *errno);

#endif
