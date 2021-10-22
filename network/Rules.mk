d := $(dir $(lastword $(MAKEFILE_LIST)))

FILES = transport.cpp configuration.cpp simtransport.cpp fasttransport.cpp

SRCS += $(addprefix $(d), $(FILES))

LIB-configuration := $(o)configuration.o $(LIB-debug)

LIB-transport := $(o)transport.o $(LIB-configuration) $(LIB-debug)

LIB-simtransport += $(o)simtransport.o

LIB-transport_implementation := $(LIB-transport) $(LIB-debug) $(LIB-simtransport) $(o)fasttransport.o
