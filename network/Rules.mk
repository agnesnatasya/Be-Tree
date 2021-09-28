d := $(dir $(lastword $(MAKEFILE_LIST)))

SRCS += $(addprefix $(d), transport.cpp fasttransport.cpp configuration.cpp)

LIB-configuration := $(o)configuration.o $(LIB-debug)

LIB-transport := $(o)transport.o $(LIB-configuration) $(LIB-debug)

LIB-fasttransport := $(o)fasttransport.o $(LIB-transport) $(LIB-debug)
