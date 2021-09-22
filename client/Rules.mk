d := $(dir $(lastword $(MAKEFILE_LIST)))

SRCS += $(addprefix $(d), test.cpp swap_space.cpp backing_store.cpp)

OBJS-swap_space := $(o)swap_space.o

OBJS-backing_store :=  $(o)backing_store.o

$(d)test: $(OBJS-swap_space) $(OBJS-backing_store) $(o)test.o

BINS += $(d)test



#test: test.cpp betree.hpp swap_space.o backing_store.o

#swap_space.o: swap_space.cpp swap_space.hpp backing_store.hpp

#backing_store.o: backing_store.hpp backing_store.cpp

#clean:
#	$(RM) *.o test
