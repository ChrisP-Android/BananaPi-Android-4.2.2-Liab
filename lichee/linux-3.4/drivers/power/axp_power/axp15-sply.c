/*
 * Battery charger driver for KrossPower AXP15X
 *
 * Copyright (C) 2012 X-Power, Ltd.
 *  Zhang Donglu <zhangdonglu@x-powers.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/slab.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/input.h>
#include <linux/mfd/axp-mfd.h>
#include <asm/div64.h>

#include <mach/sys_config.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include "axp-cfg.h"
#include "axp-sply.h"

#define DBG_AXP_PSY 1
#if  DBG_AXP_PSY
#define DBG_PSY_MSG(format,args...)   printk("[AXP]"format,##args)
#else
#define DBG_PSY_MSG(format,args...)   do {} while (0)
#endif
static int axp_debug = 0;
struct axp_adc_res adc;
struct delayed_work usbwork;
#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend axp_early_suspend;
int early_suspend_flag = 0;
#endif

int pmu_usbvolnew ;
int pmu_usbcurnew ;
int axp_usbcurflag ;
int axp_usbvolflag ;

int axp_usb_det(void)
{
    uint8_t ret;
    
    axp_read(axp_charger->master, AXP20_CHARGE_STATUS, &ret);
    if(ret & 0x10)
        return 1;
    else
        return 0;
}
EXPORT_SYMBOL_GPL(axp_usb_det);

int axp_usbvol(void)
{
	axp_usbvolflag = 1;
    return 0;
}
EXPORT_SYMBOL_GPL(axp_usbvol);

int axp_usbcur(void)
{
    axp_usbcurflag = 1;
    return 0;
}
EXPORT_SYMBOL_GPL(axp_usbcur);

int axp_usbvol_restore(void)
{
 	axp_usbvolflag = 0;
    return 0;
}
EXPORT_SYMBOL_GPL(axp_usbvol_restore);

int axp_usbcur_restore(void)
{
	axp_usbcurflag = 0;
    return 0;
}
EXPORT_SYMBOL_GPL(axp_usbcur_restore);

static ssize_t axpdebug_store(struct class *class,
			struct class_attribute *attr,	const char *buf, size_t count)
{
	if(buf[0] == '1'){
	   axp_debug = 1;
    }
    else{
	   axp_debug = 0;
    }
	return count;
}

static ssize_t axpdebug_show(struct class *class,
			struct class_attribute *attr,	char *buf)
{
	return sprintf(buf, "bat-debug value is %d\n", axp_debug);
}

static struct class_attribute axppower_class_attrs[] = {
	__ATTR(axpdebug,S_IRUGO|S_IWUSR,axpdebug_show,axpdebug_store),
	__ATTR_NULL
};
static struct class axppower_class = {
    .name = "axppower",
    .class_attrs = axppower_class_attrs,
};



static enum power_supply_property axp_battery_props[] = {
  POWER_SUPPLY_PROP_MODEL_NAME,
  POWER_SUPPLY_PROP_STATUS,
  POWER_SUPPLY_PROP_PRESENT,
  POWER_SUPPLY_PROP_ONLINE,
  POWER_SUPPLY_PROP_HEALTH,
  POWER_SUPPLY_PROP_TECHNOLOGY,
  POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
  POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
  POWER_SUPPLY_PROP_VOLTAGE_NOW,
  POWER_SUPPLY_PROP_CURRENT_NOW,
  //POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
  //POWER_SUPPLY_PROP_CHARGE_FULL,
  POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN,
  POWER_SUPPLY_PROP_CAPACITY,
  //POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
  //POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
  POWER_SUPPLY_PROP_TEMP,
};

static enum power_supply_property axp_ac_props[] = {
  POWER_SUPPLY_PROP_MODEL_NAME,
  POWER_SUPPLY_PROP_PRESENT,
  POWER_SUPPLY_PROP_ONLINE,
  POWER_SUPPLY_PROP_VOLTAGE_NOW,
  POWER_SUPPLY_PROP_CURRENT_NOW,
};

static enum power_supply_property axp_usb_props[] = {
  POWER_SUPPLY_PROP_MODEL_NAME,
  POWER_SUPPLY_PROP_PRESENT,
  POWER_SUPPLY_PROP_ONLINE,
  POWER_SUPPLY_PROP_VOLTAGE_NOW,
  POWER_SUPPLY_PROP_CURRENT_NOW,
};


static void axp_battery_check_status(struct axp_charger *charger,
            union power_supply_propval *val)
{
    val->intval = POWER_SUPPLY_STATUS_FULL;
}


static void axp_battery_check_health(struct axp_charger *charger,
            union power_supply_propval *val)
{
    val->intval = POWER_SUPPLY_HEALTH_GOOD;
}


static int axp_battery_get_property(struct power_supply *psy,
           enum power_supply_property psp,
           union power_supply_propval *val)
{
  struct axp_charger *charger;
  int ret = 0;
  charger = container_of(psy, struct axp_charger, batt);

  switch (psp) {
  case POWER_SUPPLY_PROP_STATUS:
    axp_battery_check_status(charger, val);
    break;
  case POWER_SUPPLY_PROP_HEALTH:
    axp_battery_check_health(charger, val);
    break;
  case POWER_SUPPLY_PROP_TECHNOLOGY:
    val->intval = charger->battery_info->technology;
    break;
  case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
    val->intval = charger->battery_info->voltage_max_design;
    break;
  case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
    val->intval = charger->battery_info->voltage_min_design;
    break;
  case POWER_SUPPLY_PROP_VOLTAGE_NOW:
    val->intval = charger->ocv * 1000;
    break;
  case POWER_SUPPLY_PROP_CURRENT_NOW:
    val->intval = charger->ibat * 1000;
    break;
  case POWER_SUPPLY_PROP_MODEL_NAME:
    val->strval = charger->batt.name;
    break;

  case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
    val->intval = charger->battery_info->energy_full_design;
       break;
  case POWER_SUPPLY_PROP_CAPACITY:
  	charger->rest_vol = 100;
    val->intval = charger->rest_vol;
    break;
  case POWER_SUPPLY_PROP_ONLINE:
    val->intval = (!charger->is_on)&&(charger->bat_det) && (! charger->ext_valid);
    break;
  case POWER_SUPPLY_PROP_PRESENT:
    val->intval = charger->bat_det;
    break;
  case POWER_SUPPLY_PROP_TEMP:
    //val->intval = charger->ic_temp - 150;
    val->intval =  300;
    break;
  default:
    ret = -EINVAL;
    break;
  }

  return ret;
}

static int axp_ac_get_property(struct power_supply *psy,
           enum power_supply_property psp,
           union power_supply_propval *val)
{
  struct axp_charger *charger;
  int ret = 0;
  charger = container_of(psy, struct axp_charger, ac);

  switch(psp){
  case POWER_SUPPLY_PROP_MODEL_NAME:
    val->strval = charger->ac.name;break;
  case POWER_SUPPLY_PROP_PRESENT:
    val->intval = charger->ac_det;
    break;
  case POWER_SUPPLY_PROP_ONLINE:
    val->intval = charger->ac_valid;break;
  case POWER_SUPPLY_PROP_VOLTAGE_NOW:
    val->intval = charger->vac * 1000;
    break;
  case POWER_SUPPLY_PROP_CURRENT_NOW:
    val->intval = charger->iac * 1000;
    break;
  default:
    ret = -EINVAL;
    break;
  }
   return ret;
}

static int axp_usb_get_property(struct power_supply *psy,
           enum power_supply_property psp,
           union power_supply_propval *val)
{
  struct axp_charger *charger;
  int ret = 0;
  charger = container_of(psy, struct axp_charger, usb);

  switch(psp){
  case POWER_SUPPLY_PROP_MODEL_NAME:
    val->strval = charger->usb.name;break;
  case POWER_SUPPLY_PROP_PRESENT:
    val->intval = charger->usb_det;
    break;
  case POWER_SUPPLY_PROP_ONLINE:
    val->intval = charger->usb_valid;
    break;
  case POWER_SUPPLY_PROP_VOLTAGE_NOW:
    val->intval = charger->vusb * 1000;
    break;
  case POWER_SUPPLY_PROP_CURRENT_NOW:
    val->intval = charger->iusb * 1000;
    break;
  default:
    ret = -EINVAL;
    break;
  }
   return ret;
}


static void axp_presslong(struct axp_charger *charger)
{
	DBG_PSY_MSG("press long\n");
	input_report_key(powerkeydev, KEY_POWER, 1);
	input_sync(powerkeydev);
	ssleep(2);
	DBG_PSY_MSG("press long up\n");
	input_report_key(powerkeydev, KEY_POWER, 0);
	input_sync(powerkeydev);
}

static void axp_pressshort(struct axp_charger *charger)
{
	DBG_PSY_MSG("press short\n");
  input_report_key(powerkeydev, KEY_POWER, 1);
  input_sync(powerkeydev);
  msleep(100);
  input_report_key(powerkeydev, KEY_POWER, 0);
  input_sync(powerkeydev);
}


static int axp_battery_event(struct notifier_block *nb, unsigned long event,
        void *data)
{
    struct axp_charger *charger =
    container_of(nb, struct axp_charger, nb);

    uint8_t w[5];

    w[0] = (uint8_t) ((event) & 0xFF);
    w[1] = POWER15_INTSTS2;
    w[2] = (uint8_t) ((event >> 8) & 0xFF);
    w[3] = POWER15_INTSTS3;
    w[4] = (uint8_t) ((event >> 16) & 0xFF);

    if(event & AXP15_IRQ_PEKLO) {
    	axp_presslong(charger);
    }

    if(event & AXP15_IRQ_PEKSH) {
    	axp_pressshort(charger);
    }

    DBG_PSY_MSG("event = 0x%x\n",(int) event);
    axp_writes(charger->master,POWER15_INTSTS1,9,w);

    return 0;
}


static char *supply_list[] = {
  "battery",
};


static void axp_battery_setup_psy(struct axp_charger *charger)
{
  struct power_supply *batt = &charger->batt;         
  struct power_supply *ac = &charger->ac;
  struct power_supply *usb = &charger->usb;
  struct power_supply_info *info = charger->battery_info;       

/*may not used*/
  batt->name = "battery";
  batt->use_for_apm = info->use_for_apm;
  batt->type = POWER_SUPPLY_TYPE_BATTERY;
  batt->get_property = axp_battery_get_property;

  batt->properties = axp_battery_props;
  batt->num_properties = ARRAY_SIZE(axp_battery_props);

  ac->name = "ac";
  ac->type = POWER_SUPPLY_TYPE_MAINS;
  ac->get_property = axp_ac_get_property;

  ac->supplied_to = supply_list,
  ac->num_supplicants = ARRAY_SIZE(supply_list),

  ac->properties = axp_ac_props;
  ac->num_properties = ARRAY_SIZE(axp_ac_props);

  usb->name = "usb";
  usb->type = POWER_SUPPLY_TYPE_USB;
  usb->get_property = axp_usb_get_property;

  usb->supplied_to = supply_list,
  usb->num_supplicants = ARRAY_SIZE(supply_list),

  usb->properties = axp_usb_props;
  usb->num_properties = ARRAY_SIZE(axp_usb_props);
};


#if defined CONFIG_HAS_EARLYSUSPEND
static void axp_earlysuspend(struct early_suspend *h)
{
	DBG_PSY_MSG("======early suspend=======\n");

#if defined (CONFIG_AXP_CHGCHANGE)
  	early_suspend_flag = 1;
#endif

}
static void axp_lateresume(struct early_suspend *h)
{
	DBG_PSY_MSG("======late resume=======\n");

#if defined (CONFIG_AXP_CHGCHANGE)
	early_suspend_flag = 0;
#endif

}
#endif

static void axp_charging_monitor(struct work_struct *work)
{
    struct axp_charger *charger;

    charger->rest_vol = 100;
    power_supply_changed(&charger->batt);

    /* reschedule for the next time */
    schedule_delayed_work(&charger->work, charger->interval);
}

static int axp_battery_probe(struct platform_device *pdev)
{
  struct axp_charger *charger;
  struct axp_supply_init_data *pdata = pdev->dev.platform_data;
  int ret;
  
  powerkeydev = input_allocate_device();
  if (!powerkeydev) {
    kfree(powerkeydev);
    return -ENODEV;
  }

  powerkeydev->name = pdev->name;
  powerkeydev->phys = "m1kbd/input2";
  powerkeydev->id.bustype = BUS_HOST;
  powerkeydev->id.vendor = 0x0001;
  powerkeydev->id.product = 0x0001;
  powerkeydev->id.version = 0x0100;
  powerkeydev->open = NULL;
  powerkeydev->close = NULL;
  powerkeydev->dev.parent = &pdev->dev;

  set_bit(EV_KEY, powerkeydev->evbit);
  set_bit(EV_REL, powerkeydev->evbit);
  //set_bit(EV_REP, powerkeydev->evbit);
  set_bit(KEY_POWER, powerkeydev->keybit);

  ret = input_register_device(powerkeydev);
  if(ret) {
    printk("Unable to Register the power key\n");
    }

  if (pdata == NULL)
    return -EINVAL;

  if (pdata->chgcur > 1800000 ||
      pdata->chgvol < 4100000 ||
      pdata->chgvol > 4360000){
        printk("charger milliamp is too high or target voltage is over range\n");
        return -EINVAL;
    }

  if (pdata->chgpretime < 40 || pdata->chgpretime >70 ||
    pdata->chgcsttime < 360 || pdata->chgcsttime > 720){
            printk("prechaging time or constant current charging time is over range\n");
        return -EINVAL;
  }

  charger = kzalloc(sizeof(*charger), GFP_KERNEL);
  if (charger == NULL)
    return -ENOMEM;

  charger->master = pdev->dev.parent;

  charger->chgcur      = pdata->chgcur;
  charger->chgvol     = pdata->chgvol;
  charger->chgend           = pdata->chgend;
  charger->sample_time          = pdata->sample_time;
  charger->chgen                   = pdata->chgen;
  charger->chgpretime      = pdata->chgpretime;
  charger->chgcsttime = pdata->chgcsttime;
  charger->battery_info         = pdata->battery_info;
  charger->disvbat			= 0;
  charger->disibat			= 0;

  charger->nb.notifier_call = axp_battery_event;
  ret = axp_register_notifier(charger->master, &charger->nb, AXP15_NOTIFIER_ON);
  if (ret)
    goto err_notifier;

  axp_battery_setup_psy(charger);
  ret = power_supply_register(&pdev->dev, &charger->batt);
  if (ret)
    goto err_ps_register;

  ret = power_supply_register(&pdev->dev, &charger->ac);
  if (ret){
    power_supply_unregister(&charger->batt);
    goto err_ps_register;
  }
  
  ret = power_supply_register(&pdev->dev, &charger->usb);
  if (ret){
    power_supply_unregister(&charger->ac);
    power_supply_unregister(&charger->batt);
    goto err_ps_register;
  }

  platform_set_drvdata(pdev, charger);

	  axp_charger = charger;

#ifdef CONFIG_HAS_EARLYSUSPEND
	  axp_early_suspend.suspend = axp_earlysuspend;
	  axp_early_suspend.resume = axp_lateresume;
	  axp_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 2;
	  register_early_suspend(&axp_early_suspend);
#endif

	  /* µ÷ÊÔ½Ó¿Ú×¢²á */
	  class_register(&axppower_class);

	return ret;

  err_ps_register:
	axp_unregister_notifier(charger->master, &charger->nb, AXP15_NOTIFIER_ON);

  err_notifier:
	cancel_delayed_work_sync(&charger->work);

  err_charger_init:
	kfree(charger);
	input_unregister_device(powerkeydev);
	kfree(powerkeydev);

	return ret;
}


static int axp_battery_remove(struct platform_device *dev)
{
    struct axp_charger *charger = platform_get_drvdata(dev);

    if(main_task){
        kthread_stop(main_task);
        main_task = NULL;
    }

    axp_unregister_notifier(charger->master, &charger->nb, AXP15_NOTIFIER_ON);
    cancel_delayed_work_sync(&charger->work);
    power_supply_unregister(&charger->usb);
    power_supply_unregister(&charger->ac);
    power_supply_unregister(&charger->batt);

    kfree(charger);
    input_unregister_device(powerkeydev);
    kfree(powerkeydev);

    return 0;
}


static int axp15_suspend(struct platform_device *dev, pm_message_t state)
{
    uint8_t irq_w[5];

    struct axp_charger *charger = platform_get_drvdata(dev);

	
    cancel_delayed_work_sync(&charger->work);
    /*clear all irqs events*/
    irq_w[0] = 0xff;
    irq_w[1] = POWER15_INTSTS2;
    irq_w[2] = 0xff;
    irq_w[3] = POWER15_INTSTS3;
    irq_w[4] = 0xff;

    axp_writes(charger->master, POWER15_INTSTS1, 5, irq_w);

    /* close all irqs*/
    axp_unregister_notifier(charger->master, &charger->nb, AXP15_NOTIFIER_ON);
    return 0;
}

static int axp15_resume(struct platform_device *dev)
{
    struct axp_charger *charger = platform_get_drvdata(dev);

    axp_register_notifier(charger->master, &charger->nb, AXP15_NOTIFIER_ON);
	printk("%s,line:%d\n", __func__, __LINE__);

	charger->rest_vol = 100;
	printk("%s,line:%d\n", __func__, __LINE__);

    return 0;
}

static void axp15_shutdown(struct platform_device *dev)
{
    struct axp_charger *charger = platform_get_drvdata(dev);

    cancel_delayed_work_sync(&charger->work);
  	//axp_clr_bits(charger->master,AXP15_CAP,0x80);
}

static struct platform_driver axp_battery_driver = {
  .driver = {
    .name = "axp15-supplyer",
    .owner  = THIS_MODULE,
  },
  .probe = axp_battery_probe,
  .remove = axp_battery_remove,
  .suspend = axp15_suspend,
  .resume = axp15_resume,
  .shutdown = axp15_shutdown,
};

static int axp_battery_init(void)
{
  return platform_driver_register(&axp_battery_driver);
}

static void axp_battery_exit(void)
{
  platform_driver_unregister(&axp_battery_driver);
}

module_init(axp_battery_init);
module_exit(axp_battery_exit);

MODULE_DESCRIPTION("axp15 battery charger driver");
MODULE_AUTHOR("Kyle Cheung, X-power");
MODULE_LICENSE("GPL");

