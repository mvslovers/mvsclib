********************************************
* struct REGS
* {
*   unsigned int R0;
*   unsigned int R1;
*   unsigned int R15;
* };
* void EXSVC(int svc, struct REGS *regs);
********************************************
         COPY  PDPTOP
         CSECT
* Program text area
         DS    0F
* X-func EXSVC prologue
EXSVC    PDPPRLG CINDEX=0,FRAME=88,BASER=12,ENTRY=YES
         B     @@FEN0
         LTORG
@@FEN0   EQU   *
         DROP  12
         BALR  12,0
         USING *,12
@@PG0    EQU   *
         LR    11,1
         L     10,=A(@@PGT0)
* Function EXSVC code
* Initialize registers
         L     2,0(11) *  svc
         L     3,4(11) *  regs
         L     0,0(3)  *  regs->R0
         L     1,4(3)  *  regs->R1
         L     15,8(3) *  regs->R15
* Execute service
         EX    2,DOSVC
* Return registers
         ST    0,0(3)  *  regs->R0
         ST    1,4(3)  *  regs->R1
         ST    15,8(3) *  regs->R15
         L     15,0
* Function EXSVC epilogue
         PDPEPIL
* Function EXSVC literal pool
         DS    0F
         LTORG
         DS    0F
DOSVC    SVC   0         * Executed Instruction
* Function EXSVC page table
         DS    0F
@@PGT0   EQU   *
         DC    A(@@PG0)
         END
