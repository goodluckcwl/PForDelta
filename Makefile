BOOSTDIR=/home/xczou/build/boost_1_53_0
INSTALLDIR=./build

all: libridcompress.a


DOFILES= \
  patchedframeofreference.o \
  gen_fixedlengthcode.o \
  pfordelta-c-interface.o \


CFLAGS_INCLUDE += \
   -I$(BOOSTDIR)/include
#  -I/home/xczou/build/timer/include \
  #-I. \
  #-I/usr/local/include/ \
  
CFLAGS_MACRO += \
  -D__STDC_LIMIT_MACROS \

DCXXFLAGS += \
  $(CFLAGS_INCLUDE) \
  $(CFLAGS_MACRO) \
  -g \
  -E \
  # kill the warning for __VA_ARGS__
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


unittest: $(DOFILES)
	$(CXX) -o $@ $+ $(UNITTEST_LDFLAGS)

libridcompress.a: patchedframeofreference.o gen_fixedlengthcode.o pfordelta-c-interface.o
	ar rcs libridcompress.a patchedframeofreference.o gen_fixedlengthcode.o pfordelta-c-interface.o

install:
	mkdir -p $(INSTALLDIR)
	mkdir -p $(INSTALLDIR)/lib
	mkdir -p $(INSTALLDIR)/include
	cp *.h $(INSTALLDIR)/include
	cp *.a $(INSTALLDIR)/lib

clean:
	rm -f unittest *.o *.a

.PHONY: all clean
