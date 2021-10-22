d := $(dir $(lastword $(MAKEFILE_LIST)))

ifeq ($(IS_DEV),0)
SRCS += $(addprefix $(d), storage_server.cpp main.cpp)
else 
SRCS += $(addprefix $(d), storage_server.cpp)
endif

OBJS-storage_server :=  $(o)storage_server.o $(LIB-debug) $(LIB-configuration) $(LIB-transport) 

$(d)server: $(LIB-transport_implementation) $(OBJS-storage_server) $(o)main.o

BINS += $(d)server
