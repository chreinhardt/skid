#
# Makefile for SKID (Spline Kernel Interpolative Denmax).
#
# on an SGI R4400 add -mips2 to CFLAGS
# on an SGI R8000 (TFP) add -mips4 -O3 to CFLAGS
#
CC=cc
#CFLAGS	=	-O2
#Following added for gcc
#CFLAGS = -O3 -funroll-loops
CFLAGS = -O3

# tirpc library (needed if glibc >= 2.32)
RPC_LIB = -ltirpc

LIBS	=   -lm $(RPC_LIB)

default:	skid totipnat
	@echo To try the demo, type demo!

clean:
	rm -f *.o

skid: main.o kd.o smooth1.o grav.o cosmo.o romberg.o runge.o
	$(CC) $(CFLAGS) -o skid main.o kd.o smooth1.o grav.o cosmo.o romberg.o runge.o $(LIBS)

main.o: main.c kd.h smooth1.h cosmo.h

kd.o: kd.c kd.h tipsydefs.h cosmo.h

smooth1.o: smooth1.c kd.h smooth1.h cosmo.h

grav.o: grav.c grav.h kd.h cosmo.h

#
#	May need to specify -lrpc on some systems
#
totipnat: totipnat.o
	$(CC) $(CFLAGS) -o totipnat totipnat.o $(LIBS)

totipnat.o: tipsydefs.h
