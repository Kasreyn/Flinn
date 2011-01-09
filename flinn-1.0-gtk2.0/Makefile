
# add -DUSE_JOYSTICK to COPTS to enable joystick support under Linux
COPTS = -O2 -Wall

LIBS = `pkg-config gtk+-2.0 --libs` -lm
INCS = `pkg-config gtk+-2.0 --cflags`

CC = gcc
COPTS = -O8 -funroll-loops -fomit-frame-pointer -Wall -ffast-math -DUSE_JOYSTICK

#CC = gcc31
#COPTS = -O8 -funroll-loops -fomit-frame-pointer -Wall -ffast-math -mcpu=k6-2 -march=k6-2

#COPTS = -O3 -g -fno-inline-functions -ffast-math -pg -Wall
#COPTS = -ggdb -Wall

#CC = icc
#COPTS = -O3 -tpp5 -wp_ipo -Xa -xM

CFLAGS = $(COPTS) $(INCS)

EXEC_SRCS = flinn.c
EXECS = flinn


all: $(EXECS)

clean:
	rm -f $(EXECS) *.o gmon.out

dist:
	tar -hzvcf flinn-`cat VERSION`.tar.gz `cat MANIFEST`
	ls -la flinn-`cat VERSION`.tar.gz

testdist:
	tar -zvcf flinn-testdir.tar.gz test
	ls -la flinn-testdir.tar.gz

$(EXECS): $(EXEC_SRCS)
	$(CC) $(CFLAGS) $@.c -o $@ $(LIBS)
