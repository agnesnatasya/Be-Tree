d := $(dir $(lastword $(MAKEFILE_LIST)))

ifeq ($(IS_DEV),1)
FILES=simtransport_test.cpp
endif

SRCS += $(addprefix $(d), $(FILES))

ifeq ($(IS_DEV),1)
LIB-test := $(o)simtransport_test.o
endif