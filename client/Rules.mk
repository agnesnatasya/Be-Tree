d := $(dir $(lastword $(MAKEFILE_LIST)))

SRCS += $(addprefix $(d), main.cpp swap_space.cpp backing_store.cpp storage_client.cpp)

OBJS-storage_client := $(o)storage_client.o

OBJS-swap_space := $(o)swap_space.o $(OBJS-storage_client)

OBJS-backing_store :=  $(o)backing_store.o

$(d)client := $(LIB-fasttransport) $(OBJS-swap_space) $(OBJS-backing_store) $(OBJS-storage_client) $(o)main.o

BINS += $(d)client
