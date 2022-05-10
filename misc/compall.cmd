@echo off
del *.obj pdptest.exe
make -f makefile.bcc
pdptest
del *.obj pdptest.exe
nmake -nologo -f makefile.ibm
pdptest
del *.obj pdptest.exe
make -f makefile.emx
pdptest
del *.obj pdptest.exe
wmake -u -h -f makefile.wat
pdptest
