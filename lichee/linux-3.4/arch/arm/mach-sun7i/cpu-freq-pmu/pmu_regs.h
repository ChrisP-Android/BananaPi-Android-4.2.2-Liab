/*
 * arch/arm/mach-sun7i/cpu-freq-pmu/pmu_regs.h
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Pan Nan <pannan@reuuimllatech.com>
 *
 * SUN7I PMU Register Definition
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __PMU_REGS_H__
#define __PMU_REGS_H__


typedef struct __PMU_DVFS_CTRL_REG0
{
    __u32   DvfsEn:1;                   //bit0,  pmu dvfs enable, 0-disable, 1-enable
    __u32   reserved0:1;                //bit1,  reserved
    __u32   DvfsModEn:1;                //bit2,  twi dvfs mode enable
    __u32   reserved1:2;                //bit3,  reserved
    __u32   VtChgEn:1;                  //bit5,  voltage change enable, 0-disable, 1-enable
    __u32   reserved2:8;                //bit6,  reserved
    __u32   AtbApbAutoSwtEn:1;          //bit14, System ATB/APB CLK auto switch enable
    __u32   AxiAutoSwtEn:1;             //bit15, AXICLK auto switch enable, 0-disable, 1-enable
    __u32   reserved3:16;               //bit16, reserved
} __pmu_dvfs_ctrl_reg0_t;


typedef struct __PMU_DVFS_CTRL_REG1
{
    __u32   SmthInterVal:8;             //bit0,  smooth interval value, default 0x10
    __u32   PllStaTime:16;              //bit8,  pll stable time, default 0x10
    __u32   reserved0:8;                //bit23, reserved
} __pmu_dvfs_ctrl_reg1_t;


typedef struct __PMU_DVFS_CTRL_REG2
{
    __u32   VtSetEn:1;                  //bit0,  volt set enable, set to 1 will start the voltage setting
    __u32   reserved0:31;               //bit1,  reserved
} __pmu_dvfs_ctrl_reg2_t;


typedef struct __PMU_DVFS_TIMEOUT_CTRL_REG
{
    __u32   DvfsTiCyc:6;                //bit0,  dvfs operate on TWI timeout cycles in TWI peripheral clock, 
                                        //default 0x27 (40 cycles)
    __u32   reserved0:26;               //bit6,  reserved
} __pmu_dvfs_timeout_ctrl_reg_t;


typedef struct __PMU_AXI_ATB_APB_RNG_REG0
{
    __u32   AxiClkLevel0:11;            //bit0,  axiclk level 0
    __u32   reserved0:5;                //bit11, reserved
    __u32   AxiClkLevel1:11;            //bit16, axiclk level 1
    __u32   reserved1:5;                //bit27, reserved
} __pmu_axi_atb_apb_reg0_t;


typedef struct __PMU_AXI_ATB_APB_RNG_REG1
{
    __u32   AxiClkLevel2:11;            //bit0,  axiclk level 2
    __u32   reserved0:5;                //bit11, reserved
    __u32   AxiClkLevel3:11;            //bit16, axiclk level 3
    __u32   reserved1:5;                //bit27, reserved
} __pmu_axi_atb_apb_reg1_t;


typedef struct __PMU_IRQ_EN_REG
{
    __u32   DvfsFinIrqEn:1;             //bit0,  dvfs finished IRQ enable, 0-disable, 1-enable
    __u32   reserved0:1;				//bit1,  reserved
    __u32   DvfsVtChgFinEn:1;           //bit2,  dvfs voltage change finished enable, 0-disable, 1-enable
    __u32   DvfsClkSwtFinIrqEn:1;       //bit3,  dvfs clock switch operation finished IRQ enable, 0-disable, 1-enable
    __u32   VtDetFinIrqEn:1;            //bit4,  voltage detect finished IRQ enable, 0-disable, 1-enable
    __u32   reserved1:5;                //bit5,  reserved
    __u32   DvfsVtChgErrEn:1;           //bit10, dvfs voltage change error enable, 0-disable, 1-enable
    __u32   reserved2:1;				//bit11, reserved
    __u32   DvfsVtDetErrIrqEn:1;        //bit12, voltage detect error IRQ enable, 0-disable, 1-enable
    __u32   reserved3:19;               //bit13, reserved
} __pmu_irq_en_reg_t;


typedef struct __PMU_IRQ_STATUS_REG
{
    __u32   DvfsFinIrqPd:1;             //bit0,  dvfs finished IRQ pending, 0-no effect, 1-pending
    __u32   reserved0:1;				//bit1,  reserved
    __u32   DvfsVtChgFinPd:1;           //bit2,  dvfs voltage change finished pending, 0-no effect, 1-pending
    __u32   DvfsClkSwtFinIrqPd:1;       //bit3,  dvfs clock switch operation finished IRQ pending, 0-no effect, 1-pending
    __u32   VtDetFinIrqPd:1;            //bit4,  voltage detect finished IRQ pending, 0-no effect, 1-pending
    __u32   reserved1:5;                //bit5,  reserved
    __u32   DvfsVtChgErrPd:1;           //bit10, dvfs voltage change error pending, 0-no effect, 1-pending
    __u32   reserved2:1;				//bit11, reserved
    __u32   VtDetErrIrqPd:1;            //bit12, voltage detect error IRQ pending, 0-no effect, 1-pending
    __u32   reserved3:19;               //bit13, reserved
} __pmu_irq_status_reg_t;


typedef struct __PMU_STATUS_REG
{
    __u32   DvfsBusy:1;                 //bit0,  dvfs busy, 0-no effect, 1-dvfs is busy
    __u32   reserved0:31;               //bit1,  reserved
} __pmu_status_reg_t;


typedef struct __PMU_CPUVDD_CTRL_REG_ADDR
{
    __u32   CpuvddCtrlRegAddr:8;        //bit0,  pmu cpuvdd dcdc control register address, default 0x23
    __u32   reserved0:24;               //bit8,  reserved
} __pmu_cpuvdd_crtl_reg_addr_t;


typedef struct __PMU_TWI_ADDR_REG
{
    __u32   PmuTwiAddr:8;               //bit0,  pmu twi address with write operation, default 0x68
    __u32   reserved0:24;               //bit8,  reserved
} __pmu_twi_addr_reg_t;


typedef struct __PMU_CPUVDD_VALUE_REG
{
    __u32   CpuvddDefVal:8;             //bit0,  pmu cpuvdd default value, default 0x16
                                        //0x00=0.70v, 0x02=0.75v, 0x04=0.80v, 0x06=0.85v, 0x08=0.90v, 0x0A=0.95v,
                                        //0x0C=1.00v, 0x0E=1.05v, 0x10=1.10v, 0x12=1.15v, 0x14=1.20v, 0x16=1.25v,
                                        //0x18=1.30v, 0x1A=1.35v, 0x1C=1.40v, 0x1E=1.45v, 0x20=1.50v, 0x22=1.55v, 
                                        //0x24=1.60v
    __u32   reserved0:24;               //bit8,  reserved
} __pmu_cpuvdd_value_reg_t;


typedef struct __PMU_CPUVDD_RAMP_CTRL_REG
{
    __u32   CpuvddVtRampCtrl:1;         //bit0,  cpuvdd voltage ramp control in DVM, 0-15.625us, 1-31.25us
    __u32   reserved0:31;               //bit1,  reserved
} __pmu_cpuvdd_ramp_ctrl_reg_t;


typedef struct __PMU_VF_TABLE_REG0
{
    __u32   CpuMaxFreq070:11;           //bit0,  cpu max frequency if cpuvdd=0.70v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg0_t;


typedef struct __PMU_VF_TABLE_REG1
{
    __u32   CpuMaxFreq075:11;           //bit0,  cpu max frequency if cpuvdd=0.75v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg1_t;


typedef struct __PMU_VF_TABLE_REG2
{
    __u32   CpuMaxFreq080:11;           //bit0,  cpu max frequency if cpuvdd=0.80v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg2_t;


typedef struct __PMU_VF_TABLE_REG3
{
    __u32   CpuMaxFreq085:11;           //bit0,  cpu max frequency if cpuvdd=0.85v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg3_t;


typedef struct __PMU_VF_TABLE_REG4
{
    __u32   CpuMaxFreq090:11;           //bit0,  cpu max frequency if cpuvdd=0.90v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg4_t;


typedef struct __PMU_VF_TABLE_REG5
{
    __u32   CpuMaxFreq095:11;           //bit0,  cpu max frequency if cpuvdd=0.95v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg5_t;


typedef struct __PMU_VF_TABLE_REG6
{
    __u32   CpuMaxFreq100:11;           //bit0,  cpu max frequency if cpuvdd=1.00v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg6_t;


typedef struct __PMU_VF_TABLE_REG7
{
    __u32   CpuMaxFreq105:11;           //bit0,  cpu max frequency if cpuvdd=1.05v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg7_t;


typedef struct __PMU_VF_TABLE_REG8
{
    __u32   CpuMaxFreq110:11;           //bit0,  cpu max frequency if cpuvdd=1.10v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg8_t;


typedef struct __PMU_VF_TABLE_REG9
{
    __u32   CpuMaxFreq115:11;           //bit0,  cpu max frequency if cpuvdd=1.15v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg9_t;


typedef struct __PMU_VF_TABLE_REG10
{
    __u32   CpuMaxFreq120:11;           //bit0,  cpu max frequency if cpuvdd=1.20v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg10_t;


typedef struct __PMU_VF_TABLE_REG11
{
    __u32   CpuMaxFreq125:11;           //bit0,  cpu max frequency if cpuvdd=1.25v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg11_t;


typedef struct __PMU_VF_TABLE_REG12
{
    __u32   CpuMaxFreq130:11;           //bit0,  cpu max frequency if cpuvdd=1.30v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg12_t;


typedef struct __PMU_VF_TABLE_REG13
{
    __u32   CpuMaxFreq135:11;           //bit0,  cpu max frequency if cpuvdd=1.35v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg13_t;


typedef struct __PMU_VF_TABLE_REG14
{
    __u32   CpuMaxFreq140:11;           //bit0,  cpu max frequency if cpuvdd=1.40v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg14_t;


typedef struct __PMU_VF_TABLE_REG15
{
    __u32   CpuMaxFreq145:11;           //bit0,  cpu max frequency if cpuvdd=1.45v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg15_t;


typedef struct __PMU_VF_TABLE_REG16
{
    __u32   CpuMaxFreq150:11;           //bit0,  cpu max frequency if cpuvdd=1.50v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg16_t;


typedef struct __PMU_VF_TABLE_REG17
{
    __u32   CpuMaxFreq155:11;           //bit0,  cpu max frequency if cpuvdd=1.55v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg17_t;


typedef struct __PMU_VF_TABLE_REG18
{
    __u32   CpuMaxFreq160:11;           //bit0,  cpu max frequency if cpuvdd=1.60v (MHz)
    __u32   reserved0:21;               //bit11, reserved
} __pmu_vf_table_reg18_t;


typedef struct __PMU_VF_TABLE_VALID_REG
{
    __u32   VfTable13Valid:1;           //bit0,  pmu v-f table register 13 valid, 0-valid, 1-invalid, default 0
    __u32   VfTable14Valid:1;           //bit1,  pmu v-f table register 14 valid, 0-valid, 1-invalid, default 0
    __u32   VfTable15Valid:1;           //bit2,  pmu v-f table register 15 valid, 0-valid, 1-invalid, default 1
    __u32   VfTable16Valid:1;           //bit3,  pmu v-f table register 16 valid, 0-valid, 1-invalid, default 1
    __u32   VfTable17Valid:1;           //bit4,  pmu v-f table register 17 valid, 0-valid, 1-invalid, default 1
    __u32   VfTable18Valid:1;           //bit5,  pmu v-f table register 18 valid, 0-valid, 1-invalid, default 1
    __u32   reserved0:10;               //bit6,  reserved
    __u32   KeyField:16;                //bit16, key field for valid bit, bit[5:0] can be modified (key field=0x1623)
} __pum_vf_table_valid_reg_t;


typedef struct __PMU_VF_TABLE_INDEX_REG
{
    __u32   VfTableIdx:2;               //bit0,  pmu vf table index
    __u32   reserved0:30;               //bit2,  reserved
} __pmu_vf_table_index_reg_t;


typedef struct __PMU_VF_TABLE_RANGE_REG
{
    __u32   VfTableRng0:8;              //bit0,  pmu v-f table range 0
    __u32   VfTableRng1:8;              //bit8,  pmu v-f table range 1
    __u32   VfTableRng2:8;              //bit16, pmu v-f table range 2
    __u32   reserved0:8;                //bit24, reserved
} __pmu_vf_table_range_reg_t;


typedef struct __PMU_REG_LIST
{
    volatile __pmu_dvfs_ctrl_reg0_t         PmuCtl0;            //0x0000, PMU control register 0
    volatile __pmu_dvfs_ctrl_reg1_t         PmuCtl1;            //0x0004, PMU control register 1
    volatile __u32                          reserved0;          //0x0008, reserved
    volatile __pmu_dvfs_ctrl_reg2_t         PmuCtl2;            //0x000C, PMU control register 2
    volatile __u32                          reserved1[2];       //0x0010, reserved
    volatile __u32                          PmuCtl3;            //0x0018, PMU control register 3, reserved
    volatile __pmu_dvfs_timeout_ctrl_reg_t  PmuTOut;            //0x001C, PMU timeout control register
    volatile __pmu_axi_atb_apb_reg0_t		PmuAxiAtbApb0;		//0x0020, PMU axi range register 0
    volatile __pmu_axi_atb_apb_reg1_t		PmuAxiAtbApb1;		//0x0024, PMU axi range register 1
    volatile __u32                          reserved2[6];       //0x0028, reserved
    volatile __pmu_irq_en_reg_t             PmuIrqEn;           //0x0040, PMU IRQ enable register
    volatile __pmu_irq_status_reg_t         PmuIrqSta;          //0x0044, PMU IRQ status register
    volatile __pmu_status_reg_t             PmuSta;             //0x0048, PMU status register
    volatile __pmu_cpuvdd_crtl_reg_addr_t   PmuCpuvddAddr;      //0x004C, PMU cpuvdd register address
    volatile __pmu_twi_addr_reg_t           TwiAddr;            //0x0050, PMU TWI address
    volatile __pmu_cpuvdd_value_reg_t       PmuCpuvddVal;       //0x0054, PMU cpuvdd value
    volatile __pmu_cpuvdd_ramp_ctrl_reg_t   PmuCpuvddRamp;      //0x0058, PMU cpuvdd voltage ramp control
    volatile __u32                          reserved3[9];       //0x005C, reserved
    volatile __pmu_vf_table_reg0_t          PmuVFT0;            //0x0080, cpu max speed when cpuvdd=0.70v
    volatile __pmu_vf_table_reg1_t          PmuVFT1;            //0x0084, cpu max speed when cpuvdd=0.75v
    volatile __pmu_vf_table_reg2_t          PmuVFT2;            //0x0088, cpu max speed when cpuvdd=0.80v
    volatile __pmu_vf_table_reg3_t          PmuVFT3;            //0x008C, cpu max speed when cpuvdd=0.85v
    volatile __pmu_vf_table_reg4_t          PmuVFT4;            //0x0090, cpu max speed when cpuvdd=0.90v
    volatile __pmu_vf_table_reg5_t          PmuVFT5;            //0x0094, cpu max speed when cpuvdd=0.95v
    volatile __pmu_vf_table_reg6_t          PmuVFT6;            //0x0098, cpu max speed when cpuvdd=1.00v
    volatile __pmu_vf_table_reg7_t          PmuVFT7;            //0x009C, cpu max speed when cpuvdd=1.05v
    volatile __pmu_vf_table_reg8_t          PmuVFT8;            //0x00A0, cpu max speed when cpuvdd=1.10v
    volatile __pmu_vf_table_reg9_t          PmuVFT9;            //0x00A4, cpu max speed when cpuvdd=1.15v
    volatile __pmu_vf_table_reg10_t         PmuVFT10;           //0x00A8, cpu max speed when cpuvdd=1.20v
    volatile __pmu_vf_table_reg11_t         PmuVFT11;           //0x00AC, cpu max speed when cpuvdd=1.25v
    volatile __pmu_vf_table_reg12_t         PmuVFT12;           //0x00B0, cpu max speed when cpuvdd=1.30v
    volatile __pmu_vf_table_reg13_t         PmuVFT13;           //0x00B4, cpu max speed when cpuvdd=1.35v
    volatile __pmu_vf_table_reg14_t         PmuVFT14;           //0x00B8, cpu max speed when cpuvdd=1.40v
    volatile __pmu_vf_table_reg15_t         PmuVFT15;           //0x00BC, cpu max speed when cpuvdd=1.45v
    volatile __pmu_vf_table_reg16_t         PmuVFT16;           //0x00C0, cpu max speed when cpuvdd=1.50v
    volatile __pmu_vf_table_reg17_t         PmuVFT17;           //0x00C4, cpu max speed when cpuvdd=1.55v
    volatile __pmu_vf_table_reg18_t         PmuVFT18;           //0x00C8, cpu max speed when cpuvdd=1.60v
    volatile __pum_vf_table_valid_reg_t     PmuVFTValid;        //0x00CC, PMU VF table valid control
    volatile __pmu_vf_table_index_reg_t     PmuVFTIdx;          //0x00D0, PMU VF table index
    volatile __pmu_vf_table_range_reg_t     PmuVFTRng;          //0x00D4, PMU VF table range
} __pmu_reg_list_t;


#endif /* __PMU_REGS_H__ */