/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
#define _RTL8192E_PHYCFG_C_

//#include <drv_types.h>

#include <rtl8192e_hal.h>


const char *const GLBwSrc[]={
	"CHANNEL_WIDTH_20",
	"CHANNEL_WIDTH_40",
	"CHANNEL_WIDTH_80",
	"CHANNEL_WIDTH_160",
	"CHANNEL_WIDTH_80_80"
};
#define		ENABLE_POWER_BY_RATE		1
#define		POWERINDEX_ARRAY_SIZE		48 //= cckRatesSize + ofdmRatesSize + htRates1TSize + htRates2TSize + vhtRates1TSize + vhtRates1TSize;

/*---------------------Define local function prototype-----------------------*/

/*----------------------------Function Body----------------------------------*/

//
// 1. BB register R/W API
//

u32
PHY_QueryBBReg8192E(
	IN	PADAPTER	Adapter,
	IN	u32			RegAddr,
	IN	u32			BitMask
	)
{
	u32	ReturnValue = 0, OriginalValue, BitShift;

#if (DISABLE_BB_RF == 1)
	return 0;
#endif

	//DBG_871X("--->PHY_QueryBBReg8812(): RegAddr(%#x), BitMask(%#x)\n", RegAddr, BitMask);

	
	OriginalValue = rtw_read32(Adapter, RegAddr);
	BitShift = PHY_CalculateBitShift(BitMask);
	ReturnValue = (OriginalValue & BitMask) >> BitShift;

	//DBG_871X("BBR MASK=0x%x Addr[0x%x]=0x%x\n", BitMask, RegAddr, OriginalValue);
	return (ReturnValue);
}


VOID
PHY_SetBBReg8192E(
	IN	PADAPTER	Adapter,
	IN	u4Byte		RegAddr,
	IN	u4Byte		BitMask,
	IN	u4Byte		Data
	)
{
	u4Byte			OriginalValue, BitShift;

#if (DISABLE_BB_RF == 1)
	return;
#endif

	if(BitMask!= bMaskDWord)
	{//if not "double word" write
		OriginalValue = rtw_read32(Adapter, RegAddr);
		BitShift = PHY_CalculateBitShift(BitMask);
		Data = ((OriginalValue) & (~BitMask)) |( ((Data << BitShift)) & BitMask);
	}

	rtw_write32(Adapter, RegAddr, Data);
	
	//DBG_871X("BBW MASK=0x%x Addr[0x%x]=0x%x\n", BitMask, RegAddr, Data);
}

//
// 2. RF register R/W API
//

static	u32
phy_RFSerialRead(
	IN	PADAPTER		Adapter,
	IN	u8				eRFPath,
	IN	u32				Offset
	)
{

	u4Byte						retValue = 0;
	HAL_DATA_TYPE				*pHalData = GET_HAL_DATA(Adapter);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &pHalData->PHYRegDef[eRFPath];
	u4Byte						NewOffset;
	u4Byte 						tmplong2;
	u1Byte						RfPiEnable=0;
	u1Byte						i;
	u4Byte						MaskforPhySet=0;

	Offset &= 0xff;		

//	RT_TRACE(COMP_INIT, DBG_LOUD, ("phy_RFSerialRead offset 0x%x\n", Offset));
	
	//
	// Switch page for 8256 RF IC
	//
	NewOffset = Offset;

	// For 92S LSSI Read RFLSSIRead
	// For RF A/B write 0x824/82c(does not work in the future) 
	// We must use 0x824 for RF A and B to execute read trigger

	if(eRFPath == RF_PATH_A)
	{
		tmplong2 = PHY_QueryBBReg(Adapter, rFPGA0_XA_HSSIParameter2|MaskforPhySet, bMaskDWord);;
		tmplong2 = (tmplong2 & (~bLSSIReadAddress)) | (NewOffset<<23) | bLSSIReadEdge;	//T65 RF
		PHY_SetBBReg(Adapter, rFPGA0_XA_HSSIParameter2|MaskforPhySet, bMaskDWord, tmplong2&(~bLSSIReadEdge));	
	}
	else
	{
		tmplong2 = PHY_QueryBBReg(Adapter, rFPGA0_XB_HSSIParameter2|MaskforPhySet, bMaskDWord);
		tmplong2 = (tmplong2 & (~bLSSIReadAddress)) | (NewOffset<<23) | bLSSIReadEdge;	//T65 RF
		PHY_SetBBReg(Adapter, rFPGA0_XB_HSSIParameter2|MaskforPhySet, bMaskDWord, tmplong2&(~bLSSIReadEdge));	
	}

	tmplong2 = PHY_QueryBBReg(Adapter, rFPGA0_XA_HSSIParameter2|MaskforPhySet, bMaskDWord);
	PHY_SetBBReg(Adapter, rFPGA0_XA_HSSIParameter2|MaskforPhySet, bMaskDWord, tmplong2 & (~bLSSIReadEdge));			
	PHY_SetBBReg(Adapter, rFPGA0_XA_HSSIParameter2|MaskforPhySet, bMaskDWord, tmplong2 | bLSSIReadEdge);		
	
	rtw_udelay_os(10);// PlatformStallExecution(10);

	//for(i=0;i<2;i++)
	//	PlatformStallExecution(MAX_STALL_TIME);	
	rtw_udelay_os(10);//PlatformStallExecution(10);

	if(eRFPath == RF_PATH_A)
		RfPiEnable = (u1Byte)PHY_QueryBBReg(Adapter, rFPGA0_XA_HSSIParameter1|MaskforPhySet, BIT8);
	else if(eRFPath == RF_PATH_B)
		RfPiEnable = (u1Byte)PHY_QueryBBReg(Adapter, rFPGA0_XB_HSSIParameter1|MaskforPhySet, BIT8);
	
	if(RfPiEnable)
	{	// Read from BBreg8b8, 12 bits for 8190, 20bits for T65 RF
		retValue = PHY_QueryBBReg(Adapter, pPhyReg->rfLSSIReadBackPi|MaskforPhySet, bLSSIReadBackData);
	
		//RT_DISP(FINIT, INIT_RF, ("Readback from RF-PI : 0x%x\n", retValue));
	}
	else
	{	//Read from BBreg8a0, 12 bits for 8190, 20 bits for T65 RF
		retValue = PHY_QueryBBReg(Adapter, pPhyReg->rfLSSIReadBack|MaskforPhySet, bLSSIReadBackData);
		
		//RT_DISP(FINIT, INIT_RF,("Readback from RF-SI : 0x%x\n", retValue));
	}
	//RT_DISP(FPHY, PHY_RFR, ("RFR-%d Addr[0x%x]=0x%x\n", eRFPath, pPhyReg->rfLSSIReadBack, retValue));
	
	return retValue;	
		
}



static	VOID
phy_RFSerialWrite(
	IN	PADAPTER		Adapter,
	IN	u8				eRFPath,
	IN	u32				Offset,
	IN	u32				Data
	)
{
	u32							DataAndAddr = 0;
	HAL_DATA_TYPE				*pHalData = GET_HAL_DATA(Adapter);
	BB_REGISTER_DEFINITION_T		*pPhyReg = &pHalData->PHYRegDef[eRFPath];
	u32							NewOffset,MaskforPhySet=0;

	// 2009/06/17 MH We can not execute IO for power save or other accident mode.
	//if(RT_CANNOT_IO(Adapter))
	//{
	//	RTPRINT(FPHY, PHY_RFW, ("phy_RFSerialWrite stop\n"));
	//	return;
	//}

	// <20121026, Kordan> If 0x818 == 1, the second value written on the previous address.
	PHY_SetBBReg(Adapter, ODM_AFE_SETTING, 0x20000, 0x0);
    
	Offset &= 0xff;

	// Shadow Update
	//PHY_RFShadowWrite(Adapter, eRFPath, Offset, Data);

	//
	// Switch page for 8256 RF IC
	//
	NewOffset = Offset;

	//
	// Put write addr in [5:0]  and write data in [31:16]
	//
	//DataAndAddr = (Data<<16) | (NewOffset&0x3f);
	DataAndAddr = ((NewOffset<<20) | (Data&0x000fffff)) & 0x0fffffff;	// T65 RF

	//
	// Write Operation
	//
	PHY_SetBBReg(Adapter, pPhyReg->rf3wireOffset|MaskforPhySet, bMaskDWord, DataAndAddr);

	// <20121026, Kordan> Restore the value on exit.
	if (IS_HARDWARE_TYPE_8192EU(Adapter))
		PHY_SetBBReg(Adapter, ODM_AFE_SETTING, 0x20000, 0x1);
}

u32
PHY_QueryRFReg8192E(
	IN	PADAPTER		Adapter,
	IN	u8				eRFPath,
	IN	u32				RegAddr,
	IN	u32				BitMask
	)
{
	u32				Original_Value, Readback_Value, BitShift;	

#if (DISABLE_BB_RF == 1)
	return 0;
#endif

	Original_Value = phy_RFSerialRead(Adapter, eRFPath, RegAddr);	   	
	
	BitShift =  PHY_CalculateBitShift(BitMask);
	Readback_Value = (Original_Value & BitMask) >> BitShift;	

	return (Readback_Value);
}

VOID
PHY_SetRFReg8192E(
	IN	PADAPTER		Adapter,
	IN	u8				eRFPath,
	IN	u32				RegAddr,
	IN	u32				BitMask,
	IN	u32				Data
	)
{
	u32 			Original_Value, BitShift;
#if (DISABLE_BB_RF == 1)
	return;
#endif

	if(BitMask == 0)
		return;

	// RF data is 20 bits only
	if (BitMask != bRFRegOffsetMask) {
		Original_Value = phy_RFSerialRead(Adapter, eRFPath, RegAddr);
		BitShift =  PHY_CalculateBitShift(BitMask);
		Data = ((Original_Value) & (~BitMask)) | (Data<< BitShift);
	}
	
	
	phy_RFSerialWrite(Adapter, eRFPath, RegAddr, Data);

}

//
// 3. Initial MAC/BB/RF config by reading MAC/BB/RF txt.
//

s32 PHY_MACConfig8192E(PADAPTER Adapter)
{
	int				rtStatus = _SUCCESS;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	s8				*pszMACRegFile;
	s8				sz8192EMACRegFile[] = RTL8192E_PHY_MACREG;

	pszMACRegFile = sz8192EMACRegFile;

	//
	// Config MAC
	//
#ifdef CONFIG_EMBEDDED_FWIMG
	if(HAL_STATUS_SUCCESS != ODM_ConfigMACWithHeaderFile(&pHalData->odmpriv))
		rtStatus = _FAIL;
#else

	// Not make sure EEPROM, add later
	DBG_871X("Read MACREG.txt\n");
	rtStatus = phy_ConfigMACWithParaFile(Adapter, pszMACRegFile);	
#endif//CONFIG_EMBEDDED_FWIMG

	return rtStatus;
}


static	VOID
phy_InitBBRFRegisterDefinition(
	IN	PADAPTER		Adapter
)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);	

	// RF Interface Sowrtware Control
	pHalData->PHYRegDef[RF_PATH_A].rfintfs = rFPGA0_XAB_RFInterfaceSW; // 16 LSBs if read 32-bit from 0x870
	pHalData->PHYRegDef[RF_PATH_B].rfintfs = rFPGA0_XAB_RFInterfaceSW; // 16 MSBs if read 32-bit from 0x870 (16-bit for 0x872)

	// RF Interface Output (and Enable)
	pHalData->PHYRegDef[RF_PATH_A].rfintfo = rFPGA0_XA_RFInterfaceOE; // 16 LSBs if read 32-bit from 0x860
	pHalData->PHYRegDef[RF_PATH_B].rfintfo = rFPGA0_XB_RFInterfaceOE; // 16 LSBs if read 32-bit from 0x864

	// RF Interface (Output and)  Enable
	pHalData->PHYRegDef[RF_PATH_A].rfintfe = rFPGA0_XA_RFInterfaceOE; // 16 MSBs if read 32-bit from 0x860 (16-bit for 0x862)
	pHalData->PHYRegDef[RF_PATH_B].rfintfe = rFPGA0_XB_RFInterfaceOE; // 16 MSBs if read 32-bit from 0x864 (16-bit for 0x866)

	pHalData->PHYRegDef[RF_PATH_A].rf3wireOffset = rFPGA0_XA_LSSIParameter; //LSSI Parameter
	pHalData->PHYRegDef[RF_PATH_B].rf3wireOffset = rFPGA0_XB_LSSIParameter;

	pHalData->PHYRegDef[RF_PATH_A].rfHSSIPara2 = rFPGA0_XA_HSSIParameter2;  //wire control parameter2
	pHalData->PHYRegDef[RF_PATH_B].rfHSSIPara2 = rFPGA0_XB_HSSIParameter2;  //wire control parameter2

        // Tranceiver Readback LSSI/HSPI mode 
      pHalData->PHYRegDef[RF_PATH_A].rfLSSIReadBack = rFPGA0_XA_LSSIReadBack;
	pHalData->PHYRegDef[RF_PATH_B].rfLSSIReadBack = rFPGA0_XB_LSSIReadBack;
	pHalData->PHYRegDef[RF_PATH_A].rfLSSIReadBackPi = TransceiverA_HSPI_Readback;
	pHalData->PHYRegDef[RF_PATH_B].rfLSSIReadBackPi = TransceiverB_HSPI_Readback;
	
	//pHalData->bPhyValueInitReady=TRUE;
}

static	int
phy_BB8192E_Config_ParaFile(
	IN	PADAPTER	Adapter
	)
{
	EEPROM_EFUSE_PRIV	*pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	int			rtStatus = _SUCCESS;

	s8				sz8192EBBRegFile[] = RTL8192E_PHY_REG;	
	s8				sz8192EAGCTableFile[] = RTL8192E_AGC_TAB;
	s8				sz8192EBBRegPgFile[] = RTL8192E_PHY_REG_PG;
	s8				sz8192EBBRegMpFile[] = RTL8192E_PHY_REG_MP;	
	s8				sz8192ERFRegLimitFile[] = RTL8192E_TXPWR_LMT;	

	s8				*pszBBRegFile = NULL, *pszAGCTableFile = NULL, 
					*pszBBRegPgFile = NULL, *pszBBRegMpFile=NULL,
					*pszBBRegLimitFile = NULL,*pszRFTxPwrLmtFile = NULL;


	//DBG_871X("==>phy_BB8192E_Config_ParaFile\n");

	pszBBRegFile=sz8192EBBRegFile ;
	pszAGCTableFile =sz8192EAGCTableFile;
	pszBBRegPgFile = sz8192EBBRegPgFile;
	pszBBRegMpFile = sz8192EBBRegMpFile;
	pszRFTxPwrLmtFile = sz8192ERFRegLimitFile;

	DBG_871X("===> phy_BB8192E_Config_ParaFile() EEPROMRegulatory %d\n", pHalData->EEPROMRegulatory );

	//DBG_871X(" ===> phy_BB8192E_Config_ParaFile() phy_reg:%s\n",pszBBRegFile);
	//DBG_871X(" ===> phy_BB8192E_Config_ParaFile() phy_reg_pg:%s\n",pszBBRegPgFile);
	//DBG_871X(" ===> phy_BB8192E_Config_ParaFile() agc_table:%s\n",pszAGCTableFile);

	PHY_InitTxPowerLimit( Adapter );

	if ( ( Adapter->registrypriv.RegEnableTxPowerLimit == 1 && pHalData->EEPROMRegulatory != 2 ) || 
		 pHalData->EEPROMRegulatory == 1 )
	{
#ifdef CONFIG_EMBEDDED_FWIMG
		if (HAL_STATUS_SUCCESS != ODM_ConfigRFWithHeaderFile(&pHalData->odmpriv, CONFIG_RF_TXPWR_LMT, (ODM_RF_RADIO_PATH_E)0))
			rtStatus = _FAIL;
#else
		rtStatus = PHY_ConfigRFWithPowerLimitTableParaFile( Adapter, pszRFTxPwrLmtFile );
#endif

		if(rtStatus != _SUCCESS){
			DBG_871X("phy_BB8192E_Config_ParaFile():Read Tx power limit fail\n");
			goto phy_BB_Config_ParaFile_Fail;
		}
	}

	// Read PHY_REG.TXT BB INIT!!
#ifdef CONFIG_EMBEDDED_FWIMG
	if (HAL_STATUS_SUCCESS != ODM_ConfigBBWithHeaderFile(&pHalData->odmpriv, CONFIG_BB_PHY_REG))
		rtStatus = _FAIL;
#else	
	// No matter what kind of CHIP we always read PHY_REG.txt. We must copy different 
	// type of parameter files to phy_reg.txt at first.	
	rtStatus = phy_ConfigBBWithParaFile(Adapter,pszBBRegFile);
#endif

	if(rtStatus != _SUCCESS){
		DBG_871X("phy_BB8192E_Config_ParaFile():Write BB Reg Fail!!\n");
		goto phy_BB_Config_ParaFile_Fail;
	}

//f (MP_DRIVER == 1)
#if 0
	// Read PHY_REG_MP.TXT BB INIT!!
#ifdef CONFIG_EMBEDDED_FWIMG
	if (HAL_STATUS_SUCCESS != ODM_ConfigBBWithHeaderFile(&pHalData->odmpriv, CONFIG_BB_PHY_REG))
		rtStatus = _FAIL; 
#else	
	// No matter what kind of CHIP we always read PHY_REG.txt. We must copy different 
	// type of parameter files to phy_reg.txt at first.	
	rtStatus = phy_ConfigBBWithMpParaFile(Adapter,pszBBRegMpFile);
#endif

	if(rtStatus != _SUCCESS){
		DBG_871X("phy_BB8192E_Config_ParaFile():Write BB Reg MP Fail!!");
		goto phy_BB_Config_ParaFile_Fail;
	}	
#endif	// #if (MP_DRIVER == 1)

	// If EEPROM or EFUSE autoload OK, We must config by PHY_REG_PG.txt
	PHY_InitTxPowerByRate( Adapter );
	if (pEEPROM->bautoload_fail_flag == _FALSE)
	{
#ifdef CONFIG_EMBEDDED_FWIMG
		if (HAL_STATUS_SUCCESS != ODM_ConfigBBWithHeaderFile(&pHalData->odmpriv, CONFIG_BB_PHY_REG_PG))
			rtStatus = _FAIL;
#else
		rtStatus = phy_ConfigBBWithPgParaFile(Adapter, pszBBRegPgFile);
#endif
		if ( pHalData->odmpriv.PhyRegPgValueType == PHY_REG_PG_EXACT_VALUE )
			PHY_TxPowerByRateConfiguration( Adapter );

		if(rtStatus != _SUCCESS){
			DBG_871X("phy_BB8192E_Config_ParaFile():BB_PG Reg Fail!!\n");
		}

		if ( ( Adapter->registrypriv.RegEnableTxPowerLimit == 1 && pHalData->EEPROMRegulatory != 2 ) || 
		 	 pHalData->EEPROMRegulatory == 1 )
			PHY_ConvertTxPowerLimitToPowerIndex( Adapter );
	}


	// BB AGC table Initialization
#ifdef CONFIG_EMBEDDED_FWIMG
	if (HAL_STATUS_SUCCESS != ODM_ConfigBBWithHeaderFile(&pHalData->odmpriv, CONFIG_BB_AGC_TAB))
		rtStatus = _FAIL;
#else
	rtStatus = phy_ConfigBBWithParaFile(Adapter, pszAGCTableFile);
#endif

	if(rtStatus != _SUCCESS){
		DBG_871X("phy_BB8192E_Config_ParaFile():AGC Table Fail\n");
		goto phy_BB_Config_ParaFile_Fail;
	}

phy_BB_Config_ParaFile_Fail:

	return rtStatus;
}

int
PHY_BBConfig8192E(
	IN	PADAPTER	Adapter
	)
{
	int	rtStatus = _SUCCESS;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8	TmpU1B=0;
	u8	CrystalCap;

	phy_InitBBRFRegisterDefinition(Adapter);

	// Enable BB and RF
	TmpU1B = rtw_read16(Adapter, REG_SYS_FUNC_EN);

#ifdef CONFIG_PCI_HCI	
	if(IS_HARDWARE_TYPE_8192EE(Adapter))
		TmpU1B |= (FEN_PPLL|FEN_PCIEA|FEN_DIO_PCIE);
#endif
#ifdef CONFIG_USB_HCI
	if(IS_HARDWARE_TYPE_8192EU(Adapter))
		TmpU1B |= (FEN_USBA| FEN_USBD);
#endif

	TmpU1B |=  (BIT13|FEN_BB_GLB_RSTn|FEN_BBRSTB);

	rtw_write8(Adapter, REG_SYS_FUNC_EN, TmpU1B);

	//6. 0x1f[7:0] = 0x07 PathA RF Power On
	rtw_write8(Adapter, REG_RF_CTRL, RF_EN|RF_RSTB|RF_SDMRSTB);

	//rtw_write8(Adapter, REG_AFE_XTAL_CTRL+1, 0x80);
	//
	// Config BB and AGC
	//
	rtStatus = phy_BB8192E_Config_ParaFile(Adapter);
	
	
	CrystalCap = pHalData->CrystalCap & 0x3F;
	
#if 1	
	// write 0x2C[17:12] = 0x2C[23:18] = CrystalCap [6 bits]
	if(CrystalCap == 0)
		CrystalCap = EEPROM_Default_CrystalCap_8192E;
	DBG_871X( "%s ==> CrystalCap:0x%02x \n",__FUNCTION__,CrystalCap);
	PHY_SetBBReg(Adapter, REG_AFE_CTRL3_8192E, 0xFFF000, (CrystalCap | (CrystalCap << 6)));

	// write 0x24= 000f81fb ,suggest by Ed		
	rtw_write32(Adapter,REG_AFE_CTRL1_8192E,0x000f81fb);
#endif

	return rtStatus;
	
}
#if 0
// <20130320, VincentLan> A workaround to eliminate the 2480MHz spur for 92E
void 
phy_FixSpur_8192E(
	IN	PADAPTER				Adapter,
	IN	SPUR_CAL_METHOD		Method
	)
{
	u32			reg0x18 = 0;
	u8			retryNum = 0;
	u8			MaxRetryCount = 8;
	u8			Pass_A = FALSE, Pass_B = FALSE;
	u8			SpurOccur = FALSE;
	u32			PSDReport = 0;

	//DBG_871X("==0419v2==Vincent== FixSpur_8192E \n");

	if (Method == PLL_RESET){
		MaxRetryCount = 3;
	}
	else if (Method == AFE_PHASE_SEL){
		//PHY_SetMacReg(Adapter, RF_TX_G1, BIT4, 0x1); // enable
		rtw_write8(Adapter, RF_TX_G1, (rtw_read8(Adapter, RF_TX_G1)|BIT4)); // enable
		//DBG_871X("===0x20 = %x \n", rtw_read8(Adapter, RF_TX_G1));
	}
	
	// Backup current channel
	reg0x18 = PHY_QueryRFReg(Adapter, ODM_RF_PATH_A, RF_CHNLBW, bRFRegOffsetMask);

	
	while ((retryNum++ < MaxRetryCount))
	{
		PHY_SetRFReg(Adapter, ODM_RF_PATH_A, RF_CHNLBW, bRFRegOffsetMask, 0x7C0D); //CH13
		PHY_SetBBReg(Adapter, rOFDM0_XAAGCCore1, bMaskByte0, 0x30); //Path A initial gain
		PHY_SetBBReg(Adapter, rOFDM0_XBAGCCore1, bMaskByte0, 0x30); //Path B initial gain
		PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter4, bMaskDWord, 0xccf000c0); 	// disable 3-wire

		// Path A
		PHY_SetBBReg(Adapter, rFPGA0_TxInfo, bMaskByte0, 0x3);
		DBG_871X(" ** 0x804 = 0x%x\n",PHY_QueryBBReg(Adapter, rFPGA0_TxInfo, bMaskByte0));
		PHY_SetBBReg(Adapter, rFPGA0_PSDFunction, bMaskDWord, 0xfccd);
		DBG_871X(" ** 0x808 = 0x%x\n",PHY_QueryBBReg(Adapter, rFPGA0_PSDFunction, bMaskDWord));
		PHY_SetBBReg(Adapter, rFPGA0_PSDFunction, bMaskDWord, 0x40fccd);
		DBG_871X(" ** 0x808 = 0x%x\n",PHY_QueryBBReg(Adapter, rFPGA0_PSDFunction, bMaskDWord));
		//rtw_msleep_os(100);
		rtw_mdelay_os(100);
		PSDReport = PHY_QueryBBReg(Adapter, rFPGA0_PSDReport, bMaskDWord);
		DBG_871X(" Path A== PSDReport = 0x%x\n",PSDReport);
		if (PSDReport < 0x16)
			Pass_A = TRUE;
		
		// Path B
		PHY_SetBBReg(Adapter, rFPGA0_TxInfo, bMaskByte0, 0x13);
		PHY_SetBBReg(Adapter, rFPGA0_PSDFunction, bMaskDWord, 0xfccd);
		PHY_SetBBReg(Adapter, rFPGA0_PSDFunction, bMaskDWord, 0x40fccd);
		//rtw_msleep_os(100);
		rtw_mdelay_os(100);
		PSDReport = PHY_QueryBBReg(Adapter, rFPGA0_PSDReport, bMaskDWord);
		DBG_871X(" Path B== PSDReport = 0x%x\n",PSDReport);
		if (PSDReport < 0x16)
			Pass_B = TRUE;

		if (Pass_A && Pass_B)
		{
			DBG_871X("=== PathA=%d, PathB=%d\n", Pass_A, Pass_B);
			DBG_871X("===FixSpur Pass!\n");
			PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter4, bMaskDWord, 0xcc0000c0); 	// enable 3-wire
			PHY_SetBBReg(Adapter, rFPGA0_PSDFunction, bMaskDWord, 0xfc00);
			PHY_SetBBReg(Adapter, rOFDM0_XAAGCCore1, bMaskByte0, 0x20);
			PHY_SetBBReg(Adapter, rOFDM0_XBAGCCore1, bMaskByte0, 0x20);
			break;
		}
		else
		{
			Pass_A = FALSE;
			Pass_B = FALSE;
			if (Method == PLL_RESET)
			{
				//PHY_SetMacReg(Adapter, 0x28, bMaskByte1, 0x7);	// PLL gated 320M CLK disable
				//PHY_SetMacReg(Adapter, 0x28, bMaskByte1, 0x47);	// PLL gated 320M CLK enable
				rtw_write8(Adapter,0x29, 0x7);	// PLL gated 320M CLK disable
				rtw_write8(Adapter,0x29, 0x47);	// PLL gated 320M CLK enable
				
			}
			else if (Method == AFE_PHASE_SEL)
			{
				if (!SpurOccur)
				{
					SpurOccur = TRUE;
					DBG_871X("===FixSpur NOT Pass!\n");
					//PHY_SetMacReg(Adapter, RF_TX_G1, BIT4, 0x1);
					//PHY_SetMacReg(Adapter, 0x28, bMaskByte0, 0x80);
					//PHY_SetMacReg(Adapter, 0x28, bMaskByte0, 0x83);
					rtw_write8(Adapter, RF_TX_G1, (rtw_read8(Adapter, RF_TX_G1)|BIT4)); // enable
					rtw_write8(Adapter, 0x28, 0x80);
					rtw_write8(Adapter, 0x28, 0x83);	
					
				}
				DBG_871X("===Round %d\n", retryNum+1);
				if (retryNum < 6){
					//PHY_SetMacReg(Adapter, RF_TX_G1, BIT5|BIT6|BIT7, 1+retryNum);
					rtw_write8(Adapter, RF_TX_G1, (rtw_read8(Adapter, RF_TX_G1)&0x1F)|((1+retryNum)<<5));
				}
				else{
					break;
				}
			}
		}
	}
	PHY_SetBBReg(Adapter, rFPGA0_PSDFunction, bMaskDWord, 0xfccd); // reset PSD
	PHY_SetRFReg(Adapter, ODM_RF_PATH_A, RF_CHNLBW, bRFRegOffsetMask, reg0x18); //restore chnl
	// 0x20 NOT RESET
	// PHY_SetMacReg(Adapter, RF_TX_G1, BIT4, 0x0); // reset 0x20

}
#endif

int
PHY_RFConfig8192E(
	IN	PADAPTER	Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	int		rtStatus = _SUCCESS;

	if (Adapter->bSurpriseRemoved)
		return _FAIL;

	switch(pHalData->rf_chip)
	{
		case RF_6052:
			rtStatus = PHY_RF6052_Config_8192E(Adapter);
			break;

		case RF_PSEUDO_11N:
			break;
		default: //for MacOs Warning: "RF_TYPE_MIN" not handled in switch
			break;
	}
// <20121002, Kordan> Do LCK, because the PHY reg files make no effect. (Asked by Edlu)
// Only Test chip need set 0xb1= 0x55418, (Edlu)
	//PHY_SetRFReg(Adapter, RF_PATH_A, RF_LDO, bRFRegOffsetMask, 0x55418);
	//PHY_SetRFReg(Adapter, RF_PATH_B, RF_LDO, bRFRegOffsetMask, 0x55418);	
	
	return rtStatus;
}

static u8
phy_DbmToTxPwrIdx(
	IN	PADAPTER		Adapter,
	IN	WIRELESS_MODE	WirelessMode,
	IN	int			PowerInDbm
	)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	u8	TxPwrIdx = 0;
	s32	Offset = 0;

#if 0
	//
	// Tested by MP, we found that CCK Index 0 equals to 8dbm, OFDM legacy equals to 
	// 3dbm, and OFDM HT equals to 0dbm repectively.
	// Note:
	//	The mapping may be different by different NICs. Do not use this formula for what needs accurate result.  
	// By Bruce, 2008-01-29.
	// 
	switch(WirelessMode)
	{
	case WIRELESS_MODE_B:		
		//Offset = -7;
		Offset = -6;	// For 88 RU test only		
		TxPwrIdx = (u8)((pHalData->OriginalCckTxPwrIdx*( PowerInDbm-pHalData->MinCCKDbm))/(pHalData->MaxCCKDbm-pHalData->MinCCKDbm));		
		break;

	case WIRELESS_MODE_G:
	case WIRELESS_MODE_N_24G:
		Offset = -8;		
		TxPwrIdx = (u8)((pHalData->OriginalOfdm24GTxPwrIdx* (PowerInDbm-pHalData->MinHOFDMDbm))/(pHalData->MaxHOFDMDbm-pHalData->MinHOFDMDbm));
		break;
	
	default: //for MacOSX compiler warning
		break;		
	}

	if (PowerInDbm <= pHalData->MinCCKDbm || 
		PowerInDbm <= pHalData->MinLOFDMDbm ||
		PowerInDbm <= pHalData->MinHOFDMDbm)
	{
		TxPwrIdx = 0;
	}

	// Simple judge to prevent tx power exceed the limitation.
	if (PowerInDbm >= pHalData->MaxCCKDbm || 
		PowerInDbm >= pHalData->MaxLOFDMDbm ||
		PowerInDbm >= pHalData->MaxHOFDMDbm)
	{
		if (WirelessMode == WIRELESS_MODE_B)
			TxPwrIdx = pHalData->OriginalCckTxPwrIdx;
		else
			TxPwrIdx = pHalData->OriginalOfdm24GTxPwrIdx;
	}
#endif
	return TxPwrIdx;
}

void getTxPowerIndex92E(
	IN	PADAPTER		Adapter,
	IN	u8				channel,
	IN OUT u8*			cckPowerLevel,
	IN OUT u8*			ofdmPowerLevel,
	IN OUT u8*			BW20PowerLevel,
	IN OUT u8*			BW40PowerLevel
	)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	u8				index = (channel -1);
	u8				PathNum=0;

	for(PathNum=0;PathNum<MAX_TX_COUNT;PathNum++)
	{
		if(PathNum==ODM_RF_PATH_A)
		{
			// 1. CCK
			cckPowerLevel[PathNum]		= pHalData->Index24G_CCK_Base[PathNum][index];
			//2. OFDM
			ofdmPowerLevel[PathNum]		= pHalData->Index24G_BW40_Base[ODM_RF_PATH_A][index]+
				pHalData->OFDM_24G_Diff[PathNum][ODM_RF_PATH_A];	
			// 1. BW20
			BW20PowerLevel[PathNum]	= pHalData->Index24G_BW40_Base[ODM_RF_PATH_A][index]+
				pHalData->BW20_24G_Diff[PathNum][ODM_RF_PATH_A];
			//2. BW40
			BW40PowerLevel[PathNum]	= pHalData->Index24G_BW40_Base[PathNum][index];
		}
		else if(PathNum==ODM_RF_PATH_B)
		{
			// 1. CCK
			cckPowerLevel[PathNum]		= pHalData->Index24G_CCK_Base[PathNum][index];
			//2. OFDM
			ofdmPowerLevel[PathNum]		= pHalData->Index24G_BW40_Base[ODM_RF_PATH_A][index]+
			pHalData->BW20_24G_Diff[ODM_RF_PATH_A][index]+
			pHalData->BW20_24G_Diff[PathNum][index];	
			// 1. BW20
			BW20PowerLevel[PathNum]	= pHalData->Index24G_BW40_Base[ODM_RF_PATH_A][index]+
			pHalData->BW20_24G_Diff[PathNum][ODM_RF_PATH_A]+
			pHalData->BW20_24G_Diff[PathNum][index];
			//2. BW40
			BW40PowerLevel[PathNum]	= pHalData->Index24G_BW40_Base[PathNum][index];		
		}
		else if(PathNum==ODM_RF_PATH_C)
		{
			// 1. CCK
			cckPowerLevel[PathNum]		= pHalData->Index24G_CCK_Base[PathNum][index];
			//2. OFDM
			ofdmPowerLevel[PathNum]		= pHalData->Index24G_BW40_Base[ODM_RF_PATH_A][index]+
			pHalData->BW20_24G_Diff[ODM_RF_PATH_A][index]+
			pHalData->BW20_24G_Diff[ODM_RF_PATH_B][index]+
			pHalData->BW20_24G_Diff[PathNum][index];
			// 1. BW20
			BW20PowerLevel[PathNum]	= pHalData->Index24G_BW40_Base[ODM_RF_PATH_A][index]+
			pHalData->BW20_24G_Diff[ODM_RF_PATH_A][index]+
			pHalData->BW20_24G_Diff[ODM_RF_PATH_B][index]+
			pHalData->BW20_24G_Diff[PathNum][index];
			//2. BW40
			BW40PowerLevel[PathNum]	= pHalData->Index24G_BW40_Base[PathNum][index];		
		}
		else if(PathNum==ODM_RF_PATH_D)
		{
			// 1. CCK
			cckPowerLevel[PathNum]		= pHalData->Index24G_CCK_Base[PathNum][index];
			//2. OFDM
			ofdmPowerLevel[PathNum]		= pHalData->Index24G_BW40_Base[ODM_RF_PATH_A][index]+
				pHalData->BW20_24G_Diff[ODM_RF_PATH_A][index]+
				pHalData->BW20_24G_Diff[ODM_RF_PATH_B][index]+
				pHalData->BW20_24G_Diff[ODM_RF_PATH_C][index]+
				pHalData->BW20_24G_Diff[PathNum][index];

			// 1. BW20
			BW20PowerLevel[PathNum]	= pHalData->Index24G_BW40_Base[ODM_RF_PATH_A][index]+
				pHalData->BW20_24G_Diff[ODM_RF_PATH_A][index]+
				pHalData->BW20_24G_Diff[ODM_RF_PATH_B][index]+
				pHalData->BW20_24G_Diff[ODM_RF_PATH_C][index]+
				pHalData->BW20_24G_Diff[PathNum][index];

			//2. BW40
			BW40PowerLevel[PathNum]	= pHalData->Index24G_BW40_Base[PathNum][index];		
		}	
		else
		{
		}
	}

#if 0 // (INTEL_PROXIMITY_SUPPORT == 1)
	switch(pMgntInfo->IntelProximityModeInfo.PowerOutput){
		case 1: // 100%
			break;
		case 2: // 70%
			cckPowerLevel[0] -= 3;
			cckPowerLevel[1] -= 3;
			ofdmPowerLevel[0] -=3;
			ofdmPowerLevel[1] -= 3; 
			break;
		case 3: // 50%
			cckPowerLevel[0] -= 6;
			cckPowerLevel[1] -= 6;
			ofdmPowerLevel[0] -=6;
			ofdmPowerLevel[1] -= 6; 
			break;
		case 4: // 35%
			cckPowerLevel[0] -= 9;
			cckPowerLevel[1] -= 9;
			ofdmPowerLevel[0] -=9;
			ofdmPowerLevel[1] -= 9; 
			break;
		case 5: // 15%
			cckPowerLevel[0] -= 17;
			cckPowerLevel[1] -= 17;
			ofdmPowerLevel[0] -=17;
			ofdmPowerLevel[1] -= 17; 
			break;
	
		default:
			break;
	}	
#endif
	//RT_DISP(FPHY, PHY_TXPWR, ("Channel-%d, set tx power index !!\n", channel));
}

VOID
PHY_GetTxPowerLevel8192E(
	IN	PADAPTER		Adapter,
	OUT u32*    		powerlevel
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8			TxPwrLevel = 0;
	int			TxPwrDbm;
#if 0
	//
	// Because the Tx power indexes are different, we report the maximum of them to
	// meet the CCX TPC request. By Bruce, 2008-01-31.
	//

	// CCK
	TxPwrLevel = pHalData->CurrentCckTxPwrIdx;
	TxPwrDbm = phy_TxPwrIdxToDbm(Adapter, WIRELESS_MODE_B, TxPwrLevel);
	pHalData->MaxCCKDbm = TxPwrDbm;

	// Legacy OFDM
	TxPwrLevel = pHalData->CurrentOfdm24GTxPwrIdx + pHalData->LegacyHTTxPowerDiff;

	// Compare with Legacy OFDM Tx power.
	pHalData->MaxLOFDMDbm = phy_TxPwrIdxToDbm(Adapter, WIRELESS_MODE_G, TxPwrLevel);
	if(phy_TxPwrIdxToDbm(Adapter, WIRELESS_MODE_G, TxPwrLevel) > TxPwrDbm)
		TxPwrDbm = phy_TxPwrIdxToDbm(Adapter, WIRELESS_MODE_G, TxPwrLevel);

	// HT OFDM
	TxPwrLevel = pHalData->CurrentOfdm24GTxPwrIdx;

	// Compare with HT OFDM Tx power.
	pHalData->MaxHOFDMDbm = phy_TxPwrIdxToDbm(Adapter, WIRELESS_MODE_G, TxPwrLevel);
	if(phy_TxPwrIdxToDbm(Adapter, WIRELESS_MODE_N_24G, TxPwrLevel) > TxPwrDbm)
		TxPwrDbm = phy_TxPwrIdxToDbm(Adapter, WIRELESS_MODE_N_24G, TxPwrLevel);
	pHalData->MaxHOFDMDbm = TxPwrDbm;

	*powerlevel = TxPwrDbm;
#endif
}

void phy_PowerIndexCheck92E(
	IN	PADAPTER	Adapter,
	IN	u8			channel,
	IN OUT u8 *		cckPowerLevel,
	IN OUT u8 *		ofdmPowerLevel,
	IN OUT u8 *		BW20PowerLevel,
	IN OUT u8 *		BW40PowerLevel	
	)
{

	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
#if 0//(CCX_SUPPORT == 1)
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_CCX_INFO		pCcxInfo = GET_CCX_INFO(pMgntInfo);

	//
	// CCX 2 S31, AP control of client transmit power:
	// 1. We shall not exceed Cell Power Limit as possible as we can.
	// 2. Tolerance is +/- 5dB.
	// 3. 802.11h Power Contraint takes higher precedence over CCX Cell Power Limit.
	// 
	// TODO: 
	// 1. 802.11h power contraint 
	//
	// 071011, by rcnjko.
	//
	if(	pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE && 
		pMgntInfo->mAssoc &&
		pCcxInfo->bUpdateCcxPwr &&
		pCcxInfo->bWithCcxCellPwr &&
		channel == pMgntInfo->dot11CurrentChannelNumber)
	{
		u1Byte	CckCellPwrIdx = phy_DbmToTxPwrIdx(Adapter, WIRELESS_MODE_B, pCcxInfo->CcxCellPwr);
		u1Byte	LegacyOfdmCellPwrIdx = phy_DbmToTxPwrIdx(Adapter, WIRELESS_MODE_G, pCcxInfo->CcxCellPwr);
		u1Byte	OfdmCellPwrIdx = phy_DbmToTxPwrIdx(Adapter, WIRELESS_MODE_N_24G, pCcxInfo->CcxCellPwr);

		RT_TRACE(COMP_TXAGC, DBG_LOUD, 
		("CCX Cell Limit: %d dbm => CCK Tx power index : %d, Legacy OFDM Tx power index : %d, OFDM Tx power index: %d\n", 
		pCcxInfo->CcxCellPwr, CckCellPwrIdx, LegacyOfdmCellPwrIdx, OfdmCellPwrIdx));
		RT_TRACE(COMP_TXAGC, DBG_LOUD, 
		("EEPROM channel(%d) => CCK Tx power index: %d, Legacy OFDM Tx power index : %d, OFDM Tx power index: %d\n",
		channel, cckPowerLevel[0], ofdmPowerLevel[0] + pHalData->LegacyHTTxPowerDiff, ofdmPowerLevel[0])); 

		// CCK
		if(cckPowerLevel[0] > CckCellPwrIdx)
			cckPowerLevel[0] = CckCellPwrIdx;
		// Legacy OFDM, HT OFDM
		if(ofdmPowerLevel[0] + pHalData->LegacyHTTxPowerDiff > LegacyOfdmCellPwrIdx)
		{
			if((OfdmCellPwrIdx - pHalData->LegacyHTTxPowerDiff) > 0)
			{
				ofdmPowerLevel[0] = OfdmCellPwrIdx - pHalData->LegacyHTTxPowerDiff;
			}
			else
			{
				ofdmPowerLevel[0] = 0;
			}
		}

		RT_TRACE(COMP_TXAGC, DBG_LOUD, 
		("Altered CCK Tx power index : %d, Legacy OFDM Tx power index: %d, OFDM Tx power index: %d\n", 
		cckPowerLevel[0], ofdmPowerLevel[0] + pHalData->LegacyHTTxPowerDiff, ofdmPowerLevel[0]));
	}
#else
	// Add or not ???
#endif

	pHalData->CurrentCckTxPwrIdx = cckPowerLevel[0];
	pHalData->CurrentOfdm24GTxPwrIdx = ofdmPowerLevel[0];
	pHalData->CurrentBW2024GTxPwrIdx = BW20PowerLevel[0];
	pHalData->CurrentBW4024GTxPwrIdx = BW40PowerLevel[0];

	//DBG_871X("phy_PowerIndexCheck8812(): CurrentCckTxPwrIdx : 0x%x,CurrentOfdm24GTxPwrIdx: 0x%x, CurrentBW2024GTxPwrIdx: 0x%dx, CurrentBW4024GTxPwrIdx: 0x%x \n", 
	//	pHalData->CurrentCckTxPwrIdx, pHalData->CurrentOfdm24GTxPwrIdx, pHalData->CurrentBW2024GTxPwrIdx, pHalData->CurrentBW4024GTxPwrIdx);
}

u8
phy_GetCurrentTxNum_8192E(
	IN	PADAPTER		pAdapter
	)
{
	return RF_TX_NUM_NONIMPLEMENT;
}

VOID
PHY_SetTxPowerIndex_8192E(
	IN	PADAPTER			Adapter,
	IN	u32					PowerIndex,
	IN	u8					RFPath,	
	IN	u8					Rate
	)
{
    if (RFPath == ODM_RF_PATH_A)
    {
        switch (Rate)
        {
            case MGN_1M:    PHY_SetBBReg(Adapter, rTxAGC_A_CCK1_Mcs32,      bMaskByte1, PowerIndex); break;
            case MGN_2M:    PHY_SetBBReg(Adapter, rTxAGC_B_CCK11_A_CCK2_11, bMaskByte1, PowerIndex); break;
            case MGN_5_5M:  PHY_SetBBReg(Adapter, rTxAGC_B_CCK11_A_CCK2_11, bMaskByte2, PowerIndex); break;
            case MGN_11M:   PHY_SetBBReg(Adapter, rTxAGC_B_CCK11_A_CCK2_11, bMaskByte3, PowerIndex); break;

			case MGN_6M:    PHY_SetBBReg(Adapter, rTxAGC_A_Rate18_06, bMaskByte0, PowerIndex); break;
            case MGN_9M:    PHY_SetBBReg(Adapter, rTxAGC_A_Rate18_06, bMaskByte1, PowerIndex); break;
            case MGN_12M:   PHY_SetBBReg(Adapter, rTxAGC_A_Rate18_06, bMaskByte2, PowerIndex); break;
            case MGN_18M:   PHY_SetBBReg(Adapter, rTxAGC_A_Rate18_06, bMaskByte3, PowerIndex); break;

            case MGN_24M:   PHY_SetBBReg(Adapter, rTxAGC_A_Rate54_24, bMaskByte0, PowerIndex); break;
            case MGN_36M:   PHY_SetBBReg(Adapter, rTxAGC_A_Rate54_24, bMaskByte1, PowerIndex); break;
            case MGN_48M:   PHY_SetBBReg(Adapter, rTxAGC_A_Rate54_24, bMaskByte2, PowerIndex); break;
            case MGN_54M:   PHY_SetBBReg(Adapter, rTxAGC_A_Rate54_24, bMaskByte3, PowerIndex); break;

            case MGN_MCS0:  PHY_SetBBReg(Adapter, rTxAGC_A_Mcs03_Mcs00, bMaskByte0, PowerIndex); break;
            case MGN_MCS1:  PHY_SetBBReg(Adapter, rTxAGC_A_Mcs03_Mcs00, bMaskByte1, PowerIndex); break;
            case MGN_MCS2:  PHY_SetBBReg(Adapter, rTxAGC_A_Mcs03_Mcs00, bMaskByte2, PowerIndex); break;
            case MGN_MCS3:  PHY_SetBBReg(Adapter, rTxAGC_A_Mcs03_Mcs00, bMaskByte3, PowerIndex); break;

            case MGN_MCS4:  PHY_SetBBReg(Adapter, rTxAGC_A_Mcs07_Mcs04, bMaskByte0, PowerIndex); break;
            case MGN_MCS5:  PHY_SetBBReg(Adapter, rTxAGC_A_Mcs07_Mcs04, bMaskByte1, PowerIndex); break;
            case MGN_MCS6:  PHY_SetBBReg(Adapter, rTxAGC_A_Mcs07_Mcs04, bMaskByte2, PowerIndex); break;
            case MGN_MCS7:  PHY_SetBBReg(Adapter, rTxAGC_A_Mcs07_Mcs04, bMaskByte3, PowerIndex); break;

            case MGN_MCS8:  PHY_SetBBReg(Adapter, rTxAGC_A_Mcs11_Mcs08, bMaskByte0, PowerIndex); break;
            case MGN_MCS9:  PHY_SetBBReg(Adapter, rTxAGC_A_Mcs11_Mcs08, bMaskByte1, PowerIndex); break;
            case MGN_MCS10: PHY_SetBBReg(Adapter, rTxAGC_A_Mcs11_Mcs08, bMaskByte2, PowerIndex); break;
            case MGN_MCS11: PHY_SetBBReg(Adapter, rTxAGC_A_Mcs11_Mcs08, bMaskByte3, PowerIndex); break;

            case MGN_MCS12: PHY_SetBBReg(Adapter, rTxAGC_A_Mcs15_Mcs12, bMaskByte0, PowerIndex); break;
            case MGN_MCS13: PHY_SetBBReg(Adapter, rTxAGC_A_Mcs15_Mcs12, bMaskByte1, PowerIndex); break;
            case MGN_MCS14: PHY_SetBBReg(Adapter, rTxAGC_A_Mcs15_Mcs12, bMaskByte2, PowerIndex); break;
            case MGN_MCS15: PHY_SetBBReg(Adapter, rTxAGC_A_Mcs15_Mcs12, bMaskByte3, PowerIndex); break;

            default:
                 DBG_871X("Invalid Rate!!\n");
                 break;				
        }
    }
    else if (RFPath == ODM_RF_PATH_B)
    {
        switch (Rate)
        {
            case MGN_1M:    PHY_SetBBReg(Adapter, rTxAGC_B_CCK11_A_CCK2_11, bMaskByte0, PowerIndex); break;
            case MGN_2M:    PHY_SetBBReg(Adapter, rTxAGC_B_CCK1_55_Mcs32, bMaskByte3, PowerIndex); break;
            case MGN_5_5M:  PHY_SetBBReg(Adapter, rTxAGC_B_CCK1_55_Mcs32, bMaskByte2, PowerIndex); break;
            case MGN_11M:   PHY_SetBBReg(Adapter, rTxAGC_B_CCK1_55_Mcs32, bMaskByte1, PowerIndex); break;
                                                         
            case MGN_6M:    PHY_SetBBReg(Adapter, rTxAGC_B_Rate18_06, bMaskByte0, PowerIndex); break;
            case MGN_9M:    PHY_SetBBReg(Adapter, rTxAGC_B_Rate18_06, bMaskByte1, PowerIndex); break;
            case MGN_12M:   PHY_SetBBReg(Adapter, rTxAGC_B_Rate18_06, bMaskByte2, PowerIndex); break;
            case MGN_18M:   PHY_SetBBReg(Adapter, rTxAGC_B_Rate18_06, bMaskByte3, PowerIndex); break;
                                                         
            case MGN_24M:   PHY_SetBBReg(Adapter, rTxAGC_B_Rate54_24, bMaskByte0, PowerIndex); break;
            case MGN_36M:   PHY_SetBBReg(Adapter, rTxAGC_B_Rate54_24, bMaskByte1, PowerIndex); break;
            case MGN_48M:   PHY_SetBBReg(Adapter, rTxAGC_B_Rate54_24, bMaskByte2, PowerIndex); break;
            case MGN_54M:   PHY_SetBBReg(Adapter, rTxAGC_B_Rate54_24, bMaskByte3, PowerIndex); break;
                                                         
            case MGN_MCS0:  PHY_SetBBReg(Adapter, rTxAGC_B_Mcs03_Mcs00, bMaskByte0, PowerIndex); break;
            case MGN_MCS1:  PHY_SetBBReg(Adapter, rTxAGC_B_Mcs03_Mcs00, bMaskByte1, PowerIndex); break;
            case MGN_MCS2:  PHY_SetBBReg(Adapter, rTxAGC_B_Mcs03_Mcs00, bMaskByte2, PowerIndex); break;
            case MGN_MCS3:  PHY_SetBBReg(Adapter, rTxAGC_B_Mcs03_Mcs00, bMaskByte3, PowerIndex); break;
                                                         
            case MGN_MCS4:  PHY_SetBBReg(Adapter, rTxAGC_B_Mcs07_Mcs04, bMaskByte0, PowerIndex); break;
            case MGN_MCS5:  PHY_SetBBReg(Adapter, rTxAGC_B_Mcs07_Mcs04, bMaskByte1, PowerIndex); break;
            case MGN_MCS6:  PHY_SetBBReg(Adapter, rTxAGC_B_Mcs07_Mcs04, bMaskByte2, PowerIndex); break;
            case MGN_MCS7:  PHY_SetBBReg(Adapter, rTxAGC_B_Mcs07_Mcs04, bMaskByte3, PowerIndex); break;
                                                         
            case MGN_MCS8:  PHY_SetBBReg(Adapter, rTxAGC_B_Mcs11_Mcs08, bMaskByte0, PowerIndex); break;
            case MGN_MCS9:  PHY_SetBBReg(Adapter, rTxAGC_B_Mcs11_Mcs08, bMaskByte1, PowerIndex); break;
            case MGN_MCS10: PHY_SetBBReg(Adapter, rTxAGC_B_Mcs11_Mcs08, bMaskByte2, PowerIndex); break;
            case MGN_MCS11: PHY_SetBBReg(Adapter, rTxAGC_B_Mcs11_Mcs08, bMaskByte3, PowerIndex); break;
                                                         
            case MGN_MCS12: PHY_SetBBReg(Adapter, rTxAGC_B_Mcs15_Mcs12, bMaskByte0, PowerIndex); break;
            case MGN_MCS13: PHY_SetBBReg(Adapter, rTxAGC_B_Mcs15_Mcs12, bMaskByte1, PowerIndex); break;
            case MGN_MCS14: PHY_SetBBReg(Adapter, rTxAGC_B_Mcs15_Mcs12, bMaskByte2, PowerIndex); break;
            case MGN_MCS15: PHY_SetBBReg(Adapter, rTxAGC_B_Mcs15_Mcs12, bMaskByte3, PowerIndex); break;

            default:
                 DBG_871X("Invalid Rate!!\n");
                 break;			
        }
    }
    else
    {
		DBG_871X("Invalid RFPath!!\n");
    }
    
}

VOID
PHY_ConvertTxPowerByRateInDbmToRelativeValues_8192E(
	IN	PADAPTER	pAdapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA( pAdapter );
	u8 			base = 0, rfPath = 0;

	//DBG_871X("===>PHY_ConvertTxPowerByRateInDbmToRelativeValues_8192E()\n" );

	for ( rfPath = ODM_RF_PATH_A; rfPath <= ODM_RF_PATH_B; ++rfPath )
	{
		if ( rfPath == ODM_RF_PATH_A )
		{
			base = PHY_GetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, rfPath, RF_1TX, CCK );
			//DBG_871X( "base of 2.4G CCK 1TX: %d\n", base );
			PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
				&( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][rfPath][RF_1TX][2] ),
				1, 1, base );
			PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
				&( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][rfPath][RF_1TX][3] ),
				1, 3, base );
		}
		else if ( rfPath == ODM_RF_PATH_B )
		{
			base = PHY_GetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, rfPath, RF_1TX, CCK );
			//DBG_871X("base of 2.4G CCK 1TX: %d\n", base );
			PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
				&( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][rfPath][RF_1TX][2] ),
				2, 2, base );
			PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
				&( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][rfPath][RF_1TX][3] ),
				1, 3, base );
		}

		base = PHY_GetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, rfPath, RF_1TX, OFDM );
		//DBG_871X("base of 2.4G OFDM 1TX: %d\n", base );
		PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][rfPath][RF_1TX][0] ),
			0, 3, base );
		PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][rfPath][RF_1TX][1] ),
			0, 3, base );
		
		base = PHY_GetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, rfPath, RF_1TX, HT_MCS0_MCS7 );
		//DBG_871X("base of 2.4G HTMCS0-7 1TX: %d\n", base );
		PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][rfPath][RF_1TX][4] ),
			0, 3, base );
		PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][rfPath][RF_1TX][5] ),
			0, 3, base );

		base = PHY_GetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, rfPath, RF_2TX, HT_MCS8_MCS15 );
		//DBG_871X("base of 2.4G HTMCS8-15 2TX: %d\n", base );
		PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][rfPath][RF_2TX][6] ),
			0, 3, base );
		PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][rfPath][RF_2TX][7] ),
			0, 3, base );
	}

	//DBG_871X("<===PHY_ConvertTxPowerByRateInDbmToRelativeValues_8192E()\n" );
}

VOID
PHY_StoreTxPowerByRateBase_8192E(	
	IN	PADAPTER	pAdapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA( pAdapter );
	u16		rawValue = 0;
	u8		base = 0, path = 0, rate_section;

	for ( path = ODM_RF_PATH_A; path <= ODM_RF_PATH_B; ++path )
	{
		if ( path == ODM_RF_PATH_A )
		{
			rawValue = ( u16 ) ( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][path][RF_1TX][3] >> 24 ) & 0xFF;
			base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
			PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, path, CCK, RF_1TX, base );
		}
		else if( path == ODM_RF_PATH_B )
		{
			rawValue = ( u16 ) ( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][path][RF_1TX][3] >> 0 ) & 0xFF;
			base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
			PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, path, CCK, RF_1TX, base );
		}

		rawValue = ( u16 ) ( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][path][RF_1TX][1] >> 24 ) & 0xFF; 
		base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
		PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, path, OFDM, RF_1TX, base );

		rawValue = ( u16 ) ( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][path][RF_1TX][5] >> 24 ) & 0xFF; 
		base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
		PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, path, HT_MCS0_MCS7, RF_1TX, base );

		rawValue = ( u16 ) ( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][path][RF_2TX][7] >> 24 ) & 0xFF; 
		base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
		PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, path, HT_MCS8_MCS15, RF_2TX, base );
	}
}

u8
PHY_GetRateSectionInTxPowerByRate_8192E(
	IN	PADAPTER	pAdapter,
	IN	u8			Rf_Path,
	IN	u32			Rate
	)
{
	u8 rate_section = 0;
	
	switch	(Rate)
	{
		case	MGN_1M:
			rate_section = 2;
			break;
			
		case	MGN_2M:
			if ( Rf_Path == ODM_RF_PATH_A )
				rate_section = 3;
			else if ( Rf_Path == ODM_RF_PATH_B )
				rate_section = 2;
			break;
			
		case	MGN_5_5M:	
			if ( Rf_Path == ODM_RF_PATH_A )
				rate_section = 3;
			else if ( Rf_Path == ODM_RF_PATH_B )
				rate_section = 2;
			break;
			
		case	MGN_11M:
			rate_section = 3;
			break;

		case	MGN_6M:		
		case	MGN_9M:		
		case	MGN_12M:	
		case	MGN_18M:	
			rate_section = 0;
			break;

		case	MGN_24M:	
		case	MGN_36M:    
		case	MGN_48M:    
		case	MGN_54M:  
			rate_section = 1;
			break;
		
		case	MGN_MCS0:	
		case	MGN_MCS1:   
		case	MGN_MCS2:   
		case	MGN_MCS3: 
			rate_section = 4;
			break;

		case	MGN_MCS4:	
		case	MGN_MCS5:   
		case	MGN_MCS6:   
		case	MGN_MCS7:  
			rate_section = 5;
			break;

		case	MGN_MCS8:	
		case	MGN_MCS9:   
		case	MGN_MCS10:  
		case	MGN_MCS11:  
			rate_section = 6;
			break;

		case	MGN_MCS12:	
		case	MGN_MCS13:  
		case	MGN_MCS14:  
		case	MGN_MCS15:  
			rate_section = 7;
			break;
			
		default:
			DBG_871X("rate_section is Illegal\n");
			break;
	}

	return rate_section;
}

s32 
phy_GetTxPowerByRate_8192E(
	IN	PADAPTER		pAdapter,
	IN	u8				Band,
	IN	u8				Rf_Path,
	IN	u8				Rate
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);
	struct registry_priv	*pregistrypriv = &pAdapter->registrypriv;
	u8				shift = 0,
					rate_section = PHY_GetRateSectionInTxPowerByRate_8192E( pAdapter, Rf_Path, Rate ),
					tx_num = phy_GetCurrentTxNum_8192E( pAdapter );
	s32				tx_pwr_diff = 0;

	if ( tx_num == RF_TX_NUM_NONIMPLEMENT )
	{
		if ( Rate >= MGN_MCS8 && Rate <= MGN_MCS15 )
			tx_num = RF_2TX;
		else 
			tx_num = RF_1TX;
	}
	
	switch	(Rate)
	{
		case	MGN_1M:		shift = 0;		break;
		case	MGN_2M:		shift = 8;		break;
		case	MGN_5_5M:	shift = 16;		break;
		case	MGN_11M:	shift = 24;		break;			

		case	MGN_6M:		shift = 0;		break;
		case	MGN_9M:		shift = 8;      break;
		case	MGN_12M:	shift = 16;     break;
		case	MGN_18M:	shift = 24;     break;
			
		case	MGN_24M:	shift = 0;    	break;
		case	MGN_36M:    shift = 8;      break;
		case	MGN_48M:    shift = 16;     break;
		case	MGN_54M:    shift = 24;     break;
			
		case	MGN_MCS0:	shift = 0; 		break;
		case	MGN_MCS1:   shift = 8;      break;
		case	MGN_MCS2:   shift = 16;     break;
		case	MGN_MCS3:   shift = 24;     break;
			
		case	MGN_MCS4:	shift = 0; 		break;
		case	MGN_MCS5:   shift = 8;      break;
		case	MGN_MCS6:   shift = 16;     break;
		case	MGN_MCS7:   shift = 24;     break;
			
		case	MGN_MCS8:	shift = 0; 		break;
		case	MGN_MCS9:   shift = 8;      break;
		case	MGN_MCS10:  shift = 16;     break;
		case	MGN_MCS11:  shift = 24;     break;
			
		case	MGN_MCS12:	shift = 0; 		break;
		case	MGN_MCS13:  shift = 8;      break;
		case	MGN_MCS14:  shift = 16;     break;
		case	MGN_MCS15:  shift = 24;     break;
			
		default:
			DBG_871X("Rate_Section is Illegal\n");
			break;
	}

	tx_pwr_diff = (pHalData->TxPwrByRateOffset[Band][Rf_Path][tx_num][rate_section] >> shift) & 0xff;

	//DBG_871X("TxPowerByRate-BAND(%d)-RF(%d)-TxNum(%d)-RAS(%d)-Rate(0x%x)=%x tx_pwr_diff=%d shift=%d\n", 
	//Band, Rf_Path, tx_num, rate_section, Rate, pHalData->TxPwrByRateOffset[Band][Rf_Path][tx_num][rate_section], tx_pwr_diff, shift);

	if ( ( pregistrypriv->RegEnableTxPowerLimit == 1 && pHalData->EEPROMRegulatory != 2 ) || 
		   pHalData->EEPROMRegulatory == 1 ) 
	{
		s8 limit = PHY_GetTxPowerLimit(pAdapter,pregistrypriv->RegPwrTblSel, Band, pHalData->CurrentChannelBW, Rf_Path, Rate, pHalData->CurrentChannel);

		if ( limit < 0 )
			tx_pwr_diff = limit;
		else
			tx_pwr_diff = tx_pwr_diff > limit ? limit : tx_pwr_diff;
		
		//DBG_871X("Maximum power by rate %d, final power by rate %d\n", limit, tx_pwr_diff );
	}
	
	return	tx_pwr_diff;
}

u32
PHY_GetTxPowerIndex_8192E(
	IN	PADAPTER			pAdapter,
	IN	u8					RFPath,
	IN	u8					Rate,	
	IN	CHANNEL_WIDTH		BandWidth,	
	IN	u8					Channel
	)
{
	PHAL_DATA_TYPE		pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T			pDM_Odm = &pHalData->odmpriv;
	u8				i = 0;	//default set to 1S
	struct registry_priv	*pregistrypriv = &pAdapter->registrypriv;
	s32				powerDiffByRate = 0;
	u32				txPower = 0;
	u8				chnlIdx = (Channel-1);

	//DBG_871X("===> PHY_GetTxPowerIndex_8192E\n");
	
	if (HAL_IsLegalChannel(pAdapter, Channel) == _FALSE)
	{
		chnlIdx = 0;
		DBG_871X("Illegal channel!!\n");
	}

	if ( IS_CCK_RATE(Rate) )
	{
		txPower = pHalData->Index24G_CCK_Base[RFPath][chnlIdx];	
	}
	else if ( MGN_6M <= Rate )
	{				
		txPower = pHalData->Index24G_BW40_Base[RFPath][chnlIdx];
	}
	else
	{
		//DBG_871X("===> PHY_GetTxPowerIndex_8192E: INVALID Rate.\n");
	}

	//DBG_871X("Base Tx power(RF-%c, Rate #%d, Channel Index %d) = 0x%X\n", ((RFPath==0)?'A':'B'), Rate, chnlIdx, txPower);
	
	// OFDM-1T
	if ( MGN_6M <= Rate && Rate <= MGN_54M && ! IS_CCK_RATE(Rate) )
	{
		txPower += pHalData->OFDM_24G_Diff[RFPath][TX_1S];
		//DBG_871X("+PowerDiff 2.4G (RF-%c): (OFDM-1T) = (%d)\n", ((RFPath==0)?'A':'B'), pHalData->OFDM_24G_Diff[RFPath][TX_1S]);
	}
	// BW20-1S, BW20-2S
	if (BandWidth == CHANNEL_WIDTH_20)
	{
		if ( (MGN_MCS0 <= Rate && Rate <= MGN_MCS15) || (MGN_VHT2SS_MCS0 <= Rate && Rate <= MGN_VHT2SS_MCS9))
			txPower += pHalData->BW20_24G_Diff[RFPath][TX_1S];
		if ( (MGN_MCS8 <= Rate && Rate <= MGN_MCS15) || (MGN_VHT2SS_MCS0 <= Rate && Rate <= MGN_VHT2SS_MCS9))
			txPower += pHalData->BW20_24G_Diff[RFPath][TX_2S];

		//DBG_871X("+PowerDiff 2.4G (RF-%c): (BW20-1S, BW20-2S) = (%d, %d)\n", ((RFPath==0)?'A':'B'), 
		//	pHalData->BW20_24G_Diff[RFPath][TX_1S], pHalData->BW20_24G_Diff[RFPath][TX_2S]);
	}
	// BW40-1S, BW40-2S
	else if (BandWidth == CHANNEL_WIDTH_40)
	{
		if ( (MGN_MCS0 <= Rate && Rate <= MGN_MCS15) || (MGN_VHT1SS_MCS0 <= Rate && Rate <= MGN_VHT2SS_MCS9))
			txPower += pHalData->BW40_24G_Diff[RFPath][TX_1S];
		if ( (MGN_MCS8 <= Rate && Rate <= MGN_MCS15) || (MGN_VHT2SS_MCS0 <= Rate && Rate <= MGN_VHT2SS_MCS9))
			txPower += pHalData->BW40_24G_Diff[RFPath][TX_2S];

		//DBG_871X("+PowerDiff 2.4G (RF-%c): (BW40-1S, BW40-2S) = (%d, %d)\n", ((RFPath==0)?'A':'B'), 
		//	pHalData->BW40_24G_Diff[RFPath][TX_1S], pHalData->BW40_24G_Diff[RFPath][TX_2S]);
	}
	// Willis suggest adopt BW 40M power index while in BW 80 mode
	else if ( BandWidth == CHANNEL_WIDTH_80 )
	{
		if ( (MGN_MCS0 <= Rate && Rate <= MGN_MCS15) || (MGN_VHT1SS_MCS0 <= Rate && Rate <= MGN_VHT2SS_MCS9))
			txPower += pHalData->BW40_24G_Diff[RFPath][TX_1S];
		if ( (MGN_MCS8 <= Rate && Rate <= MGN_MCS15) || (MGN_VHT2SS_MCS0 <= Rate && Rate <= MGN_VHT2SS_MCS9))
			txPower += pHalData->BW40_24G_Diff[RFPath][TX_2S];

		//DBG_871X("+PowerDiff 2.4G (RF-%c): (BW40-1S, BW40-2S) = (%d, %d) P.S. Current is in BW 80MHz\n", ((RFPath==0)?'A':'B'), 
		//	pHalData->BW40_24G_Diff[RFPath][TX_1S], pHalData->BW40_24G_Diff[RFPath][TX_2S]);
	}
		
	if ( pHalData->EEPROMRegulatory != 2 )
	{
		powerDiffByRate = phy_GetTxPowerByRate_8192E( pAdapter, BAND_ON_2_4G, RFPath, Rate );
	}

	txPower += powerDiffByRate;

	if(pDM_Odm->Modify_TxAGC_Flag_PathA || pDM_Odm->Modify_TxAGC_Flag_PathB)    //20130424 Mimic whether path A or B has to modify TxAGC
	{
		//DBG_871X("Before add Remanant_OFDMSwingIdx[rfpath %u] %d", txPower);
		txPower += pDM_Odm->Remnant_OFDMSwingIdx[RFPath]; 
		//DBG_871X("After add Remanant_OFDMSwingIdx[rfpath %u] %d => txPower %d", RFPath, pDM_Odm->Remnant_OFDMSwingIdx[RFPath], txPower);
	}
	
	if(txPower > MAX_POWER_INDEX)
		txPower = MAX_POWER_INDEX;

	return txPower;	
}

VOID
PHY_GetTxPowerIndexByRateArray_8192E(
	IN	PADAPTER			pAdapter,
	IN	u8					RFPath,
	IN	CHANNEL_WIDTH		BandWidth,	
	IN	u8					Channel,
	IN	u8*					Rate,
	OUT	u8*					PowerIndex,
	IN	u8					ArraySize
	)
{	
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(pAdapter);
	u8 i;

	for(i=0 ; i<ArraySize; i++)
	{
		PowerIndex[i] = (u8)PHY_GetTxPowerIndex_8192E(pAdapter, RFPath, Rate[i], BandWidth, Channel);
	}
	
}

VOID 
phy_SetTxPowerIndexByRateArray_8192E(
	IN	PADAPTER			pAdapter,
	IN	u8					RFPath,
	IN	CHANNEL_WIDTH		BandWidth,	
	IN	u8					Channel,
	IN	u8*					Rates,
	IN	u8					RateArraySize
	)
{
	u32			powerIndex = 0;
	int			i = 0;

	for (i = 0; i < RateArraySize; ++i) 
	{
		powerIndex = PHY_GetTxPowerIndex_8192E(pAdapter, RFPath, Rates[i], BandWidth, Channel);
		PHY_SetTxPowerIndex_8192E(pAdapter, powerIndex, RFPath, Rates[i]);
	}

}

VOID
PHY_SetTxPowerLevelByPath8192E(
	IN	PADAPTER		Adapter,
	IN	u8				channel,
	IN	u8				path
	)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	u8			cckRates[]   = {MGN_1M, MGN_2M, MGN_5_5M, MGN_11M};
	u8			ofdmRates[]  = {MGN_6M, MGN_9M, MGN_12M, MGN_18M, MGN_24M, MGN_36M, MGN_48M, MGN_54M};
	u8			htRates1T[]  = {MGN_MCS0, MGN_MCS1, MGN_MCS2, MGN_MCS3, MGN_MCS4, MGN_MCS5, MGN_MCS6, MGN_MCS7};
	u8			htRates2T[]  = {MGN_MCS8, MGN_MCS9, MGN_MCS10, MGN_MCS11, MGN_MCS12, MGN_MCS13, MGN_MCS14, MGN_MCS15};

		
	//DBG_871X("==>PHY_SetTxPowerLevelByPath8192E()\n");
#if(MP_DRIVER == 1)
	if (Adapter->registrypriv.mp_mode == 1)
		return;
#endif

	//if(pMgntInfo->bDisableTXPowerByRate)
	//	return;	

	phy_SetTxPowerIndexByRateArray_8192E(Adapter, path, pHalData->CurrentChannelBW, channel,
								  cckRates, sizeof(cckRates)/sizeof(u1Byte));
		
	phy_SetTxPowerIndexByRateArray_8192E(Adapter, path, pHalData->CurrentChannelBW, channel,
								  ofdmRates, sizeof(ofdmRates)/sizeof(u1Byte));
	
	phy_SetTxPowerIndexByRateArray_8192E(Adapter, path, pHalData->CurrentChannelBW, channel,
								  htRates1T, sizeof(htRates1T)/sizeof(u1Byte));

	if(pHalData->NumTotalRFPath >= 2)
	{
		phy_SetTxPowerIndexByRateArray_8192E(Adapter, path, pHalData->CurrentChannelBW, channel,
							  		htRates2T, sizeof(htRates2T)/sizeof(u1Byte));
	}

	//DBG_871X("<==PHY_SetTxPowerLevelByPath8192E()\n");
}

VOID
PHY_SetTxPowerLevel8192E(
	IN	PADAPTER		Adapter,
	IN	u8				Channel
	)
{

	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	u8			path = 0;
		
	//DBG_871X("==>PHY_SetTxPowerLevel8192E()\n");

	for( path = ODM_RF_PATH_A; path < pHalData->NumTotalRFPath; ++path )
	{
		PHY_SetTxPowerLevelByPath8192E(Adapter, Channel, path);
	}
	
	//DBG_871X("<==PHY_SetTxPowerLevel8192E()\n");
}

u8 
phy_GetSecondaryChnl_8192E(
	IN	PADAPTER	Adapter
)
{
	u8					SCSettingOf40 = 0, SCSettingOf20 = 0;
	PHAL_DATA_TYPE		pHalData = GET_HAL_DATA(Adapter);

	//DBG_871X("SCMapping: VHT Case: pHalData->CurrentChannelBW %d, pHalData->nCur80MhzPrimeSC %d, pHalData->nCur40MhzPrimeSC %d \n",pHalData->CurrentChannelBW,pHalData->nCur80MhzPrimeSC,pHalData->nCur40MhzPrimeSC);
	if(pHalData->CurrentChannelBW== CHANNEL_WIDTH_80)
	{
		if(pHalData->nCur80MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_LOWER)
			SCSettingOf40 = VHT_DATA_SC_40_LOWER_OF_80MHZ;
		else if(pHalData->nCur80MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_UPPER)
			SCSettingOf40 = VHT_DATA_SC_40_UPPER_OF_80MHZ;
		else
			DBG_871X("SCMapping: Not Correct Primary40MHz Setting \n");
		
		if((pHalData->nCur40MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_LOWER) && (pHalData->nCur80MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_LOWER))
			SCSettingOf20 = VHT_DATA_SC_20_LOWEST_OF_80MHZ;
		else if((pHalData->nCur40MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_UPPER) && (pHalData->nCur80MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_LOWER))
			SCSettingOf20 = VHT_DATA_SC_20_LOWER_OF_80MHZ;
		else if((pHalData->nCur40MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_LOWER) && (pHalData->nCur80MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_UPPER))
			SCSettingOf20 = VHT_DATA_SC_20_UPPER_OF_80MHZ;
		else if((pHalData->nCur40MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_UPPER) && (pHalData->nCur80MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_UPPER))
			SCSettingOf20 = VHT_DATA_SC_20_UPPERST_OF_80MHZ;
		else
			DBG_871X("SCMapping: Not Correct Primary40MHz Setting \n");
	}
	else if(pHalData->CurrentChannelBW == CHANNEL_WIDTH_40)
	{
		//DBG_871X("SCMapping: VHT Case: pHalData->CurrentChannelBW %d, pHalData->nCur40MhzPrimeSC %d \n",pHalData->CurrentChannelBW,pHalData->nCur40MhzPrimeSC);

		if(pHalData->nCur40MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_UPPER)
			SCSettingOf20 = VHT_DATA_SC_20_UPPER_OF_80MHZ;
		else if(pHalData->nCur40MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_LOWER)
			SCSettingOf20 = VHT_DATA_SC_20_LOWER_OF_80MHZ;
		else
			DBG_871X("SCMapping: Not Correct Primary40MHz Setting \n");
	}

	//DBG_871X("SCMapping: SC Value %x \n", ( (SCSettingOf40 << 4) | SCSettingOf20));
	return  ( (SCSettingOf40 << 4) | SCSettingOf20);
}

VOID
phy_SetRegBW_8192E(
	IN	PADAPTER		Adapter,
	CHANNEL_WIDTH 	CurrentBW
)	
{
	u16	RegRfMod_BW, u2tmp = 0;
	RegRfMod_BW = rtw_read16(Adapter, REG_WMAC_TRXPTCL_CTL);

	switch(CurrentBW)
	{
		case CHANNEL_WIDTH_20:
			rtw_write16(Adapter, REG_WMAC_TRXPTCL_CTL, (RegRfMod_BW & 0xFE7F)); // BIT 7 = 0, BIT 8 = 0
			break;

		case CHANNEL_WIDTH_40:
			u2tmp = RegRfMod_BW | BIT7;
			rtw_write16(Adapter, REG_WMAC_TRXPTCL_CTL, (u2tmp & 0xFEFF)); // BIT 7 = 1, BIT 8 = 0
			break;

		case CHANNEL_WIDTH_80:
			u2tmp = RegRfMod_BW | BIT8;
			rtw_write16(Adapter, REG_WMAC_TRXPTCL_CTL, (u2tmp & 0xFF7F)); // BIT 7 = 0, BIT 8 = 1
			break;

		default:
			DBG_871X("phy_PostSetBWMode8812():	unknown Bandwidth: %#X\n",CurrentBW);
			break;
	}

}


VOID
phy_PostSetBwMode8192E(
	IN	PADAPTER	Adapter
)
{
	u1Byte			SubChnlNum = 0;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8				regBwOpMode;
	u8				regRRSR_RSC;

	regBwOpMode = rtw_read8(Adapter, REG_BWOPMODE);
	regRRSR_RSC = rtw_read8(Adapter, REG_RRSR+2);
	//regBwOpMode = rtw_hal_get_hwreg(Adapter,HW_VAR_BWMODE,(pu1Byte)&regBwOpMode);

	switch(pHalData->CurrentChannelBW)
	{
		case CHANNEL_WIDTH_20:
			regBwOpMode |= BW_OPMODE_20MHZ;
			   // 2007/02/07 Mark by Emily becasue we have not verify whether this register works
			rtw_write8(Adapter, REG_BWOPMODE, regBwOpMode);
			break;

		case CHANNEL_WIDTH_40:
			regBwOpMode &= ~BW_OPMODE_20MHZ;
				// 2007/02/07 Mark by Emily becasue we have not verify whether this register works
			rtw_write8(Adapter, REG_BWOPMODE, regBwOpMode);

			regRRSR_RSC = (regRRSR_RSC&0x90) |(pHalData->nCur40MhzPrimeSC<<5);
			rtw_write8(Adapter, REG_RRSR+2, regRRSR_RSC);
			break;

		default:
			/*RT_TRACE(COMP_DBG, DBG_LOUD, ("PHY_SetBWModeCallback8192C():
						unknown Bandwidth: %#X\n",pHalData->CurrentChannelBW));*/
			break;
	}
	

	switch(pHalData->CurrentChannelBW)
	{
		case CHANNEL_WIDTH_20:
			PHY_SetBBReg(Adapter, rFPGA0_RFMOD, BIT0, 0x0); 
			PHY_SetBBReg(Adapter, rFPGA1_RFMOD, BIT0, 0x0); 
			PHY_SetRFReg(Adapter, RF_PATH_A, RF_CHNLBW, BIT11|BIT10, 0x3);			
			PHY_SetRFReg(Adapter, RF_PATH_B, RF_CHNLBW, BIT11|BIT10, 0x3);	

			//PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter2, BIT10, 1);			
			PHY_SetBBReg(Adapter, rOFDM0_TxPseudoNoiseWgt, (BIT31|BIT30), 0x0);
			
			break;
			   
		case CHANNEL_WIDTH_40:
			PHY_SetBBReg(Adapter, rFPGA0_RFMOD, BIT0, 0x1); 
			PHY_SetBBReg(Adapter, rFPGA1_RFMOD, BIT0, 0x1); 
			PHY_SetRFReg(Adapter, RF_PATH_A, RF_CHNLBW, BIT11|BIT10, 0x1);						
			PHY_SetRFReg(Adapter, RF_PATH_B, RF_CHNLBW, BIT11|BIT10, 0x1);	

			// Set Control channel to upper or lower. These settings are required only for 40MHz
			PHY_SetBBReg(Adapter, rCCK0_System, bCCKSideBand, (pHalData->nCur40MhzPrimeSC>>1));
		
			PHY_SetBBReg(Adapter, rOFDM1_LSTF, 0xC00, pHalData->nCur40MhzPrimeSC);
			
//			PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter2, BIT10, 0);
			
			PHY_SetBBReg(Adapter, 0x818, (BIT26|BIT27), (pHalData->nCur40MhzPrimeSC==HAL_PRIME_CHNL_OFFSET_LOWER)?2:1);
			break;

		default:
			//RT_DISP(FPHY, PHY_BBW, ("phy_PostSetBWMode8192E():	unknown Bandwidth: %#X\n",pHalData->CurrentChannelBW));
			break;
	}
}

// <20130320, VincentLan> A workaround to eliminate the 2480MHz spur for 92E
void 
phy_SpurCalibration_8192E(
	IN	PADAPTER			Adapter,
	IN	SPUR_CAL_METHOD		Method
	)
{
	u32			reg0x18 = 0;
	u8 			retryNum = 0;
	u8			MaxRetryCount = 8;
	u8			Pass_A = _FALSE, Pass_B = _FALSE;
	u8			SpurOccur = _FALSE;
	u32			PSDReport = 0;
	u32			Best_PSD_PathA = 999;
	u32			Best_Phase_PathA = 0;


	if (Method == PLL_RESET){
		MaxRetryCount = 3;
		DBG_871X("%s =>PLL_RESET \n",__FUNCTION__);
	}
	else if (Method == AFE_PHASE_SEL){		
		rtw_write8(Adapter, RF_TX_G1,rtw_read8(Adapter, RF_TX_G1)|BIT4); // enable 0x20[4]
		DBG_871X("%s =>AFE_PHASE_SEL \n",__FUNCTION__);
	}

	
	// Backup current channel
	reg0x18 = PHY_QueryRFReg(Adapter, ODM_RF_PATH_A, RF_CHNLBW, bRFRegOffsetMask);

	
	while (retryNum++ < MaxRetryCount)
	{
		PHY_SetRFReg(Adapter, ODM_RF_PATH_A, RF_CHNLBW, bRFRegOffsetMask, 0x7C0D); //CH13
		PHY_SetBBReg(Adapter, rOFDM0_XAAGCCore1, bMaskByte0, 0x30); //Path A initial gain
		PHY_SetBBReg(Adapter, rOFDM0_XBAGCCore1, bMaskByte0, 0x30); //Path B initial gain
		PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter4, bMaskDWord, 0xccf000c0); 	// disable 3-wire

		// Path A
		PHY_SetBBReg(Adapter, rFPGA0_TxInfo, bMaskByte0, 0x3);
		PHY_SetBBReg(Adapter, rFPGA0_PSDFunction, bMaskDWord, 0xfccd);
		PHY_SetBBReg(Adapter, rFPGA0_PSDFunction, bMaskDWord, 0x40fccd);
		//rtw_msleep_os(30);
		rtw_mdelay_os(30);
		PSDReport = PHY_QueryBBReg(Adapter, rFPGA0_PSDReport, bMaskDWord);
		//DBG_871X(" Path A== PSDReport = 0x%x (%d)\n",PSDReport,PSDReport);
		if (PSDReport < 0x16)
			Pass_A = _TRUE;
		if (PSDReport < Best_PSD_PathA){
			Best_PSD_PathA = PSDReport;
			Best_Phase_PathA = rtw_read8(Adapter, RF_TX_G1)>>5;
		}
		
		// Path B
		PHY_SetBBReg(Adapter, rFPGA0_TxInfo, bMaskByte0, 0x13);
		PHY_SetBBReg(Adapter, rFPGA0_PSDFunction, bMaskDWord, 0xfccd);
		PHY_SetBBReg(Adapter, rFPGA0_PSDFunction, bMaskDWord, 0x40fccd);
		//rtw_msleep_os(30);
		rtw_mdelay_os(30);
		PSDReport = PHY_QueryBBReg(Adapter, rFPGA0_PSDReport, bMaskDWord);
		//DBG_871X(" Path B== PSDReport = 0x%x (%d)\n",PSDReport,PSDReport);
		if (PSDReport < 0x16)
			Pass_B = _TRUE;

		if (Pass_A && Pass_B)
		{
			DBG_871X("=== PathA=%d, PathB=%d\n", Pass_A, Pass_B);
			DBG_871X("===FixSpur Pass!\n");
			PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter4, bMaskDWord, 0xcc0000c0); 	// enable 3-wire
			PHY_SetBBReg(Adapter, rFPGA0_PSDFunction, bMaskDWord, 0xfc00);
			PHY_SetBBReg(Adapter, rOFDM0_XAAGCCore1, bMaskByte0, 0x20);
			PHY_SetBBReg(Adapter, rOFDM0_XBAGCCore1, bMaskByte0, 0x20);
			break;
		}
		else
		{
			Pass_A = _FALSE;
			Pass_B = _FALSE;
			if (Method == PLL_RESET)
			{
				//PHY_SetMacReg(Adapter, 0x28, bMaskByte1, 0x7);	// PLL gated 320M CLK disable
				//PHY_SetMacReg(Adapter, 0x28, bMaskByte1, 0x47);	// PLL gated 320M CLK enable
				rtw_write8(Adapter, 0x29, 0x7);	// PLL gated 320M CLK disable
				rtw_write8(Adapter, 0x29, 0x47);	// PLL gated 320M CLK enable
			}
			else if (Method == AFE_PHASE_SEL)
			{
				if (!SpurOccur)
				{
					SpurOccur = _TRUE;
					DBG_871X("===FixSpur NOT Pass!\n");
					//PHY_SetMacReg(Adapter, RF_TX_G1, BIT4, 0x1);
					//PHY_SetMacReg(Adapter, 0x28, bMaskByte0, 0x80);
					//PHY_SetMacReg(Adapter, 0x28, bMaskByte0, 0x83);
					rtw_write8(Adapter, RF_TX_G1,rtw_read8(Adapter, RF_TX_G1)|BIT4); // enable 0x20[4]
					rtw_write8(Adapter, 0x28, 0x80);
					rtw_write8(Adapter, 0x28, 0x83);	
					
				}
				//DBG_871X("===Round %d\n", retryNum+1);
				if (retryNum < 7)
					//PHY_SetMacReg(Adapter, RF_TX_G1, BIT5|BIT6|BIT7, 1+retryNum);
					rtw_write8(Adapter,RF_TX_G1,(rtw_read8(Adapter, RF_TX_G1)&0x1F)|((1+retryNum)<<5));
				else
					break;
			}
		}
	}
	
	if (Pass_A && Pass_B)
		;
	// 0x20 Selection Focus on Path A PSD Result
	else if (Method == AFE_PHASE_SEL){
		if (Best_Phase_PathA < 8)
			//PHY_SetMacReg(Adapter, RF_TX_G1, BIT5|BIT6|BIT7, Best_Phase_PathA);
			rtw_write8(Adapter,RF_TX_G1,(rtw_read8(Adapter, RF_TX_G1)&0x1F)|(Best_Phase_PathA<<5));
		else
			//PHY_SetMacReg(Adapter, RF_TX_G1, BIT5|BIT6|BIT7, 0);
			rtw_write8(Adapter,RF_TX_G1,(rtw_read8(Adapter, RF_TX_G1)&0x1F));
	}
	// Restore the settings
	PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter4, bMaskDWord, 0xcc0000c0); 	// enable 3-wire
	PHY_SetBBReg(Adapter, rFPGA0_PSDFunction, bMaskDWord, 0xfccd); // reset PSD
	PHY_SetBBReg(Adapter, rOFDM0_XAAGCCore1, bMaskByte0, 0x20);
	PHY_SetBBReg(Adapter, rOFDM0_XBAGCCore1, bMaskByte0, 0x20);
	
	PHY_SetRFReg(Adapter, ODM_RF_PATH_A, RF_CHNLBW, bRFRegOffsetMask, reg0x18); //restore chnl

}

VOID
phy_SwChnl8192E(	
	IN	PADAPTER	pAdapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	u8 				channelToSW = pHalData->CurrentChannel;

	if(pHalData->rf_chip == RF_PSEUDO_11N)
	{
		//RT_TRACE(COMP_MLME,DBG_LOUD,("phy_SwChnl8192E: return for PSEUDO \n"));
		return;
	}
	pHalData->RfRegChnlVal[0] = ((pHalData->RfRegChnlVal[0] & 0xfffff00) | channelToSW  );
	PHY_SetRFReg(pAdapter, RF_PATH_A, RF_CHNLBW, 0x3FF, pHalData->RfRegChnlVal[0] );
	PHY_SetRFReg(pAdapter, RF_PATH_B, RF_CHNLBW, 0x3FF, pHalData->RfRegChnlVal[0] );


#if 0 //to do	
	// <20130422, VincentLan> A workaround to eliminate the 2480MHz spur for 92E
	if (channelToSW == 13)
	{
		if (pMgntInfo->RegSpurCalMethod == 1)//if AFE == 40MHz,MAC REG_0X78
			phy_SpurCalibration_8192E(pAdapter, AFE_PHASE_SEL); 
		else
			phy_SpurCalibration_8192E(pAdapter, PLL_RESET);

	}
#endif
	
}

VOID
phy_SwChnlAndSetBwMode8192E(
	IN  PADAPTER		Adapter
)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);

	//DBG_871X("phy_SwChnlAndSetBwMode8192E(): bSwChnl %d, bSetChnlBW %d \n", pHalData->bSwChnl, pHalData->bSetChnlBW);

	if((Adapter->bDriverStopped) || (Adapter->bSurpriseRemoved))
	{
		return;
	}

	if(pHalData->bSwChnl)
	{
		phy_SwChnl8192E(Adapter);
		pHalData->bSwChnl = _FALSE;
	}	

	if(pHalData->bSetChnlBW)
	{
		phy_PostSetBwMode8192E(Adapter);
		pHalData->bSetChnlBW = _FALSE;
	}	

	PHY_SetTxPowerLevel8192E(Adapter, pHalData->CurrentChannel);

}

VOID
PHY_HandleSwChnlAndSetBW8192E(
	IN	PADAPTER			Adapter,
	IN	BOOLEAN				bSwitchChannel,
	IN	BOOLEAN				bSetBandWidth,
	IN	u8					ChannelNum,
	IN	CHANNEL_WIDTH	ChnlWidth,
	IN	EXTCHNL_OFFSET	ExtChnlOffsetOf40MHz,
	IN	EXTCHNL_OFFSET	ExtChnlOffsetOf80MHz,
	IN	u8					CenterFrequencyIndex1
)
{
	//static BOOLEAN		bInitialzed = _FALSE;
	PADAPTER  			pDefAdapter =  GetDefaultAdapter(Adapter);
	PHAL_DATA_TYPE		pHalData = GET_HAL_DATA(pDefAdapter);
	u8					tmpChannel = pHalData->CurrentChannel;
	CHANNEL_WIDTH 	tmpBW= pHalData->CurrentChannelBW;
	u8					tmpnCur40MhzPrimeSC = pHalData->nCur40MhzPrimeSC;
	u8					tmpnCur80MhzPrimeSC = pHalData->nCur80MhzPrimeSC;
	u8					tmpCenterFrequencyIndex1 =pHalData->CurrentCenterFrequencyIndex1;
	struct mlme_ext_priv	*pmlmeext = &Adapter->mlmeextpriv;

	//DBG_871X("=> PHY_HandleSwChnlAndSetBW8812: bSwitchChannel %d, bSetBandWidth %d \n",bSwitchChannel,bSetBandWidth);

	//check is swchnl or setbw
	if(!bSwitchChannel && !bSetBandWidth)
	{
		DBG_871X("PHY_HandleSwChnlAndSetBW8812:  not switch channel and not set bandwidth \n");
		return;
	}

	//skip change for channel or bandwidth is the same
	if(bSwitchChannel)
	{
		if(pHalData->CurrentChannel != ChannelNum)
		{
			if (HAL_IsLegalChannel(Adapter, ChannelNum))
				pHalData->bSwChnl = _TRUE;
		}
	}

	if(bSetBandWidth)
	{
		#if 0
		if(bInitialzed == _FALSE)
		{
			bInitialzed = _TRUE;
			pHalData->bSetChnlBW = _TRUE;
		}
		else if((pHalData->CurrentChannelBW != ChnlWidth) ||(pHalData->nCur40MhzPrimeSC != ExtChnlOffsetOf40MHz) || (pHalData->CurrentCenterFrequencyIndex1!= CenterFrequencyIndex1))
		{
			pHalData->bSetChnlBW = _TRUE;
		}
		#else
			pHalData->bSetChnlBW = _TRUE;
		#endif
	}

	if(!pHalData->bSetChnlBW && !pHalData->bSwChnl)
	{
		//DBG_871X("<= PHY_HandleSwChnlAndSetBW8812: bSwChnl %d, bSetChnlBW %d \n",pHalData->bSwChnl,pHalData->bSetChnlBW);
		return;
	}


	if(pHalData->bSwChnl)
	{
		pHalData->CurrentChannel=ChannelNum;
		pHalData->CurrentCenterFrequencyIndex1 = ChannelNum;
	}
	

	if(pHalData->bSetChnlBW)
	{
		pHalData->CurrentChannelBW = ChnlWidth;
#if 0
		if(ExtChnlOffsetOf40MHz==EXTCHNL_OFFSET_LOWER)
			pHalData->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_UPPER;
		else if(ExtChnlOffsetOf40MHz==EXTCHNL_OFFSET_UPPER)
			pHalData->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_LOWER;
		else
			pHalData->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_DONT_CARE;

		if(ExtChnlOffsetOf80MHz==EXTCHNL_OFFSET_LOWER)
			pHalData->nCur80MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_UPPER;
		else if(ExtChnlOffsetOf80MHz==EXTCHNL_OFFSET_UPPER)
			pHalData->nCur80MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_LOWER;
		else
			pHalData->nCur80MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_DONT_CARE;
#else
		pHalData->nCur40MhzPrimeSC = ExtChnlOffsetOf40MHz;
		pHalData->nCur80MhzPrimeSC = ExtChnlOffsetOf80MHz;
#endif

		pHalData->CurrentCenterFrequencyIndex1 = CenterFrequencyIndex1;		
	}

	//Switch workitem or set timer to do switch channel or setbandwidth operation
	if((!pDefAdapter->bDriverStopped) && (!pDefAdapter->bSurpriseRemoved))
	{
		phy_SwChnlAndSetBwMode8192E(Adapter);
	}
	else
	{
		if(pHalData->bSwChnl)
		{
			pHalData->CurrentChannel = tmpChannel;
			pHalData->CurrentCenterFrequencyIndex1 = tmpChannel;
		}	
		if(pHalData->bSetChnlBW)
		{
			pHalData->CurrentChannelBW = tmpBW;
			pHalData->nCur40MhzPrimeSC = tmpnCur40MhzPrimeSC;
			pHalData->nCur80MhzPrimeSC = tmpnCur80MhzPrimeSC;
			pHalData->CurrentCenterFrequencyIndex1 = tmpCenterFrequencyIndex1;
		}
	}

	//DBG_871X("Channel %d ChannelBW %d ",pHalData->CurrentChannel, pHalData->CurrentChannelBW);
	//DBG_871X("40MhzPrimeSC %d 80MhzPrimeSC %d ",pHalData->nCur40MhzPrimeSC, pHalData->nCur80MhzPrimeSC);
	//DBG_871X("CenterFrequencyIndex1 %d \n",pHalData->CurrentCenterFrequencyIndex1);

	//DBG_871X("<= PHY_HandleSwChnlAndSetBW8812: bSwChnl %d, bSetChnlBW %d \n",pHalData->bSwChnl,pHalData->bSetChnlBW);

}

VOID
PHY_SwChnl8192E(
	IN	PADAPTER	Adapter,
	IN	u8			channel
	)
{
	//DBG_871X("%s()===>\n",__FUNCTION__);

	PHY_HandleSwChnlAndSetBW8192E(Adapter, _TRUE, _FALSE, channel, 0, 0, 0, channel);

#if (MP_DRIVER == 1) 
	// <20120712, Kordan> IQK on each channel, asked by James.
	//PHY_IQCalibrate(Adapter, _FALSE);
#endif

	//DBG_871X("<==%s()\n",__FUNCTION__);
}
VOID
PHY_SetBWMode8192E(
	IN	PADAPTER			Adapter,
	IN	CHANNEL_WIDTH	Bandwidth,	// 20M or 40M
	IN	u8					Offset		// Upper, Lower, or Don't care
)
{
	PHAL_DATA_TYPE		pHalData = GET_HAL_DATA(Adapter);

	//DBG_871X("%s()===>\n",__FUNCTION__);

	PHY_HandleSwChnlAndSetBW8192E(Adapter, _FALSE, _TRUE, pHalData->CurrentChannel, Bandwidth, Offset, Offset, pHalData->CurrentChannel);

	//DBG_871X("<==%s()\n",__FUNCTION__);
}
VOID
PHY_SetSwChnlBWMode8192E(
	IN	PADAPTER			Adapter,
	IN	u8					channel,
	IN	CHANNEL_WIDTH	Bandwidth,
	IN	u8					Offset40,
	IN	u8					Offset80
)
{
	//DBG_871X("%s()===>\n",__FUNCTION__);

	PHY_HandleSwChnlAndSetBW8192E(Adapter, _TRUE, _TRUE, channel, Bandwidth, Offset40, Offset80, channel);

	//DBG_871X("<==%s()\n",__FUNCTION__);
}

#if 0
static VOID
_PHY_SetBWMode8192E(
	IN	PADAPTER	Adapter
)
{
//	PADAPTER			Adapter = (PADAPTER)pTimer->Adapter;
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	u8				regBwOpMode;
	u8				regRRSR_RSC;

	//return;

	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//u4Byte				NowL, NowH;
	//u8Byte				BeginTime, EndTime;

	/*RT_TRACE(COMP_SCAN, DBG_LOUD, ("==>PHY_SetBWModeCallback8192C()  Switch to %s bandwidth\n", \
					pHalData->CurrentChannelBW == CHANNEL_WIDTH_20?"20MHz":"40MHz"))*/

	if(pHalData->rf_chip == RF_PSEUDO_11N)
	{
		//pHalData->SetBWModeInProgress= _FALSE;
		return;
	}

	// There is no 40MHz mode in RF_8225.
	if(pHalData->rf_chip==RF_8225)
		return;

	if(Adapter->bDriverStopped)
		return;

	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//NowL = PlatformEFIORead4Byte(Adapter, TSFR);
	//NowH = PlatformEFIORead4Byte(Adapter, TSFR+4);
	//BeginTime = ((u8Byte)NowH << 32) + NowL;

	//3//
	//3//<1>Set MAC register
	//3//
	//Adapter->HalFunc.SetBWModeHandler();

	regBwOpMode = rtw_read8(Adapter, REG_BWOPMODE);
	regRRSR_RSC = rtw_read8(Adapter, REG_RRSR+2);
	//regBwOpMode = rtw_hal_get_hwreg(Adapter,HW_VAR_BWMODE,(pu1Byte)&regBwOpMode);

	switch(pHalData->CurrentChannelBW)
	{
		case CHANNEL_WIDTH_20:
			regBwOpMode |= BW_OPMODE_20MHZ;
			   // 2007/02/07 Mark by Emily becasue we have not verify whether this register works
			rtw_write8(Adapter, REG_BWOPMODE, regBwOpMode);
			break;

		case CHANNEL_WIDTH_40:
			regBwOpMode &= ~BW_OPMODE_20MHZ;
				// 2007/02/07 Mark by Emily becasue we have not verify whether this register works
			rtw_write8(Adapter, REG_BWOPMODE, regBwOpMode);

			regRRSR_RSC = (regRRSR_RSC&0x90) |(pHalData->nCur40MhzPrimeSC<<5);
			rtw_write8(Adapter, REG_RRSR+2, regRRSR_RSC);
			break;

		default:
			/*RT_TRACE(COMP_DBG, DBG_LOUD, ("PHY_SetBWModeCallback8192C():
						unknown Bandwidth: %#X\n",pHalData->CurrentChannelBW));*/
			break;
	}

	//3//
	//3//<2>Set PHY related register
	//3//
	switch(pHalData->CurrentChannelBW)
	{
		/* 20 MHz channel*/
		case CHANNEL_WIDTH_20:
			PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bRFMOD, 0x0);
			PHY_SetBBReg(Adapter, rFPGA1_RFMOD, bRFMOD, 0x0);
			//PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter2, BIT10, 1);
			PHY_SetBBReg(Adapter, rOFDM0_TxPseudoNoiseWgt, (BIT31|BIT30), 0x0);
			break;


		/* 40 MHz channel*/
		case CHANNEL_WIDTH_40:
			PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bRFMOD, 0x1);
			PHY_SetBBReg(Adapter, rFPGA1_RFMOD, bRFMOD, 0x1);

			// Set Control channel to upper or lower. These settings are required only for 40MHz
			PHY_SetBBReg(Adapter, rCCK0_System, bCCKSideBand, (pHalData->nCur40MhzPrimeSC>>1));
			PHY_SetBBReg(Adapter, rOFDM1_LSTF, 0xC00, pHalData->nCur40MhzPrimeSC);
			//PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter2, BIT10, 0);

			PHY_SetBBReg(Adapter, 0x818, (BIT26|BIT27), (pHalData->nCur40MhzPrimeSC==HAL_PRIME_CHNL_OFFSET_LOWER)?2:1);

			break;



		default:
			/*RT_TRACE(COMP_DBG, DBG_LOUD, ("PHY_SetBWModeCallback8192C(): unknown Bandwidth: %#X\n"\
						,pHalData->CurrentChannelBW));*/
			break;

	}
	//Skip over setting of J-mode in BB register here. Default value is "None J mode". Emily 20070315

	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//NowL = PlatformEFIORead4Byte(Adapter, TSFR);
	//NowH = PlatformEFIORead4Byte(Adapter, TSFR+4);
	//EndTime = ((u8Byte)NowH << 32) + NowL;
	//RT_TRACE(COMP_SCAN, DBG_LOUD, ("SetBWModeCallback8190Pci: time of SetBWMode = %I64d us!\n", (EndTime - BeginTime)));

	//3<3>Set RF related register
	switch(pHalData->rf_chip)
	{
		case RF_PSEUDO_11N:
			// Do Nothing
			break;

		case RF_6052:
			PHY_RF6052SetBandwidth8192E(Adapter, pHalData->CurrentChannelBW);
			break;

		default:
			//RT_ASSERT(FALSE, ("Unknown RFChipID: %d\n", pHalData->rfchip));
			break;
	}

	//pHalData->SetBWModeInProgress= FALSE;

	//RT_TRACE(COMP_SCAN, DBG_LOUD, ("<==PHY_SetBWModeCallback8192C() \n" ));
}

VOID
PHY_SetBWMode8192E(
	IN	PADAPTER			Adapter,
	IN	CHANNEL_WIDTH	Bandwidth,	// 20M or 40M
	IN	u8					Offset		// Upper, Lower, or Don't care
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	CHANNEL_WIDTH 	tmpBW= pHalData->CurrentChannelBW;
	// Modified it for 20/40 mhz switch by guangan 070531
	//PMGNT_INFO	pMgntInfo=&Adapter->MgntInfo;

	//return;

	//if(pHalData->SwChnlInProgress)
//	if(pMgntInfo->bScanInProgress)
//	{
//		RT_TRACE(COMP_SCAN, DBG_LOUD, ("PHY_SetBWMode8192C() %s Exit because bScanInProgress!\n",
//					Bandwidth == CHANNEL_WIDTH_20?"20MHz":"40MHz"));
//		return;
//	}

//	if(pHalData->SetBWModeInProgress)
//	{
//		// Modified it for 20/40 mhz switch by guangan 070531
//		RT_TRACE(COMP_SCAN, DBG_LOUD, ("PHY_SetBWMode8192C() %s cancel last timer because SetBWModeInProgress!\n",
//					Bandwidth == CHANNEL_WIDTH_20?"20MHz":"40MHz"));
//		PlatformCancelTimer(Adapter, &pHalData->SetBWModeTimer);
//		//return;
//	}

	//if(pHalData->SetBWModeInProgress)
	//	return;

	//pHalData->SetBWModeInProgress= TRUE;

	pHalData->CurrentChannelBW = Bandwidth;

#if 0
	if(Offset==EXTCHNL_OFFSET_LOWER)
		pHalData->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_UPPER;
	else if(Offset==EXTCHNL_OFFSET_UPPER)
		pHalData->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_LOWER;
	else
		pHalData->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_DONT_CARE;
#else
	pHalData->nCur40MhzPrimeSC = Offset;
#endif

	if((!Adapter->bDriverStopped) && (!Adapter->bSurpriseRemoved))
	{
	#if 0
		//PlatformSetTimer(Adapter, &(pHalData->SetBWModeTimer), 0);
	#else
		_PHY_SetBWMode8192E(Adapter);
	#endif
	}
	else
	{
		//RT_TRACE(COMP_SCAN, DBG_LOUD, ("PHY_SetBWMode8192C() SetBWModeInProgress FALSE driver sleep or unload\n"));
		//pHalData->SetBWModeInProgress= FALSE;
		pHalData->CurrentChannelBW = tmpBW;
	}
	
}

#endif
