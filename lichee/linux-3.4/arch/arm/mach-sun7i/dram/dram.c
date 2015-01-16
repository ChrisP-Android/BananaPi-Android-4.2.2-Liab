/*
 * arch/arm/mach-sun7i/include/mach/dram.h
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Pan Nan <pannan@Reuuimllatech.com>
 *
 * Dram Sysfs Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <mach/includes.h>


#define SUNXI_DRAM_DEBUG

#ifdef SUNXI_DRAM_DEBUG
    #define DRAM_DBG(x...)  printk(x)
    #define SHOW_HOST_CFG_REG(x)  printk("HostCfgReg: 0x%p\n",x)
#else
    #define DRAM_DBG(x...)  do{}while(0)
    #define SHOW_HOST_CFG_REG(x...)  do{}while(0)
#endif

static __dram_host_cfg_reg_t *HostCfgReg = NULL;
static struct dram_para_t *dram_para = NULL;
static struct kobject *port_kobj[HOST_PORT_SIZE];

#if defined(CONFIG_ARCH_SUN4I)
u8 host_port[HOST_PORT_SIZE] = {DRAM_HOST_NDMA,DRAM_HOST_ATH,DRAM_HOST_SATA,
            DRAM_HOST_SDHC,DRAM_HOST_USB1,DRAM_HOST_USB2,DRAM_HOST_CPU,
            DRAM_HOST_GPU,DRAM_HOST_BE0,DRAM_HOST_FE0,DRAM_HOST_CSI0,
            DRAM_HOST_TSC,DRAM_HOST_VE,DRAM_HOST_BE1,DRAM_HOST_FE1,
            DRAM_HOST_MP,DRAM_HOST_TVIN,DRAM_HOST_CSI1,DRAM_HOST_ACE,
            DRAM_HOST_DDMA,DRAM_HOST_GPS
};
#elif defined(CONFIG_ARCH_SUN5I) || defined(CONFIG_ARCH_SUN7I)
u8 host_port[HOST_PORT_SIZE] = {DRAM_HOST_CPU,DRAM_HOST_GPU,DRAM_HOST_BE,
            DRAM_HOST_FE,DRAM_HOST_CSI,DRAM_HOST_TSDM,DRAM_HOST_VE,
            DRAM_HOST_USB1,DRAM_HOST_NDMA,DRAM_HOST_ATH,DRAM_HOST_IEP,
            DRAM_HOST_SDHC,DRAM_HOST_DDMA,DRAM_HOST_GPS
};
#endif

struct class *dram_class;

static ssize_t dram_type_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	return sprintf(buf, "%u\n", dram_para->dram_type);
}

static ssize_t dram_bus_width_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	return sprintf(buf, "%u\n", dram_para->dram_bus_width);
}

static struct device_attribute dram_attrs[] = {
	__ATTR(dram_type, 0400, dram_type_show, NULL),
	__ATTR(dram_bus_width, 0400, dram_bus_width_show, NULL),
	__ATTR_NULL,
};


/* calculate host port id */
static int host_port_id_target(struct kobject *kobj, u8 *port)
{
    u8 i;
    for(i=0;i<ARRAY_SIZE(port_kobj);i++){
        if(kobj == port_kobj[i]){
            *port = host_port[i];
            break;
        }
    }
    return 0;
}

/* set command number according to port */
int dram_host_port_cmd_num_set(__dram_host_port_e port, unsigned int num)
{
    HostCfgReg = DRAM_HOST_CFG_PORT;
    SHOW_HOST_CFG_REG(HostCfgReg);
    HostCfgReg->CmdNum = num;
    return 0;
}
EXPORT_SYMBOL_GPL(dram_host_port_cmd_num_set);

/* get command number according to port */
int dram_host_port_cmd_num_get(__dram_host_port_e port)
{
    HostCfgReg = DRAM_HOST_CFG_PORT;
    SHOW_HOST_CFG_REG(HostCfgReg);
    return HostCfgReg->CmdNum;
}
EXPORT_SYMBOL_GPL(dram_host_port_cmd_num_get);

/* set wait state according to port */
int dram_host_port_wait_state_set(__dram_host_port_e port, unsigned int state)
{
    HostCfgReg = DRAM_HOST_CFG_PORT;
    SHOW_HOST_CFG_REG(HostCfgReg);
    HostCfgReg->WaitState = state;
    return 0;
}
EXPORT_SYMBOL_GPL(dram_host_port_wait_state_set);

/* get wait state according to port */
int dram_host_port_wait_state_get(__dram_host_port_e port)
{
    HostCfgReg = DRAM_HOST_CFG_PORT;
    SHOW_HOST_CFG_REG(HostCfgReg);
    return HostCfgReg->WaitState;
}
EXPORT_SYMBOL_GPL(dram_host_port_wait_state_get);

/* set priority level according to port */
int dram_host_port_prio_level_set(__dram_host_port_e port, unsigned int level)
{
    HostCfgReg = DRAM_HOST_CFG_PORT;
    SHOW_HOST_CFG_REG(HostCfgReg);
    HostCfgReg->PrioLevel = level;
    return 0;
}
EXPORT_SYMBOL_GPL(dram_host_port_prio_level_set);

/* get priority level according to port */
int dram_host_port_prio_level_get(__dram_host_port_e port)
{
    HostCfgReg = DRAM_HOST_CFG_PORT;
    SHOW_HOST_CFG_REG(HostCfgReg);
    return HostCfgReg->PrioLevel;
}
EXPORT_SYMBOL_GPL(dram_host_port_prio_level_get);

/* enable access according to port */
int dram_host_port_acs_enable(__dram_host_port_e port)
{
    HostCfgReg = DRAM_HOST_CFG_PORT;
    SHOW_HOST_CFG_REG(HostCfgReg);
    HostCfgReg->AcsEn = 1;
    return 0;
}
EXPORT_SYMBOL_GPL(dram_host_port_acs_enable);

/* disable access according to port */
int dram_host_port_acs_disable(__dram_host_port_e port)
{
    HostCfgReg = DRAM_HOST_CFG_PORT;
    SHOW_HOST_CFG_REG(HostCfgReg);
    HostCfgReg->AcsEn = 0;
    return 0;
}
EXPORT_SYMBOL_GPL(dram_host_port_acs_disable);

/* get access state according to port */
int dram_host_port_acs_get(__dram_host_port_e port)
{
    HostCfgReg = DRAM_HOST_CFG_PORT;
    SHOW_HOST_CFG_REG(HostCfgReg);
    return HostCfgReg->AcsEn;
}
EXPORT_SYMBOL_GPL(dram_host_port_acs_get);


static ssize_t cmd_num_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
    u8 port = 0;
    host_port_id_target(kobj, &port);
	return sprintf(buf, "%u\n", dram_host_port_cmd_num_get(port));
}

static ssize_t cmd_num_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	u8 num, port = 0;
    if(strlen(buf) < 2 || strlen(buf) > 4)
        return -EINVAL;
    if(strlen(buf) == 2) {
        if(buf[0] < '0' || buf[0] > '9')
            return -EINVAL;
    } else if(strlen(buf) == 3) {
        if(buf[0] < '1' || buf[0] > '9')
            return -EINVAL;
        if(buf[1] < '0' || buf[1] > '9')
            return -EINVAL;
    } else {
        if(buf[0] < '1' || buf[0] > '2')
            return -EINVAL;
        if(buf[0] == '1') {
            if((buf[1] < '0' || buf[1] > '9') || (buf[2] < '0' || buf[2] > '9'))
                return -EINVAL;
        } else {
            if((buf[1] < '0' || buf[1] > '5') || (buf[2] < '0' || buf[2] > '5'))
                return -EINVAL;
        }
    }
    num = simple_strtoul(buf, NULL, 10);
    host_port_id_target(kobj, &port);
    dram_host_port_cmd_num_set(port, num);
    return count;
}


static ssize_t wait_state_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
    u8 port = 0;
    host_port_id_target(kobj, &port);
	return sprintf(buf, "%u\n", dram_host_port_wait_state_get(port));
}

static ssize_t wait_state_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	u8 state, port = 0;
    if(strlen(buf) < 2 || strlen(buf) > 3)
        return -EINVAL;
    if(strlen(buf) == 2) {
        if(buf[0] < '0' || buf[0] > '9')
            return -EINVAL;
    } else {
        if(buf[0] != '1')
            return -EINVAL;
        if(buf[1] < '0' || buf[1] > '5')
            return -EINVAL;
    }
	state = simple_strtoul(buf, NULL, 10);
    host_port_id_target(kobj, &port);
    dram_host_port_wait_state_set(port, state);
    return count;
}


static ssize_t priority_level_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	u8 port = 0;
    host_port_id_target(kobj, &port);
    return sprintf(buf, "%u\n", dram_host_port_prio_level_get(port));
}

static ssize_t priority_level_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
    u8 level, port = 0;
    char value;
    if(strlen(buf) != 2)
        return -EINVAL;
    if(buf[0] < '0' || buf[0] > '3')
		return -EINVAL;
	value = buf[0];
    switch(value)
    {
        case '3':
            level = 3;
            break;
        case '2':
            level = 2;
            break;
        case '1':
            level = 1;
            break;
        case '0':
            level = 0;
            break;
        default:
            return -EINVAL;
    }
    host_port_id_target(kobj, &port);
    dram_host_port_prio_level_set(port, level);
    return count;
}


static ssize_t access_en_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
    u8 port = 0;
    host_port_id_target(kobj, &port);
	return sprintf(buf, "%u\n", dram_host_port_acs_get(port));
}

static ssize_t access_en_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
    u8 port = 0;
    char value;
    if(strlen(buf) != 2)
        return -EINVAL;
    if(buf[0] < '0' || buf[0] > '1')
		return -EINVAL;
    value = buf[0];
    host_port_id_target(kobj, &port);
    switch(value)
    {
        case '1':
            dram_host_port_acs_enable(port);
            break;
        case '0':
            dram_host_port_acs_disable(port);
            break;
        default:
            return -EINVAL;
    }
	return count;
}


static struct kobj_attribute cmd_num        = HOST_PORT_ATTR(cmd_num);
static struct kobj_attribute wait_state     = HOST_PORT_ATTR(wait_state);
static struct kobj_attribute priority_level = HOST_PORT_ATTR(priority_level);
static struct kobj_attribute access_en      = HOST_PORT_ATTR(access_en);

static const struct attribute *host_port_attrs[] = {
	&cmd_num.attr,
	&wait_state.attr,
    &priority_level.attr,
    &access_en.attr,
	NULL,
};

/* fetch dram sysconfig parameters */
static int dram_fetch_sysconfig_para(void)
{
    int ret = -1;
    dram_para = kzalloc(sizeof(struct dram_para_t), GFP_KERNEL);
    if (!dram_para) {
		ret = -ENOMEM;
        goto emalloc;
	}
	if(SCRIPT_PARSER_OK != script_parser_fetch("dram_para", "dram_type",
        &dram_para->dram_type, sizeof(dram_para->dram_type)/sizeof(unsigned int))){
		goto script_parser_fetch_err;
	}
    if(SCRIPT_PARSER_OK != script_parser_fetch("dram_para", "dram_bus_width",
        &dram_para->dram_bus_width, sizeof(dram_para->dram_bus_width)/sizeof(unsigned int))){
		goto script_parser_fetch_err;
	}

    return 0;

script_parser_fetch_err:
    kfree(dram_para);
emalloc:
	return ret;
}

static int dram_probe(struct platform_device *pdev)
{
	int ret,i;
    char host_port_name[16] = {0};

	device_create(dram_class, &pdev->dev, 0, NULL, "dram.0");

    for (i=0;i<ARRAY_SIZE(host_port);i++) {
        sprintf(host_port_name, "port.%d", host_port[i]);
        port_kobj[i] = kobject_create_and_add(host_port_name, &pdev->dev.kobj);
        if (!port_kobj[i]) {
            ret = -ENOMEM;
            DRAM_DBG("host port: Failed to create dram kobj\n");
            goto out_err1;
        }

        ret = sysfs_create_files(port_kobj[i], host_port_attrs);
        if (ret) {
            DRAM_DBG("host port: Failed to add attrs");
            goto out_err2;
        }
    }

    ret = dram_fetch_sysconfig_para();
    if (ret) {
        DRAM_DBG("fetch dram sysconfig failed\n");
        goto out_err2;
    }
    DRAM_DBG("%s finished!\n", __func__);

	return 0;

out_err2:
    kobject_put(port_kobj[i]);
out_err1:
    return ret;
}

static int __exit dram_remove(struct platform_device *pdev)
{
	device_unregister(&pdev->dev);

	return 0;
}

static struct platform_device dram_device = {
	.name		= "dram_sysfs",
};

static struct platform_driver dram_driver = {
	.probe		= dram_probe,
	.remove		= __exit_p(dram_remove),
	.driver		= {
		.name	= "dram_sysfs",
		.owner	= THIS_MODULE,
	},
};

static int __init dram_sys_init(void)
{
	dram_class = class_create(THIS_MODULE, "dram");
	if (IS_ERR(dram_class)) {
		DRAM_DBG("%s: couldn't create dram class\n", __FILE__);
		return PTR_ERR(dram_class);
	}
	dram_class->dev_attrs = dram_attrs;

	platform_device_register(&dram_device);
	platform_driver_register(&dram_driver);

	return 0;
}
module_init(dram_sys_init);

static void __exit dram_sys_exit(void)
{
	platform_driver_unregister(&dram_driver);
	platform_device_unregister(&dram_device);
	class_destroy(dram_class);
}
module_exit(dram_sys_exit);
