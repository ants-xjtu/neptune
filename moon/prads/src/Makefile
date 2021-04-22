#Makefile
CC=clang-10
LLINK=llvm-link-10
OPT=opt-10
RW         ?= w
CHECK_METHOD ?= ifelseheap
CHECK_EXT_CALL_ARGS ?= false
OPTFLAGS := -load ../Gmem.min/obj/libGmem-pass.so -gmem 			\
            -Gmem-rw=$(RW)  										\
			-Gmem-check-method=$(CHECK_METHOD)  						\
			-Gmem-verify-external-call-args=$(CHECK_EXT_CALL_ARGS) 		\
			-Gmem-whitelist-section=safe_functions -O3

AUXFILES := $(wildcard ../Gmem.min/obj/Auxiliary.o)

UNAME := $(shell uname -s)
CFLAGS+=-O3

# for finding static libraries
LIBDIR=/usr/lib/x86_64-linux-gnu

# RHEL5 libresolv doesnt export certain essential symbols, but they are present
ifneq ($(findstring 5.,$(shell [ -f /etc/redhat-release ] && cat /etc/redhat-release | awk '{ print $3 }')),)
LDFLAGS+=/usr/lib64/libresolv.a
endif

# no, you are not under a git directory...
# ifeq (${RELEASE},)
# CFLAGS+=-DRELEASE='"$(shell git describe | cut -f2- -d_ | sed s/`git tag -l | grep -v /| tail -n1`//)"'
# endif

# for using a newer libpcap
ifneq (${PCAPDIR},)
LDFLAGS+=-L${PCAPDIR}
else
PCAPDIR=${LIBDIR}
endif

ifeq (${STATIC},)

ifeq (${JEMALLOC},y)
CFLAGS+=-DUSE_JEMALLOC
LDFLAGS+=-ljemalloc
endif
# if you want to use perftools
ifeq (${TCMALLOC},y)
LDFLAGS+=-ltcmalloc
endif
LDFLAGS+=-lpcap -lpcre -lresolv
CFLAGS+=-DCONFDIR='"${CONFDIR}/"'

else

ifeq (${JEMALLOC},y)
CFLAGS+=-DUSE_JEMALLOC
LDFLAGS+=${LIBDIR}/libjemalloc.a
endif

ifeq (${TCMALLOC},y)
LDFLAGS+=/usr/lib/libtcmalloc_minimal.a
endif

CFLAGS+=-DCONFDIR='"prads.d/"' -static
LDFLAGS=-static
LDFLAGS+=${LIBDIR}/libpcre.a ${PCAPDIR}/libpcap.a ${LIBDIR}/libresolv.a

endif

ifneq (${DEBUG},)
CFLAGS = -g -DDEBUG -Wall -DCONFDIR='"${CONFDIR}/"'
endif
ifneq (${PROFILE},)
CFLAGS=-g -pg -DCONFDIR='"${CONFDIR}/"'
endif
ifneq (${PPROF},)
LDFLAGS+=-lprofiler
endif
ifneq (${EXTRADEBUG},)
CFLAGS+= -DDUMP_HASH -DDEBUG_HASH -DDEBUG=2 -DDEBUG=2
endif
ifneq (${DEBUG_PACKET},)
CFLAGS+= -DDEBUG_PACKET
endif
ifeq ($(UNAME), FreeBSD)
LDFLAGS= -L/usr/local/lib -lpcre -lpcap
CPPFLAGS= -I/usr/local/include
endif

BUILD=gnu
ifeq (${BUILD},osx)
CFLAGS+=-I/opt/local/include/ -DOSX
CONFDIR=/opt/etc/prads
else
CFLAGS+=-D__USE_GNU
CONFDIR=/home/hypermoon/neptune-yh/moon/prads/etc
endif
# LDFLAGS+= -lpthread
SERVICE_OBJ = servicefp/servicefp.o servicefp/tcpc.o servicefp/tcps.o servicefp/udps.o 

IPFP_OBJ = ipfp/ipfp.o ipfp/udp_fp.o ipfp/icmp_fp.o
LOG_OBJ = output-plugins/log_dispatch.o output-plugins/log_stdout.o output-plugins/log_file.o output-plugins/log_fifo.o output-plugins/log_ringbuffer.o output-plugins/log_sguil.o
CXT_OBJ = cxt.o
MODULES = dhcp.o dump_dns.o mac.o ${SERVICE_OBJ} ${IPFP_OBJ} ${CXT_OBJ} ${LOG_OBJ}
OBJECTS = bstrlib.o sig_tcp.o config.o sys_func.o assets.o prads.o ${MODULES}
OBJECTS_BC = $(patsubst %.o, %.bc, $(OBJECTS))
PRADS_LINK_BC = prads-link.bc
PRADS_OPT_BC = prads-opt.bc

SHM_CLIENT_OBJECTS = shm-client.o
SHM_CLIENT = $(patsubst %.o, %, $(SHM_CLIENT_OBJECTS))

all: prads shm-client

osx:
	$(MAKE) BUILD=osx

prads: $(OBJECTS_BC)
	$(LLINK) $(OBJECTS_BC) $(AUXFILES) > $(PRADS_LINK_BC)
	$(OPT) $(OPTFLAGS) < $(PRADS_LINK_BC) > $(PRADS_OPT_BC)
	$(CC) $(CFLAGS) -o $@ $(PRADS_OPT_BC) $(LDFLAGS)
	rm -f $(OBJECTS_BC) $(PRADS_LINK_BC) $(PRADS_OPT_BC)

prads_orig: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS) 

# make sure everything is fPIC
$(OBJECTS_BC): %.bc:%.c
	$(CC) $(CFLAGS) -emit-llvm -fPIC -c -o $@ $<
	
$(PRADS_LINK_BC): $(OBJECTS_BC)
	$(LLINK) $(OBJECTS_BC) > $(PRADS_LINK_BC)

shm-client: $(SHM_CLIENT).c
	$(CC) $(CFLAGS) -emit-llvm -c -o $(SHM_CLIENT).bc $<
	$(LLINK) $(SHM_CLIENT).bc $(AUXFILES) > $(SHM_CLIENT)-link.bc
	$(OPT) $(OPTFLAGS) < $(SHM_CLIENT)-link.bc > $(SHM_CLIENT)-opt.bc
	$(CC) $(CFLAGS) -o $@ $(SHM_CLIENT)-opt.bc $(LDFLAGS)
	rm -f $(SHM_CLIENT)-opt.bc $(SHM_CLIENT)-link.bc $(SHM_CLIENT).bc

static: 
	$(MAKE) STATIC=y prads

debug:
	 ${MAKE} DEBUG=y

profile:
	${MAKE} PROFILE=y

clean:
	-rm -fv $(OBJECTS)
	-rm -f $(OBJECTS_BC) $(PRADS_LINK_BC) $(PRADS_OPT_BC)
	-rm -f prads

indent:
	find -type f -name '*.[ch]' | xargs indent -kr -i4 -cdb -sc -sob -ss -ncs -ts8 -nut

# oldschool header file dependency checking.
deps:
	-rm -f deps.d
	for i in $(subst .o,.c,$(OBJECTS)); do gcc -MM $$i >> deps.d; done

ifneq ($(wildcard deps.d),)
include deps.d
endif

tags: *.h *.c
	ctags -R
