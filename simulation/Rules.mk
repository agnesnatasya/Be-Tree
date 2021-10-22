d := $(dir $(lastword $(MAKEFILE_LIST)))

SIM_SRCS := $(d)simtransport_test.cpp

$(d)simtransport_test: $(o)simtransport_test.o \
	$(OBJS-swap_space) $(OBJS-backing_store) \
	$(LIB-simtransport) $(OBJS-storage_server) \
	$(OBJS-storage_client)

SIM_BINS += $(d)simtransport_test 
