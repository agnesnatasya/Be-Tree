d := $(dir $(lastword $(MAKEFILE_LIST)))

SIM_SRCS := $(d)simulation.cpp

$(d)simulation: $(o)simulation.o \
	$(OBJS-swap_space) $(OBJS-backing_store) \
	$(LIB-simtransport) $(OBJS-storage_server) \
	$(OBJS-storage_client)

SIM_BINS += $(d)simulation 
