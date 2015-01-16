/*
 * script_test.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * test application for script sysfs module
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>

#include <script.h>

/**
 * test for /sys/class/script/dump attribute node
 *
 * return 0 if susccess, otherwise failed
 */
int test_dump_mainkey(void)
{
#define OUT_BUFSIZE     (128 * 1024)
    int     fd = 0, ret = -1;
    char    in_buf[250] = {0}, *out_buf = NULL;

    fd = open(DUMP_NOD, O_RDWR);
    if(0 == fd) {
        printf("%s(%d): open \"%s\" failed, err!\n", __func__, __LINE__);
        goto end;
    }
    /* store para */
    strcpy(in_buf, DUMP_TEST_STRING);
    ret = write(fd, in_buf, strlen(in_buf));
    if(ret < 0) {
        printf("%s(%d): write \"%s\" failed, err!\n", __func__, __LINE__, in_buf);
        goto end;
    }
    printf("%s(%d): write %s success!\n", __func__, __LINE__, in_buf);
    /* seek to begin */
    if(lseek(fd, 0, SEEK_SET) < 0) {
        printf("%s(%d): seek to begin failed, err!\n", __func__, __LINE__);
        goto end;
    }
    printf("%s(%d): seek to begin success!\n", __func__, __LINE__);
    /* show output value */
    out_buf = malloc(OUT_BUFSIZE); /* must large enough to store the message */
    if(!out_buf) {
        printf("%s(%d): alloc outbuf failed, err!\n", __func__, __LINE__);
        goto end;
    }
    ret = read(fd, out_buf, OUT_BUFSIZE);
    if(ret < 0) {
        printf("%s(%d): read failed, err!\n", __func__, __LINE__);
        goto end;
    }
    printf("%s(%d): read success! out string:\n", __func__, __LINE__);
    printf("%s\n", out_buf);
    /* return success */
    ret = 0;
end:
    if(fd)
        close(fd);
    if(NULL != out_buf)
        free(out_buf);
    return ret;
}

int dump_subkey(get_item_out_t *pitem_out)
{
    if(NULL == pitem_out || SCIRPT_ITEM_VALUE_TYPE_INVALID == pitem_out->type)
        return -1;
    printf("+++++++++++++++++%s+++++++++++++++++\n", __func__);
    printf("        type: %s\n", ITEM_TYPE_TO_STR(pitem_out->type));
    switch(pitem_out->type) {
    case SCIRPT_ITEM_VALUE_TYPE_INT:
        printf("        value: %d\n", pitem_out->item.val);
        break;
    case SCIRPT_ITEM_VALUE_TYPE_STR:
        printf("        value: %s\n", pitem_out->str);
        break;
    case SCIRPT_ITEM_VALUE_TYPE_PIO:
        printf("        value: (gpio(%s): %3d, mul: %d, pull %d, drv %d, data %d)\n",
            pitem_out->str, pitem_out->item.gpio.gpio, pitem_out->item.gpio.mul_sel,
            pitem_out->item.gpio.pull, pitem_out->item.gpio.drv_level, pitem_out->item.gpio.data);
        break;
    default:
        printf("        invalid type!\n");
        break;
    }
    printf("-----------------%s-----------------\n", __func__);
    return 0;
}

/**
 * test for /sys/class/script/get_item attribute node
 *
 * return 0 if susccess, otherwise failed
 */
get_item_out_t* get_sub_item(char* mainkey,char* subkey)
{
    int     fd = 0, ret = -1;
    char    in_buf[250] = {0};
    get_item_out_t *pitem;

    fd = open(GET_ITEM_NOD, O_RDWR);
    if(0 == fd) {
        printf("%s(%d): open \"%s\" failed, err!\n", __func__, __LINE__);
        goto end;
    }
    /* store para */
    sprintf(in_buf, "%s %s", mainkey, subkey);
    ret = write(fd, in_buf, strlen(in_buf));
    if(ret < 0) {
        printf("%s(%d): write \"%s\" failed, err!\n", __func__, __LINE__, in_buf);
        goto end;
    }
    printf("%s(%d): write %s success!\n", __func__, __LINE__, in_buf);
    /* seek to begin */
    if(lseek(fd, 0, SEEK_SET) < 0) {
        printf("%s(%d): seek to begin failed, err!\n", __func__, __LINE__);
        goto end;
    }
    printf("%s(%d): seek to begin success!\n", __func__, __LINE__);
    /* read item
     * 256 to store the string, only for string item; you can change size according to string len
     */
    pitem = malloc(sizeof(get_item_out_t) + 256);
    memset(pitem, 0, sizeof(get_item_out_t) + 256);
    if(!pitem) {
        printf("%s(%d): alloc out buf failed, err!\n", __func__, __LINE__);
        goto end;
    }
    ret = read(fd, pitem, sizeof(get_item_out_t) + 256);
    if(ret < 0) {
        printf("%s(%d): read failed, err!\n", __func__, __LINE__);
        goto end;
    }
    printf("%s(%d): read type success! type %d->%s\n", __func__, __LINE__, pitem->type, ITEM_TYPE_TO_STR(pitem->type));
    
    /* return success */
    return pitem;
end:
    if(fd)
        close(fd);
    if(NULL != pitem)
        free(pitem);
    return NULL;
}


//int main(){

 // get_sub_item(GET_ITEM_MAINKEY,"csi_mname",mChar);
 
// get_item_out_t *pitem;
 // pitem =get_sub_item(GET_ITEM_MAINKEY,"sprite_gpio0");
//  dump_subkey(pitem);
  //get_sub_item(GET_ITEM_MAINKEY,"led_normal");
  //get_sub_item(GET_ITEM_MAINKEY,"led_error");

  //int led;
  //get_sub_item(GET_ITEM_MAINKEY,"csi_twi_addr",&led);
  //printf("-----mChar = %x--------\n",(int)led);
//return 0;
//}

