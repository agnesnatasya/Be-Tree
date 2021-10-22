d := $(dir $(lastword $(MAKEFILE_LIST)))

FILES = transport.cpp configuration.cpp simtransport.cpp fasttransport.cpp

SRCS += $(addprefix $(d), $(FILES))

LIB-configuration := $(o)configuration.o $(LIB-debug)

LIB-transport := $(o)transport.o $(LIB-configuration) $(LIB-debug)

LIB-simtransport := $(LIB-transport) $(LIB-debug) $(o)simtransport.o

LIB-fasttransport := $(LIB-transport) $(LIB-debug) $(o)fasttransport.o
