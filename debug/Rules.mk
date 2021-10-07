d := $(dir $(lastword $(MAKEFILE_LIST)))

SRCS += $(addprefix $(d), message.cpp)

LIB-debug := $(o)message.o
