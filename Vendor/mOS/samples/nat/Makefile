TARGET = nat

######################################################################
# GCC and compilation options
######################################################################
GCC = $(CC)
GCC_OPT = -m64 -Wall -Werror -fgnu89-inline
GCC_OPT += -DNDEBUG -O3 -g -DNETSTAT -DINFO -DDBGERR -DDBGCERR
GCC_OPT += $(DBG_OPT)
ifeq ($V,) # no echo
    export MSG=@echo
    export HIDE=@
else
    export MSG=@\#
    export HIDE=
endif

######################################################################
# LIBRARIES AND INCLUDES
######################################################################
MTCP_FLD    = ../../core
MTCP_INC    =-I$(MTCP_FLD)/include
MTCP_TARGET = $(MTCP_FLD)/lib/libmtcp.a
LIBS        += -lmtcp -lssl -lcrypto -lnuma -lpthread -lrt
LIB_DIR     += -L$(MTCP_FLD)/lib
CMN_DIR     = ../common
CMN_INC     = -I$(CMN_DIR)
UTIL_INC    = -I../../util/include

# I/O library parameter (PSIO or DPDK)
GCC_OPT += -D__thread="" -DBE_RESILIENT_TO_PACKET_DROP
INC += -DENABLE_PCAP
LIBS += -lpcap

######################################################################

default: $(TARGET)

$(MTCP_TARGET):
	cd $(MTCP_FLD)/src && make	

$(TARGET): $(MTCP_TARGET) $(TARGET).c $(CMN_DIR)/*.c
	$(MSG) "   CC $<"
	$(HIDE) $(GCC) $(GCC_OPT) -o $@ $^ $(MTCP_INC) $(CMN_INC) $(UTIL_INC) $(LIB_DIR) $(LIBS)

clean:
	rm -rf *~ *.o $(TARGET) logs/*

cleanall: clean
	rm -rf Makefile


