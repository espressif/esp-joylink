include ../Makefile.rule 
HEADERS = $(wildcard *.h)
ORG_SOURCES = $(wildcard *.c)
OUT_SRC = test.c 

SOURCES=$(filter-out ${OUT_SRC}, ${ORG_SOURCES})
OBJS = $(patsubst %.c, %.o, $(SOURCES))

LIBNAME = $(strip ${shell pwd |xargs basename})

INCLUDES += -I${PROJECT_ROOT_PATH}/json
INCLUDES += -I${PROJECT_ROOT_PATH}/list
INCLUDES += -I${PROJECT_ROOT_PATH}/joylink

STATIC_LIBS += ${TARGET_LIB}/libauth.a
STATIC_LIBS += ${TARGET_LIB}/libjson.a
STATIC_LIBS += ${TARGET_LIB}/liblist.a
#STATIC_LIBS += ${TARGET_LIB}/libjoylinksdk.a

STATIC_LIBS += 
LIBS += -lm  

ifeq (${ARCH}, x86)  
all:${OBJS} liba libso
else
all:${OBJS} liba 
endif

.SUFFIXES: .c .o
.c.o:
	${CC} ${CFLAGS} -c $(INCLUDES) ${STATIC_LIBS} $(LIBS) $*.c

liba:${OBJS}
	${AR} -crs lib${LIBNAME}.a ${OBJS}
	${MV} lib${LIBNAME}.a ${TARGET_LIB}

libso:${OBJS}
	${CC}  ${OBJS} -shared -fPIC -o lib${LIBNAME}.so
	${MV} lib${LIBNAME}.so ${TARGET_LIB} 

test:${OBJS}
	${CC} -DLIB_TEST ${OBJS} test.c -o $@ ${CFLAGS} ${INCLUDES}  ${TARGET_LIB}/libjoylinksdk.a ${LIBS} 

clean:
	${RM} *.o *.so *.a test

distclean:clean
	${RM} ./*.a ./*.so ${TARGET_LIB}/lib${LIBNAME}.*

.PHONY:all clean test jt
