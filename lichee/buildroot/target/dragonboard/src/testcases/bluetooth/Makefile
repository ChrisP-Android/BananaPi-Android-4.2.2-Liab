
# define sources root directory before everything
SRC_ROOT := ../..

# change compiler and linker option before you include rule.mk
#
# link to libscript.a when you need to fetch configuration
# from test_script
#
#CFLAGS := $(CFLAGS) -g
LDFLAGS := -lscript
# include rule.mk
include $(SRC_ROOT)/rule.mk

# define objects collection variable
bluetoothtester_objs = bluetoothtester.o

# add your target(s) to all
.PHONY: all
all: bluetoothtester
	cp bttester.sh $(BINDIR)/
	cp hcitool $(BINDIR)/
	cp cciconfig $(BINDIR)/
bluetoothtester: $(bluetoothtester_objs)
		$(LINK_MSG)
		$(LINKX)

# change 'example_objs' to your objects collection variable
$(bluetoothtester_objs): %.o: %.c
	$(COMPILE_MSG)
	$(COMPILEX)
