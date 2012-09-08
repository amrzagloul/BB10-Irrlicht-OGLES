ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

USEFILE=

# Extra include path for libfreetype and for target overrides and patches
EXTRA_INCVPATH+=$(QNX_TARGET)/usr/include/freetype2 \
	$(QNX_TARGET)/../target-override/usr/include

# Extra library search path for target overrides and patches
EXTRA_LIBVPATH+=$(QNX_TARGET)/../target-override/$(CPUVARDIR)/lib \
	$(QNX_TARGET)/../target-override/$(CPUVARDIR)/usr/lib

# Compiler options for enhanced security and recording the compiler options in release builds
CCFLAGS+=-fstack-protector-all -D_FORTIFY_SOURCE=2 \
	$(if $(filter g so shared,$(VARIANTS)),,-fPIE) \
	$(if $(filter g,$(VARIANTS)),,-frecord-gcc-switches)

# Linker options for enhanced security
LDFLAGS+=-Wl,-z,relro -Wl,-z,now $(if $(filter g so shared,$(VARIANTS)),,-pie)

# Add your required library names, here
LIBS+= bps screen z bz2 m freetype EGL GLESv1_CM GLESv2 png

IRRLICHT_INC_DIRS := $(IRRLICHT_ROOT)/include
EXTRA_INCVPATH +=$(IRRLICHT_INC_DIRS)

IRRLICHT_SRC_DIRS := $(IRRLICHT_ROOT)/source/Irrlicht
EXTRA_SRCVPATH +=$(IRRLICHT_SRC_DIRS)
EXTRA_SRCVPATH +=$(IRRLICHT_SRC_DIRS)/lzma
EXTRA_SRCVPATH +=$(IRRLICHT_SRC_DIRS)/aesGladman

EXTRA_SRCVPATH +=$(IRRLICHT_SRC_DIRS)/jpeglib

CCFLAGS+= -D__BB10__


NAME = irrlicht

include $(MKFILES_ROOT)/qmacros.mk

EXCLUDE_OBJS += ansi2knr.o cjpeg.o djpeg.o rdbmp.o rdgif.o rdppm.o rdswitch.o transupp.o wrgif.o wrppm.o wrtarga.o\
	cdjpeg.o ckconfig.o example.o rdcolmap.o rdjpgcom.o rdrle.o rdtarga.o wrbmp.o wrjpgcom.o wrrle.o jmemdos.o\
	jmemmac.o jmemansi.o jmemname.o jpegtran.o

# Suppress the _g suffix from the debug variant
BUILDNAME=$(IMAGE_PREF_$(BUILD_TYPE))$(NAME)$(IMAGE_SUFF_$(BUILD_TYPE))

include $(MKFILES_ROOT)/qtargets.mk



OPTIMIZE_TYPE_g=none
OPTIMIZE_TYPE=$(OPTIMIZE_TYPE_$(filter g, $(VARIANTS)))
