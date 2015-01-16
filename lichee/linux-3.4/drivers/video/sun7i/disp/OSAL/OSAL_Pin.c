/*
*************************************************************************************
*                         			eBsp
*					   Operation System Adapter Layer
*
*				(c) Copyright 2006-2010, All winners Co,Ld.
*							All	Rights Reserved
*
* File Name 	: OSAL_Pin.h
*
* Author 		: javen
*
* Description 	: C?⺯??
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   2010-09-07          1.0         create this word
*       holi     	   2010-12-02          1.1         ???Ӿ????Ľӿڣ?
*************************************************************************************
*/
#include "OSAL_Pin.h"


#ifndef  __OSAL_PIN_MASK__
__hdle OSAL_GPIO_Request(disp_gpio_set_t *gpio_list, __u32 group_count_max)
{    
        __hdle ret = 0;
        struct gpio_config pcfg;

        if(gpio_list == NULL)
        return 0;

        pcfg.gpio = gpio_list->gpio;
        pcfg.mul_sel = gpio_list->mul_sel;
        pcfg.pull = gpio_list->pull;
        pcfg.drv_level = gpio_list->drv_level;
        pcfg.data = gpio_list->data;
        if(0 != gpio_request(pcfg.gpio, NULL))
        {
                __wrn("OSAL_GPIO_Request failed, gpio_name=%s, gpio=%d\n", gpio_list->gpio_name, gpio_list->gpio);
                return ret;
        }

        if(0 != sw_gpio_setall_range(&pcfg, group_count_max))
        {
                __wrn("OSAL_GPIO_Request: setall_range fail, gpio_name=%s, gpio=%d,mul_sel=%d\n", gpio_list->gpio_name, gpio_list->gpio, gpio_list->mul_sel);
        }
        else
        {
                __inf("OSAL_GPIO_Request ok, gpio_name=%s, gpio=%d,mul_sel=%d\n", gpio_list->gpio_name, gpio_list->gpio, gpio_list->mul_sel);
                ret = pcfg.gpio;
        }

        return ret;
}

__hdle OSAL_GPIO_Request_Ex(char *main_name, const char *sub_name)
{
        __wrn("OSAL_GPIO_Request_Ex is NULL\n");
        return 0;
}

//if_release_to_default_status:
    //??????0????1????ʾ?ͷź???GPIO????????״̬??????״״̬???ᵼ???ⲿ??ƽ?Ĵ?????
    //??????2????ʾ?ͷź???GPIO״̬???䣬???ͷŵ?ʱ?򲻹??���ǰGPIO??Ӳ???Ĵ?????
__s32 OSAL_GPIO_Release(__hdle p_handler, __s32 if_release_to_default_status)
{
        if(p_handler)
        {
                gpio_free(p_handler);
        }
        else
        {
                __wrn("OSAL_GPIO_Release, hdl is NULL\n");
        }
        return 0;
}

__s32 OSAL_GPIO_DevGetAllPins_Status(unsigned p_handler, disp_gpio_set_t *gpio_status, unsigned gpio_count_max, unsigned if_get_from_hardware)
{
        __wrn("OSAL_GPIO_DevGetAllPins_Status is NULL\n");
        return 0;
}

__s32 OSAL_GPIO_DevGetONEPins_Status(unsigned p_handler, disp_gpio_set_t *gpio_status,const char *gpio_name,unsigned if_get_from_hardware)
{
        __wrn("OSAL_GPIO_DevGetONEPins_Status is NULL\n");
        return 0;
}

__s32 OSAL_GPIO_DevSetONEPin_Status(u32 p_handler, disp_gpio_set_t *gpio_status, const char *gpio_name, __u32 if_set_to_current_input_status)
{
        __wrn("OSAL_GPIO_DevSetONEPin_Status is NULL\n");
        return 0;
}

__s32 OSAL_GPIO_DevSetONEPIN_IO_STATUS(u32 p_handler, __u32 if_set_to_output_status, const char *gpio_name)
{
        int ret = -1;

        if(p_handler)
        {
                if(if_set_to_output_status)
                {
                        ret = gpio_direction_output(p_handler, 0);
                        if(ret != 0)
                        {
                                __wrn("gpio_direction_output fail!\n");
                        }
                }
                else
                {
                        ret = gpio_direction_input(p_handler);
                        if(ret != 0)
                        {
                                __wrn("gpio_direction_input fail!\n");
                        }
                }
        }
        else
        {
                __wrn("OSAL_GPIO_DevSetONEPIN_IO_STATUS, hdl is NULL\n");
                ret = -1;
        }
        return ret;
}

__s32 OSAL_GPIO_DevSetONEPIN_PULL_STATUS(u32 p_handler, __u32 set_pull_status, const char *gpio_name)
{
        __wrn("OSAL_GPIO_DevSetONEPIN_PULL_STATUS is NULL\n");
        return 0;
}

__s32 OSAL_GPIO_DevREAD_ONEPIN_DATA(u32 p_handler, const char *gpio_name)
{
        if(p_handler)
        {
                return __gpio_get_value(p_handler);
        }
        __wrn("OSAL_GPIO_DevREAD_ONEPIN_DATA, hdl is NULL\n");
        return -1;
}

__s32 OSAL_GPIO_DevWRITE_ONEPIN_DATA(u32 p_handler, __u32 value_to_gpio, const char *gpio_name)
{
        if(p_handler)
        {
                __gpio_set_value(p_handler, value_to_gpio);
        }
        else
        {
                __wrn("OSAL_GPIO_DevWRITE_ONEPIN_DATA, hdl is NULL\n");
        }

        return 0;
}

#else

__hdle OSAL_GPIO_Request(disp_gpio_set_t *gpio_list, __u32 group_count_max)
{    

        return 0;
}

__hdle OSAL_GPIO_Request_Ex(char *main_name, const char *sub_name)
{
        return 0;
}

//if_release_to_default_status:
    //??????0????1????ʾ?ͷź???GPIO????????״̬??????״״̬???ᵼ???ⲿ??ƽ?Ĵ?????
    //??????2????ʾ?ͷź???GPIO״̬???䣬???ͷŵ?ʱ?򲻹??���ǰGPIO??Ӳ???Ĵ?????
__s32 OSAL_GPIO_Release(__hdle p_handler, __s32 if_release_to_default_status)
{
        return 0;
}

__s32 OSAL_GPIO_DevGetAllPins_Status(unsigned p_handler, disp_gpio_set_t *gpio_status, unsigned gpio_count_max, unsigned if_get_from_hardware)
{
        return 0;
}

__s32 OSAL_GPIO_DevGetONEPins_Status(unsigned p_handler, disp_gpio_set_t *gpio_status,const char *gpio_name,unsigned if_get_from_hardware)
{
        return 0;
}

__s32 OSAL_GPIO_DevSetONEPin_Status(u32 p_handler, disp_gpio_set_t *gpio_status, const char *gpio_name, __u32 if_set_to_current_input_status)
{
        return 0;
}

__s32 OSAL_GPIO_DevSetONEPIN_IO_STATUS(u32 p_handler, __u32 if_set_to_output_status, const char *gpio_name)
{
        return 0;
}

__s32 OSAL_GPIO_DevSetONEPIN_PULL_STATUS(u32 p_handler, __u32 set_pull_status, const char *gpio_name)
{
        return 0;
}

__s32 OSAL_GPIO_DevREAD_ONEPIN_DATA(u32 p_handler, const char *gpio_name)
{
        return 0;
}

__s32 OSAL_GPIO_DevWRITE_ONEPIN_DATA(u32 p_handler, __u32 value_to_gpio, const char *gpio_name)
{
        return 0;
}
#endif
