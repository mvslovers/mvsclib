bcc -O2 perf2.c
copy perf2.exe e:\scratch\test2\bcc.exe
gcc -O perf2.c
copy perf2.exe e:\scratch\test2\gcc.exe
icc -O perf2.c
copy perf2.exe e:\scratch\test2\icc.exe
wcl386 -oneatx perf2.c
copy perf2.exe e:\scratch\test2\wcc.exe

call pbcc perf2 -O2
copy perf2.exe e:\scratch\test2\pbcc.exe
call pgcc perf2 -O
copy perf2.exe e:\scratch\test2\pgcc.exe
call picc perf2 -O
copy perf2.exe e:\scratch\test2\picc.exe
call pwcc -oneatx perf2.c
copy perf2.exe e:\scratch\test2\pwcc.exe

bcc -O2 perf3.c
copy perf3.exe e:\scratch\test3\bcc.exe
gcc -O perf3.c
copy perf3.exe e:\scratch\test3\gcc.exe
icc -O perf3.c
copy perf3.exe e:\scratch\test3\icc.exe
wcl386 -oneatx perf3.c
copy perf3.exe e:\scratch\test3\wcc.exe

call pbcc perf3 -O2
copy perf3.exe e:\scratch\test3\pbcc.exe
call pgcc perf3 -O
copy perf3.exe e:\scratch\test3\pgcc.exe
call picc perf3 -O
copy perf3.exe e:\scratch\test3\picc.exe
call pwcc -oneatx perf3.c
copy perf3.exe e:\scratch\test3\pwcc.exe

call pbcc perf4 -O2
copy perf4.exe e:\scratch\test4\pbcc.exe
call pgcc perf4 -O
copy perf4.exe e:\scratch\test4\pgcc.exe
call picc perf4 -O
copy perf4.exe e:\scratch\test4\picc.exe
call pwcc -oneatx perf4.c
copy perf4.exe e:\scratch\test4\pwcc.exe

bcc -O2 perf4.c
copy perf4.exe e:\scratch\test4\bcc.exe
gcc -O perf4.c
copy perf4.exe e:\scratch\test4\gcc.exe
icc -O perf4.c
copy perf4.exe e:\scratch\test4\icc.exe
wcl386 -oneatx perf4.c
copy perf4.exe e:\scratch\test4\wcc.exe


