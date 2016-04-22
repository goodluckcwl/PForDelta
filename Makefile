BOOSTDIR=/home/xczou/build/boost_1_53_0
INSTALLDIR=~/build/sith/pfordelta-adios

all: rpfdexp expperf

HFILES= \
  patchedframeofreference.h \
  varbyte.h \
  pfordelta.h \
  common.h \
  pfordelta-c-interface.h

EXP_PERF_DOTFILES= \
  patchedframeofreference.o \
  gen_fixedlengthcode.o \
  pfordelta.o \
  common.o \
  pfordelta-c-interface.o \
  test_exception_vs_nonexception.o \


RPF_EXP_DOTFILES= \
  patchedframeofreference.o \
  gen_fixedlengthcode.o \
  pfordelta.o \
  common.o \
  pfordelta-c-interface.o \
  test_rpfd_exp.o \

DOFILES= \
  patchedframeofreference.o \
  gen_fixedlengthcode.o \
  pfordelta.o \
  common.o \
  pfordelta-c-interface.o \
  test_patchedframeofreference.o \
  #test_exception_vs_nonexception.o \
 # varbyte.o \
  #test_compare.o \
  # test_interface.o \
 # test_pfordelta.o

CFLAGS_INCLUDE += \
   #-I$(BOOSTDIR)/include
#  -I/home/xczou/build/timer/include \
  #-I. \
  #-I/usr/local/include/ \
  
CFLAGS_MACRO += \
  -D__STDC_LIMIT_MACROS \

DCXXFLAGS += \
  $(CFLAGS_INCLUDE) \
  $(CFLAGS_MACRO) \
  -g \
  #-O2 \
  #-Wall \
  #-Wno-missing-braces \
  #-Wno-format \
  #-Wextra \
  #-fPIC \

LDFLAGS_LIBS += \
#   -L/home/xczou/build/timer/lib/libtimer.a #\
#   -lhpm

LDFLAGS_DIRS += \
   -L./ \

UNITTEST_LDFLAGS = \
  $(LDFLAGS_DIRS) \
  $(LDFLAGS_LIBS) \

CXX ?= g++

%.o: %.cpp $(HFILES)
	$(CXX) -o $@ $(DCXXFLAGS) -c $*.cpp

rpfdexp: $(RPF_EXP_DOTFILES)
	$(CXX) -o $@ $+ $(UNITTEST_LDFLAGS)
	
expperf: $(EXP_PERF_DOTFILES)
	$(CXX) -o $@ $+ $(UNITTEST_LDFLAGS)

unittest: $(DOFILES)
	$(CXX) -o $@ $+ $(UNITTEST_LDFLAGS)

libridcompress.a: patchedframeofreference.o gen_fixedlengthcode.o pfordelta-c-interface.o
	ar rcs libridcompress.a patchedframeofreference.o gen_fixedlengthcode.o pfordelta-c-interface.o

install:
	mkdir -p $(INSTALLDIR)
	mkdir -p $(INSTALLDIR)/lib
	mkdir -p $(INSTALLDIR)/include
	cp *.h $(INSTALLDIR)/include
	cp *.a $(INSTALLDIR)/include

clean:
	rm -f unittest *.o *.a

.PHONY: all clean
