#ifndef _MACH_AW_GPIO_H_
#define _MACH_AW_GPIO_H_

/* gpio bank size */                    
#define AW_GPIO_A_NR                    (20)
#define AW_GPIO_B_NR                    (18)
#define AW_GPIO_C_NR                    (20)
#define AW_GPIO_D_NR                    (28)
#define AW_GPIO_E_NR                    (12)
#define AW_GPIO_F_NR                    (1) 
#define AW_GPIO_X_NR                    (6) 


#define AW_GPIO_NEXT(__gpio)            ((__gpio##_START) + (__gpio##_NR))

enum aw_gpio_number {
	AW_GPIO_A_START = 0,
	AW_GPIO_B_START = AW_GPIO_NEXT(AW_GPIO_A),
	AW_GPIO_C_START = AW_GPIO_NEXT(AW_GPIO_B),
	AW_GPIO_D_START = AW_GPIO_NEXT(AW_GPIO_C),
	AW_GPIO_E_START = AW_GPIO_NEXT(AW_GPIO_D),
	AW_GPIO_F_START = AW_GPIO_NEXT(AW_GPIO_E),
	AW_GPIO_X_START = AW_GPIO_NEXT(AW_GPIO_F)
};

/* AW GPIO number definitions. */
#define AW_GPA(_nr)                     (AW_GPIO_A_START + (_nr))
#define AW_GPB(_nr)                     (AW_GPIO_B_START + (_nr))
#define AW_GPC(_nr)                     (AW_GPIO_C_START + (_nr))
#define AW_GPD(_nr)                     (AW_GPIO_D_START + (_nr))
#define AW_GPE(_nr)                     (AW_GPIO_E_START + (_nr))
#define AW_GPF(_nr)                     (AW_GPIO_F_START + (_nr))
#define AW_GPX(_nr)                     (AW_GPIO_X_START + (_nr))

/* define the number of gpios we need to the one after the GPQ() range */
#define ARCH_NR_GPIOS	                (AW_GPX(AW_GPIO_X_NR) + 1)

extern struct gpio_chip *gpio_to_chip(unsigned gpio);

extern unsigned aw_gpio_getcfg(unsigned gpio);
extern int aw_gpio_setcfg(unsigned gpio, unsigned config);
extern unsigned aw_gpio_get_pull(unsigned gpio);
int aw_gpio_set_pull(unsigned gpio, unsigned pull);
extern unsigned aw_gpio_get_drv_level(unsigned gpio);
extern int aw_gpio_set_drv_level(unsigned gpio, unsigned level);

extern void axp19x_gpio_config(int gpio, int func,int cur, int ldo, int res, 
			int pull, int pwm1,int pwm2);

#include <asm-generic/gpio.h>
#define gpio_get_value __gpio_get_value
#define gpio_set_value __gpio_set_value
#define gpio_to_irq    __gpio_to_irq

#endif
