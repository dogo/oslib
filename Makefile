#----------------------------------------------------------------------------
#   MAKEFILE
#
#   Controlling makefile for File Assistant
#
#   Created:    1st August 2005
#
#   Copyright (C) 1995-2005 T Swann
#----------------------------------------------------------------------------

#----------------------------------------------------------------------------
#   Target to make
#   --------------

TARGET_LIB      = libosl.a
TARGET          := OSLib
PSP_FW_VERSION  = 371

#----------------------------------------------------------------------------
#   Project folders
#   ---------------
LIB_DIR         := lib
SOURCE_DIR      := src

#----------------------------------------------------------------------------
#   Source to make
#   --------------

SFONTOBJS := $(SOURCE_DIR)/sfont.o

PSPMATHOBJS := \
	$(LIB_DIR)/libpspmath/src/printMatrixFloat.o \
	$(LIB_DIR)/libpspmath/src/vfpu_srand.o \
	$(LIB_DIR)/libpspmath/src/vfpu_randf.o \
	$(LIB_DIR)/libpspmath/src/vfpu_rand_8888.o \
	$(LIB_DIR)/libpspmath/src/vfpu_identity_matrix.o \
	$(LIB_DIR)/libpspmath/src/vfpu_translate_matrix.o \
	$(LIB_DIR)/libpspmath/src/vfpu_perspective_matrix.o \
	$(LIB_DIR)/libpspmath/src/vfpu_ortho_matrix.o \
	$(LIB_DIR)/libpspmath/src/vfpu_sinf.o \
	$(LIB_DIR)/libpspmath/src/vfpu_cosf.o \
	$(LIB_DIR)/libpspmath/src/vfpu_tanf.o \
	$(LIB_DIR)/libpspmath/src/vfpu_asinf.o \
	$(LIB_DIR)/libpspmath/src/vfpu_acosf.o \
	$(LIB_DIR)/libpspmath/src/vfpu_atanf.o \
	$(LIB_DIR)/libpspmath/src/vfpu_sinhf.o \
	$(LIB_DIR)/libpspmath/src/vfpu_coshf.o \
	$(LIB_DIR)/libpspmath/src/vfpu_tanhf.o \
	$(LIB_DIR)/libpspmath/src/vfpu_sincos.o \
	$(LIB_DIR)/libpspmath/src/vfpu_expf.o \
	$(LIB_DIR)/libpspmath/src/vfpu_logf.o \
	$(LIB_DIR)/libpspmath/src/vfpu_fabsf.o \
	$(LIB_DIR)/libpspmath/src/vfpu_sqrtf.o \
	$(LIB_DIR)/libpspmath/src/vfpu_powf.o \
	$(LIB_DIR)/libpspmath/src/vfpu_fmodf.o \
	$(LIB_DIR)/libpspmath/src/vfpu_fminf.o \
	$(LIB_DIR)/libpspmath/src/vfpu_fmaxf.o \
	$(LIB_DIR)/libpspmath/src/vfpu_ease_in_out.o \
	$(LIB_DIR)/libpspmath/src/vfpu_normalize_vector.o \
	$(LIB_DIR)/libpspmath/src/vfpu_zero_vector.o \
	$(LIB_DIR)/libpspmath/src/vfpu_scale_vector.o \
	$(LIB_DIR)/libpspmath/src/vfpu_add_vector.o \
	$(LIB_DIR)/libpspmath/src/vfpu_envmap_matrix.o \
	$(LIB_DIR)/libpspmath/src/vfpu_sphere_to_cartesian.o \
	$(LIB_DIR)/libpspmath/src/vfpu_quaternion_identity.o \
	$(LIB_DIR)/libpspmath/src/vfpu_quaternion_copy.o \
	$(LIB_DIR)/libpspmath/src/vfpu_quaternion_multiply.o \
	$(LIB_DIR)/libpspmath/src/vfpu_quaternion_normalize.o \
	$(LIB_DIR)/libpspmath/src/vfpu_quaternion_exp.o \
	$(LIB_DIR)/libpspmath/src/vfpu_quaternion_ln.o \
	$(LIB_DIR)/libpspmath/src/vfpu_quaternion_sample_linear.o \
	$(LIB_DIR)/libpspmath/src/vfpu_quaternion_from_euler.o \
	$(LIB_DIR)/libpspmath/src/vfpu_quaternion_to_matrix.o \
	$(LIB_DIR)/libpspmath/src/vfpu_quaternion_sample_hermite.o \
	$(LIB_DIR)/libpspmath/src/vfpu_quaternion_hermite_tangent.o

INTRAFONTOBJS := \
	$(LIB_DIR)/libintraFont/src/intraFont.o \
	$(LIB_DIR)/libintraFont/src/libccc.o

GIFLIBOBJS := \
	$(LIB_DIR)/giflib/gif2rgb.o \
	$(LIB_DIR)/giflib/dgif_lib.o \
	$(LIB_DIR)/giflib/egif_lib.o \
	$(LIB_DIR)/giflib/gif_err.o \
	$(LIB_DIR)/giflib/gifalloc.o \
	$(LIB_DIR)/giflib/quantize.o

LIBOBJS := \
	$(SFONTOBJS) \
	$(PSPMATHOBJS) \
	$(SOURCE_DIR)/oslib.o \
	$(SOURCE_DIR)/vfpu.o \
	$(SOURCE_DIR)/drawing.o \
	$(SOURCE_DIR)/image.o \
	$(SOURCE_DIR)/palette.o \
	$(SOURCE_DIR)/shape.o \
	$(SOURCE_DIR)/map.o \
	$(SOURCE_DIR)/messagebox.o \
	$(SOURCE_DIR)/oslHandleLoadNoFailError.o \
	$(SOURCE_DIR)/keys.o \
	$(SOURCE_DIR)/text.o \
	$(SOURCE_DIR)/vram_mgr.o \
	$(SOURCE_DIR)/stub.o \
	$(SOURCE_DIR)/audio/audio.o \
	$(SOURCE_DIR)/audio/bgm.o \
	$(SOURCE_DIR)/audio/mod.o \
	$(SOURCE_DIR)/audio/media.o \
	$(SOURCE_DIR)/usb.o \
	$(SOURCE_DIR)/dialog.o \
	$(SOURCE_DIR)/osk.o \
	$(SOURCE_DIR)/saveload.o \
	$(SOURCE_DIR)/net.o \
	$(SOURCE_DIR)/browser.o \
	$(SOURCE_DIR)/adhoc/pspadhoc.o \
	$(SOURCE_DIR)/vfile/VirtualFile.o \
	$(SOURCE_DIR)/vfile/vfsFile.o \
	$(SOURCE_DIR)/image/oslConvertImageTo.o \
	$(SOURCE_DIR)/image/oslSetImagePixel.o \
	$(SOURCE_DIR)/image/oslGetImagePixel.o \
	$(SOURCE_DIR)/image/oslDrawImage.o \
	$(SOURCE_DIR)/image/oslDrawImageSimple.o \
	$(SOURCE_DIR)/image/oslDrawImageBig.o \
	$(SOURCE_DIR)/image/oslLockImage.o \
	$(SOURCE_DIR)/image/oslMoveImageTo.o \
	$(SOURCE_DIR)/image/oslSwizzleImage.o \
	$(SOURCE_DIR)/image/oslUnswizzleImage.o \
	$(SOURCE_DIR)/image/oslSetDrawBuffer.o \
	$(SOURCE_DIR)/image/oslResetImageProperties.o \
	$(SOURCE_DIR)/image/oslScaleImage.o \
	$(SOURCE_DIR)/image/format/oslLoadImageFilePNG.o \
	$(SOURCE_DIR)/image/format/oslWriteImageFilePNG.o \
	$(SOURCE_DIR)/image/format/oslLoadImageFileJPEG.o \
	$(SOURCE_DIR)/image/format/oslLoadImageFileGIF.o \
	$(SOURCE_DIR)/image/oslLoadImageFile.o \
	$(SOURCE_DIR)/image/oslWriteImageFile.o \
	$(SOURCE_DIR)/splash/oslShowSplashScreen1.o \
	$(SOURCE_DIR)/splash/oslShowSplashScreen2.o \
	$(SOURCE_DIR)/mem/oslGetRamStatus.o

OBJS := $(LIBOBJS) $(INTRAFONTOBJS) $(GIFLIBOBJS)

#----------------------------------------------------------------------------
#   Additional includes
#   -------------------

INCDIR := $(INCDIR) \
	$(SOURCE_DIR) \
	$(LIB_DIR)/libintraFont/include \
	$(LIB_DIR)/giflib \
	$(LIB_DIR)/libpspmath/include

#----------------------------------------------------------------------------
#   Additional libraries
#   ---------------------
SDK_LIBS := \
	-lpspsdk \
	-lpspctrl -lpsphprm \
	-lpspumd \
	-lpsprtc \
	-lpspgu -lpspgum \
	-lpspaudiolib \
	-lpspaudio -lpspaudiocodec \
	-lpsppower \
	-lpspusb -lpspusbstor \
	-lpsputility \
	-lpspssl -lpsphttp -lpspwlan \
	-lpspnet_adhoc -lpspnet_adhocctl -lpspnet_adhocmatching

EXTERN_LIBS := \
	-lpng -ljpeg \
	-lz

LIBS := $(EXTERN_LIBS) $(SDK_LIBS) -lm

#----------------------------------------------------------------------------
#   Preprocessor defines
#   --------------------

DEFINES := \
	-D_DEBUG \
	-DPSP \
	-DOSL_IMAGE_LOADER_PNG \
	-DOSL_IMAGE_LOADER_JPEG \
	-DOSL_IMAGE_LOADER_GIF \
	-DOSL_IMAGE_WRITER_PNG

#----------------------------------------------------------------------------
#   Compiler settings
#   -----------------

CFLAGS   := $(DEFINES) -O2 -G0 -ggdb -Wall -DHAVE_AV_CONFIG_H -fno-strict-aliasing -fverbose-asm
CXXFLAGS := $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS  := $(CFLAGS)

# Suppress warnings for third-party library sources
CFLAGS_THIRD_PARTY := $(DEFINES) -O2 -G0 -ggdb -w -DHAVE_AV_CONFIG_H -fno-strict-aliasing -fverbose-asm

LIBDIR   :=
LDFLAGS  :=

#----------------------------------------------------------------------------
#   PBP parameters
#   --------------

#EXTRA_TARGETS :=
#PSP_EBOOT_ICON :=
#PSP_EBOOT_PIC1 :=
#PSP_EBOOT_TITLE :=

#----------------------------------------------------------------------------
#   Default build settings
#   ----------------------

#test:
#   echo $(OBJS)

PSPSDK := $(shell psp-config --pspsdk-path)
PSPDIR := $(shell psp-config --psp-prefix)

include $(PSPSDK)/lib/build.mak

#----------------------------------------------------------------------------
#   Copy to PSP
#   -----------

lib: $(STATICLIB)

$(STATICLIB): $(LIBOBJS)
	$(AR) rcs $@ $(LIBOBJS)
	$(RANLIB) $@

install: lib
	install -d $(DESTDIR)$(PSPDIR)/lib
	install -m644 $(TARGET_LIB) $(DESTDIR)$(PSPDIR)/lib
	install -d $(DESTDIR)$(PSPDIR)/include/oslib/
	install -d $(DESTDIR)$(PSPDIR)/include/oslib/adhoc/
	install -m644 $(LIB_DIR)/libintraFont/include/intraFont.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(LIB_DIR)/libintraFont/include/libccc.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(LIB_DIR)/libpspmath/include/pspmath.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/adhoc/pspadhoc.h $(DESTDIR)$(PSPDIR)/include/oslib/adhoc/
	install -m644 $(SOURCE_DIR)/oslmath.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/net.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/browser.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/audio.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/bgm.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/dialog.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/drawing.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/keys.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/map.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/messagebox.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/osk.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/saveload.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/oslib.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/text.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/usb.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/vfpu_ops.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/VirtualFile.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/vram_mgr.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/ccc.h $(DESTDIR)$(PSPDIR)/include/oslib/
	install -m644 $(SOURCE_DIR)/sfont.h $(DESTDIR)$(PSPDIR)/include/oslib/

gendoc:
	doxygen

release: lib
	./release.sh

ghpages: gendoc
	rm -rf /tmp/ghpages
	mkdir -p /tmp/ghpages
	cp -Rv OSLib_Documentation/html/* /tmp/ghpages
	cd /tmp/ghpages && \
		git init && \
		git config user.name "$${GIT_AUTHOR_NAME:-github-actions}" && \
		git config user.email "$${GIT_AUTHOR_EMAIL:-github-actions@github.com}" && \
		git add . && \
		git commit -q -m "Automatic gh-pages"
	cd /tmp/ghpages && \
		git remote add remote https://x-access-token:$${GITHUB_TOKEN}@github.com/dogo/oslib.git && \
		git push --force remote +master:gh-pages
	rm -rf /tmp/ghpages

#----------------------------------------------------------------------------
#   Third-party library rules (suppress warnings)
#   ---------------------------------------------

# libpspmath objects
$(PSPMATHOBJS): %.o: %.c
	$(CC) $(CFLAGS_THIRD_PARTY) $(addprefix -I,$(INCDIR)) -c $< -o $@

# giflib objects
$(GIFLIBOBJS): %.o: %.c
	$(CC) $(CFLAGS_THIRD_PARTY) $(addprefix -I,$(INCDIR)) -c $< -o $@

# libintraFont objects
$(INTRAFONTOBJS): %.o: %.c
	$(CC) $(CFLAGS_THIRD_PARTY) $(addprefix -I,$(INCDIR)) -c $< -o $@
