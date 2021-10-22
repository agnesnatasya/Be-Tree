d := $(dir $(lastword $(MAKEFILE_LIST)))

ifeq ($(IS_DEV),1)
FILES=simtransport_test.cpp
endif

echo heeeeeelo

TEST_SRCS += $(addprefix $(d), $(FILES))

ifeq ($(IS_DEV),1)
GTEST_SRCS += $(d)simtransport_test
endif
$(d)simtransport_test: $(o)simtransport_test.o $(LIB-transport_implementation) $(OBJS-swap_space) $(OBJS-backing_store) $(LIB-transport) $(LIB-debug) $(LIB-simtransport) $(OBJS-storage_server) \
	$(GTEST_MAIN)

# $(o)simtransport_test.o \
# 	$(OBJS-spec-replica) $(OBJS-spec-client) \
# 	$(LIB-simtransport) \
	$(GTEST_MAIN)

TEST_BINS += $(d)simtransport_test 
