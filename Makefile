#
# Top-level makefile
#

CC = gcc
CXX = g++
LD = g++
EXPAND = lib/tmpl/expand

ifeq ($(RPC),g)
RPC_PATH= "./third_party/grpc"
else ifeq ($(RPC),e)
RPC_PATH= "./third_party/eRPC"

#ERPC_CFLAGS_DPDK := -I $(RPC_PATH)/src -I /usr/include/dpdk -DDPDK=true -march=native
#ERPC_LDFLAGS_DPDK := -L $(RPC_PATH)/build -lerpc -lnuma -ldl -lgflags -ldpdk
endif

RPC_CFLAGS_RAW := -I $(RPC_PATH)/src -DRAW=true
RPC_LDFLAGS_RAW := -L $(RPC_PATH)/build -lerpc -lnuma -ldl -lgflags -libverbs

CFLAGS_WARNINGS:= -Wno-unused-function -Wno-nested-anon-types -Wno-keyword-macro -Wno-uninitialized

# -fno-omit-frame-pointer is needed to get accurate flame graphs. See [1] for
# more information.
#
# [1]: http://www.brendangregg.com/perf.html#FlameGraphs
CFLAGS := -g -Wall $(CFLAGS_WARNINGS) -iquote.obj/gen -O2 -DNASSERT -fno-omit-frame-pointer
CXXFLAGS := -g -std=c++11
LDFLAGS := -levent_pthreads -pthread -lboost_fiber -lboost_context -lboost_system -lboost_thread

## Add ERPC flags ##
CFLAGS += $(RPC_CFLAGS_RAW)
LDFLAGS += $(RPC_LDFLAGS_RAW)

## Debian package: check ##
#CHECK_CFLAGS := $(shell pkg-config --cflags check)
#CHECK_LDFLAGS := $(shell pkg-config --cflags --libs check)

## Debian package: libprotobuf-dev ##
#PROTOBUF_CFLAGS := $(shell pkg-config --cflags protobuf)
#PROTOBUF_LDFLAGS := $(shell pkg-config --cflags --libs protobuf)
#CFLAGS += $(PROTOBUF_CFLAGS)
#LDFLAGS += $(PROTOBUF_LDFLAGS)
#PROTOC := protoc

## Debian package: libevent-dev ##
LIBEVENT_CFLAGS := $(shell pkg-config --cflags libevent)
LIBEVENT_LDFLAGS := $(shell pkg-config --libs libevent)
CFLAGS += $(LIBEVENT_CFLAGS)
LDFLAGS += $(LIBEVENT_LDFLAGS)

## Debian package: libssl-dev ##
LIBSSL_CFLAGS := $(shell pkg-config --cflags openssl)
LIBSSL_LDFLAGS := $(shell pkg-config --libs openssl)
CFLAGS += $(LIBSSL_CFLAGS)
LDFLAGS += $(LIBSSL_LDFLAGS)

# Google test framework. This doesn't use pkgconfig
GTEST_DIR := /usr/src/gtest

# Additional flags
PARANOID = 1
ifneq ($(PARANOID),0)
override CFLAGS += -DPARANOID=1
$(info WARNING: Paranoid mode enabled)
endif

PERFTOOLS = 0
ifneq ($(PERFTOOLS),0)
override CFLAGS += -DPPROF=1
override LDFLAGS += -lprofiler
endif

# Make sure all is the default
.DEFAULT_GOAL := all

# Eliminate default suffix rules
.SUFFIXES:

# Delete target files if there is an error (or make is interrupted)
.DELETE_ON_ERROR:

# make it so that no intermediate .o files are ever deleted
.PRECIOUS: %.o

##################################################################
# Tracing
#

# Specify V=1 if you want it to be verbose
# Why is trace defined as = instead of :=
# Is it because it's like a function so it doesn't matter

ifeq ($(V),1)
trace = $(3)
Q =
else
trace = @printf "+ %-6s " $(1) ; echo $(2) ; $(3)
Q = @
endif

GTEST := .obj/gtest/gtest.a
GTEST_MAIN := .obj/gtest/gtest_main.a

##################################################################
# Sub-directories
#

# The directory of the current make fragment.  Each file should
# redefine this at the very top with
#  d := $(dir $(lastword $(MAKEFILE_LIST)))
d :=

# The object directory corresponding to the $(d)
o = .obj/$(d)

# SRCS is the list of all non-test-related source files.
SRCS :=
# TEST_SRCS is just like SRCS, but these source files will be compiled
# with testing related flags.
TEST_SRCS :=
# GTEST_SRCS is tests that use Google's testing framework
GTEST_SRCS :=

# PROTOS is the list of protobuf *.proto files
PROTOS :=

# BINS is a list of target names for non-test binaries.  These targets
# should depend on the appropriate object files, but should not
# contain any commands.
BINS :=
# TEST_BINS is like BINS, but for test binaries.  They will be linked
# using the appropriate flags.  This is also used as the list of tests
# to run for the `test' target.
TEST_BINS :=

# add-CFLAGS is a utility macro that takes a space-separated list of
# sources and a set of CFLAGS.  It sets the CFLAGS for each provided
# source.  This should be used like
#
#  $(call add-CFLAGS,$(d)a.c $(d)b.c,$(PG_CFLAGS))
# The first argument for (call add-CFLAGS) will be the $(1)
# https://www.gnu.org/software/make/manual/html_node/Foreach-Function.html
# https://www.gnu.org/software/make/manual/html_node/Eval-Function.html#Eval-Function

# After calling this, it will be like 
# CFLAGS-filename = the second argument to the (call add-CFLAGS)
define add-CFLAGS
$(foreach src,$(1),$(eval CFLAGS-$(src) += $(2)))
endef

# Like add-CFLAGS, but for LDFLAGS.  This should be given a list of
# binaries.
define add-LDFLAGS
$(foreach bin,$(1),$(eval LDFLAGS-$(bin) += $(2)))
endef

include debug/Rules.mk
include network/Rules.mk
include client/Rules.mk
include server/Rules.mk
#include replication/common/Rules.mk
#include replication/meerkatir/Rules.mk
#include replication/leadermeerkatir/Rules.mk
#include store/common/Rules.mk
#include store/meerkatstore/Rules.mk
#include store/meerkatstore/meerkatir/Rules.mk
#include store/meerkatstore/leadermeerkatir/Rules.mk
#include store/benchmark/Rules.mk
##################################################################
# General rules
#

#
# Protocols
#
#PROTOOBJS := $(PROTOS:%.proto=.obj/%.o)
#PROTOSRCS := $(PROTOS:%.proto=.obj/gen/%.pb.cc)
#PROTOHEADERS := $(PROTOS:%.proto=%.pb.h)

#$(PROTOSRCS) : .obj/gen/%.pb.cc: %.proto
#	@mkdir -p .obj/gen
#	$(call trace,PROTOC,$^,$(PROTOC) --cpp_out=.obj/gen $^)

#
# Compilation
#

# -MD Enable dependency generation and compilation and output to the
# .obj directory.  -MP Add phony targets so make doesn't complain if
# a header file is removed.  -MT Explicitly set the target in the
# generated rule to the object file we're generating.
# https://stackoverflow.com/questions/38220081/what-does-o-d-mean-in-a-makefile

DEPFLAGS = -M -MF ${@:.o=.d} -MP -MT $@ -MG

# $(call add-CFLAGS,$(TEST_SRCS),$(CHECK_CFLAGS))
# Translation of the cpp files to its obejcts
OBJS := $(SRCS:%.cpp=.obj/%.o) $(TEST_SRCS:%.cpp=.obj/%.o) $(GTEST_SRCS:%.cpp=.obj/%.o)

# https://stackoverflow.com/questions/14300541/wildcard-in-makefile-what-does-1-mean
# The call trace is basically setting trace the compilation progress i think

define compile
	@mkdir -p $(dir $@)
	$(call trace,$(1),$<,\
	  $(CC) -iquote. $(CFLAGS) $(CFLAGS-$<) $(2) $(DEPFLAGS) -E $<)
	$(Q)$(CC) -iquote. $(CFLAGS) $(CFLAGS-$<) $(2) -E -o .obj/$*.t $<
	$(Q)$(EXPAND) $(EXPANDARGS) -o .obj/$*.i .obj/$*.t
	$(Q)$(CC) $(CFLAGS) $(CFLAGS-$<) $(2) -c -o $@ .obj/$*.i
endef

# $@ is the target
# $< is the first frereq
# $(1) is the first argument after (call compilecxx, ...)

define compilecxx
	@mkdir -p $(dir $@)
	$(call trace,$(1),$<,\
	  $(CXX) -iquote. $(CFLAGS) $(CXXFLAGS) $(CFLAGS-$<) $(2) $(DEPFLAGS) -E $<)
	$(Q)$(CXX) -iquote. $(CFLAGS) $(CXXFLAGS) $(CFLAGS-$<) $(2) -c -o $@ $<
endef

# All object files come in two flavors: regular and
# position-independent.  PIC objects end in -pic.o instead of just .o.
# Link targets that build shared objects must depend on the -pic.o
# versions.
# Slightly different rules for protobuf object files
# because their source files have different locations.

# https://makefiletutorial.com/#static-pattern-rules

$(OBJS): .obj/%.o: %.cpp $(PROTOSRCS)
	$(call compilecxx,CC,)

# https://eli.thegreenplace.net/2011/11/03/position-independent-code-pic-in-shared-libraries/
# https://stackoverflow.com/questions/18026333/what-does-compiling-with-pic-dwith-pic-with-pic-actually-do

# Why does this one use the .cc while the above use the .cpp??

$(OBJS:%.o=%-pic.o): .obj/%-pic.o: %.cc $(PROTOSRCS)
	$(call compilecxx,CCPIC,-fPIC)

$(PROTOOBJS): .obj/%.o: .obj/gen/%.pb.cc
	$(call compilecxx,CC,)

$(PROTOOBJS:%.o=%-pic.o): .obj/%-pic.o: .obj/gen/%.pb.cc $(PROTOSRCS)
	$(call compilecxx,CCPIC,-fPIC)

#
# Linking
#

$(call add-LDFLAGS,$(TEST_BINS),$(CHECK_LDFLAGS))

# This means that it depends on everything? 
# Maybe this is because the binaries can only be built when everything is available
$(BINS) $(TEST_BINS): %:
	$(call trace,LD,$@,$(LD) -o $@ $^ $(LDFLAGS) $(LDFLAGS-$@))

#
# Automatic dependencies
#

DEPS := $(OBJS:.o=.d) $(OBJS:.o=-pic.d)

# Include the dependencies but ignore if anything deos not exist, don't raise error
-include $(DEPS)

#
# Testing
#
GTEST_INTERNAL_SRCS := $(wildcard $(GTEST_DIR)/src/*.cpp)
GTEST_OBJS := $(patsubst %.cpp,.obj/gtest/%.o,$(notdir $(GTEST_INTERNAL_SRCS)))

$(GTEST_OBJS): .obj/gtest/%.o: $(GTEST_DIR)/src/%.cpp
	$(call compilecxx,CC,-I$(GTEST_DIR) -Wno-missing-field-initializers)

$(GTEST) : .obj/gtest/gtest-all.o
	$(call trace,AR,$@,$(AR) $(ARFLAGS) $@ $^)

$(GTEST_MAIN) : .obj/gtest/gtest-all.o .obj/gtest/gtest_main.o
	$(call trace,AR,$@,$(AR) $(ARFLAGS) $@ $^)

#
# Cleaning
#

.PHONY: clean
clean:
	$(call trace,RM,binaries,rm -f $(BINS) $(TEST_BINS))
	$(call trace,RM,objects,rm -rf .obj)

#
# Debugging
#
print-%:
	@echo '$*=$($*)'

##################################################################
# Targets
#

.PHONY: all
all: 
	$(BINS)

# I think you don't call these target, you call the `make test` then it will call this

$(TEST_BINS:%=run-%): run-%: %
	$(call trace,RUN,$<,$<)

$(TEST_BINS:%=gdb-%): gdb-%: %
	$(call trace,GDB,$<,CK_FORK=no gdb $<)

# Why are they doing the same thing here?

.PHONY: test
test: $(TEST_BINS:%=run-%)
.PHONY: check
check: test

# https://www.tutorialspoint.com/unix_commands/etags.htm#:~:text=The%20etags%20program%20is%20used,understood%20by%20vi(1)%20.
.PHONY: TAGS
TAGS:
	$(Q)rm -f $@
	$(call trace,ETAGS,sources,\
	  etags $(SRCS) $(TEST_SRCS))
	$(call trace,ETAGS,headers,\
	  etags -a $(foreach dir,$(sort $(dir $(SRCS) $(TEST_SRCS))),\
		     $(wildcard $(dir)*.h)))

