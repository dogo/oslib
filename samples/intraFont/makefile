TARGET = testIntraFont
OBJS = main.o

INCDIR =
CFLAGS = -O2 -g -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

BUILD_PRX = 1
PSP_FW_VERSION=371

LIBDIR =
LDFLAGS =

MYLIBS=
STDLIBS= -losl -lpng -lz \
         -lpspsdk -lpspctrl -lpspumd -lpsprtc -lpsppower -lpspgu -lpspaudiolib -lpspaudio -lpsphttp -lpspwlan -lm
LIBS=$(STDLIBS) $(MYLIBS)

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = intraFont Test

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

