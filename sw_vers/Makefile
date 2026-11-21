PROG?=		sw_vers
TARGET?=	${PROG}
SRCS=		sw_vers.c

CFLAGS?=	-g -O2
WARNFLAGS?=	-Wall -Wextra -Wformat
LDADD?=		-framework CoreFoundation

.PHONY: all clean show-config

all: ${TARGET}

show-config:
	@printf 'PROG=%s\n' '${PROG}'
	@printf 'CC=%s\n' '${CC}'
	@printf 'CPPFLAGS=%s\n' '${CPPFLAGS}'
	@printf 'CFLAGS=%s\n' '${CFLAGS}'
	@printf 'WARNFLAGS=%s\n' '${WARNFLAGS}'
	@printf 'LDFLAGS=%s\n' '${LDFLAGS}'
	@printf 'LDADD=%s\n' '${LDADD}'
	@printf 'TARGET=%s\n' '${TARGET}'

${TARGET}: ${SRCS}
	${CC} ${CPPFLAGS} ${CFLAGS} ${WARNFLAGS} ${LDFLAGS} -o $@ ${SRCS} ${LDADD}

clean:
	rm -f ${TARGET}
	rm -rf ${TARGET}.dSYM
