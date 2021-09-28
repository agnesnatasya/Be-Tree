d := $(dir $(lastword $(MAKEFILE_LIST)))

SRCS += $(addprefix $(d), main.cpp swap_space.cpp backing_store.cpp)

OBJS-swap_space := $(o)swap_space.o

OBJS-backing_store :=  $(o)backing_store.o

$(d)client: $(LIB-fasttransport) $(OBJS-swap_space) $(OBJS-backing_store) $(o)main.o

BINS += $(d)client
