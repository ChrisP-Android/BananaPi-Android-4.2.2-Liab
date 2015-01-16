#this file is used for Android compile configuration of AW_OMX

############################################################################
AW_OMX_EXT_CFLAGS :=

AW_OPENMAX_DEBUG_ENABLE := N
ifeq ($(AW_OPENMAX_DEBUG_ENABLE), Y)
AW_OPENMAX_DEBUG_CEDARV := Y
else
AW_OPENMAX_DEBUG_CEDARV := N
endif
ifeq ($(PLATFORM_VERSION),4.0.4)
AW_OMX_PLATFORM_VERSION := 6
AW_OMX_EXT_CFLAGS += -DAW_OMX_PLATFORM_VERSION=6
CEDARX_TOP := $(TOP)/frameworks/base/media/CedarX-Projects/CedarX
else
AW_OMX_PLATFORM_VERSION := 7
AW_OMX_EXT_CFLAGS += -DAW_OMX_PLATFORM_VERSION=7
CEDARX_TOP := $(TOP)/frameworks/av/media/CedarX-Projects/CedarX
endif
