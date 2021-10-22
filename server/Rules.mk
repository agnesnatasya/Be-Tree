d := $(dir $(lastword $(MAKEFILE_LIST)))

SRCS += $(addprefix $(d), storage_server.cpp main.cpp)

OBJS-storage_server :=  $(o)storage_server.o $(LIB-debug) $(LIB-configuration) $(LIB-transport) 

$(d)server := $(LIB-fasttransport) $(OBJS-storage_server) $(o)main.o

BINS += $(d)server
