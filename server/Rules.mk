d := $(dir $(lastword $(MAKEFILE_LIST)))

SRCS += $(addprefix $(d), store_server.cpp main.cpp)

OBJS-store_server :=  $(o)store_server.o                  \
                   $(OBJS-replication-common) $(LIB-message) \

OBJS-meerkatir-replica :=  $(o)replica.o     \
                   $(OBJS-replication-common) $(LIB-message) \
                   $(LIB-configuration)

