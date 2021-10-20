d := $(dir $(lastword $(MAKEFILE_LIST)))

FILES=transport.cpp configuration.cpp
ifeq ($(IS_DEV),0)
FILES += fasttransport.cpp
endif

SRCS += $(addprefix $(d), $(FILES))

LIB-configuration := $(o)configuration.o $(LIB-debug)

LIB-transport := $(o)transport.o $(LIB-configuration) $(LIB-debug)

LIB-fasttransport := $(o)fasttransport.o $(LIB-transport) $(LIB-debug)
