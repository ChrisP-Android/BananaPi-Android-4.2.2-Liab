#
# Makefile for the sw_touchscreen drivers.
#

# Each configuration option enables a list of files.

obj-$(CONFIG_SW_TOUCHSCREEN_GSLX680)    += gslX680.o
obj-$(CONFIG_SW_TOUCHSCREEN_GT82X)      += gt82x.o
obj-$(CONFIG_SW_TOUCHSCREEN_ZET622X)    += zet622x.o
obj-$(CONFIG_SW_TOUCHSCREEN_SUNXI_TS)   += sunxi-ts.o
obj-$(CONFIG_SW_TOUCHSCREEN_GT811)      += gt811.o

obj-$(CONFIG_SW_TOUCHSCREEN_GT9XX)      += gt9xx/
obj-$(CONFIG_SW_TOUCHSCREEN_FT5X)       += ft5x/
obj-$(CONFIG_TOUCHSCREEN_AW5306)        += aw5306_ts.o
aw5306_ts-objs := AW5306_ts.o AW5306_userpara.o $@libAW5306.a
obj-$(CONFIG_TOUCHSCREEN_ILITEK)        += ilitek_ts.o

