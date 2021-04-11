#
# Makefile for libnids.
#
# Dug Song <dugsong@monkey.org>

srcdir		= .


install_prefix	=
prefix		= /usr/local
exec_prefix	= /usr/local
includedir	= ${prefix}/include
libdir		= ${exec_prefix}/lib
mandir		= ${prefix}/man
LIBSTATIC      = libnids.a
LIBSHARED      = libnids.so.1.25

CC		= clang-10
LLINK=llvm-link-10
OPT=opt-10
RW         ?= w
CHECK_METHOD ?= ifelseall
CHECK_EXT_CALL_ARGS ?= false

OPTFLAGS := -load $(GMEM)/obj/libGmem-pass.so -gmem 			\
            -Gmem-rw=$(RW)  										\
			-Gmem-check-method=$(CHECK_METHOD)  						\
			-Gmem-verify-external-call-args=$(CHECK_EXT_CALL_ARGS) 		\
			-Gmem-whitelist-section=safe_functions -O3

AUXFILES := $(GMEM)/obj/Auxiliary.o

CFLAGS		= -g -O2 -D_BSD_SOURCE -DLIBNET_VER=1 -DHAVE_ICMPHDR=1 -DHAVE_TCP_STATES=1 -DHAVE_BSD_UDPHDR=1
LDFLAGS		= 

PCAP_CFLAGS	= -I/usr/include/pcap
PCAPLIB		= -lpcap

LNET_CFLAGS	= -D_BSD_SOURCE -D__BSD_SOURCE -D__FAVOR_BSD -DHAVE_NET_ETHERNET_H
LNETLIB		= -lnet

LIBS_CFLAGS	= $(PCAP_CFLAGS) $(LNET_CFLAGS) -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -pthread -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
LIBS		= -lgthread-2.0 -lnsl  -lglib-2.0 -lgthread-2.0 -pthread -lglib-2.0
RANLIB		= ranlib
INSTALL		= /usr/bin/install -c

OBJS		= checksum.o ip_fragment.o ip_options.o killtcp.o \
		  libnids.o scan.o tcp.o util.o allpromisc.o hash.o
OBJS_SHARED	= $(OBJS:.o=_pic.o)
OBJS_BC	= $(patsubst %.o, %.bc, $(OBJS))
NIDS_LINK_BC = NIDS-link.bc
NIDS_OPT_BC = NIDS-opt.bc
NIDS_OBJ = NIDS.o

%.bc:%.c
	$(CC) -emit-llvm -c $(CFLAGS) -I. $(LIBS_CFLAGS) $< -o $@
$(NIDS_OBJ):$(OBJS_BC)
	$(LLINK) $(OBJS_BC) $(AUXFILES) > $(NIDS_LINK_BC)
	# $(LLINK) $(OBJS_BC) > $(NIDS_LINK_BC)
	$(OPT) $(OPTFLAGS) < $(NIDS_LINK_BC) > $(NIDS_OPT_BC)
	$(CC) -c $(CFLAGS) -I. $(LIBS_CFLAGS) $(NIDS_OPT_BC) -o $@
	# $(CC) -c $(CFLAGS) -I. $(LIBS_CFLAGS) $(NIDS_LINK_BC) -o $@

static: $(LIBSTATIC)
shared: $(LIBSHARED) 

# How to write the following rules compactly and portably ? 
# gmake accepts "%_pic.o: %.c", bsd make does not.
OBJS_BC_SHARED	= $(patsubst %.o, %_pic.bc, $(OBJS))
NIDS_LINK_BC_SHARED = NIDS-link_pic.bc
NIDS_OPT_BC_SHARED = NIDS-opt_pic.bc
NIDS_OBJ_SHARED = NIDS_pic.o

%_pic.bc:%.c
	$(CC) -fPIC -emit-llvm -c $(CFLAGS) -I. $(LIBS_CFLAGS) $< -o $@

$(NIDS_LINK_BC_SHARED): $(OBJS_BC_SHARED)
	$(LLINK) $(OBJS_BC_SHARED) > $(NIDS_LINK_BC_SHARED)

$(NIDS_OBJ_SHARED): $(OBJS_BC_SHARED)
	$(LLINK) $(OBJS_BC_SHARED) $(AUXFILES) > $(NIDS_LINK_BC_SHARED)
	# $(LLINK) $(OBJS_BC_SHARED) > $(NIDS_LINK_BC_SHARED)
	$(OPT) $(OPTFLAGS) < $(NIDS_LINK_BC_SHARED) > $(NIDS_OPT_BC_SHARED)
	$(CC) -c -fPIC $(CFLAGS) -I. $(LIBS_CFLAGS) $(NIDS_OPT_BC_SHARED) -o $@
	# $(CC) -c -fPIC $(CFLAGS) -I. $(LIBS_CFLAGS) $(NIDS_LINK_BC_SHARED) -o $@

$(LIBSTATIC): $(NIDS_OBJ)
	ar -cr $@ $(NIDS_OBJ)
	$(RANLIB) $@
$(LIBSHARED): $(NIDS_OBJ_SHARED)
	$(CC) -shared -Wl,-soname,$(LIBSHARED) -o $(LIBSHARED) $(NIDS_OBJ_SHARED) $(LIBS) $(LNETLIB) $(PCAPLIB)

_install install: $(LIBSTATIC)
	../mkinstalldirs $(install_prefix)$(libdir)
	../mkinstalldirs $(install_prefix)$(includedir)
	../mkinstalldirs $(install_prefix)$(mandir)/man3
	$(INSTALL) -c -m 644 libnids.a $(install_prefix)$(libdir)
	$(INSTALL) -c -m 644 nids.h $(install_prefix)$(includedir)
	$(INSTALL) -c -m 644 libnids.3 $(install_prefix)$(mandir)/man3
_installshared installshared: install $(LIBSHARED)
	$(INSTALL) -c -m 755 $(LIBSHARED) $(install_prefix)$(libdir)
	ln -s -f $(LIBSHARED) $(install_prefix)$(libdir)/libnids.so
 
clean:
	rm -f *.o *~ $(LIBSTATIC) $(LIBSHARED)
	rm -f *.bc

# EOF
