###########################################
###		Chenle YU		###
###	part of IRT Internship		###
###	24 / 08 / 2018 			###
###########################################

#What are these files?
This folder is a modified libgomp library of gcc's OpenMP implementation.
Not all files are modified, but only single.c, parallel.c, team.c, and critical.c.
Arinc653.c and Arinc653.h are newly created files, trying to make OpenMP runnable on 
a Arinc OS. One can try the layer by following the steps below.



# DIRECTLY use this version of libgomp
1) Download libgomp.so (not uploaded yet, #TODOFOR16/08)
2) defien the environment variable LD_LIBRARY_PATH to the location of the downloaded dynamic library
3) compile your program using OpenMP directives with -fopenmp, things should work

# How to modify and let gcc take these modifications into account:
1) Add new files (.c, .h) to the libgomp directory
2) If needed, modify the libgomp.h file in order to include your .h files (careful of recrusive inclusion, in order to respect the ONE-DEFINITON rule)
3) Modify the libgomp/Makefile.am file by adding the new .c files that we have used (the variable libgomp_la_sources)
4) Modify the libgomp/configure.ac file and ../config/override.m4 by changing the autoconf version to ours one, skip this step if you are using the same version of autoconf with gcc's 
5) Call autoreconf in the libgomp directory
6) Rebuild by using “make” in the objdir (cf. the manual of building gcc provided by gnu)
7) "make install" in the same place
8) Link correctly to the newly created library by defining the LD_LIBRARY_PATH variable to its full path
