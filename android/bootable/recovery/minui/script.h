#ifndef _SCRIPT_H_
#define _SCRIPT_H_

#ifdef __cplusplus
extern "C" {
#endif
/* input test string, which will be passed to xxx_store function */
#define DUMP_TEST_STRING    "gmac_para"
/* file node path */
#define DUMP_NOD        "/sys/class/script/dump"
#define GET_ITEM_NOD        "/sys/class/script/get_item"

typedef unsigned int u32;
typedef enum {
    SCIRPT_ITEM_VALUE_TYPE_INVALID = 0,
    SCIRPT_ITEM_VALUE_TYPE_INT,
    SCIRPT_ITEM_VALUE_TYPE_STR,
    SCIRPT_ITEM_VALUE_TYPE_PIO,
} script_item_value_type_e;

struct gpio_config {
    u32 gpio;       /* gpio global index, must be unique */
    u32     mul_sel;    /* multi sel val: 0 - input, 1 - output... */
    u32     pull;       /* pull val: 0 - pull up/down disable, 1 - pull up... */
    u32     drv_level;  /* driver level val: 0 - level 0, 1 - level 1... */
    u32 data;       /* data val: 0 - low, 1 - high, only vaild when mul_sel is input/output */
};

typedef union {
    int                 val;
    char                *str;
    struct gpio_config  gpio;
} script_item_u;

#define ITEM_TYPE_TO_STR(type)  ((SCIRPT_ITEM_VALUE_TYPE_INT == (type)) ?  "int"    :   \
                ((SCIRPT_ITEM_VALUE_TYPE_STR == (type))  ?  "string" :  \
                ((SCIRPT_ITEM_VALUE_TYPE_PIO == (type))  ?   "gpio" : "invalid")))

/**
 * test for /sys/class/script/dump attribute node
 *
 * return 0 if susccess, otherwise failed
 */
 typedef struct {
    script_item_u item;
    script_item_value_type_e type;
    char str[0]; /* if item is string, store the string;
              * if item is gpio, store the gpio name, eg: PH5;
              * otherwise not used.
              */
}get_item_out_t;

get_item_out_t* get_sub_item(char* mainkey,char* subkey);
int dump_subkey(get_item_out_t *pitem_out);

#ifdef __cplusplus
}
#endif

#endif
