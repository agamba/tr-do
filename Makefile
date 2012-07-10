SRC_C_FILES=
SRC_H_FILES=global.h dialog.h net.h
SRC_CPP_FILES=main.cpp dialog.cpp net.cpp
SRC_HPP_FILES=
SRC_OTH_FILES=
SRCFILES=${SRC_C_FILES} ${SRC_H_FILES} ${SRC_CPP_FILES} ${SRC_HPP_FILES} \
	${SRC_OTH_FILES}
OBJFILES=main.o dialog.o net.o
UTEST=
LOPTS=-lstdc++
COPTS=-g -funsigned-char -pedantic -Wall -Wpointer-arith -Wconversion\
	-Wstrict-prototypes -Wmissing-prototypes
DOL=$$
	
.SUFFIXES: .cpp .o .c

.c.o:
	cc ${COPTS} -c $<

.cpp.o:
	gcc -g -I. -c $<

all: ${OBJFILES} wmtrcmd

unittest:	${UTEST}

cppscan:	cppscan.o libparse.a
	gcc cppscan.o ${LOPTS} -o cppscan

wmtrcmd:	${OBJFILES}
	g++ ${LOPTS} ${OBJFILES} -o wmtrcmd
	sudo chown 0:0 wmtrcmd
	sudo chmod u+s wmtrcmd

clean:
	rm -f *.o *.a ${UTEST} wmtrcmd

checkin:
	check_in ${SRCFILES}

mark_stable:
	for f in ${SRCFILES}; do rcs -sStab ${DOL}f ; done

mark_released:
	for f in ${SRCFILES}; do rcs -sRel ${DOL}f ; done

