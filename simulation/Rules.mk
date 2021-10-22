d := $(dir $(lastword $(MAKEFILE_LIST)))

$(d)simtransport_test: $(o)simtransport_test.o $(OBJS-swap_space) $(OBJS-backing_store) $(LIB-transport) $(LIB-debug) $(LIB-simtransport) $(OBJS-storage_server) \
	$(OBJS-storage_client)


# $(o)simtransport_test.o \
# 	$(OBJS-spec-replica) $(OBJS-spec-client) \
# 	$(LIB-simtransport) \
	$(GTEST_MAIN)

SIM_BINS += $(d)simtransport_test 
