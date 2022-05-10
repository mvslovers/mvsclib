rem watc on MUSIC/SP is currently not working

del pdpmusw.zip
zip -0X pdpmusw *.c *.h *.asm *.mac pdpmusw.job
call runmus pdpmusw.job output.txt pdpmusw.zip
