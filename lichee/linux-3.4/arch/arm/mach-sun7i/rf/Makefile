
#
# Makefile for wifi bluetooth power controller drivers
#

obj-$(CONFIG_MMC)    += wifi_gpio.o

wifi_gpio-y := wifi_pm.o  wifi_pm_bcm40181.o wifi_pm_bcm40183.o \
	           wifi_pm_rtl8723as.o wifi_pm_rtl8189es.o wifi_pm_rtl8192cu.o \
	           wifi_pm_rtl8188eu.o wifi_pm_ap6xxx.o wifi_pm_rtl8723au.o

obj-$(CONFIG_RFKILL)  += bt_gpio.o

bt_gpio-y := bt_pm.o