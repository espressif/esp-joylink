include ./Makefile.rule
         
DEPS= auth json list example joylink agent 

STATICLIBS = $(wildcard $(TARGET_LIB)/*.a)
SDKLIBNAME = libjoylinksdk.a

all: ${DEPS} sdk_lib jt

clean: ${DEPS} 

distclean: ${DEPS}
	${RM} ${TARGET_LIB}/*

${DEPS}: 
	make -C $@ $(MAKECMDGOALS) 

jt:
	make jt -C joylink

sdk_lib:
	@for name in $(STATICLIBS); \
	do              \
	${AR} x "$$name";           \
	if [ "$$name" != "$(TARGET_LIB)/libjoylinksdk.a" ]; then ${RM} "$$name" ; fi;\
	done                
	@${AR} -crs ${SDKLIBNAME} *.o
	@${RANLIB} ${SDKLIBNAME} 
	@${RM} *.o
	@${MV} ${SDKLIBNAME} $(TARGET_LIB)

.PHONY:all clean distclean ${DEPS} jt sdk_lib
