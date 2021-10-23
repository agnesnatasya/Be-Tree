d := $(dir $(lastword $(MAKEFILE_LIST)))

SIM_SRCS := $(d)simtransport_sim.cpp

$(d)simtransport_sim: $(o)simtransport_sim.o \
	$(OBJS-swap_space) $(OBJS-backing_store) \
	$(LIB-simtransport) $(OBJS-storage_server) \
	$(OBJS-storage_client)

SIM_BINS += $(d)simtransport_sim 
