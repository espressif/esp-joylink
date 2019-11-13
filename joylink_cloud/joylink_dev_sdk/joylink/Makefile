include ../Makefile.rule

HEADERS = $(wildcard *.h)
ORG_SOURCES = $(wildcard *.c)

ifeq (${DEV_REQ_ACTIVE_EN}, yes)
OUT_SRC =  test.c
else
OUT_SRC = test.c joylink_dev_active.c joylink_cloud_log.c
endif

SOURCES=$(filter-out ${OUT_SRC}, ${ORG_SOURCES})
OBJS = $(patsubst %.c, %.o, $(SOURCES))

LIBNAME = $(strip ${shell pwd |xargs basename})

INCLUDES += -I${PROJECT_ROOT_PATH}/json
INCLUDES += -I${PROJECT_ROOT_PATH}/joylink
INCLUDES += -I${PROJECT_ROOT_PATH}/example
INCLUDES += -I${PROJECT_ROOT_PATH}/auth
INCLUDES += -I${PROJECT_ROOT_PATH}/list
INCLUDES += -I${PROJECT_ROOT_PATH}/softap

ifeq ($(TARGET_SRC), extern)
STATIC_LIBS += ${TARGET_LIB}/libextern.a
else
#STATIC_LIBS += ${TARGET_LIB}/libexample.a
endif

#STATIC_LIBS += ${TARGET_LIB}/libauth.a
#STATIC_LIBS += ${TARGET_LIB}/libjson.a
#STATIC_LIBS += ${TARGET_LIB}/liblist.a
STATIC_LIBS += ${TARGET_LIB}/libjoylinksdk.a
LIBS += 

ifeq (${ARCH}, x86)  
#all:${OBJS} liba libso jt test
all:${OBJS} liba libso 
else
#all:${OBJS} liba jt
all:${OBJS} liba 
endif

.SUFFIXES: .c .o
.c.o:
	${CC} ${CFLAGS} -c $(INCLUDES)  $*.c

liba:${OBJS}
	${AR} -crs lib${LIBNAME}.a ${OBJS}
	${MV} lib${LIBNAME}.a ${TARGET_LIB}

libso:${OBJS}
	${CC}  ${OBJS} -shared -fPIC -o lib${LIBNAME}.so
	${MV} lib${LIBNAME}.so ${TARGET_LIB} 

jt:
	${CC} -D_TEST_ test.c ${OBJS} -o $@ ${CFLAGS} ${INCLUDES} ${STATIC_LIBS} ${LIBS} 
	${MV} $@ ${TARGET_BIN} 

test:
	${CC} -D_TEST_ test.c joylink_utils.c -o $@ ${CFLAGS} ${INCLUDES} ${STATIC_LIBS} ${LIBS} 

clean:
	${RM} *.o *.so *.a test ${TARGET_BIN}/jt

distclean:clean
	${RM} ./*.a ./*.so ${TARGET_LIB}/lib${LIBNAME}.*

.PHONY:all clean test jt
