#include "svc.h"

void EXSVC(int svc, struct REGS *regs)
{
    asm("L     10,%[iSvc]\n"
        "\tL     0,%[iR0]\n"
        "\tL     1,%[iR1]\n"
        "\tL     15,%[iR15]\n"
        "\tEX    10,DOSVC\n"
        "\tST    0,%[oR0]\n"
        "\tST    1,%[oR1]\n"
        "\tST    15,%[oR15]"
        : [oR0] "=r"(regs->R0), [oR1] "=r"(regs->R1), [oR15] "=r"(regs->R15)                                    /* output 0,1,2 */
        : [iSvc] "r"(svc), [iR0] "r"(regs->R0), [iR1] "r"(regs->R1), [iR15] "r"(regs->R15) /* input  3,4,5,6 */ /* clobbered register */
    );
}
