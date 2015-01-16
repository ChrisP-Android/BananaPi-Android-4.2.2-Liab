/*
********************************************************************************
*                                                   OSAL
*
*                                     (c) Copyright 2008-2009, Kevin China
*                                             				All Rights Reserved
*
* File    : OSAL_Clock.c
* By      : Sam.Wu
* Version : V1.00
* Date    : 2011/3/25 20:25 
* Description :  
* Update   :  date      author      version     notes    
********************************************************************************
*/
#include "OSAL.h"
#include "OSAL_Clock.h"

#ifndef __OSAL_CLOCK_MASK__

#define disp_clk_inf(clk_id, clk_name)   {.id = clk_id, .name = clk_name}

__disp_clk_t disp_clk_tbl[] =
{
        disp_clk_inf(SYS_CLK_PLL3,      CLK_SYS_PLL3            ),
        disp_clk_inf(SYS_CLK_PLL7,      CLK_SYS_PLL7            ),
        disp_clk_inf(SYS_CLK_PLL3X2,    CLK_SYS_PLL3X2          ),
        disp_clk_inf(SYS_CLK_PLL5P,     CLK_SYS_PLL5P           ),
        disp_clk_inf(SYS_CLK_PLL6,      CLK_SYS_PLL6            ),
        disp_clk_inf(SYS_CLK_PLL6M,     CLK_SYS_PLL6M           ),
        disp_clk_inf(SYS_CLK_PLL7X2,    CLK_SYS_PLL7X2          ),

        disp_clk_inf(MOD_CLK_DEBE0,     CLK_MOD_DEBE0           ),
        disp_clk_inf(MOD_CLK_DEBE1,     CLK_MOD_DEBE1           ),
        disp_clk_inf(MOD_CLK_DEFE0,     CLK_MOD_DEFE0           ),
        disp_clk_inf(MOD_CLK_DEFE1,     CLK_MOD_DEFE1           ),
        disp_clk_inf(MOD_CLK_LCD0CH0,   CLK_MOD_LCD0CH0         ),
        disp_clk_inf(MOD_CLK_LCD0CH1_S1,CLK_MOD_LCD0CH1_S1      ),
        disp_clk_inf(MOD_CLK_LCD0CH1_S2,CLK_MOD_LCD0CH1_S2      ),
        disp_clk_inf(MOD_CLK_LCD1CH0,   CLK_MOD_LCD1CH0         ),
        disp_clk_inf(MOD_CLK_LCD1CH1_S1,CLK_MOD_LCD1CH1_S1      ),
        disp_clk_inf(MOD_CLK_LCD1CH1_S2,CLK_MOD_LCD1CH1_S2      ),
        disp_clk_inf(MOD_CLK_HDMI,      CLK_MOD_HDMI            ),
        disp_clk_inf(MOD_CLK_LVDS,      CLK_MOD_LVDS            ),

        disp_clk_inf(AHB_CLK_LCD0,      CLK_AHB_LCD0            ),
        disp_clk_inf(AHB_CLK_LCD1,      CLK_AHB_LCD1            ),
        disp_clk_inf(AHB_CLK_HDMI,      CLK_AHB_HDMI            ),
        disp_clk_inf(AHB_CLK_HDMI1,     CLK_AHB_HDMI1           ),
        disp_clk_inf(AHB_CLK_DEBE0,     CLK_AHB_DEBE0           ),
        disp_clk_inf(AHB_CLK_DEBE1,     CLK_AHB_DEBE1           ),
        disp_clk_inf(AHB_CLK_DEFE0,     CLK_AHB_DEFE0           ),
        disp_clk_inf(AHB_CLK_DEFE1,     CLK_AHB_DEFE1           ),
        disp_clk_inf(AHB_CLK_TVE0,      CLK_AHB_TVE0            ),
        disp_clk_inf(AHB_CLK_TVE1,      CLK_AHB_TVE1            ),


        disp_clk_inf(DRAM_CLK_DEFE0,    CLK_DRAM_DEFE0          ),
        disp_clk_inf(DRAM_CLK_DEFE1,    CLK_DRAM_DEFE1          ),
        disp_clk_inf(DRAM_CLK_DEBE0,    CLK_DRAM_DEBE0          ),
        disp_clk_inf(DRAM_CLK_DEBE1,    CLK_DRAM_DEBE1          ),
};

__s32 osal_ccmu_get_clk_name(__disp_clk_id_t clk_no, char *clk_name)
{
        __u32 i;
        __u32 count;

        count = sizeof(disp_clk_tbl)/sizeof(__disp_clk_t);
        __inf("osal_ccmu_get_clk_name, count=%d\n",count);

        for(i=0;i<count;i++)
        {
                if(disp_clk_tbl[i].id == clk_no)
                {
                memcpy(clk_name, disp_clk_tbl[i].name, strlen(disp_clk_tbl[i].name)+1);
                return 0;
                }
        }

        return -1;
}

__s32 OSAL_CCMU_SetSrcFreq( __u32 nSclkNo, __u32 nFreq )
{
        struct clk* hSysClk = NULL;
        s32 retCode = -1;
        char clk_name[20];


        if(osal_ccmu_get_clk_name(nSclkNo, clk_name) != 0)
        {
                __wrn("Fail to get clk name from clk id [%d].\n", nSclkNo);
                return -1;
        }
        __inf("OSAL_CCMU_SetSrcFreq,  <%s,%d>\n", clk_name, nFreq);

        hSysClk = clk_get(NULL, clk_name);

        if(NULL == hSysClk || IS_ERR(hSysClk))
        {
                __wrn("Fail to get handle for system clock [%d].\n", nSclkNo);
                return -1;
        }
        if(nFreq == clk_get_rate(hSysClk))
        {
                __inf("Sys clk[%d] freq is alreay %d, not need to set.\n", nSclkNo, nFreq);
                clk_put(hSysClk);
                return 0;
        }
        retCode = clk_set_rate(hSysClk, nFreq);
        if(0 != retCode)
        {
                __wrn("Fail to set nFreq[%d] for sys clk[%d].\n", nFreq, nSclkNo);
                clk_put(hSysClk);
                return retCode;
        }
        clk_put(hSysClk);
        hSysClk = NULL;

        return retCode;
}

__u32 OSAL_CCMU_GetSrcFreq( __u32 nSclkNo )
{
        struct clk* hSysClk = NULL;
        u32 nFreq = 0;

        char clk_name[20];

        if(osal_ccmu_get_clk_name(nSclkNo, clk_name) != 0)
        {
                __wrn("Fail to get clk name from clk id [%d].\n", nSclkNo);
                return -1;
        }
        __inf("OSAL_CCMU_GetSrcFreq,  clk_name[%d]=%s\n", nSclkNo,clk_name);

        hSysClk = clk_get(NULL, clk_name);

        if(NULL == hSysClk || IS_ERR(hSysClk))
        {
                __wrn("Fail to get handle for system clock [%d].\n", nSclkNo);
                return -1;
        }
        nFreq = clk_get_rate(hSysClk);
        clk_put(hSysClk);
        hSysClk = NULL;

        return nFreq;
}

__hdle OSAL_CCMU_OpenMclk( __s32 nMclkNo )
{
        struct clk* hModClk = NULL;
        char clk_name[20];

        if(osal_ccmu_get_clk_name(nMclkNo, clk_name) != 0)
        {
                __wrn("Fail to get clk name from clk id [%d].\n", nMclkNo);
                return -1;
        }

        hModClk = clk_get(NULL, clk_name);

        if(NULL == hModClk || IS_ERR(hModClk))
        {
                __wrn("clk_get fail\n");
                return -1;
        }

        __inf("OSAL_CCMU_OpenMclk,  clk_name[%d]=%s, hdl=0x%x\n", nMclkNo,clk_name, (unsigned int)hModClk);

        return (__hdle)hModClk;
}

__s32 OSAL_CCMU_CloseMclk( __hdle hMclk )
{
        struct clk* hModClk = (struct clk*)hMclk;

        if(NULL == hModClk || IS_ERR(hModClk))
        {
                __wrn("NULL hdle\n");
                return -1;
        }

        clk_put(hModClk);

        return 0;
}

__s32 OSAL_CCMU_SetMclkSrc( __hdle hMclk, __u32 nSclkNo )
{
        struct clk* hSysClk = NULL;
        struct clk* hModClk = (struct clk*)hMclk;
        s32 retCode = -1;
        char clk_name[20];

        if(NULL == hModClk || IS_ERR(hModClk))
        {
                __wrn("NULL hdle\n");
                return -1;
        }

        if(osal_ccmu_get_clk_name(nSclkNo, clk_name) != 0)
        {
                __wrn("Fail to get clk name from clk id [%d].\n", nSclkNo);
                return -1;
        }


        __inf("OSAL_CCMU_SetMclkSrc, hMclk= hdl=0x%x, %s\n", (int)hModClk, clk_name);

        __inf("OSAL_CCMU_SetMclkSrc,  clk_name[%d]=%s\n", nSclkNo,clk_name);

        hSysClk = clk_get(NULL, clk_name);

        if((NULL == hSysClk) || (IS_ERR(hSysClk)))
        {
                __wrn("Fail to get handle for system clock [%d].\n", nSclkNo);
                return -1;
        }

        if(clk_get_parent(hModClk) == hSysClk)
        {
                __inf("Parent is alreay %d, not need to set.\n", nSclkNo);
                clk_put(hSysClk);
                return 0;
        }
        retCode = clk_set_parent(hModClk, hSysClk);
        if(0 != retCode)
        {
                __wrn("Fail to set parent %s for clk.\n", clk_name);
                clk_put(hSysClk);
                return -1;
        }

        clk_put(hSysClk);

        return retCode;
}

__s32 OSAL_CCMU_GetMclkSrc( __hdle hMclk )
{
        int sysClkNo = 0;
#if 0
        struct clk* hModClk = (struct clk*)hMclk;
        struct clk* hParentClk = clk_get_parent(hModClk);
        const int TOTAL_SYS_CLK = sizeof(_sysClkName)/sizeof(char*);

        for (; sysClkNo <  TOTAL_SYS_CLK; sysClkNo++)
        {
                struct clk* tmpSysClk = clk_get(NULL, _sysClkName[sysClkNo]);

                if(tmpSysClk == NULL)
        	        continue;

                if(hParentClk == tmpSysClk)
                {
                        clk_put(tmpSysClk);
                        break;
                }
                clk_put(tmpSysClk);
        }

        if(sysClkNo >= TOTAL_SYS_CLK)
        {
                __wrn("Failed to get parent clk.\n");
                return -1;
        }
#endif
        return sysClkNo;
}

__s32 OSAL_CCMU_SetMclkDiv( __hdle hMclk, __s32 nDiv )
{
        struct clk* hModClk     = (struct clk*)hMclk;
        struct clk* hParentClk;
        u32         srcRate;

        if(NULL == hModClk || IS_ERR(hModClk))
        {
                __wrn("NULL hdle\n");
                return -1;
        }

        __inf("OSAL_CCMU_SetMclkDiv<0x%0x,%d>\n",(unsigned int)hModClk, nDiv);

        if(nDiv == 0)
        {
        	return -1;
        }

        hParentClk  = clk_get_parent(hModClk);
        if(NULL == hParentClk || IS_ERR(hParentClk))
        {
                __inf("fail to get parent of clk 0x%x \n", (unsigned int)hModClk);
                return -1;
        }

        srcRate = clk_get_rate(hParentClk);

        __inf("clk_set_rate<0x%0x,%d>\n",(unsigned int)hModClk, srcRate/nDiv);
        return clk_set_rate(hModClk, srcRate/nDiv);
}

__u32 OSAL_CCMU_GetMclkDiv( __hdle hMclk )
{
        struct clk* hModClk = (struct clk*)hMclk;
        struct clk* hParentClk;
        u32 mod_freq;
        u32 srcRate;

        if(NULL == hModClk || IS_ERR(hModClk))
        {
                __wrn("NULL hdle\n");
                return -1;
        }

        __inf("OSAL_CCMU_GetMclkDiv of clk 0x%0x\n",(unsigned int)hModClk);

        hParentClk  = clk_get_parent(hModClk);
        if(NULL == hParentClk || IS_ERR(hParentClk))
        {
                __wrn("fail to get parent of clk 0x%x \n", (unsigned int)hModClk);
                return -1;
        }

        srcRate = clk_get_rate(hParentClk);
        mod_freq = clk_get_rate(hModClk);

        if(mod_freq == 0)
        {
        	return 0;	
        }

        return srcRate/mod_freq;
}

__s32 OSAL_CCMU_MclkOnOff( __hdle hMclk, __s32 bOnOff )
{
        struct clk* hModClk = (struct clk*)hMclk;
        __s32 ret = 0;


        if(NULL == hModClk || IS_ERR(hModClk))
        {
                __wrn("NULL hdle\n");
                return -1;
        }

        __inf("OSAL_CCMU_MclkOnOff<0x%0x,%d>\n",(unsigned int)hModClk,bOnOff);

        if(bOnOff)
        {
                if(!hModClk->enable)
                {
                        ret = clk_enable(hModClk);
                        __inf("OSAL_CCMU_MclkOnOff, clk_enable, ret=%d\n", ret);
                }
        }
        else
        {
                while(hModClk->enable)
                {
                        clk_disable(hModClk);
                }
        }
        return ret;
}

__s32 OSAL_CCMU_MclkReset(__hdle hMclk, __s32 bReset)
{
        struct clk* hModClk = (struct clk*)hMclk;

        if(NULL == hModClk || IS_ERR(hModClk))
        {
                __wrn("NULL hdle\n");
                return -1;
        }

        __inf("OSAL_CCMU_MclkReset<0x%x,%d>\n",(unsigned int)hModClk,bReset);

        return clk_reset(hModClk, bReset);
}
#else

typedef __u32 CSP_CCM_sysClkNo_t;


__s32 OSAL_CCMU_SetSrcFreq( CSP_CCM_sysClkNo_t nSclkNo, __u32 nFreq )
{
        return 0;
}

__u32 OSAL_CCMU_GetSrcFreq( CSP_CCM_sysClkNo_t nSclkNo )
{
        return 0;
}

__hdle OSAL_CCMU_OpenMclk( __s32 nMclkNo )
{
        return 0;
}

__s32 OSAL_CCMU_CloseMclk( __hdle hMclk )
{
        return 0;
}

__s32 OSAL_CCMU_SetMclkSrc( __hdle hMclk, CSP_CCM_sysClkNo_t nSclkNo )
{
        return 0;
}

__s32 OSAL_CCMU_GetMclkSrc( __hdle hMclk )
{
        return 0;
}

__s32 OSAL_CCMU_SetMclkDiv( __hdle hMclk, __s32 nDiv )
{
        return 0;
}

__u32 OSAL_CCMU_GetMclkDiv( __hdle hMclk )
{
        return 0;
}

__s32 OSAL_CCMU_MclkOnOff( __hdle hMclk, __s32 bOnOff )
{
        return 0;
}

__s32 OSAL_CCMU_MclkReset(__hdle hMclk, __s32 bReset)
{
        return 0;
}
#endif

