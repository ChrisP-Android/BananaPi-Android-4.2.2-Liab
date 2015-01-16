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
#define _HAL_COM_PHYCFG_C_

#include <drv_types.h>
#include <hal_data.h>


BOOLEAN 
eqNByte(
	u8*	str1,
	u8*	str2,
	u32	num
	)
{
	if(num==0)
		return _FALSE;
	while(num>0)
	{
		num--;
		if(str1[num]!=str2[num])
			return _FALSE;
	}
	return _TRUE;
}

BOOLEAN
GetU1ByteIntegerFromStringInDecimal(
	IN		s8*	Str,
	IN OUT	u8*	pInt
	)
{
	u16 i = 0;
	*pInt = 0;

	while ( Str[i] != '\0' )
	{
		if ( Str[i] >= '0' && Str[i] <= '9' )
		{
			*pInt *= 10;
			*pInt += ( Str[i] - '0' );
		}
		else
		{
			return _FALSE;
		}
		++i;
	}

	return _TRUE;
}

//
//	Description:
//		Map Tx power index into dBm according to 
//		current HW model, for example, RF and PA, and
//		current wireless mode.
//	By Bruce, 2008-01-29.
//
s32
phy_TxPwrIdxToDbm(
	IN	PADAPTER		Adapter,
	IN	WIRELESS_MODE	WirelessMode,
	IN	u8				TxPwrIdx
	)
{
	s32				Offset = 0;
	s32				PwrOutDbm = 0;
	
	//
	// Tested by MP, we found that CCK Index 0 equals to -7dbm, OFDM legacy equals to -8dbm.
	// Note:
	//	The mapping may be different by different NICs. Do not use this formula for what needs accurate result.  
	// By Bruce, 2008-01-29.
	// 
	switch(WirelessMode)
	{
	case WIRELESS_MODE_B:
		Offset = -7;		
		break;

	case WIRELESS_MODE_G:
	case WIRELESS_MODE_N_24G:
		Offset = -8;
		break;
		
	default: //for MacOSX compiler warning
		break;		
	}

	PwrOutDbm = TxPwrIdx / 2 + Offset; // Discard the decimal part.

	return PwrOutDbm;
}

VOID 
PHY_ConvertTxPowerByRateInDbmToRelativeValues(
	IN	u32		*pData,
	IN	u8		Start,
	IN	u8		End,
	IN	u8		BaseValue
	)
{
	s8	i = 0;
	u8	TempValue = 0;
	u32	TempData = 0;
	
	for ( i = 3; i >= 0; --i )
	{
		if ( i >= Start && i <= End )
		{
			// Get the exact value
			TempValue = ( u8 ) ( *pData >> ( i * 8 ) ) & 0xF; 
			TempValue += ( ( u8 ) ( ( *pData >> ( i * 8 + 4 ) ) & 0xF ) ) * 10; 
			
			// Change the value to a relative value
			TempValue = ( TempValue > BaseValue ) ? TempValue - BaseValue : BaseValue - TempValue;
		}
		else
		{
			TempValue = ( u8 ) ( *pData >> ( i * 8 ) ) & 0xFF;
		}
		
		TempData <<= 8;
		TempData |= TempValue;
	}

	*pData = TempData;
}

u8
PHY_GetTxPowerByRateBase(
	IN	PADAPTER		Adapter,
	IN	u8				Band,
	IN	u8				RfPath,
	IN	u8				TxNum,
	IN	RATE_SECTION	RateSection
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8 			value = 0;

	if ( RfPath > ODM_RF_PATH_D )
	{
		DBG_871X("Invalid Rf Path %d in PHY_GetTxPowerByRateBase()\n", RfPath );
		return 0;
	}
	
	if ( Band == BAND_ON_2_4G )
	{
		switch ( RateSection ) {
			case CCK:
				value = pHalData->TxPwrByRateBase2_4G[RfPath][TxNum][0];
				break;
			case OFDM:
				value = pHalData->TxPwrByRateBase2_4G[RfPath][TxNum][1];
				break;
			case HT_MCS0_MCS7:
				value = pHalData->TxPwrByRateBase2_4G[RfPath][TxNum][2];
				break;
			case HT_MCS8_MCS15:
				value = pHalData->TxPwrByRateBase2_4G[RfPath][TxNum][3];
				break;
			default:
				DBG_871X("Invalid RateSection %d in Band 2.4G, Rf Path %d, %dTx in PHY_GetTxPowerByRateBase()\n", 
						 RateSection, RfPath, TxNum );
				break;
				
		};
	}
	else if ( Band == BAND_ON_5G )
	{
		switch ( RateSection ) {
			case OFDM:
				value = pHalData->TxPwrByRateBase5G[RfPath][TxNum][0];
				break;
			case HT_MCS0_MCS7:
				value = pHalData->TxPwrByRateBase5G[RfPath][TxNum][1];
				break;
			case HT_MCS8_MCS15:
				value = pHalData->TxPwrByRateBase5G[RfPath][TxNum][2];
				break;
			case VHT_1SSMCS0_1SSMCS9:
				value = pHalData->TxPwrByRateBase5G[RfPath][TxNum][3];
				break;
			case VHT_2SSMCS0_2SSMCS9:
				value = pHalData->TxPwrByRateBase5G[RfPath][TxNum][4];
				break;
			default:
				DBG_871X("Invalid RateSection %d in Band 5G, Rf Path %d, %dTx in PHY_GetTxPowerByRateBase()\n", 
						 RateSection, RfPath, TxNum );
				break;
		};
	}
	else
	{
		DBG_871X("Invalid Band %d in PHY_GetTxPowerByRateBase()\n", Band );
	}

	return value;
}

VOID
PHY_SetTxPowerByRateBase(
	IN	PADAPTER		Adapter,
	IN	u8				Band,
	IN	u8				RfPath,
	IN	RATE_SECTION	RateSection,
	IN	u8				TxNum,
	IN	u8				Value
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	
	if ( RfPath > ODM_RF_PATH_D )
	{
		DBG_871X("Invalid Rf Path %d in phy_SetTxPowerByRatBase()\n", RfPath );
		return;
	}
	
	if ( Band == BAND_ON_2_4G )
	{
		switch ( RateSection ) {
			case CCK:
				pHalData->TxPwrByRateBase2_4G[RfPath][TxNum][0] = Value;
				break;
			case OFDM:
				pHalData->TxPwrByRateBase2_4G[RfPath][TxNum][1] = Value;
				break;
			case HT_MCS0_MCS7:
				pHalData->TxPwrByRateBase2_4G[RfPath][TxNum][2] = Value;
				break;
			case HT_MCS8_MCS15:
				pHalData->TxPwrByRateBase2_4G[RfPath][TxNum][3] = Value;
				break;
			default:
				DBG_871X("Invalid RateSection %d in Band 2.4G, Rf Path %d, %dTx in PHY_SetTxPowerByRateBase()\n", 
						 RateSection, RfPath, TxNum );
				break;
		};
	}
	else if ( Band == BAND_ON_5G )
	{
		switch ( RateSection ) {
			case OFDM:
				pHalData->TxPwrByRateBase5G[RfPath][TxNum][0] = Value;
				break;
			case HT_MCS0_MCS7:
				pHalData->TxPwrByRateBase5G[RfPath][TxNum][1] = Value;
				break;
			case HT_MCS8_MCS15:
				pHalData->TxPwrByRateBase5G[RfPath][TxNum][2] = Value;
				break;
			case VHT_1SSMCS0_1SSMCS9:
				pHalData->TxPwrByRateBase5G[RfPath][TxNum][3] = Value;
				break;
			case VHT_2SSMCS0_2SSMCS9:
				pHalData->TxPwrByRateBase5G[RfPath][TxNum][4] = Value;
				break;
			default:
				DBG_871X("Invalid RateSection %d in Band 5G, Rf Path %d, %dTx in PHY_SetTxPowerByRateBase()\n", 
						 RateSection, RfPath, TxNum );
				break;
		};
	}
	else
	{
		DBG_871X("Invalid Band %d in PHY_SetTxPowerByRateBase()\n", Band );
	}
}

VOID
phy_StoreTxPowerByRateBaseNew(	
	IN	PADAPTER	pAdapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA( pAdapter );
	u16			rawValue = 0;
	u8			base = 0, path = 0, rate_section;

	for ( path = ODM_RF_PATH_A; path <= ODM_RF_PATH_D; ++path )
	{

		rawValue = ( u16 ) ( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][path][RF_1TX][0] >> 24 ) & 0xFF;
		base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
		PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, path, CCK, RF_1TX, base );

		rawValue = ( u16 ) ( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][path][RF_1TX][2] >> 24 ) & 0xFF; 
		base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
		PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, path, OFDM, RF_1TX, base );

		rawValue = ( u16 ) ( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][path][RF_1TX][4] >> 24 ) & 0xFF; 
		base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
		PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, path, HT_MCS0_MCS7, RF_1TX, base );

		rawValue = ( u16 ) ( pHalData->TxPwrByRateOffset[BAND_ON_2_4G][path][RF_2TX][6] >> 24 ) & 0xFF; 
		base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
		PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, path, HT_MCS8_MCS15, RF_2TX, base );

		rawValue = ( u16 ) ( pHalData->TxPwrByRateOffset[BAND_ON_5G][path][RF_1TX][2] >> 24 ) & 0xFF; 
		base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
		PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_5G, path, OFDM, RF_1TX, base );

		rawValue = ( u16 ) ( pHalData->TxPwrByRateOffset[BAND_ON_5G][path][RF_1TX][4] >> 24 ) & 0xFF;
		base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
		PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_5G, path, HT_MCS0_MCS7, RF_1TX, base );

		rawValue = ( u16 ) ( pHalData->TxPwrByRateOffset[BAND_ON_5G][path][RF_2TX][6] >> 24 ) & 0xFF; 
		base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
		PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_5G, path, HT_MCS8_MCS15, RF_2TX, base );

		rawValue = ( u16 ) ( pHalData->TxPwrByRateOffset[BAND_ON_5G][path][RF_1TX][8] >> 24 ) & 0xFF; 
		base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
		PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_5G, path, VHT_1SSMCS0_1SSMCS9, RF_1TX, base );

		rawValue = ( u16 ) ( pHalData->TxPwrByRateOffset[BAND_ON_5G][path][RF_2TX][11] >> 8 ) & 0xFF; 
		base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
		PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_5G, path, VHT_2SSMCS0_2SSMCS9, RF_2TX, base );
	}
}

VOID
phy_StoreTxPowerByRateBaseOld(	
	IN	PADAPTER	pAdapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA( pAdapter );
	u16			rawValue = 0;
	u8			base = 0;
	u8			path = 0;

	rawValue = ( u16 ) ( pHalData->MCSTxPowerLevelOriginalOffset[0][7] >> 8 ) & 0xFF; 
	base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
	PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, ODM_RF_PATH_A, CCK, RF_1TX, base );

	rawValue = ( u16 ) ( pHalData->MCSTxPowerLevelOriginalOffset[0][1] >> 24 ) & 0xFF; 
	base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
	PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, ODM_RF_PATH_A, OFDM, RF_1TX, base );

	rawValue = ( u16 ) ( pHalData->MCSTxPowerLevelOriginalOffset[0][3] >> 24 ) & 0xFF; 
	base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
	PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, ODM_RF_PATH_A, HT_MCS0_MCS7, RF_1TX, base );

	rawValue = ( u16 ) ( pHalData->MCSTxPowerLevelOriginalOffset[0][5] >> 24 ) & 0xFF; 
	base = ( rawValue >> 4) * 10 + ( rawValue & 0xF );
	PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, ODM_RF_PATH_A, HT_MCS8_MCS15, RF_2TX, base );

	rawValue = ( u16 ) ( pHalData->MCSTxPowerLevelOriginalOffset[0][7] & 0xFF ); 
	base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
	PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, ODM_RF_PATH_B, CCK, RF_1TX, base );

	rawValue = ( u16 ) ( pHalData->MCSTxPowerLevelOriginalOffset[0][9] >> 24 ) & 0xFF; 
	base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
	PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, ODM_RF_PATH_B, OFDM, RF_1TX, base );

	rawValue = ( u16 ) ( pHalData->MCSTxPowerLevelOriginalOffset[0][11] >> 24 ) & 0xFF; 
	base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
	PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, ODM_RF_PATH_B, HT_MCS0_MCS7, RF_1TX, base );

	rawValue = ( u16 ) ( pHalData->MCSTxPowerLevelOriginalOffset[0][13] >> 24 ) & 0xFF; 
	base = ( rawValue >> 4 ) * 10 + ( rawValue & 0xF );
	PHY_SetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, ODM_RF_PATH_B, HT_MCS8_MCS15, RF_2TX, base );
}

VOID
PHY_StoreTxPowerByRateBase(	
	IN	PADAPTER	pAdapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA( pAdapter );
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;

	if ( pDM_Odm->PhyRegPgVersion == 0 )
	{
		phy_StoreTxPowerByRateBaseOld( pAdapter );
	}
	else 
	{
		if ( IS_HARDWARE_TYPE_8723B( pAdapter ) ) {
#if (RTL8723B_SUPPORT == 1)
			PHY_StoreTxPowerByRateBase_8723B( pAdapter );
#endif
		} else if ( IS_HARDWARE_TYPE_8192E( pAdapter ) ) {
#if (RTL8192E_SUPPORT == 1)
			PHY_StoreTxPowerByRateBase_8192E( pAdapter );
#endif
		} else if ( IS_HARDWARE_TYPE_JAGUAR( pAdapter ) ) {
#if (RTL8812A_SUPPORT == 1)
			PHY_StoreTxPowerByRateBase_8812A( pAdapter );
#endif
		}
	}
}

u8
PHY_GetRateSectionIndexOfTxPowerByRate(
	IN	PADAPTER	pAdapter,
	IN	u32			RegAddr,
	IN	u32			BitMask
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA( pAdapter );
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
	u8 			index = 0;
	
	if ( pDM_Odm->PhyRegPgVersion == 0 )
	{
		switch ( RegAddr )
		{
			case rTxAGC_A_Rate18_06:	 index = 0;		break;
			case rTxAGC_A_Rate54_24:	 index = 1;		break;
			case rTxAGC_A_CCK1_Mcs32:	 index = 6;		break;
			case rTxAGC_B_CCK11_A_CCK2_11:
				if ( BitMask == 0xffffff00 )
					index = 7;
				else if ( BitMask == 0x000000ff )
					index = 15;
				break;
				
			case rTxAGC_A_Mcs03_Mcs00:	 index = 2;		break;
			case rTxAGC_A_Mcs07_Mcs04:	 index = 3;		break;
			case rTxAGC_A_Mcs11_Mcs08:	 index = 4;		break;
			case rTxAGC_A_Mcs15_Mcs12:	 index = 5;		break;
			case rTxAGC_B_Rate18_06:	 index = 8;		break;
			case rTxAGC_B_Rate54_24:	 index = 9;		break;
			case rTxAGC_B_CCK1_55_Mcs32: index = 14;	break;
			case rTxAGC_B_Mcs03_Mcs00:	 index = 10;	break;
			case rTxAGC_B_Mcs07_Mcs04:	 index = 11;	break;
			case rTxAGC_B_Mcs11_Mcs08:	 index = 12;	break;
			case rTxAGC_B_Mcs15_Mcs12:	 index = 13;	break;
			default:
				DBG_871X("Invalid RegAddr 0x3%x in PHY_GetRateSectionIndexOfTxPowerByRate()", RegAddr );
				break;
		};
	}
	else if ( pDM_Odm->PhyRegPgVersion > 0 )
	{
		switch ( RegAddr )
		{
			case rTxAGC_A_Rate18_06:	   index = 0;		break;
			case rTxAGC_A_Rate54_24:	   index = 1;		break;
			case rTxAGC_A_CCK1_Mcs32:	   index = 2;		break;
			case rTxAGC_B_CCK11_A_CCK2_11: index = 3;		break;			
			case rTxAGC_A_Mcs03_Mcs00:	   index = 4;		break;
			case rTxAGC_A_Mcs07_Mcs04:	   index = 5;		break;
			case rTxAGC_A_Mcs11_Mcs08:	   index = 6;		break;
			case rTxAGC_A_Mcs15_Mcs12:	   index = 7;		break;
			case rTxAGC_B_Rate18_06:	   index = 0;		break;
			case rTxAGC_B_Rate54_24:	   index = 1;		break;
			case rTxAGC_B_CCK1_55_Mcs32:   index = 2;		break;
			case rTxAGC_B_Mcs03_Mcs00:	   index = 4;		break;
			case rTxAGC_B_Mcs07_Mcs04:	   index = 5;		break;
			case rTxAGC_B_Mcs11_Mcs08:	   index = 6;		break;
			case rTxAGC_B_Mcs15_Mcs12:	   index = 7;		break;
			default:
				RegAddr =  ( RegAddr & 0xFFF );
				if ( RegAddr >= 0xC20 && RegAddr <= 0xC4C )
					index = ( u8 ) ( ( RegAddr - 0xC20 ) / 4 );
				else if ( RegAddr >= 0xE20 && RegAddr <= 0xE4C )
					index = ( u8 ) ( ( RegAddr - 0xE20 ) / 4 );
				else
					DBG_871X("Invalid RegAddr 0x%x in PHY_GetRateSectionIndexOfTxPowerByRate()\n", RegAddr);
				break;
		};
	}
	
	return index;
}

u8
PHY_GetTxPowerByRateNew(
	IN	PADAPTER	pAdapter,
	IN	u8			Band,
	IN	u8			RfPath,
	IN	u8			TxNum,
	IN	u8			Rate
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);
	u8			shift = 0;
	u8			tx_pwr_diff = 0;
	u8			rate_section = 0;

	if ( Band != BAND_ON_2_4G && Band != BAND_ON_5G )
	{
		DBG_871X("Invalid Band %d\n", Band );
	}

	if ( RfPath > ODM_RF_PATH_MAX )
	{
		DBG_871X("Invalid RfPath %d\n", RfPath );
	}

	if ( TxNum > ODM_RF_PATH_MAX )
	{
		DBG_871X("Invalid TxNum %d\n", TxNum );
	}

	switch	(Rate)
	{
		case	MGN_1M:
		case	MGN_2M:
		case	MGN_5_5M:	
		case	MGN_11M:	
			rate_section = 0;
			break;

		case	MGN_6M:		
		case	MGN_9M:		
		case	MGN_12M:	
		case	MGN_18M:	
			rate_section = 1;
			break;

		case	MGN_24M:	
		case	MGN_36M:    
		case	MGN_48M:    
		case	MGN_54M:    
			rate_section = 2;
			break;
		
		case	MGN_MCS0:	
		case	MGN_MCS1:   
		case	MGN_MCS2:   
		case	MGN_MCS3:   
			rate_section = 3;
			break;

		case	MGN_MCS4:	
		case	MGN_MCS5:   
		case	MGN_MCS6:   
		case	MGN_MCS7:   
			rate_section = 4;
			break;

		case	MGN_MCS8:	
		case	MGN_MCS9:   
		case	MGN_MCS10:  
		case	MGN_MCS11:  
			rate_section = 5;
			break;

		case	MGN_MCS12:	
		case	MGN_MCS13:  
		case	MGN_MCS14:  
		case	MGN_MCS15:  
			rate_section = 6;
			break;
		
		case	MGN_VHT1SS_MCS0:	
		case	MGN_VHT1SS_MCS1:    
		case	MGN_VHT1SS_MCS2:    
		case	MGN_VHT1SS_MCS3:    
			rate_section = 7;
			break;

		case	MGN_VHT1SS_MCS4:	
		case	MGN_VHT1SS_MCS5:    
		case	MGN_VHT1SS_MCS6:    
		case	MGN_VHT1SS_MCS7:    
			rate_section = 8;
			break;

		case	MGN_VHT1SS_MCS8:	
		case	MGN_VHT1SS_MCS9:    
		case	MGN_VHT2SS_MCS0:    
		case	MGN_VHT2SS_MCS1:    
			rate_section = 9;
			break;

		case	MGN_VHT2SS_MCS2:	
		case	MGN_VHT2SS_MCS3:    
		case	MGN_VHT2SS_MCS4:    
		case	MGN_VHT2SS_MCS5:    
			rate_section = 10;
			break;

		case	MGN_VHT2SS_MCS6:	
		case	MGN_VHT2SS_MCS7:    
		case	MGN_VHT2SS_MCS8:    
		case	MGN_VHT2SS_MCS9:    
			rate_section = 11;
			break;
			
		default:
			DBG_871X("Rate %dis Illegal\n", Rate);
			break;
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
			
		case	MGN_VHT1SS_MCS0:	shift = 0; 		break;
		case	MGN_VHT1SS_MCS1:    shift = 8;      break;
		case	MGN_VHT1SS_MCS2:    shift = 16;     break;
		case	MGN_VHT1SS_MCS3:    shift = 24;     break;
			
		case	MGN_VHT1SS_MCS4:	shift = 0; 		break;
		case	MGN_VHT1SS_MCS5:    shift = 8;      break;
		case	MGN_VHT1SS_MCS6:    shift = 16;     break;
		case	MGN_VHT1SS_MCS7:    shift = 24;     break;
			
		case	MGN_VHT1SS_MCS8:	shift = 0; 		break;
		case	MGN_VHT1SS_MCS9:    shift = 8;      break;
		case	MGN_VHT2SS_MCS0:    shift = 16;     break;
		case	MGN_VHT2SS_MCS1:    shift = 24;     break;
			
		case	MGN_VHT2SS_MCS2:	shift = 0; 		break;
		case	MGN_VHT2SS_MCS3:    shift = 8;      break;
		case	MGN_VHT2SS_MCS4:    shift = 16;     break;
		case	MGN_VHT2SS_MCS5:    shift = 24;     break;
			
		case	MGN_VHT2SS_MCS6:	shift = 0; 		break;
		case	MGN_VHT2SS_MCS7:    shift = 8;      break;
		case	MGN_VHT2SS_MCS8:    shift = 16;     break;
		case	MGN_VHT2SS_MCS9:    shift = 24;     break;
			
		default:
			DBG_871X("Rate_Section is Illegal\n");
			break;
	}

	tx_pwr_diff = ( u8 ) (pHalData->TxPwrByRateOffset[Band][RfPath][TxNum][rate_section] >> shift) & 0xff;

	//DBG_871X("TxPwrByRateOffset-BAND(%d)-RF(%d)-RAS(%d)=%x tx_pwr_diff=%d shift=%d\n", 
	//Band, RfPath, TxNum, rate_section, pHalData->TxPwrByRateOffset[Band][RfPath][TxNum][rate_section], tx_pwr_diff, shift);

	return	tx_pwr_diff;
}

void
PHY_SetTxPowerByRateNew(
	IN	PADAPTER	pAdapter,
	IN	u32			Band,
	IN	u32			RfPath,
	IN	u32			TxNum,
	IN	u32			RegAddr,
	IN	u32			BitMask,
	IN	u32			Data
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);
	u8			rate_section = PHY_GetRateSectionIndexOfTxPowerByRate( pAdapter, RegAddr, BitMask );

	if ( Band != BAND_ON_2_4G && Band != BAND_ON_5G )
	{
		DBG_871X("Invalid Band %d\n", Band );
	}

	if ( RfPath > ODM_RF_PATH_MAX )
	{
		DBG_871X("Invalid RfPath %d\n", RfPath );
	}

	if ( TxNum > ODM_RF_PATH_MAX )
	{
		DBG_871X("Invalid TxNum %d\n", TxNum );
	}
	
	pHalData->TxPwrByRateOffset[Band][RfPath][TxNum][rate_section] = Data;
	//DBG_871X("pHalData->TxPwrByRateOffset[Band %d][RfPath %d][TxNum %d][RateSection %d] = 0x%x\n", 
	//		Band, RfPath, TxNum, rate_section, pHalData->TxPwrByRateOffset[Band][RfPath][TxNum][rate_section]);
}

void 
PHY_SetTxPowerByRateOld(
	IN	PADAPTER		pAdapter,
	IN	u32				RegAddr,
	IN	u32				BitMask,
	IN	u32				Data
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	u8			index = PHY_GetRateSectionIndexOfTxPowerByRate( pAdapter, RegAddr, BitMask );

	pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][index] = Data;
	//DBG_871X("MCSTxPowerLevelOriginalOffset[%d][0] = 0x%x\n", pHalData->pwrGroupCnt,
	//	pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][0]);
}

VOID
PHY_InitTxPowerByRate(
	IN	PADAPTER	pAdapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	u1Byte band = 0, rfPath = 0, TxNum = 0, rateSection = 0, i = 0, j = 0;

	if ( IS_HARDWARE_TYPE_8188E( pAdapter ) || IS_HARDWARE_TYPE_8723A( pAdapter ) )
	{
		for ( i = 0; i < MAX_PG_GROUP; ++i )
			for ( j = 0; j < 16; ++j )
				pHalData->MCSTxPowerLevelOriginalOffset[i][j] = 0;
	}
	else
	{
		for ( band = BAND_ON_2_4G; band <= BAND_ON_5G; ++band )
				for ( rfPath = 0; rfPath < TX_PWR_BY_RATE_NUM_RF; ++rfPath )
					for ( TxNum = 0; TxNum < TX_PWR_BY_RATE_NUM_RF; ++TxNum )
						for ( rateSection = 0; rateSection < TX_PWR_BY_RATE_NUM_SECTION; ++rateSection )
							pHalData->TxPwrByRateOffset[band][rfPath][TxNum][rateSection] = 0;
	}
}

VOID
PHY_StoreTxPowerByRate(
	IN	PADAPTER	pAdapter,
	IN	u32			Band,
	IN	u32			RfPath,
	IN	u32			TxNum,
	IN	u32			RegAddr,
	IN	u32			BitMask,
	IN	u32			Data
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T  		pDM_Odm = &pHalData->odmpriv;
	
	if ( pDM_Odm->PhyRegPgVersion > 0 )
	{
		PHY_SetTxPowerByRateNew( pAdapter, Band, RfPath, TxNum, RegAddr, BitMask, Data );
	}
	else if ( pDM_Odm->PhyRegPgVersion == 0 )
	{
		PHY_SetTxPowerByRateOld( pAdapter, RegAddr, BitMask, Data );
	
		if ( RegAddr == rTxAGC_A_Mcs15_Mcs12 && pHalData->rf_type == RF_1T1R )
			pHalData->pwrGroupCnt++;
		else if ( RegAddr == rTxAGC_B_Mcs15_Mcs12 && pHalData->rf_type != RF_1T1R )
			pHalData->pwrGroupCnt++;
	}
	else
		DBG_871X("Invalid PHY_REG_PG.txt version %d\n",  pDM_Odm->PhyRegPgVersion );
	
}


VOID
PHY_ConvertTxPowerByRateInDbmToRelativeValuesOld(
	IN	PADAPTER	pAdapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA( pAdapter );
	u8			base = 0;
	
	//DBG_871X("===>PHY_ConvertTxPowerByRateInDbmToRelativeValuesOld()\n" );
	
	// CCK
	base = PHY_GetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, ODM_RF_PATH_A, RF_1TX, CCK );
	PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->MCSTxPowerLevelOriginalOffset[0][6] ), 1, 1, base );
	PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->MCSTxPowerLevelOriginalOffset[0][7] ), 1, 3, base );

	// OFDM
	base = PHY_GetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, ODM_RF_PATH_A, RF_1TX, OFDM );
	PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->MCSTxPowerLevelOriginalOffset[0][0] ), 0, 3, base );
	PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->MCSTxPowerLevelOriginalOffset[0][1] ),	0, 3, base );

	// HT MCS0~7
	base = PHY_GetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, ODM_RF_PATH_A, RF_1TX, HT_MCS0_MCS7 );
	PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->MCSTxPowerLevelOriginalOffset[0][2] ),	0, 3, base );
	PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->MCSTxPowerLevelOriginalOffset[0][3] ),	0, 3, base );

	// HT MCS8~15
	base = PHY_GetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, ODM_RF_PATH_A, RF_2TX, HT_MCS8_MCS15 );
	PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->MCSTxPowerLevelOriginalOffset[0][4] ), 0, 3, base );
	PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->MCSTxPowerLevelOriginalOffset[0][5] ), 0, 3, base );

	// CCK
	base = PHY_GetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, ODM_RF_PATH_B, RF_1TX, CCK );
	PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->MCSTxPowerLevelOriginalOffset[0][14] ), 1, 3, base );
	PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->MCSTxPowerLevelOriginalOffset[0][15] ), 0, 0, base );

	// OFDM
	base = PHY_GetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, ODM_RF_PATH_B, RF_1TX, OFDM );
	PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->MCSTxPowerLevelOriginalOffset[0][8] ), 0, 3, base );
	PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->MCSTxPowerLevelOriginalOffset[0][9] ),	0, 3, base );

	// HT MCS0~7
	base = PHY_GetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, ODM_RF_PATH_B, RF_1TX, HT_MCS0_MCS7 );
	PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->MCSTxPowerLevelOriginalOffset[0][10] ), 0, 3, base );
	PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->MCSTxPowerLevelOriginalOffset[0][11] ), 0, 3, base );

	// HT MCS8~15
	base = PHY_GetTxPowerByRateBase( pAdapter, BAND_ON_2_4G, ODM_RF_PATH_B, RF_2TX, HT_MCS8_MCS15 );
	PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->MCSTxPowerLevelOriginalOffset[0][12] ), 0, 3, base );
	PHY_ConvertTxPowerByRateInDbmToRelativeValues( 
			&( pHalData->MCSTxPowerLevelOriginalOffset[0][13] ), 0, 3, base );

	DBG_871X("<===PHY_ConvertTxPowerByRateInDbmToRelativeValuesOld()\n" );
}

/*
  * This function must be called if the value in the PHY_REG_PG.txt(or header)
  * is exact dBm values
  */
VOID
PHY_TxPowerByRateConfiguration(
	IN  PADAPTER			pAdapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA( pAdapter);

	PHY_StoreTxPowerByRateBase( pAdapter );
	if ( IS_HARDWARE_TYPE_8723B( pAdapter ) ) {
#if (RTL8723B_SUPPORT == 1)
		PHY_ConvertTxPowerByRateInDbmToRelativeValues_8723B( pAdapter );
#endif
	} else if ( IS_HARDWARE_TYPE_8192E( pAdapter ) ) {
#if (RTL8192E_SUPPORT == 1)
		PHY_ConvertTxPowerByRateInDbmToRelativeValues_8192E( pAdapter );
#endif
	} else if ( IS_HARDWARE_TYPE_JAGUAR( pAdapter ) ) {
#if (RTL8812A_SUPPORT == 1)
		PHY_ConvertTxPowerByRateInDbmToRelativeValues_8812A( pAdapter );
#endif
	} else if ( pHalData->odmpriv.PhyRegPgVersion == 0 ) {
		PHY_ConvertTxPowerByRateInDbmToRelativeValuesOld( pAdapter );
	}
	
}

s8
phy_GetChannelGroup(
	IN BAND_TYPE	Band,
	IN u8			Channel
	)
{
	s8 channelGroup = -1;
	if ( Channel <= 14 && Band == BAND_ON_2_4G )
	{
		if		( 1 <= Channel && Channel <= 2 )	channelGroup = 0;
		else if ( 3  <= Channel && Channel <= 5 )   channelGroup = 1;
		else if ( 6  <= Channel && Channel <= 8 )   channelGroup = 2;
		else if ( 9  <= Channel && Channel <= 11 )  channelGroup = 3;
		else if ( 12 <= Channel && Channel <= 14)   channelGroup = 4;
		else
		{
			DBG_871X( "==> phy_GetChannelGroup() in 2.4 G, but Channel %d in Group not found \n", Channel );
			channelGroup = -1;
		}
	}
	else if( Band == BAND_ON_5G )
	{
		if      ( 36   <= Channel && Channel <=  42 )  channelGroup = 0;
		else if ( 44   <= Channel && Channel <=  48 )  channelGroup = 1;
		else if ( 50   <= Channel && Channel <=  58 )  channelGroup = 2;
		else if ( 60   <= Channel && Channel <=  64 )  channelGroup = 3;
		else if ( 100  <= Channel && Channel <= 106 )  channelGroup = 4;
		else if ( 108  <= Channel && Channel <= 114 )  channelGroup = 5;
		else if ( 116  <= Channel && Channel <= 122 )  channelGroup = 6;
		else if ( 124  <= Channel && Channel <= 130 )  channelGroup = 7;
		else if ( 132  <= Channel && Channel <= 138 )  channelGroup = 8;
		else if ( 140  <= Channel && Channel <= 144 )  channelGroup = 9;
		else if ( 149  <= Channel && Channel <= 155 )  channelGroup = 10;
		else if ( 157  <= Channel && Channel <= 161 )  channelGroup = 11;
		else if ( 165  <= Channel && Channel <= 171 )  channelGroup = 12;
		else if ( 173  <= Channel && Channel <= 177 )  channelGroup = 13;
		else
		{
			DBG_871X("==>phy_GetChannelGroup() in 5G, but Channel %d in Group not found \n", Channel );
			channelGroup = -1;
		}
	}
	else 
	{
		DBG_871X("==>phy_GetChannelGroup() in unsupported band %d\n", Band );
		channelGroup = -1;
	}

	return channelGroup;
}

VOID
PHY_SetTxPowerLimit(
	IN	PADAPTER		Adapter,
	IN	u8				*Regulation,
	IN	u8				*Band,
	IN	u8				*Bandwidth,
	IN	u8				*RateSection,
	IN	u8				*RfPath,
	IN	u8				*Channel,
	IN	u8				*PowerLimit
	)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA( Adapter );
	u8				regulation=0, bandwidth=0, rateSection=0, 
						 channel, channelGroup;
	s8 				powerLimit = 0;

	//DBG_871X("Index of power limit table [band %s][regulation %s][bw %s][rate section %s][rf path %s][chnl %s][val %s]\n", 
	//	  Band, Regulation, Bandwidth, RateSection, RfPath, Channel, PowerLimit );

	if ( !GetU1ByteIntegerFromStringInDecimal( (ps1Byte)Channel, &channel ) ||
		 !GetU1ByteIntegerFromStringInDecimal( (ps1Byte)PowerLimit, &powerLimit ) )
	{
		//DBG_871X("Illegal index of power limit table [chnl %s][val %s]\n", Channel, PowerLimit );
	}

	powerLimit = powerLimit > MAX_POWER_INDEX ? MAX_POWER_INDEX : powerLimit;

	if ( eqNByte( Regulation, (u8 *)("FCC"), 3 ) ) regulation = 0;
	else if ( eqNByte( Regulation, (u8 *)("MKK"), 3 ) ) regulation = 1;
	else if ( eqNByte( Regulation, (u8 *)("ETSI"), 4 ) ) regulation = 2;

	if ( eqNByte( RateSection, (u8 *)("CCK"), 3 ) )
		rateSection = 0;
	else if ( eqNByte( RateSection, (u8 *)("OFDM"), 4 ) )
		rateSection = 1;
	else if ( eqNByte( RateSection, (u8 *)("HT"), 2 ) && eqNByte( RfPath, (u8 *)("1T"), 2 ) )
		rateSection = 2;
	else if ( eqNByte( RateSection, (u8 *)("HT"), 2 ) && eqNByte( RfPath, (u8 *)("2T"), 2 ) )
		rateSection = 3;
	else if ( eqNByte( RateSection, (u8 *)("VHT"), 3 ) && eqNByte( RfPath, (u8 *)("1T"), 2 ) )
		rateSection = 4;
	else if ( eqNByte( RateSection, (u8 *)("VHT"), 3 ) && eqNByte( RfPath, (u8 *)("2T"), 2 ) )
		rateSection = 5;
			

	if ( eqNByte( Bandwidth, (u8 *)("20M"), 3 ) ) bandwidth = 0;
	else if ( eqNByte( Bandwidth, (u8 *)("40M"), 3 ) ) bandwidth = 1;
	else if ( eqNByte( Bandwidth, (u8 *)("80M"), 3 ) ) bandwidth = 2;
	else if ( eqNByte( Bandwidth, (u8 *)("160M"), 4 ) ) bandwidth = 3;

	if ( eqNByte( Band, (u8 *)("2.4G"), 4 ) )
	{
		//DBG_871X( "2.4G Band value : [regulation %d][bw %d][rate_section %d][chnl %d][val %d]\n", 
		//	  regulation, bandwidth, rateSection, channel, powerLimit );
		channelGroup = phy_GetChannelGroup( BAND_ON_2_4G, channel );
		pHalData->TxPwrLimit_2_4G[regulation][bandwidth][rateSection][channelGroup][ODM_RF_PATH_A] = powerLimit;
	}
	else if ( eqNByte( Band, (u8 *)("5G"), 2 ) )
	{
		//DBG_871X("5G Band value : [regulation %d][bw %d][rate_section %d][chnl %d][val %d]\n", 
		//	  regulation, bandwidth, rateSection, channel, powerLimit );
		channelGroup = phy_GetChannelGroup( BAND_ON_5G, channel );
		pHalData->TxPwrLimit_5G[regulation][bandwidth][rateSection][channelGroup][ODM_RF_PATH_A] = powerLimit;
	}
	else
	{
		DBG_871X("Cannot recognize the band info in %s\n", Band );
		return;
	}
}

u8
PHY_GetTxPowerByRateBaseIndex(
	IN	BAND_TYPE			Band,
	IN	u8					Rate
	)
{
	u8 index = 0;
	if ( Band == BAND_ON_2_4G )
	{
		switch ( Rate )
		{
			case MGN_1M: case MGN_2M: case MGN_5_5M: case MGN_11M:
				index = 0;
				break;

			case MGN_6M: case MGN_9M: case MGN_12M: case MGN_18M:
			case MGN_24M: case MGN_36M: case MGN_48M: case MGN_54M:
				index = 1;
				break;

			case MGN_MCS0: case MGN_MCS1: case MGN_MCS2: case MGN_MCS3: 
			case MGN_MCS4: case MGN_MCS5: case MGN_MCS6: case MGN_MCS7:
				index = 2;
				break;
				
			case MGN_MCS8: case MGN_MCS9: case MGN_MCS10: case MGN_MCS11: 
			case MGN_MCS12: case MGN_MCS13: case MGN_MCS14: case MGN_MCS15:
				index = 3;
				break;

			default:
				DBG_871X("Wrong rate 0x%x to obtain index in 2.4G in PHY_GetTxPowerByRateBaseIndex()\n", Rate );
				break;
		}
	}
	else if ( Band == BAND_ON_5G )
	{
		switch ( Rate )
		{
			case MGN_6M: case MGN_9M: case MGN_12M: case MGN_18M:
			case MGN_24M: case MGN_36M: case MGN_48M: case MGN_54M:
				index = 0;
				break;

			case MGN_MCS0: case MGN_MCS1: case MGN_MCS2: case MGN_MCS3: 
			case MGN_MCS4: case MGN_MCS5: case MGN_MCS6: case MGN_MCS7:
				index = 1;
				break;
				
			case MGN_MCS8: case MGN_MCS9: case MGN_MCS10: case MGN_MCS11: 
			case MGN_MCS12: case MGN_MCS13: case MGN_MCS14: case MGN_MCS15:
				index = 2;
				break;

			case MGN_VHT1SS_MCS0: case MGN_VHT1SS_MCS1: case MGN_VHT1SS_MCS2:
			case MGN_VHT1SS_MCS3: case MGN_VHT1SS_MCS4: case MGN_VHT1SS_MCS5:
			case MGN_VHT1SS_MCS6: case MGN_VHT1SS_MCS7: case MGN_VHT1SS_MCS8:
			case MGN_VHT1SS_MCS9:
				index = 3;
				break;
				
			case MGN_VHT2SS_MCS0: case MGN_VHT2SS_MCS1: case MGN_VHT2SS_MCS2:
			case MGN_VHT2SS_MCS3: case MGN_VHT2SS_MCS4: case MGN_VHT2SS_MCS5:
			case MGN_VHT2SS_MCS6: case MGN_VHT2SS_MCS7: case MGN_VHT2SS_MCS8:
			case MGN_VHT2SS_MCS9:
				index = 4;
				break;

			default:
				DBG_871X("Wrong rate 0x%x to obtain index in 5G in PHY_GetTxPowerByRateBaseIndex()\n", Rate );
				break;
		}
	}

	return index;
}

s8
PHY_GetTxPowerLimit(
	IN	PADAPTER			Adapter,
	IN	u32					RegPwrTblSel,
	IN	BAND_TYPE			Band,
	IN	CHANNEL_WIDTH		Bandwidth,
	IN	u8					RfPath,
	IN	u8					DataRate,
	IN	u8					Channel
	)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	s16				band = -1, regulation = -1, bandwidth = -1,
					rateSection = -1, channelGroup = -1;
	s8				powerLimit = MAX_POWER_INDEX;

	if ( ( Adapter->registrypriv.RegEnableTxPowerLimit == 0 && pHalData->EEPROMRegulatory != 1 ) ||
		  pHalData->EEPROMRegulatory == 2 )
		return MAX_POWER_INDEX;

	switch( Adapter->registrypriv.RegPwrTblSel )
	{
		case 1:
				regulation = TXPWR_LMT_ETSI; 
				break;
		case 2:
				regulation = TXPWR_LMT_MKK;
				break;
		case 3:
				regulation = TXPWR_LMT_FCC;
				break;

		default:
				regulation = ( Band == BAND_ON_2_4G ) ? pHalData->Regulation2_4G 
					                                  : pHalData->Regulation5G;
				break;
	}
	
	//DBG_871X("pMgntInfo->RegPwrTblSel %d, final regulation %d\n", Adapter->registrypriv.RegPwrTblSel, regulation );

	
	if ( Band == BAND_ON_2_4G ) band = 0; 
	else if ( Band == BAND_ON_5G ) band = 1; 

	if ( Bandwidth == CHANNEL_WIDTH_20 ) bandwidth = 0;
	else if ( Bandwidth == CHANNEL_WIDTH_40 ) bandwidth = 1;
	else if ( Bandwidth == CHANNEL_WIDTH_80 ) bandwidth = 2;
	else if ( Bandwidth == CHANNEL_WIDTH_160 ) bandwidth = 3;

	switch ( DataRate )
	{
		case MGN_1M: case MGN_2M: case MGN_5_5M: case MGN_11M:
			rateSection = 0;
			break;

		case MGN_6M: case MGN_9M: case MGN_12M: case MGN_18M:
		case MGN_24M: case MGN_36M: case MGN_48M: case MGN_54M:
			rateSection = 1;
			break;

		case MGN_MCS0: case MGN_MCS1: case MGN_MCS2: case MGN_MCS3: 
		case MGN_MCS4: case MGN_MCS5: case MGN_MCS6: case MGN_MCS7:
			rateSection = 2;
			break;
			
		case MGN_MCS8: case MGN_MCS9: case MGN_MCS10: case MGN_MCS11: 
		case MGN_MCS12: case MGN_MCS13: case MGN_MCS14: case MGN_MCS15:
			rateSection = 3;
			break;

		case MGN_VHT1SS_MCS0: case MGN_VHT1SS_MCS1: case MGN_VHT1SS_MCS2:
		case MGN_VHT1SS_MCS3: case MGN_VHT1SS_MCS4: case MGN_VHT1SS_MCS5:
		case MGN_VHT1SS_MCS6: case MGN_VHT1SS_MCS7: case MGN_VHT1SS_MCS8:
		case MGN_VHT1SS_MCS9:
			rateSection = 4;
			break;
			
		case MGN_VHT2SS_MCS0: case MGN_VHT2SS_MCS1: case MGN_VHT2SS_MCS2:
		case MGN_VHT2SS_MCS3: case MGN_VHT2SS_MCS4: case MGN_VHT2SS_MCS5:
		case MGN_VHT2SS_MCS6: case MGN_VHT2SS_MCS7: case MGN_VHT2SS_MCS8:
		case MGN_VHT2SS_MCS9:
			rateSection = 5;
			break;

		default:
			DBG_871X("Wrong rate 0x%x\n", DataRate );
			break;
	}

	if ( Band == BAND_ON_5G  && rateSection == 0 )
			DBG_871X("Wrong rate 0x%x: No CCK in 5G Band\n", DataRate );

	// workaround for wrong index combination to obtain tx power limit, 
	// OFDM only exists in BW 20M
	if ( rateSection == 1 )
		bandwidth = 0;

	// workaround for wrong indxe combination to obtain tx power limit, 
	// HT on 80M will reference to HT on 40M
	if ( ( rateSection == 2 || rateSection == 3 ) && Band == BAND_ON_5G && bandwidth == 2 ) {
		bandwidth = 1;
	}
	
	if ( Band == BAND_ON_2_4G )
		channelGroup = phy_GetChannelGroup( BAND_ON_2_4G, Channel );
	else if ( Band == BAND_ON_5G )
		channelGroup = phy_GetChannelGroup( BAND_ON_5G, Channel );
	else if ( Band == BAND_ON_BOTH )
	{
		// BAND_ON_BOTH don't care temporarily 
	}
	
	if ( band == -1 || regulation == -1 || bandwidth == -1 || 
	     rateSection == -1 || channelGroup == -1 )
	{
		//DBG_871X("Wrong index value to access power limit table [band %d][regulation %d][bandwidth %d][rf_path %d][rate_section %d][chnlGroup %d]\n",
		//	  band, regulation, bandwidth, RfPath, rateSection, channelGroup );

		return MAX_POWER_INDEX;
	}

	if ( Band == BAND_ON_2_4G )
		powerLimit = pHalData->TxPwrLimit_2_4G[regulation]
			[bandwidth][rateSection][channelGroup][RfPath];
	else if ( Band == BAND_ON_5G )
		powerLimit = pHalData->TxPwrLimit_5G[regulation]
			[bandwidth][rateSection][channelGroup][RfPath];
	else 
		DBG_871X("No power limit table of the specified band\n" );

	// combine 5G VHT & HT rate
	// 5G 20M and 40M HT and VHT can cross reference
	/*
	if ( Band == BAND_ON_5G && powerLimit == MAX_POWER_INDEX ) {
		if ( bandwidth == 0 || bandwidth == 1 ) { 
			RT_TRACE( COMP_INIT, DBG_LOUD, ( "No power limit table of the specified band %d, bandwidth %d, ratesection %d, rf path %d\n", 
					  band, bandwidth, rateSection, RfPath ) );
			if ( rateSection == 2 )
				powerLimit = pHalData->TxPwrLimit_5G[regulation]
										[bandwidth][4][channelGroup][RfPath];
			else if ( rateSection == 4 )
				powerLimit = pHalData->TxPwrLimit_5G[regulation]
										[bandwidth][2][channelGroup][RfPath];
			else if ( rateSection == 3 )
				powerLimit = pHalData->TxPwrLimit_5G[regulation]
										[bandwidth][5][channelGroup][RfPath];
			else if ( rateSection == 5 )
				powerLimit = pHalData->TxPwrLimit_5G[regulation]
										[bandwidth][3][channelGroup][RfPath];
		}
	}
	*/
	//DBG_871X("TxPwrLmt[Regulation %d][Band %d][BW %d][RFPath %d][Rate 0x%x][Chnl %d] = %d\n", 
	//		regulation, pHalData->CurrentBandType, Bandwidth, RfPath, DataRate, Channel, powerLimit);
	return powerLimit;
}

VOID
phy_CrossReferenceHTAndVHTTxPowerLimit(
	IN	PADAPTER			pAdapter
	)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(pAdapter);
	u8 				regulation, bw, channel, rateSection, group;	
	s8 				tempPwrLmt = 0;
	
	for ( regulation = 0; regulation < MAX_REGULATION_NUM; ++regulation )
	{
		for ( bw = 0; bw < MAX_5G_BANDWITH_NUM; ++bw )
		{
			for ( group = 0; group < MAX_5G_CHANNEL_NUM; ++group )
			{
				for ( rateSection = 0; rateSection < MAX_RATE_SECTION_NUM; ++rateSection )
				{	
					tempPwrLmt = pHalData->TxPwrLimit_5G[regulation][bw][rateSection][group][ODM_RF_PATH_A];
					if ( tempPwrLmt == MAX_POWER_INDEX )
					{
						if ( bw == 0 || bw == 1 ) { // 5G 20M 40M VHT and HT can cross reference
							//DBG_871X("No power limit table of the specified band %d, bandwidth %d, ratesection %d, group %d, rf path %d\n",
							//			1, bw, rateSection, group, ODM_RF_PATH_A );
							if ( rateSection == 2 ) {
								pHalData->TxPwrLimit_5G[regulation][bw][2][group][ODM_RF_PATH_A] = 
									pHalData->TxPwrLimit_5G[regulation][bw][4][group][ODM_RF_PATH_A];
							}
							else if ( rateSection == 4 ) {
								pHalData->TxPwrLimit_5G[regulation][bw][4][group][ODM_RF_PATH_A] = 
									pHalData->TxPwrLimit_5G[regulation][bw][2][group][ODM_RF_PATH_A];
							}
							else if ( rateSection == 3 ) {
								pHalData->TxPwrLimit_5G[regulation][bw][3][group][ODM_RF_PATH_A] = 
									pHalData->TxPwrLimit_5G[regulation][bw][5][group][ODM_RF_PATH_A];
							}
							else if ( rateSection == 5 ) {
								pHalData->TxPwrLimit_5G[regulation][bw][5][group][ODM_RF_PATH_A] = 
									pHalData->TxPwrLimit_5G[regulation][bw][3][group][ODM_RF_PATH_A];
							}

							//DBG_871X("use other value %d", tempPwrLmt );
						}
					}
				}
			}
		}
	}
}

VOID 
PHY_ConvertTxPowerLimitToPowerIndex(
	IN	PADAPTER			Adapter
	)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	u8 				BW40PwrBasedBm2_4G, BW40PwrBasedBm5G;
	u8 				regulation, bw, channel, rateSection, group;	
	u8 				baseIndex2_4G;
	u8				baseIndex5G;
	s8 				tempValue = 0, tempPwrLmt = 0;
	u8 				rfPath = 0;

	//DBG_871X("=====> PHY_ConvertTxPowerLimitToPowerIndex()\n" );

	phy_CrossReferenceHTAndVHTTxPowerLimit( Adapter );
	
	
	phy_CrossReferenceHTAndVHTTxPowerLimit( Adapter );
	
	for ( regulation = 0; regulation < MAX_REGULATION_NUM; ++regulation )
	{
		for ( bw = 0; bw < MAX_2_4G_BANDWITH_NUM; ++bw )
		{
			for ( group = 0; group < MAX_2_4G_CHANNEL_NUM; ++group )
			{						
				for ( rateSection = 0; rateSection < MAX_RATE_SECTION_NUM; ++rateSection )
				{	
					if ( pHalData->odmpriv.PhyRegPgValueType == PHY_REG_PG_EXACT_VALUE ) {
						// obtain the base dBm values in 2.4G band
						// CCK => 11M, OFDM => 54M, HT 1T => MCS7, HT 2T => MCS15
						if ( rateSection == 0 ) { //CCK
							baseIndex2_4G = PHY_GetTxPowerByRateBaseIndex( BAND_ON_2_4G, MGN_11M );
						}
						else if ( rateSection == 1 ) { //OFDM
							baseIndex2_4G = PHY_GetTxPowerByRateBaseIndex( BAND_ON_2_4G, MGN_54M );
						}
						else if ( rateSection == 2 ) { //HT IT
							baseIndex2_4G = PHY_GetTxPowerByRateBaseIndex( BAND_ON_2_4G, MGN_MCS7 );
						}
						else if ( rateSection == 3 ) { //HT 2T
							baseIndex2_4G = PHY_GetTxPowerByRateBaseIndex( BAND_ON_2_4G, MGN_MCS15 );
						}
					}

					tempPwrLmt = pHalData->TxPwrLimit_2_4G[regulation][bw][rateSection][group][ODM_RF_PATH_A];

					for ( rfPath = ODM_RF_PATH_A; rfPath < MAX_RF_PATH_NUM; ++rfPath )
					{
						if ( pHalData->odmpriv.PhyRegPgValueType == PHY_REG_PG_EXACT_VALUE )
						{
							if ( rateSection == 3 )
								BW40PwrBasedBm2_4G = pHalData->TxPwrByRateBase2_4G[rfPath][RF_2TX][baseIndex2_4G];
							else
								BW40PwrBasedBm2_4G = pHalData->TxPwrByRateBase2_4G[rfPath][RF_1TX][baseIndex2_4G];
						}
						else
							BW40PwrBasedBm2_4G = Adapter->registrypriv.RegPowerBase * 2;

						if ( tempPwrLmt != MAX_POWER_INDEX ) {
							tempValue = tempPwrLmt - BW40PwrBasedBm2_4G;
							pHalData->TxPwrLimit_2_4G[regulation][bw][rateSection][group][rfPath] = tempValue;
						}
						
						/*DBG_871X("TxPwrLimit_2_4G[regulation %d][bw %d][rateSection %d][group %d] %d=\n\
							(TxPwrLimit in dBm %d - BW40PwrLmt2_4G[chnl group %d][rfPath %d] %d) \n",
							regulation, bw, rateSection, group, pHalData->TxPwrLimit_2_4G[regulation][bw][rateSection][group][rfPath], 
							tempPwrLmt, group, rfPath, BW40PwrBasedBm2_4G );*/
					}
				}
			}
		}
	}
	
	if ( IS_HARDWARE_TYPE_8812( Adapter ) || IS_HARDWARE_TYPE_8821( Adapter ) )
	{
		for ( regulation = 0; regulation < MAX_REGULATION_NUM; ++regulation )
		{
			for ( bw = 0; bw < MAX_5G_BANDWITH_NUM; ++bw )
			{
				for ( group = 0; group < MAX_5G_CHANNEL_NUM; ++group )
				{
					for ( rateSection = 0; rateSection < MAX_RATE_SECTION_NUM; ++rateSection )
					{	
						if ( pHalData->odmpriv.PhyRegPgValueType == PHY_REG_PG_EXACT_VALUE ) {
							// obtain the base dBm values in 5G band
							// OFDM => 54M, HT 1T => MCS7, HT 2T => MCS15, 
							// VHT => 1SSMCS7, VHT 2T => 2SSMCS7
							if ( rateSection == 1 ) { //OFDM
								baseIndex5G = PHY_GetTxPowerByRateBaseIndex( BAND_ON_5G, MGN_54M );
							}
							else if ( rateSection == 2 ) { //HT 1T
								baseIndex5G = PHY_GetTxPowerByRateBaseIndex( BAND_ON_5G, MGN_MCS7 );
							}
							else if ( rateSection == 3 ) { //HT 2T
								baseIndex5G = PHY_GetTxPowerByRateBaseIndex( BAND_ON_5G, MGN_MCS15 );
							}
							else if ( rateSection == 4 ) { //VHT 1T
								baseIndex5G = PHY_GetTxPowerByRateBaseIndex( BAND_ON_5G, MGN_VHT1SS_MCS7 );
							}
							else if ( rateSection == 5 ) { //VHT 2T
								baseIndex5G = PHY_GetTxPowerByRateBaseIndex( BAND_ON_5G, MGN_VHT2SS_MCS7 );
							}
						}

						tempPwrLmt = pHalData->TxPwrLimit_5G[regulation][bw][rateSection][group][ODM_RF_PATH_A];
	
						for ( rfPath = ODM_RF_PATH_A; rfPath < MAX_RF_PATH_NUM; ++rfPath )
						{
							if ( pHalData->odmpriv.PhyRegPgValueType == PHY_REG_PG_EXACT_VALUE )
							{
								if ( rateSection == 3 || rateSection == 5 )
									BW40PwrBasedBm5G = pHalData->TxPwrByRateBase5G[rfPath][RF_2TX][baseIndex5G];
								else
									BW40PwrBasedBm5G = pHalData->TxPwrByRateBase5G[rfPath][RF_1TX][baseIndex5G];
							}
							else
								BW40PwrBasedBm5G = Adapter->registrypriv.RegPowerBase * 2;

							if ( tempPwrLmt != MAX_POWER_INDEX ) {
								tempValue = tempPwrLmt - BW40PwrBasedBm5G;
								pHalData->TxPwrLimit_5G[regulation][bw][rateSection][group][rfPath] = tempValue;
							}
							
							/*DBG_871X("TxPwrLimit_5G[regulation %d][bw %d][rateSection %d][group %d] %d=\n\
								(TxPwrLimit in dBm %d - BW40PwrLmt5G[chnl group %d][rfPath %d] %d) \n",
								regulation, bw, rateSection, group, pHalData->TxPwrLimit_5G[regulation][bw][rateSection][group][rfPath], 
								tempPwrLmt, group, rfPath, BW40PwrBasedBm5G );*/
						}
					}
				}
			}
		}
	}
	//DBG_871X("<===== PHY_ConvertTxPowerLimitToPowerIndex()\n" );
}

VOID
PHY_InitTxPowerLimit(
	IN	PADAPTER		Adapter
	)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	u8				i, j, k, l, m;

	//DBG_871X("=====> PHY_InitTxPowerLimit()!\n" );
	
	for ( i = 0; i < MAX_REGULATION_NUM; ++i )
	{
		for ( j = 0; j < MAX_2_4G_BANDWITH_NUM; ++j )
			for ( k = 0; k < MAX_RATE_SECTION_NUM; ++k )
				for ( m = 0; m < MAX_2_4G_CHANNEL_NUM; ++m )
					for ( l = 0; l < MAX_RF_PATH_NUM; ++l )
						pHalData->TxPwrLimit_2_4G[i][j][k][m][l] = MAX_POWER_INDEX;
	}

	for ( i = 0; i < MAX_REGULATION_NUM; ++i )
    {
		for ( j = 0; j < MAX_5G_BANDWITH_NUM; ++j )
			for ( k = 0; k < MAX_RATE_SECTION_NUM; ++k )
				for ( m = 0; m < MAX_5G_CHANNEL_NUM; ++m )
					for ( l = 0; l < MAX_RF_PATH_NUM; ++l )
						pHalData->TxPwrLimit_5G[i][j][k][m][l] = MAX_POWER_INDEX;
    }
	
	//DBG_871X("<===== PHY_InitTxPowerLimit()!\n" );
}

#ifndef CONFIG_EMBEDDED_FWIMG
int
phy_ConfigMACWithParaFile(
	IN	PADAPTER	Adapter,
	IN	s8* 			pFileName
)
{
	int	rtStatus = _FAIL;

	return rtStatus;
}

int
phy_ConfigBBWithParaFile(
	IN	PADAPTER	Adapter,
	IN	s8* 			pFileName
)
{
	int	rtStatus = _SUCCESS;

	return rtStatus;
}

int
phy_ConfigBBWithPgParaFile(
	IN	PADAPTER	Adapter,
	IN	s8* 			pFileName)
{
	int	rtStatus = _SUCCESS;

	return rtStatus;
}

int
phy_ConfigBBWithMpParaFile(
	IN	PADAPTER	Adapter,
	IN	s8* 			pFileName
)
{
	int	rtStatus = _SUCCESS;

	return rtStatus;
}

int
PHY_ConfigBBWithCustomPgParaFile(
	IN	PADAPTER	Adapter,
	IN	s8 			*pFileName,
	IN	BOOLEAN		checkInit
)
{
	int	rtStatus = _SUCCESS;

	return rtStatus;
}

int
PHY_ConfigRFWithParaFile(
	IN	PADAPTER	Adapter,
	IN	s8* 			pFileName,
	IN	u8			eRFPath
)
{
	int	rtStatus = _SUCCESS;

	return rtStatus;
}

int
PHY_ConfigRFWithPowerLimitTableParaFile(
	IN	PADAPTER	Adapter,
	IN	s8*	 		pFileName
)
{
	int	rtStatus = _SUCCESS;

	return rtStatus;
}

int
PHY_ConfigRFWithCustomPowerLimitTableParaFile(
	IN	PADAPTER	Adapter,
	IN	s8*			pFileName,
	IN	BOOLEAN		checkInit
)
{
	int	rtStatus = _SUCCESS;

	return rtStatus;
}

int
PHY_ConfigRFWithTxPwrTrackParaFile(
	IN	PADAPTER		Adapter,
	IN	s8*	 			pFileName
)
{
	int	rtStatus = _SUCCESS;

	return rtStatus;
}
#endif


