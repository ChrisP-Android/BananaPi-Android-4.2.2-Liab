#
# Makefile for I2C gsensor chip drivers.
#

obj-$(CONFIG_SENSORS_BMA250)		+= bma250.o
obj-$(CONFIG_SENSORS_LIS3DH_ACC)        += lis3dh_acc.o
obj-$(CONFIG_SENSORS_LIS3DE_ACC)        += lis3de_acc.o
obj-$(CONFIG_SENSORS_DMARD10)		+= dmard10.o
obj-$(CONFIG_SENSORS_MXC622X)	        += mxc622x.o
obj-$(CONFIG_SENSORS_STK831X)	        += stk831x.o

ifeq ($(CONFIG_I2C_DEBUG_CHIP),y)
EXTRA_CFLAGS += -DDEBUG
endif
