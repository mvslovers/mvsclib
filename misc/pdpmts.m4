
$dest mtsmacs.mac ok
$cre mtsmacs.mac

$copy *source* mtsmacs.mac(1)
PDPTOP    100
PDPPRLG   200
PDPEPIL   300
PDPMAIN   400
00000000
$endfile

$copy *source*@-endfile mtsmacs.mac(100)
undivert(pdptop.mac)dnl
$ENDFILE
$continue with *dummy*

$copy *source* mtsmacs.mac(200)
undivert(pdpprlg.mac)dnl
$endfile

$copy *source* mtsmacs.mac(300)
undivert(pdpepil.mac)dnl
$endfile

$copy *source*@-endfile mtsmacs.mac(400)
undivert(pdpmain.mac)dnl
$ENDFILE
$continue with *dummy*


$dest mtsstart.asm ok
$cre mtsstart.asm
$copy *source* to mtsstart.asm
undivert(mtsstart.asm)dnl
$endfile

$dest mtssupa.asm ok
$cre mtssupa.asm
$copy *source* to mtssupa.asm
undivert(mtssupa.asm)dnl
$endfile


$dest start.asm ok
$cre start.asm
$copy *source* to start.asm
undivert(start.s)dnl
$endfile

$dest stdio.asm ok
$cre stdio.asm
$copy *source* to stdio.asm
undivert(stdio.s)dnl
$endfile

$dest stdlib.asm ok
$cre stdlib.asm
$copy *source* to stdlib.asm
undivert(stdlib.s)dnl
$endfile

$dest ctype.asm ok
$cre ctype.asm
$copy *source* to ctype.asm
undivert(ctype.s)dnl
$endfile

$dest string.asm ok
$cre string.asm
$copy *source* to string.asm
undivert(string.s)dnl
$endfile

$dest time.asm ok
$cre time.asm
$copy *source* to time.asm
undivert(time.s)dnl
$endfile

$dest errno.asm ok
$cre errno.asm
$copy *source* to errno.asm
undivert(errno.s)dnl
$endfile

$dest assert.asm ok
$cre assert.asm
$copy *source* to assert.asm
undivert(assert.s)dnl
$endfile

$dest locale.asm ok
$cre locale.asm
$copy *source* to locale.asm
undivert(locale.s)dnl
$endfile

$dest math.asm ok
$cre math.asm
$copy *source* to math.asm
undivert(math.s)dnl
$endfile

$dest setjmp.asm ok
$cre setjmp.asm
$copy *source* to setjmp.asm
undivert(setjmp.s)dnl
$endfile

$dest signal.asm ok
$cre signal.asm
$copy *source* to signal.asm
undivert(signal.s)dnl
$endfile

$dest __memmgr.asm ok
$cre __memmgr.asm
$copy *source* to __memmgr.asm
undivert(__memmgr.s)dnl
$endfile

$dest pdptest.asm ok
$cre pdptest.asm
$copy *source* to pdptest.asm
undivert(pdptest.s)dnl
$endfile




*
* define a macro for repeated assemblies
*
>macro dobld n
>define srcf="{n}.asm"
>define runf="{n}.o"
>define listf="{n}.l"
>define errf="{n}.e"
>define rest="2=mtsmacs.mac par=test,size(20)"

$dest {listf} ok
$cr {listf}

$dest {errf} ok
$cr {errf}

$dest {runf} ok
$cre {runf}

$run *asmg scards={srcf} spunch={runf} sprint={listf} sercom={errf} {rest}

*li {listf} *print*
*li {errf} *print*

>endmacro



dobld mtsstart
dobld mtssupa
dobld start
dobld stdio
dobld stdlib
dobld ctype
dobld string
dobld time
dobld errno
dobld assert
dobld locale
dobld math
dobld setjmp
dobld signal
dobld __memmgr
dobld pdptest

li mtssupa.l *print*

$dest pdptest.r
$cre pdptest.r

$copy mtsstart.o pdptest.r
$copy mtssupa.o pdptest.r(last+1)
$copy start.o pdptest.r(last+1)
$copy stdio.o pdptest.r(last+1)
$copy stdlib.o pdptest.r(last+1)
$copy ctype.o pdptest.r(last+1)
$copy string.o pdptest.r(last+1)
$copy time.o pdptest.r(last+1)
$copy errno.o pdptest.r(last+1)
$copy assert.o pdptest.r(last+1)
$copy locale.o pdptest.r(last+1)
$copy math.o pdptest.r(last+1)
$copy setjmp.o pdptest.r(last+1)
$copy signal.o pdptest.r(last+1)
$copy __memmgr.o pdptest.r(last+1)
$copy pdptest.o pdptest.r(last+1)


$r pdptest.r par=ABD AbC

