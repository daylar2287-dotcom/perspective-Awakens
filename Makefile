TARGET = PerspectiveAwakens
OBJS = src/main.o
INCDIR =
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LIBS = -lpspgum -lpspgu -lpspdebug -lpspdisplay -lpspge -lpspctrl -lm
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Perspective Awakens
BUILD_PRX = 1
PSP_FW_VERSION = 660
include $(PSPSDK)/lib/build.mak
