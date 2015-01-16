#
# Makefile for the input core drivers.
#

# Each configuration option enables a list of files.

obj-y					+= virtualmouse.o
obj-$(CONFIG_INPUT)		+= input-core.o
input-core-y := input.o input-compat.o input-mt.o ff-core.o

obj-$(CONFIG_INPUT_FF_MEMLESS)	+= ff-memless.o
obj-$(CONFIG_INPUT_POLLDEV)	+= input-polldev.o
obj-$(CONFIG_INPUT_SPARSEKMAP)	+= sparse-keymap.o

obj-$(CONFIG_INPUT_MOUSEDEV)	+= mousedev.o
obj-$(CONFIG_INPUT_JOYDEV)	+= joydev.o
obj-$(CONFIG_INPUT_EVDEV)	+= evdev.o
obj-$(CONFIG_INPUT_EVBUG)	+= evbug.o

obj-$(CONFIG_INPUT_KEYBOARD)	+= keyboard/
obj-$(CONFIG_INPUT_MOUSE)	+= mouse/
obj-$(CONFIG_INPUT_JOYSTICK)	+= joystick/
obj-$(CONFIG_INPUT_TABLET)	+= tablet/
obj-$(CONFIG_INPUT_TOUCHSCREEN)	+= touchscreen/
obj-$(CONFIG_INPUT_MISC)	+= misc/
obj-$(CONFIG_INPUT_GYR)		+= gyr/
obj-$(CONFIG_INPUT_E_COMPASS)	+= e_compass/
obj-$(CONFIG_INPUT_SW_TOUCHSCREEN)	+= sw_touchscreen/
obj-$(CONFIG_INPUT_LIGHT_SENSOR)	+= lightsensor/

obj-$(CONFIG_INPUT_APMPOWER)	+= apm-power.o
obj-$(CONFIG_INPUT_OF_MATRIX_KEYMAP) += of_keymap.o
obj-$(CONFIG_INPUT_KEYRESET)	+= keyreset.o
obj-$(CONFIG_INPUT_DEVICE)	+= sw_device.o
obj-$(CONFIG_INPUT_I2C_DEBUG)	+= i2c_device_debug.o
obj-$(CONFIG_INPUT_INIT)	+= init-input.o
