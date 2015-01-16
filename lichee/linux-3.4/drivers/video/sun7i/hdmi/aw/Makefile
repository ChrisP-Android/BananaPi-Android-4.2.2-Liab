PWD=$(shell pwd)
obj-$(CONFIG_LYCHEE_HDMI_HDCP_SUN7I) += hdcp.o
hdcp-objs := hdmi_core_hdcp.o hdmi_hal.o hdmi_interface.o hdmi_edid.o ../drv_hdmi.o ../dev_hdmi.o libhdcp.a

install: build
	cp hdcp.ko $(LICHEE_MOD_DIR)/hdcp.ko
	#if build-in ,please '#cp hdcp.ko'
build:
	@echo $(LICHEE_KDIR)
	cp libhdcp libhdcp.a
	$(MAKE) -C $(LICHEE_KDIR) M=$(PWD)

clean:
	@rm -rf *.o *.ko .*.cmd *.mod.c *.order *.symvers .tmp_versions *~