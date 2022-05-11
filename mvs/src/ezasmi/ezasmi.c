#include <string.h>
#include <stdio.h>
#include "ezasmi.h"

/*******************/
/* WORKING - Alpha */
/*******************/

/* Initialize EZASMI */
void ez_begin(void)
{
    //asm("WTO   'INITAPI'");
    asm("XC EZASMTIE(TIELENTH),EZASMTIE  clear EZASMI storage\n\t "
        "LA    R15,EZASMTIE              R15 = A(Task storage area)\n\t"
        "LA    R1,T#PARMS-EZASMTIE(R15)  R1 = A(EZASOH03 parm list)\n\t"
        "LA    R0,=C'INIT'\n\t"
        "ST    R0,0(,R1)                 Pt to INIT function ID\n\t"
        "L     R15,=V(EZASOH03)          Invoke EZASOH03\n\t"
        "BALR  R14,R15");
}

/* Terminate EZASMI */
void ez_end(void)
{
    //asm("WTO    'TERMAPI'");
    asm("LA    R15,EZASMTIE             R15 = A(Task storage area)\n\t"
        "LA    R1,T#PARMS-EZASMTIE(R15) R1 = A(EZASOH03 parm list)\n\t"
        "LA    R0,=C'TERM'              R0 = A(TERM function ID)\n\t"
        "ST    R0,0(,R1)                Pt to TERM function ID\n\t"
        "LA    R0,T#WORK-EZASMTIE(R15)  Pt to working storage\n\t"
        "ST    R0,4(,R1)\n\t"
        "L     R15,=V(EZASOH03)         Invoke EZASOH03\n\t"
        "BALR  R14,R15");
}

/* Create socket */
Sint32 ez_socket(Sint32 *errno)
{
    // https://www.ibm.com/docs/en/zvse/6.2?topic=SSB27H_6.2.0/fa2ti_ezasmi_socket.html
    Sint32 retcode = 0;

    //asm("WTO    'SOCKET'");
    asm("EZASMI TYPE=SOCKET,AF='INET',SOCTYPE='STREAM',                +\n"
        "\t      ERRNO=(%[errno]),RETCODE=(%[retcode])"
        : /* No output operands */
        : /* Input operands */
        [errno] "r"(errno),
        [retcode] "r"(&retcode)
        : "0", "1", "15");
    return retcode;
}

/* Connect */
Sint32 ez_conn(Sint16 *desc, Socket *socket, Sint32 *errno)
{
    // https://www.ibm.com/docs/en/zos/2.1.0?topic=programs-connect
    Sint32 retcode = 0;

    //asm("WTO    'CONNECT'");
    asm("EZASMI TYPE=CONNECT,                                          +\n"
        "\t      S=(%[desc]),NAME=(%[socket]),ERRNO=(%[errno]),RETCODE=(%[retcode])"
        : /* No output operands */
        : /* Input operands */
        [desc] "r"(desc),
        [errno] "r"(errno),
        [retcode] "r"(&retcode),
        [socket] "r"(socket)
        : "0", "1", "15");
    return retcode;
}

/* Close socket */
Sint32 ez_close(Sint16 *desc, Sint32 *errno)
{
    Sint32 retcode = 0;

    //asm("WTO    'CLOSE'");
    asm("EZASMI TYPE=CLOSE,S=(%[desc]),ERRNO=(%[errno]),RETCODE=(%[retcode])"
        : /* No output operands */
        : /* Input operands */
        [desc] "r"(desc),
        [errno] "r"(errno),
        [retcode] "r"(&retcode)
        : "0", "1", "15");
    return retcode;
}

/* Send request */
Sint32 ez_send(Sint16 *desc, Uint32 nbyte, void *buf, Sint32 *errno)
{
    // https://www.ibm.com/docs/en/zos/2.1.0?topic=programs-send
    Sint32 retcode = 0;

    //asm("WTO    'SEND'");

    /* Convert to ASCII*/
    asm("CALL  EZACIC04,((%[buf]),(%[nbyte])),VL"
        : /* No output operands */
        : /* Input operands */
        [buf] "r"(buf),
        [nbyte] "r"(&nbyte)
        : "0", "1", "15");

    asm("EZASMI TYPE=SEND,MF=(E,TRMPLIST),                             +\n"
        "\t      S=(%[desc]),NBYTE=(%[nbyte]),BUF=(%[buf]),ERRNO=(%[errno]),RETCODE=(%[retcode])"
        : /* No output operands */
        : /* Input operands */
        [buf] "r"(buf),
        [desc] "r"(desc),
        [errno] "r"(errno),
        [nbyte] "r"(&nbyte),
        [retcode] "r"(&retcode)
        : "0", "1", "15");
    return retcode;
}

/* Receive response */
Sint32 ez_recv(Sint16 *desc, Uint32 nbyte, void *buf, Sint32 *errno)
{
    Sint32 retcode = 0;

    //asm("WTO    'RECV'");
    asm("EZASMI TYPE=RECV,                                             +\n"
        "\t      S=(%[desc]),NBYTE=(%[nbyte]),BUF=(%[buf]),ERRNO=(%[errno]),RETCODE=(%[retcode])"
        : /* No output operands */
        : /* Input operands */
        [buf] "r"(buf),
        [desc] "r"(desc),
        [errno] "r"(errno),
        [nbyte] "r"(&nbyte),
        [retcode] "r"(&retcode)
        : "0", "1", "15");

    /* Convert to EBCDIC*/
    if (retcode > 0)
    {
        asm("CALL  EZACIC05,((%[buf]),(%[nbyte])),VL"
            : /* No output operands */
            : /* Input operands */
            [buf] "r"(buf),
            [nbyte] "r"(&retcode)
            : "0", "1", "15");
    }

    return retcode;
}

/* Try converting hostname/IP addr to network binary format */
Sint32 ez_pton(char *srcaddr, Uint32 *dstaddr, Sint32 *errno)
{
    Sint32 retcode = 0;
    Sint16 srclen = strlen(srcaddr);

    //asm("WTO   'PTON'");
    asm("EZASMI TYPE=PTON,AF='INET',                                   +\n\t"
        "      SRCADDR=(R%[srcaddr]),SRCLEN=(R%[srclen]),                               +\n\t"
        "      DSTADDR=(R%[dstaddr]),ERRNO=(R%[errno]),RETCODE=(R%[retcode])"
        : /* No output operands */
        : /* Input operands */
        [dstaddr] "r"(dstaddr),
        [errno] "r"(errno),
        [retcode] "r"(&retcode),
        [srcaddr] "r"(srcaddr),
        [srclen] "r"(&srclen)
        : "0", "1", "15"); /* clobbered registers */
    return retcode;
}

/* Convert IP address to dotted decimal format */
Sint32 ez_ntop(Uint32 *srcaddr, char *dstaddr, Sint32 *errno)
{
    Sint32 retcode = 0;
    Sint16 addrlen = strlen(srcaddr);

    //asm("WTO    'NTOP'");
    asm("EZASMI TYPE=NTOP,AF='INET',                                   +\n"
        "\t      SRCADDR=(%[srcaddr]),DSTADDR=(%[dstaddr]),DSTLEN=(%[addrlen]),ERRNO=(%[errno]),RETCODE=(%[retcode])"
        : /* No output operands */
        : /* Input operands */
        [addrlen] "r"(&addrlen),
        [dstaddr] "r"(dstaddr),
        [errno] "r"(errno),
        [retcode] "r"(&retcode),
        [srcaddr] "r"(srcaddr)
        : "0", "1", "15");
    return retcode;
}

/* Resolve IP address to hostname */
Sint32 ez_getha(char *hostaddr, char *hostent, Sint32 *errno)
{
    Sint32 retcode = 0;

    //asm("WTO    'GETHOSTBYADDR'");
    asm("EZASMI TYPE=GETHOSTBYADDR,                                    +\n"
        "\t      HOSTADR=(%[hostaddr]),HOSTENT=(%[hostent]),RETCODE=(%[retcode])"
        : /* No output operands */
        : /* Input operands */
        [hostaddr] "r"(hostaddr),
        [hostent] "r"(hostent),
        [retcode] "r"(&retcode)
        : "0", "1", "15");
    return retcode;
}

/* Resolve hostname to IP address */
Sint32 ez_gethn(char *name, char *hostent, Sint32 *errno)
{
    Sint32 retcode = 0;
    Sint16 namelen = strlen(name);

    //asm("WTO    'GETHOSTBYNAME'");
    asm("EZASMI TYPE=GETHOSTBYNAME,                                    +\n"
        "\t      NAME=(%[name]),NAMELEN=(%[namelen]),HOSTENT=(%[hostent]),RETCODE=(%[retcode])"
        : /* No output operands */
        : [name] "r"(name),
          [namelen] "r"(&namelen),
          [hostent] "r"(hostent),
          [retcode] "r"(&retcode)
        : "0", "1", "15");
    return retcode;
}

/* Identify peer server */
Sint32 ez_getpn(Sint16 *desc, Socket *socket, Sint32 *errno)
{
    Sint32 retcode = 0;

    //asm("WTO    'GETPEERNAME'");
    asm("EZASMI TYPE=GETPEERNAME,                                      +\n"
        "\t      S=(%[desc]),NAME=(%[socket]),ERRNO=(%[errno]),RETCODE=(%[retcode])"
        : /* No output operands */
        : /* Input operands */
        [desc] "r"(desc),
        [errno] "r"(&errno),
        [retcode] "r"(&retcode),
        [socket] "r"(socket)
        : "0", "1", "15");
    return retcode;
}

//***********************
//* Parm list data      *
//***********************
asm("\nTRMPLIST DS    0D");
asm("DS    16F");
//***********************
//* TCP/IP Task Storage *
//***********************
asm("\nEZASMTIE DS    0D");
asm("\nT#PARMS  DS    16F"); // EZASOH03 parm list data
asm("\nT#WORK   DS    0D");  // EZASOH03 Working storage
asm("DS    F");
asm("\nT#END    DS    0D");
asm("\nTIELENTH EQU   T#END-EZASMTIE "); // Length of task storage
asm("YREGS    ,");
