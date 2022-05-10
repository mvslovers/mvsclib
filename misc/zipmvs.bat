del pdpinc.zip
del pdpsrc.zip
del pddjcl.zip
del pdpdoc.zip
del pdpmac.zip
del all.zip
del alljcl.jcl
del pdpmvs.jcl
del output.txt
zip -9 -X -ll -j pdpsrc.zip *.c *.asm
zip -9 -X -ll -j pdpinc.zip *.h
zip -9 -X -ll -j pdpjcl.zip *.jcl
zip -9 -X -ll -j pdpmac.zip *.mac
zip -9 -X -ll -j pdpdoc.zip readme.txt pdpclib.txt pdpgoal.txt
zip -9 -X all *.zip
