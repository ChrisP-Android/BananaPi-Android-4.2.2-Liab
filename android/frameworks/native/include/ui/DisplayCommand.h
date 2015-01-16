/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _UI_DISPLAY_COMMAND_H
#define _UI_DISPLAY_COMMAND_H

#define   DISPLAYDISPATCH_MAXBUFNO      				3
#define   DISPLAYDISPATCH_STARTSAME						1
#define   DISPLAYDISPATCH_STARTCONVERT					2
#define   DISPLAYDISPATCH_NOCONVERTFB				    0xFFFF
                                        				
#define   DISPLAY_CMD_SETDISPPARA       				0
#define   DISPLAY_CMD_CHANGEDISPMODE    				1
#define   DISPLAY_CMD_OPENDISP          				2
#define   DISPLAY_CMD_CLOSEDISP         				3
#define   DISPLAY_CMD_GETHDMISTATUS     				4
#define   DISPLAY_CMD_GETTVSTATUS       				5
#define   DISPLAY_CMD_GETDISPPARA       				6
#define   DISPLAY_CMD_SETMASTERDISP     				7
#define   DISPLAY_CMD_GETMASTERDISP     				8
#define   DISPLAY_CMD_GETMAXWIDTHDISP   				9
#define   DISPLAY_CMD_GETMAXHDMIMODE    				10
#define   DISPLAY_CMD_GETDISPLAYMODE    				11
#define   DISPLAY_CMD_GETDISPCOUNT      				12
#define   DISPLAY_CMD_SETDISPMODE       				13
#define   DISPLAY_CMD_SETBACKLIGHTMODE  				14
#define   DISPLAY_CMD_SETAREAPERCENT    				15
#define   DISPLAY_CMD_GETAREAPERCENT    				16
#define   DISPLAY_CMD_SETBRIGHT         				17
#define   DISPLAY_CMD_GETBRIGHT         				18
#define   DISPLAY_CMD_SETCONTRAST       				19
#define   DISPLAY_CMD_GETCONTRAST       				20
#define   DISPLAY_CMD_SETSATURATION     				21
#define   DISPLAY_CMD_GETSATURATION     				22
#define   DISPLAY_CMD_SETHUE            				23
#define   DISPLAY_CMD_GETHUE            				24
#define   DISPLAY_CMD_ISSUPPORTHDMIMODE 				25
#define   DISPLAY_CMD_SETORIENTATION    				26
#define   DISPLAY_CMD_SET3DMODE         				27
#define   DISPLAY_CMD_SET3DLAYEROFFSET                  28
#define   DISPLAY_CMD_POSTFB                            29

#define   DISPLAY_CMD_STARTWIFIDISPLAYSEND             	0x50
#define   DISPLAY_CMD_ENDWIFIDISPLAYSEND               	0x51
#define   DISPLAY_CMD_STARTWIFIDISPLAYRECEIVE		   	0x52
#define   DISPLAY_CMD_ENDWIFIDISPLAYRECEIVE            	0x53
#define   DISPLAY_CMD_UPDATESENDCLIENT                  0x54

#define   DISPLAY_CMD_GETWIFIDISPLAYBUFHANDLE           0x55 
#define   DISPLAY_CMD_GETWIFIDISPLAYBUFWIDTH            0x56
#define   DISPLAY_CMD_GETWIFIDISPLAYBUFHEIGHT           0x57
#define   DISPLAY_CMD_GETWIFIDISPLAYBUFFORMAT           0x58
#define   DISPLAY_CMD_SETCONVERTFBID					0x59

#endif // _UI_INPUT_DISPATCHER_H
