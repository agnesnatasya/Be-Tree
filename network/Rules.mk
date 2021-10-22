d := $(dir $(lastword $(MAKEFILE_LIST)))

ifeq ($(IS_DEV),0)
FILES = transport.cpp configuration.cpp simtransport.cpp fasttransport.cpp
else
FILES=transport.cpp configuration.cpp simtransport.cpp
endif

SRCS += $(addprefix $(d), $(FILES))

LIB-configuration := $(o)configuration.o $(LIB-debug)

LIB-transport := $(o)transport.o $(LIB-configuration) $(LIB-debug)

LIB-simtransport += $(o)simtransport.o

ifeq ($(IS_DEV),0)
LIB-transport_implementation := $(LIB-transport) $(LIB-debug) $(LIB-simtransport) $(o)fasttransport.o
else
LIB-transport_implementation := $(LIB-transport) $(LIB-debug) $(LIB-simtransport)
endif