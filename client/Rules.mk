d := $(dir $(lastword $(MAKEFILE_LIST)))

ifeq ($(IS_DEV),0)
SRCS += $(addprefix $(d), main.cpp swap_space.cpp backing_store.cpp storage_client.cpp)
else 
SRCS += $(addprefix $(d), swap_space.cpp backing_store.cpp storage_client.cpp)
endif

OBJS-storage_client := $(o)storage_client.o

OBJS-swap_space := $(o)swap_space.o $(OBJS-storage_client)

OBJS-backing_store :=  $(o)backing_store.o

$(d)client: $(LIB-transport_implementation) $(OBJS-swap_space) $(OBJS-backing_store) $(o)main.o

BINS += $(d)client
