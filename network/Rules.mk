d := $(dir $(lastword $(MAKEFILE_LIST)))

SRCS += $(addprefix $(d), transport.cpp fasttransport.cpp configuration.cpp)

LIB-configuration := $(o)configuration.o

LIB-transport := $(o)transport.o $(LIB-configuration)

LIB-fasttransport := $(o)fasttransport.o $(LIB-transport)
