//////////////////////////////////////////////////////////////
//
//	Copyright Artistic Licence (UK) Ltd & Wayne Howell 1999-2007
//	Author:	Wayne Howell
//	Email:	Support@ArtisticLicence.com
//      Please use discussion forum for tech support.
//       www.ArtisticLicence.com
//
//	This file contains all key defines and structures
//	 for Art-Net
//
//      Updated 21/7/01 WDH
//       New Oem codes added
//	Updated 16/8/01 WDH
//	 Firmware upload added
//	Updated 18/2/02 WDH
//	 Art-Net V1.4n updates added
//      Updated 22/5/02 to allocate ADB OEM block
//      Updated 2/8/02 to allocate Ether-Lynx OEM block
//      Updated 12/8/02 to allocate Maxxyz DMX Box
//	Updated 29/8/02 to allocate Enttec an OEM block
//	Updated 13/9/02 for V1.4M document including:
//		ArtPollServerReply
//		IP programming
//		RDM
//	Updated 26/3/03 to add IES OEM codes
//	updated 20/5/03 to add EDI code
//	updated 19/12/03 to add Goldstage code & RDM versions
//	         AL products
//	updated 12/3/04 to add LSC codes
//	updated 17/6/04 Added Invisible Rival
//	updated 14/10/04 Added Bigfoot
//      updated 12/12/04 Added packet counter defines
//
//
//////////////////////////////////////////////////////////////

#ifndef _ARTNET_H_
#define _ARTNET_H_

#include "RDM.h"

#define uchar unsigned char
#define ushort unsigned short int
#define ulong unsigned int
// 8, 16, 32 bit fields


#define NETID0 2
	// NetID when DIP Sw #4 =OFF DEFAULT
#define NETID1 10
	// NetID when DIP Sw #4 =ON

#define DefaultPort 0x1936

#define ProtocolVersion 14
	// DMX-Hub protocol version.


// OEM Codes (Byte swapped for net order)

#define OemDMXHub      	0x0000	       //	Artistic Licence DMX-Hub    (Dip 0000 or 1000)
#define OemNetgate	0x0001	       //	ADB version DMX-Hub         (Dip 0001 or 1001)
#define OemMAHub       	0x0002	       //	MA Lighting version DMX-Hub (Dip 0010 or 1010)
#define OemEtherLynx   	0x0003	       //	Artistic Licence Ether-Lynx

#define OemLewLightCv2	0x0004	       //       Awaiting details of implementation
#define OemHighEnd	0x0005	       //       Awaiting details of implementation
#define OemAvoArt2000	0x0006	       //       Dimmer 2 x DMX input

#define OemDownLink	0x0010	       //	Artistic Licence Down-Link
#define OemUpLink	0x0011	       //	Artistic Licence Up-Link
#define OemTrussLinkOp	0x0012	       //	Artistic Licence Truss-Link O/P
#define OemTrussLinkIp	0x0013	       //	Artistic Licence Truss-Link I/P
#define OemNetLinkOp	0x0014	       //	Artistic Licence Net-Link O/P
#define OemNetLinkIp	0x0015	       //	Artistic Licence Net-Link I/P
#define OemRadioLinkOp	0x0016	       //	Artistic Licence Radio-Link O/P
#define OemRadioLinkIp	0x0017	       //	Artistic Licence Radio-Link I/P

#define OemDfdDownLink 	0x0030	       //	Doug Fleenor Design Down-Link
#define OemDfdUpLink 	0x0031	       //	Doug Fleenor Design Up-Link

#define OemGdDownLink 	0x0050	       //	Goddard Design Down-Link
#define OemGdUpLink 	0x0051	       //	Goddard Design Up-Link

#define OemAdbDownLink 	0x0070	       //	ADB Down-Link
#define OemAdbUpLink 	0x0071	       //	Adb Up-Link

#define OemAdbWifiRem 	0x0072	       //	Adb WiFi remote control

// 0x0072 - 0x007f assigned to ADB.


#define OemAux0Down	0x0080	       //	Unallocated Code
#define OemAux0Up 	0x0081	       //	Unallocated Code

#define OemAux1Down	0x0082	       //	Unallocated Code
#define OemAux1Up 	0x0083	       //	Unallocated Code

#define OemAux2Down	0x0084	       //	Unallocated Code
#define OemAux2Up 	0x0085	       //	Unallocated Code

#define OemAux3Down	0x0086	       //	Unallocated Code
#define OemAux3Up 	0x0087	       //	Unallocated Code

#define OemAux4Down	0x0088	       //	Unallocated Code
#define OemAux4Up 	0x0089	       //	Unallocated Code

#define OemAux5Down	0x008a	       //	Unallocated Code
#define OemAux5Up 	0x008b	       //	Unallocated Code

#define OemZero1	0x008c	       //	Zero 88   2 port output node
#define OemZero2	0x008d	       //	Zero 88   2 port input node

#define OemPig1	 	0x008e	       //	Flying Pig  2 port output node
#define OemPig2	 	0x008f	       //	Flying Pig  2 port input node

#define OemElcNode2 	0x0090	       //	ELC 2 port node - soft i/o
#define OemElcNode4 	0x0091	       //	ELC 4 in, 4 out node
//---0x009f reserved for Elc

#define OemEtherLynx0   	0x0100	 //	Reserved for oem versions of Ether-Lynx
#define OemEtherLynx1   	0x0101	 //	Reserved for oem versions of Ether-Lynx
#define OemEtherLynx2   	0x0102	 //	Reserved for oem versions of Ether-Lynx
#define OemEtherLynx3   	0x0103	 //	Reserved for oem versions of Ether-Lynx
#define OemEtherLynx4   	0x0104	 //	Reserved for oem versions of Ether-Lynx
#define OemEtherLynx5   	0x0105	 //	Reserved for oem versions of Ether-Lynx
#define OemEtherLynx6   	0x0106	 //	Reserved for oem versions of Ether-Lynx
#define OemEtherLynx7   	0x0107	 //	Reserved for oem versions of Ether-Lynx
#define OemEtherLynx8   	0x0108	 //	Reserved for oem versions of Ether-Lynx
#define OemEtherLynx9   	0x0109	 //	Reserved for oem versions of Ether-Lynx
#define OemEtherLynxa   	0x010a	 //	Reserved for oem versions of Ether-Lynx
#define OemEtherLynxb   	0x010b	 //	Reserved for oem versions of Ether-Lynx
#define OemEtherLynxc   	0x010c	 //	Reserved for oem versions of Ether-Lynx
#define OemEtherLynxd   	0x010d	 //	Reserved for oem versions of Ether-Lynx
#define OemEtherLynxe   	0x010e	 //	Reserved for oem versions of Ether-Lynx
#define OemEtherLynxf   	0x010f	 //	Reserved for oem versions of Ether-Lynx

#define OemCataLynx   	        0x0110	 //	Reserved for oem versions of Cata-Lynx
#define OemCataLynx1   	        0x0111	 //	Reserved for oem versions of Cata-Lynx
#define OemCataLynx2   	        0x0112	 //	Reserved for oem versions of Cata-Lynx
#define OemCataLynx3   	        0x0113	 //	Reserved for oem versions of Cata-Lynx
#define OemCataLynx4   	        0x0114	 //	Reserved for oem versions of Cata-Lynx
#define OemCataLynx5   	        0x0115	 //	Reserved for oem versions of Cata-Lynx
#define OemCataLynx6   	        0x0116	 //	Reserved for oem versions of Cata-Lynx
#define OemCataLynx7   	        0x0117	 //	Reserved for oem versions of Cata-Lynx
#define OemCataLynx8   	        0x0118	 //	Reserved for oem versions of Cata-Lynx
#define OemCataLynx9   	        0x0119	 //	Reserved for oem versions of Cata-Lynx
#define OemCataLynxa   	        0x011a	 //	Reserved for oem versions of Cata-Lynx
#define OemCataLynxb   	        0x011b	 //	Reserved for oem versions of Cata-Lynx
#define OemCataLynxc   	        0x011c	 //	Reserved for oem versions of Cata-Lynx
#define OemCataLynxd   	        0x011d	 //	Reserved for oem versions of Cata-Lynx
#define OemCataLynxe   	        0x011e	 //	Reserved for oem versions of Cata-Lynx
#define OemCataLynxf   	        0x011f	 //	Reserved for oem versions of Cata-Lynx

#define OemPixiPowerF1a   	0x0120	 	// Artistic Licence Pixi-Power F1. 2 x DMX O/P (emulated) + RDM Support

#define OemMaxNode4 		0x0180       //	Maxxyz 4 in, 4 out node

#define OemEnttec0	   	0x0190	 //	Reserved for Enttec
#define OemEnttec1	   	0x0191	 //	Reserved for Enttec
#define OemEnttec2	   	0x0192	 //	Reserved for Enttec
#define OemEnttec3	   	0x0193	 //	Reserved for Enttec
#define OemEnttec4	   	0x0194	 //	Reserved for Enttec
#define OemEnttec5	   	0x0195	 //	Reserved for Enttec
#define OemEnttec6	   	0x0196	 //	Reserved for Enttec
#define OemEnttec7	   	0x0197	 //	Reserved for Enttec
#define OemEnttec8	   	0x0198	 //	Reserved for Enttec
#define OemEnttec9	   	0x0199	 //	Reserved for Enttec
#define OemEntteca	   	0x019a	 //	Reserved for Enttec
#define OemEnttecb	   	0x019b	 //	Reserved for Enttec
#define OemEnttecc	   	0x019c	 //	Reserved for Enttec
#define OemEnttecd	   	0x019d	 //	Reserved for Enttec
#define OemEnttece	   	0x019e	 //	Reserved for Enttec
#define OemEnttecf	   	0x019f	 //	Reserved for Enttec


#define OemIesPbx	   	0x01a0	 //	Ies PBX (Uses 1 logical universe)
#define OemIesExecutive	        0x01a1	 //	Ies Executive (Uses 2 logical universe)
#define OemIesMatrix   	        0x01a2	 //	Ies Matrix (Uses 2 logical universe)
#define OemIes3	   	        0x01a3	 //	Reserved for Ies
#define OemIes4	   	        0x01a4	 //	Reserved for Ies
#define OemIes5	   	        0x01a5	 //	Reserved for Ies
#define OemIes6	   	        0x01a6	 //	Reserved for Ies
#define OemIes7	   	        0x01a7	 //	Reserved for Ies
#define OemIes8	   	        0x01a8	 //	Reserved for Ies
#define OemIes9	   	        0x01a9	 //	Reserved for Ies
#define OemIesa	   	        0x01aa	 //	Reserved for Ies
#define OemIesb	   	        0x01ab	 //	Reserved for Ies
#define OemIesc	   	        0x01ac	 //	Reserved for Ies
#define OemIesd	   	        0x01ad	 //	Reserved for Ies
#define OemIese	   	        0x01ae	 //	Reserved for Ies
#define OemIesf	   	        0x01af	 //	Reserved for Ies


#define OemEdiEdig   	        0x01b0	 //	EDI Edig (4 in, 4 out)

#define OemOpenlux   	        0x01c0	 //	Nondim Enterprises Openlux (4 in, 4 out)

#define OemHippo1   	        0x01d0	 //	Green Hippo - Hippotizer (emulates 1 in)

#define OemVnrBooster  	        0x01e0	 //	VNR - Merger-Booster (4in 4out)

#define OemRobeIle  	        0x01f0	 //	RobeShow Lighting - ILE - (1in 1out)
#define OemRobeController       0x01f1	 //	RobeShow Lighting - Controller - (4in 4out)


#define OemDownLynx2	0x0210	       //	Artistic Licence Down-Lynx 2 (RDM Version)
#define OemUpLynx2	0x0211	       //	Artistic Licence Up-Lynx 2 (RDM Version)
#define OemTrussLynxOp2	0x0212	       //	Artistic Licence Truss-Lynx O/P (RDM Version)
#define OemTrussLynxIp2	0x0213	       //	Artistic Licence Truss-Lynx I/P (RDM Version)
#define OemNetLynxOp2	0x0214	       //	Artistic Licence Net-Lynx O/P (RDM Version)
#define OemNetLynxIp2	0x0215	       //	Artistic Licence Net-Lynx I/P (RDM Version)
#define OemRadioLynxOp2	0x0216	       //	Artistic Licence Radio-Lynx O/P (RDM Version)
#define OemRadioLynxIp2	0x0217	       //	Artistic Licence Radio-Lynx I/P (RDM Version)

#define OemDfdDownLynx2	0x0230	       //	Doug Fleenor Design Down-Lynx (RDM Version)
#define OemDfdUpLynx2 	0x0231	       //	Doug Fleenor Design Up-Lynx (RDM Version)

#define OemGdDownLynx2 	0x0250	       //	Goddard Design Down-Lynx (RDM Version)
#define OemGdUpLynx2 	0x0251	       //	Goddard Design Up-Lynx (RDM Version)

#define OemAdbDownLynx2	0x0270	       //	ADB Down-Lynx (RDM Version)
#define OemAdbUpLynx2 	0x0271	       //	Adb Up-Lynx (RDM Version)

#define OemLscDownLynx2	0x0280	       //	LSC Down-Lynx (RDM version)
#define OemLscUpLynx2 	0x0281	       //	LSC Up-Lynx (RDM version)

#define OemAux1Down2	0x0282	       //	Unallocated Code
#define OemAux1Up2 	0x0283	       //	Unallocated Code

#define OemAux2Down2	0x0284	       //	Unallocated Code
#define OemAux2Up2 	0x0285	       //	Unallocated Code

#define OemAux3Down2	0x0286	       //	Unallocated Code
#define OemAux3Up2 	0x0287	       //	Unallocated Code

#define OemAux4Down2	0x0288	       //	Unallocated Code
#define OemAux4Up2 	0x0289	       //	Unallocated Code

#define OemAux5Down2	0x028a	       //	Unallocated Code
#define OemAux5Up2 	0x028b	       //	Unallocated Code

#define OemGoldOp	0x0300	 	//	Goldstage DMX-net/O = 2 x dmx out
#define OemGoldIp	0x0301	 	//	Goldstage DMX-net/I = 2 x dmx in
#define OemGold2	0x0302	 	//	Goldstage
#define OemGold3	0x0303	 	//	Goldstage
#define OemGoldGT96	0x0304	 	//	Goldstage GT-96 =1 dmx out, auditorium dimmer
#define OemGoldIII  	0x0305  	//  	Goldstage III Light Concole:=1×dmx out, with remote control.
#define OemGold6	0x0306	 	//	Goldstage
#define OemGold7	0x0307	 	//	Goldstage
#define OemGoldKTG5S  	0x0308  	//  	Goldstage KTG-5S Dimmer=2×dmx in
#define OemGold9	0x0309	 	//	Goldstage
#define OemGolda	0x030a	 	//	Goldstage
#define OemGoldb	0x030b	 	//	Goldstage
#define OemGoldc	0x030c	 	//	Goldstage
#define OemGoldd	0x030d	 	//	Goldstage
#define OemGolde	0x030e	 	//	Goldstage
#define OemGoldf	0x030f	 	//	Goldstage





#define OemStarGate	0x0310		// 	Sunset Dynamics, StarGateDMX - 4 in, 4 out, no RDM.

#define	OemEthDmx8	0x0320		//	Luminex Lighting Control Equipment Luminex LCE. 4 inputs - 4 out, no rdm
#define	OemEthDmx2	0x0321		//	Luminex Lighting Control Equipment Luminex LCE. 2 inputs - 2 out, no rdm


#define	OemBlueHyst	0x0330		//	Man: Invisible Rival, Prod: Blue Hysteria, 2x DMX In, 2 x DMX out, No RDM

#define OemAvoD4Vision	0x0340	 	//	Avolites Diamond 4 Vision - 8 DMX out
#define OemAvoD4Elite	0x0341	 	//	Avolites Diamond 4 elite - 8 DMX out
#define OemAvoPearlOff	0x0342	 	//	Avolites Peal offline 4 DMX out

#define OemBfEtherMuxRem	0x0350	 	//	Bigfoot EtherMux Remote 1 DMX in
#define OemBfEtherMuxServ	0x0351	 	//	Bigfoot EtherMux Server 1 DMX in 1 DMX out
#define OemBfEtherMuxDesk	0x0352	 	//	Bigfoot EtherMux Desktop 1 DMX out

#define	OemElink512		0x0360		// Ecue 512 output device
#define	OemElink1024		0x0361		// Ecue 1024 output device
#define	OemElink2048		0x0362		// Ecue 2048 output device

#define OemKissBox              0x0370          // Kiss-Box DMX Box 1 in 1 out RDM support by utilities

#define OemArkaosVjd            0x0380          // Arkaos V J DMX. 1 x DMX in. No o/p. No RDM.

#define OemDeShowGate           0x0390          //Digital Enlightenment - ShowGate - 4x dmx in, 4x dmx out, no rdm

#define OemDesNeli              0x03a0          // DES - NELI 6,12,24 chan. 1x dmx in, 1x dmx out, no rdm

#define	OemSunLiteEsaip		0x03b0		// SunLite Easy Standalone IP 1 x DMX out
#define	OemSunLiteMagic3d	0x03b1		// SunLite Magic 3D Easy View 4 x DMX out

#define	OemHesCatalyst1	        0x03c0		// Catalyst
#define	OemRbPixelMad1	        0x03d0		// Bleasdale PixelMad

#define	OemLehighDx2	        0x03e0		// Lehigh Electric Products Co - DX2 Dimming rack - 2 in, 1 out, no rdm

#define	OemHorizon1	        0x03f0		// Horizon Controller

#define OemAudioSceneO          0x0400          // Audio Scene 2 x out + no rdm
#define OemAudioSceneI          0x0401          // Audio Scene 2 x in + no rdm

#define OemPathportO2           0x0410          // Pathport 2 x out
#define OemPathportI2           0x0411          // Pathport 2 x in 
#define OemPathportO1           0x0412          // Pathport 1 x out
#define OemPathportI1           0x0413          // Pathport 1 x in

#define	OemBotex1		0x0420		// Botex 2 in - 2 out.

#define	OemSnLibArtNet		0x0430		// Simon Newton LibArtNet. 4 in. 4 out.
#define	OemSnLlaLive		0x0431		// Simon Newton LLA Live. 4 in. 4 out.

#define	OemTeamXlntIp		0x0440		// Team Projects. DMX Input Node.
#define	OemTeamXlntOp		0x0441		// Team Projects. DMX Output Node.

#define	OemSystemNet4e		0x0450		// Schnick-Schnack-Systems Systemnetzteil 4E 4 out. No Rdm

#define	OemDomDvNetDmx		0x0460		// Dom Dv NetDmx - User configured functionality. Max 1 in / 1 out

#define	OemSeanChrisProjPal	0x0470		// Sean Christopher - Projection Pal. 1 x i/p + RDM
#define	OemSeanChrisLxRem	0x0471		// Sean Christopher - The Lighting Remote. 4 x i/p + 4 x O/P no RDM
#define OemLssMasterGate        0x0472          // LSS Lighting MasterGate. Profibus interface
#define OemLssRailController    0x0473          // LSS Lighting Rail Controller. Profibus interface


#define OemOpenClear0           0x0490          // Reserved for Open Clear
#define OemOpenClear1           0x0491
#define OemOpenClear2           0x0492
#define OemOpenClear3           0x0493
#define OemOpenClear4           0x0494
#define OemOpenClear5           0x0495
#define OemOpenClear6           0x0496
#define OemOpenClear7           0x0497
#define OemOpenClear8           0x0498
#define OemOpenClear9           0x0499
#define OemOpenCleara           0x049a
#define OemOpenClearb           0x049b
#define OemOpenClearc           0x049c
#define OemOpenCleard           0x049d
#define OemOpenCleare           0x049e
#define OemOpenClearf           0x049f          // Reserved for Open Clear

//

#define OemMa2PortNode         	0x04b0	 	// MA 2 port node, programmable i/o
#define OemMaNsp             	0x04b1          // MA Network signal processor.
#define OemMaNdp             	0x04b2          // MA Network dimmer processor
#define OemMaMaRemote         	0x04b3          // MA GrandMA network input. Single port - not configurable
#define OemMa4             	0x04b4
#define OemMa5             	0x04b5
#define OemMa6             	0x04b6
#define OemMa7             	0x04b7
#define OemMa8             	0x04b8
#define OemMa9             	0x04b9
#define OemMaa             	0x04ba
#define OemMab             	0x04bb
#define OemMac             	0x04bc
#define OemMad             	0x04bd
#define OemMae             	0x04be
#define OemMaf             	0x04bf	 	// Reserved MA


// Bit 15 hi for enhanced funtionality devices:

#define OemNetgateXT	0x8000	       //	Dip 0001 or 1001
#define OemNetPatch	0x8001	       //	Dip 0010 or 1010
#define OemDMXHubXT	0x8002	       //	Dip 0000 or 1000
#define OemFourPlay	0x8003	       //	Network version of No-Worries

#define OemUnknown	0x00ff	       //	Lo byte == ff is unknown or non-registered type
#define OemGlobal	0xffff	       //	Used by ArtTrigger for general purpose codes

// OpCodes (byte-swapped)

#define OpPoll   	        0x2000 /* Poll */
#define OpPollReply   	  	0x2100 /* ArtPollReply */
#define OpPollFpReply    	0x2200 /* Reply from Four-Play */

#define OpOutput 	       	0x5000 /* Output */
#define OpAddress 	  	0x6000 /* Program Node Settings */
#define OpInput 	        0x7000 /* Setup DMX input enables */

#define OpTodRequest        	0x8000 /* Requests RDM Discovery ToD */
#define OpTodData        	0x8100 /* Send RDM Discovery ToD */
#define OpTodControl        	0x8200 /* Control RDM Discovery */
#define OpRdm	        	0x8300 /* Non-Discovery RDM Message */
#define OpRdmSub                0x8400 /* Compressed subdevice data */

#define OpMedia	        	0x9000 /* Streaming data from media server */
#define OpMediaPatch        	0x9100 /* Coord setup to media server */
#define OpMediaControl        	0x9200 /* Control media server */
#define OpMediaControlReply    	0x9300 /* Reply from media server */

#define OpTimeSync          	0x9800 /* Time & Date synchronise nodes */
#define OpTrigger          	0x9900 /* Trigger and macro */
#define OpDirectory          	0x9a00 /* Request node file directory */
#define OpDirectoryReply       	0x9b00 /* Send file directory macro */

#define OpVideoSetup 	      0xa010 /* Setup video scan and font */
#define OpVideoPalette 	      0xa020 /* Setup colour palette */
#define OpVideoData 	      0xa040 /* Video Data */

#define OpMacMaster 	      0xf000
#define OpMacSlave 	      0xf100
#define OpFirmwareMaster      0xf200 // For sending firmware updates
#define OpFirmwareReply       0xf300 // Node reply during firmware update
#define OpFileTnMaster        0xf400 // Upload user file to node
#define OpFileFnMaster        0xf500 // Download user file from node
#define OpFileFnReply         0xf600 // Ack file packet received from node

#define OpIpProg	      0xf800 // For sending IP programming info.
#define OpIpProgReply	      0xf900 // Node reply during IP programming.


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Style codes for ArtPollReply
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define StyleNode	0	// Responder is a Node (DMX <-> Ethernet Device)
#define StyleServer	1	// Lighting console or similar
#define StyleMedia	2	// Media Server such as Video-Map, Hippotizer, RadLight, Catalyst, Pandora's Box
#define StyleRoute	3	// Data Routing device
#define StyleBackup	4	// Data backup / real time player such as Four-Play
#define StyleConfig	5	// Configuration tool such as DMX-Workshop
#define StyleVisual     6       // Visualiser

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Video Specific Defines
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define MaxWinFontChar  	64      // Max char in windows font name
#define MaxFontChar     	256     // Max char in a soft font
#define MaxFontHeight   	16      // Max scan lines per char
#define MaxColourPalette        17      // Max colour entry in palette
#define MaxVideoX       	80      // Max Char across screen
#define MaxVideoY       	50      // Max Lines down screen
#define MaxUserFiles       	400     // Max user files in a node


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Report Codes (Status reports)
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define RcDebug		0x0000	// Booted in debug mode (Only used in development
#define RcPowerOk	0x0001	// Power On Tests successful
#define RcPowerFail	0x0002	// Hardware tests failed at Power On
#define RcSocketWr1	0x0003	// Last UDP from Node failed due to truncated length,
						//  Most likely caused by a collision.
#define RcParseFail	0x0004	// Unable to identify last UDP transmission.
						// Check OpCode and packet length
#define RcUdpFail	0x0005	// Unable to open Udp Socket in last transmission attempt
#define RcShNameOk	0x0006	// Confirms that Short Name programming via ArtAddress accepted
#define RcLoNameOk	0x0007	// Confirms that Long Name programming via ArtAddress accepted
#define RcDmxError	0x0008	// DMX512 receive errors detected
#define RcDmxUdpFull	0x0009	// Ran out of internal DMX transmit buffers
				//  Most likely caused by receiving ArtDmx
				//  packets at a rate faster than DMX512 transmit
				//  speed
#define RcDmxRxFull	0x000a	// Ran out of internal DMX rx buffers
#define RcSwitchErr	0x000b	// Rx Universe switches conflict
#define RcConfigErr	0x000c	// Node's hardware configuration is wrong
#define RcDmxShort	0x000d	// DMX output is shorted
#define RcFirmFail	0x000e	// Last firmware upload failed.
#define RcUserFail	0x000f	// User attempted to change physical switches when universe switches locked


// Defines for ArtAddress Command field

#define AcNone          0       // No Action
#define AcCancelMerge   1       // The next ArtDmx packet cancels Node's merge mode
#define AcLedNormal     2       // Node front panel indicators operate normally
#define AcLedMute       3       // Node front panel indicators are muted
#define AcLedLocate     4       // Fast flash all indicators for locate
#define AcResetRxFlags  5       // Reset the receive DMX flags for errors, SI's, Text & Test packets

#define AcMergeLtp0	0x10		// Set Port 0 to merge in LTP.
#define AcMergeLtp1	0x11		// Set Port 1 to merge in LTP.
#define AcMergeLtp2	0x12		// Set Port 2 to merge in LTP.
#define AcMergeLtp3	0x13		// Set Port 3 to merge in LTP.

#define AcMergeHtp0	0x50		// Set Port 0 to merge in HTP. (Default Mode)
#define AcMergeHtp1	0x51		// Set Port 1 to merge in HTP.
#define AcMergeHtp2	0x52		// Set Port 2 to merge in HTP.
#define AcMergeHtp3	0x53		// Set Port 3 to merge in HTP.

#define AcClearOp0	0x90		// Clear all data buffers associated with output port 0
#define AcClearOp1	0x91		// Clear all data buffers associated with output port 1
#define AcClearOp2	0x92		// Clear all data buffers associated with output port 2
#define AcClearOp3	0x93		// Clear all data buffers associated with output port 3


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Indices to Packet Count Array
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define PkOpPoll                0       /* Poll #*/
#define PkOpPollReply           1       /* ArtPollReply #*/
#define PkOpPollFpReply         2       /* Reply from Four-Play (Advanced Reply) #*/
#define PkOpOutput              3       /* DMX Stream #*/
#define PkOpAddress             4       /* Program Node Settings #*/
#define PkOpInput               5       /* Setup DMX input enables #*/
#define PkOpTodRequest          6       /* Requests RDM Discovery ToD #*/
#define PkOpTodData             7       /* Send RDM Discovery ToD #*/
#define PkOpTodControl          8       /* Control RDM Discovery #*/
#define PkOpRdm                 9       /* Non-Discovery RDM Message #*/
#define PkOpMedia               10      /* Streaming data from media server #*/
#define PkOpMediaPatch          11      /* Coord setup to media server #*/
#define PkOpMediaControl        12      /* Control media server #*/
#define PkOpMediaControlReply   13      /* Reply from media server #*/
#define PkOpTimeSync            14      /* Time & Date synchronise nodes #*/
#define PkOpTrigger             15      /* Trigger and macro #*/
#define PkOpDirectory           16      /* Request node file directory #*/
#define PkOpDirectoryReply      17      /* Send file directory list #*/
#define PkOpVideoSetup          18 	/* Setup video scan and font #*/
#define PkOpVideoPalette        19 	/* Setup colour palette #*/
#define PkOpVideoData           20      /* Video Data #*/
#define PkOpFirmwareMaster      21      // For sending firmware updates #
#define PkOpFirmwareReply       22      // Node reply during firmware update #
#define PkOpFileTnMaster        23      // Upload user file to node #
#define PkOpFileFnMaster        24      // Download user file from node  #
#define PkOpFileFnReply         25      // Ack file packet received from node  #
#define PkOpIpProg              26      // For sending IP programming info. #
#define PkOpIpProgReply         27      // Node reply during IP programming. #

#define PkErrorStart            28      // DEFINES ABOVE ARE PACKETS, BELOW ARE ERRORS & WARNINGS
#define PkPacketNotUdp          28      // Warning Non UDP packet
#define PkPacketNotArtNet       29      // Warning Udp packet is not Art-Net
#define PkPacketNotIpV4         30      // Warning Non IP V4 packet received

#define PkPacketLimBroad        31      // Warning Limited broadcast Art-Net detected -------------
#define PkPacketNetIdMismatch   32      // Warning Art-Net on different NetId detected -------------
#define PkPacketUnicastArtDmx   33      // Warning Unicast ArtDmx packets detected -------------

#define PkPacketTooLong         34      // Error Udp packet was too long
#define PkPacketUnknownOpCode   35      // Error Undefined packet opcode #
#define PkPacketBadSourceUdp    36      // Error Udp source port wrong
#define PkPacketBadDestUdp      37      // Error Udp dest port wrong
#define PkPacketBadChecksumUdp  38      // Error Udp checksum is zero
#define PkOpRdmSub              39      // Compressed rdm
#define PkMax                   40




/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Data array lengths
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define MaxNumPorts 4
#define MaxExNumPorts 32
#define ShortNameLength 18
#define LongNameLength 64
#define PortNameLength 32
#define MaxDataLength 512



/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Tod Command defines for RDM discovery.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define TodFull		0	// Full Discovery.
#define TodNak		0xff	// ToD not available.

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// RDM discovery control commands.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define AtcNone		0	// No Action.
#define AtcFlush	1	// Force full discovery.

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// RDM Message Processing commands.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define ArProcess		0	// Process Message.

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Sub-Structure for IP and Port
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
typedef struct {
	uchar IP[4];	// IP address
	ushort Port;	// UDP port	BYTE-SWAPPED MANUALLY
} T_Addr;

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Poll request from Server to Node or Server to Server.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
typedef struct S_ArtPoll {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpPoll
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar TalkToMe;                 // bit 0 = not used
					//   Prev def was bit 0 = 0 if reply is broadcast
					// 		  bit 0 = 1 if reply is to server IP
					// All replies are noe broadcast as this feature caused too many
                                        // tech support calls

                                        // bit 1 = 0 then Node only replies when polled
                                        // bit 1 = 1 then Node sends reply when it needs to
	uchar Pad;
} T_ArtPoll;

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The Node records the Server's IP:Port for reply.
// The Node then returns a packet to IP:Port (on the Server's subnet)
// as follows:
//
// NB: The server must not check the length of this packet (or only check >=).
//    The packet length will almost certainly grow as Art-Net evolves.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
typedef struct S_ArtPollReply {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpPollReply
	T_Addr BoxAddr;                 // 0 if not yet configured

	uchar VersionInfoH;             // The node's current FIRMWARE VERS hi
	uchar VersionInfo;              // The node's current FIRMWARE VERS lo

	uchar SubSwitchH;               // 0 - not used yet
	uchar SubSwitch;                // from switch on front panel (0-15)
					//

	uchar OemH;
        uchar Oem;				  // Manufacturer code, bit 15 set if
						  // extended features avail

	uchar UbeaVersion;              // Firmware version of UBEA

	uchar Status;                             // bit 0 = 0 UBEA not present
                                                  // bit 0 = 1 UBEA present
						  // bit 1 = 0 Not capable of RDM (Uni-directional DMX)
						  // bit 1 = 1 Capable of RDM (Bi-directional DMX)
						  // bit 2 = 0 Booted from flash (normal boot)
						  // bit 2 = 1 Booted from ROM (possible error condition)
                                                  // bit 3 = 0 Node can only accept broadcast data
                                                  // bit 3 = 1 Node can accept broadcast or multicast data
                                                  // bit 54 = 00 Universe programming authority unknown
                                                  // bit 54 = 01 Universe programming authority set by front panel controls
                                                  // bit 54 = 10 Universe programming authority set by network
                                                  // bit 76 = 00 Indicators Normal
                                                  // bit 76 = 01 Indicators Locate
                                                  // bit 76 = 10 Indicators Mute



        ushort EstaMan;			  // Reserved for ESTA manufacturer id lo, zero for now

	uchar ShortName[ShortNameLength]; // short name defaults to IP
	uchar LongName[LongNameLength];   // long name
	uchar NodeReport[LongNameLength]; // Text feedback of Node status or errors
					  //  also used for debug info

	uchar NumPortsH;                // 0
	uchar NumPorts;                 // 4 If num i/p ports is dif to output ports, return biggest

	uchar PortTypes[MaxNumPorts];   // bit 7 is output
                                        // bit 6 is input
					// bits 0-5 are protocol number (0= DMX, 1=MIDI)
                                        // for DMX-Hub ={0xc0,0xc0,0xc0,0xc0};


        uchar GoodInput[MaxNumPorts];  	// bit 7 is data received
                                        // bit 6 is data includes test packets
                                        // bit 5 is data includes SIP's
					// bit 4 is data includes text
					// bit 3 set is input is disabled
                                        // bit 2 is receive errors
					// bit 1-0 not used, transmitted as zero.
					// Don't test for zero!

	uchar GoodOutput[MaxNumPorts]; 	// bit 7 is data is transmitting
					// bit 6 is data includes test packets
					// bit 5 is data includes SIP's
					// bit 4 is data includes text
					// bit 3 output is merging data.
					// bit 2 set if DMX output short detected on power up
					// bit 1 set if DMX output merge mode is LTP
					// bit 0 not used, transmitted as zero.

        uchar Swin[MaxNumPorts];   	// Low nibble is the port input
					// address wheel for this port. On
					// DMX-Hub, this is the front panel
					//  rotary switch.
					// Hi Nibble is the Sub-net
					// This byte shows to which of 256 universes, the
					// data belongs


	uchar Swout[MaxNumPorts];   	// Low nibble is the port output
					// address wheel for this port. On
					// DMX-Hub, this is the front panel
					//  rotary switch.
					// Hi Nibble is the Sub-net
					// This byte shows to which of 256 universes, the
					// data belongs

	uchar SwVideo;		   	// Low nibble is the value of the video
					// output channel

	uchar SwMacro;		   	// Bit 0 is Macro input 1
					// Bit 7 is Macro input 8
					// Only implemented if OEM bit 15 set.
					// Netgate XT contains this implementation
					// Any change in state of a bit will
					//  cause a transmission.
					// Server should not assume that only
					//  one bit has changed.
					// The Node is responsible for debounce

        uchar SwRemote;		   	// Bit 0 is Remote input 1
					// Bit 7 is Remote input 8
					// Only implemented if OEM bit 15 set.
					// Netgate XT contains this implementation
					// Any change in state of a bit will
					//  cause a transmission.
					// Server should not assume that only
					//  one bit has changed.
					// The Node is responsible for debounce

	uchar Spare1;              	// Spare, currently zero
	uchar Spare2;              	// Spare, currently zero
	uchar Spare3;              	// Spare, currently zero
	uchar Style;              	// Set to Style code to describe type of equipment

	uchar Mac[6];   	        // Mac Address, zero if info not available

        uchar Filler[32];               // Filler bytes, currently zero.

} T_ArtPollReply;


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The following packet is used to transfer DMX to and from the Node
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
typedef struct S_ArtDmx {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpOutput
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Sequence;			// 0 if not used, else 1-255 incrementing sequence number
	uchar Physical;                 // The physical i/p 0-3. For Debug only
	ushort Universe;                // hi nib = subnet, lo nib = wheel position
	ushort Length;			// BYTE-SWAPPED MANUALLY
	uchar Data[MaxDataLength];
} T_ArtDmx;

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The following packet is used to transfer settings from server to node
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
typedef struct S_ArtAddress {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpAddress
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			// TalkToMe position in ArtPoll
	uchar Filler2;                  // The physical i/p 0-3. For Debug only


	uchar ShortName[ShortNameLength]; // null ignored
	uchar LongName[LongNameLength];   // null ignored
	uchar Swin[MaxNumPorts];   	// Low nibble is the port input
					// address wheel for this port. On
					// DMX-Hub, this is the front panel
					//  rotary switch.
					// Bit 7 hi = use data
					// Server gets array size from ArtPoll
					//  reply

	uchar Swout[MaxNumPorts];   	// Low nibble is the port output
					// address wheel for this port. On
					// DMX-Hub, this is the front panel
					//  rotary switch.
					// Bit 7 hi = use data
					// Server gets array size from ArtPoll
					//  reply

	uchar SwSub;		   	// Low nibble is the value of the sub-net
					// switch
					// Bit 7 hi = use data

	uchar SwVideo;		   	// Low nibble is the value of the video
					// output channel
					// Bit 7 hi = use data

        uchar   Command;                // see Acxx definition list



} T_ArtAddress;


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Transmitted by Server to set or test the Node's custom IP settings.
//
// NB. This function is provided for specialist applications. Do not implement this functionality
//     unless really needed!!!
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
typedef struct S_ArtIpProg {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpIpProg
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			  // TalkToMe position in ArtPoll
	uchar Filler2;                  // The physical i/p 0-3. For Debug only
	uchar Command;			  // Bit fields as follows: (Set to zero to poll for IP info)
							// Bit 7 hi for any programming.
							// Bit 6-4 not used, set to zero.
							// Bit 3 hi = Return all three paraameters to default. (This bit takes priority).
							// Bit 2 hi = Use custom IP in this packet.
							// Bit 1 hi = Use custom Subnet Mask in this packet.
							// Bit 0 hi = Use custom Port number in this packet. 

	uchar Filler4;			  // Fill to word boundary.
	uchar ProgIpHi;			  // Use this IP if Command.Bit2
	uchar ProgIp2;
	uchar ProgIp1;
	uchar ProgIpLo;

	uchar ProgSmHi;			  // Use this Subnet Mask if Command.Bit1
	uchar ProgSm2;
	uchar ProgSm1;
	uchar ProgSmLo;

	uchar ProgPortHi;		  // Use this Port Number if Command.Bit0
	uchar ProgPortLo;

	uchar Spare1;			  // Set to zero, do not test in receiver.
	uchar Spare2;	
	uchar Spare3;	
	uchar Spare4;	
	uchar Spare5;	
	uchar Spare6;	
	uchar Spare7;
	uchar Spare8;	

} T_ArtIpProg;


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Transmitted by Node in response to ArtIpProg.
//
// NB. This function is provided for specialist applications. Do not implement this functionality
//     unless really needed!!!
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
typedef struct S_ArtIpProgReply {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpAddress
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			  // TalkToMe position in ArtPoll
	uchar Filler2;                  // The physical i/p 0-3. For Debug only
	uchar Filler3;			  // Fill to word boundary.
	uchar Filler4;			  // Fill to word boundary.

	uchar ProgIpHi;			  // The node's current IP Address
	uchar ProgIp2;
	uchar ProgIp1;
	uchar ProgIpLo;

	uchar ProgSmHi;			  // The Node's current Subnet Mask
	uchar ProgSm2;
	uchar ProgSm1;
	uchar ProgSmLo;

	uchar ProgPortHi;			  // The Node's current Port Number
	uchar ProgPortLo;

	uchar Spare1;			  // Set to zero, do not test in receiver.
	uchar Spare2;	
	uchar Spare3;	
	uchar Spare4;	
	uchar Spare5;
	uchar Spare6;	
	uchar Spare7;	
	uchar Spare8;	

} T_ArtIpProgReply;


/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// RDM Messages
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Packet sent to request that Output Gateway node reply with ArtTodData packet. This is used
// to update RDM ToD (Table of Devices) throughout the network.
//
// 
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
typedef struct S_ArtTodRequest {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpAddress
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			  // TalkToMe position in ArtPoll
	uchar Filler2;                  // The physical i/p 0-3. For Debug only

	uchar Spare1;			  // Set to zero, do not test in receiver.
	uchar Spare2;	
	uchar Spare3;	
	uchar Spare4;	
	uchar Spare5;	
	uchar Spare6;	
	uchar Spare7;
	uchar Spare8;	

	uchar Command;			  // Always set to zero. See TodXxxx defines.


	uchar AdCount;			  // Number of DMX Port Addresses encoded in message. Max 32

	uchar Address[32];		  // Array of DMX Port Addresses that require ToD update.
							// Hi Nibble = Sub-Net
							// Lo Nibble = Port Address


} T_ArtTodRequest;


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Packet is a response to ArtTodRequest and encodes changes in the Tod.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//


typedef struct S_ArtTodData {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpAddress
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			  // TalkToMe position in ArtPoll
	uchar Port;                       // The physical i/p 1-4.

	uchar Spare1;			  // Set to zero, do not test in receiver.
	uchar Spare2;
	uchar Spare3;
	uchar Spare4;	
	uchar Spare5;
	uchar Spare6;	
	uchar Spare7;	
	uchar Spare8;	

	uchar CommandResponse;		  // See TodXxxx defines.

	uchar Address;			  // DMX Universe Addresse of this ToD.
							// Hi Nibble = Sub-Net
							// Lo Nibble = Port Address


	uchar UidTotalHi;			  // Total number of UIDs in Node's ToD 
	uchar UidTotalLo;

	uchar BlockCount;			  // Counts multiple packets when UidTotal exceeds number encoded in a packet. Max 200

	uchar UidCount;			  // Number of UIDs in this packet, also array of following field.

	T_Uid Tod[200];		  // Array of 48 bit UIDs.


} T_ArtTodData;



/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Packet used to force Node into full discovery.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
typedef struct S_ArtTodControl {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpAddress
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			  // TalkToMe position in ArtPoll
	uchar Filler2;                  // The physical i/p 0-3. For Debug only

	uchar Spare1;			  // Set to zero, do not test in receiver.
	uchar Spare2;
	uchar Spare3;
	uchar Spare4;
	uchar Spare5;
	uchar Spare6;
	uchar Spare7;
	uchar Spare8;

	uchar Command;			  // Command controls Node as follows:
							// See AtcXxxx defines.

	uchar Address;			  // DMX Port Addresses of this ToD.
							// Hi Nibble = Sub-Net
							// Lo Nibble = Port Address


} T_ArtTodControl;


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Packet used for all non-discovery RDM messages.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
typedef struct S_ArtRdm {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpAddress
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			  // TalkToMe position in ArtPoll
	uchar Filler2;

	uchar Spare1;			  // Set to zero, do not test in receiver.
	uchar Spare2;
	uchar Spare3;
	uchar Spare4;
	uchar Spare5;
	uchar Spare6;
	uchar Spare7;
	uchar Spare8;

	uchar Command;			  // Process command as follows:
							// See ArXxxx defines.

	uchar Address;			  // DMX Port Addresses that should action this command.
							// Hi Nibble = Sub-Net
							// Lo Nibble = Port Address

	T_Rdm	Rdm;			  // The RDM message. Actual data length encoded as per RDM definition.


} T_ArtRdm;


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Packet used for retrieving or setting compressed RDM sub-device data from RDM Proxy nodes.
// This is primarily used to retrieve lists of sub device start addresses or sub device sensor value
//
// This packet is used for both question and answer. Contents of CommandClass defines which
/////////////////////////////////////////////////////////////////////////////////////////////////////
//

#define RdmSubPayload 512

typedef struct S_ArtRdmSub {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpAddress
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			// TalkToMe position in ArtPoll
	uchar Filler2;

	T_Uid Uid;                      // Uid of RDM device that is being queried.
	uchar Spare1;
        uchar CommandClass;             // Class of command - get / set as per rdm definitions
        ushort ParameterId;             // The command
        ushort SubDevice;               // Start device. 0 = root, 1 = first subdevice.
        ushort SubCount;                // Number of sub/devices. 0=do nothing. 1 is smallest valid range. 512 is max.
	uchar Spare2;
	uchar Spare3;
	uchar Spare4;
	uchar Spare5;
        ushort Data[RdmSubPayload];     // Data is cast as per contents.
                                        // Actual array length on the wire is dependent on Command Class:
                                        // SET or GET_RESPONSE = Data[SubCount]
                                        // GET or SET_RESPONSE = no Data[] in packet
                                        // If responder cannot reply in a simple packet, it sends multiple packets.
} T_ArtRdmSub;





/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Used to disable or enable Node DMX port inputs
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
typedef struct S_ArtInput {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpInput
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			// 0
	uchar Filler2;                  // 0
	uchar NumPortsH;                // 0
	uchar NumPorts;                 // 4 If num i/p ports is dif to output ports, return biggest


	uchar Input[MaxNumPorts];   	// Bit 0 = 0 to enable dmx input.
                                        // Bit 0 = 1 to disable input
                                        //  Power on default is enabled
                                        // Use NumPorts in ArtPollReply for array size.


} T_ArtInput;



/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Video related packet structures
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
typedef struct S_ArtVideoSetup {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpVideoSetup
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			// 0
	uchar Filler2;                  // 0

	uchar Filler3;                  // 0
        uchar Filler4;                  // 0
	uchar Control;                  // 0 = local video, 1 = use ArtVideoData

        uchar FontHeight;               // Scan lines per char. 8-16 valid

        uchar FirstFont;                // First font to program  0-255

        uchar FontRange;                // Number fonts to program  1-(255-FirstFont)

	char WinFontName[MaxWinFontChar];
                                        // nearest windows font, null if not used
	uchar FontData[MaxFontChar*MaxFontHeight];
                                   	// font data, array defined to max size


} T_ArtVideoSetup;


typedef struct S_ArtVideoPalette {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpVideoPalette
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			// 0
	uchar Filler2;                  // 0
	uchar ColourRed[MaxColourPalette];    // New red values, 0x00 - 0x3f
	uchar ColourGreen[MaxColourPalette];  // New green values, 0x00 - 0x3f
	uchar ColourBlue[MaxColourPalette];   // New blue values, 0x00 - 0x3f


} T_ArtVideoPalette;


typedef struct S_ArtVideoData {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpVideoData
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			// 0
	uchar Filler2;                  // 0
	uchar PosX;                     // Start X Coord, 0 - VideoColumns-1
        uchar PosY;                     // Start Y Coord, 0 - VideoLines-1
        uchar LenX;                     // Number of char in each line, 0 - VideoColums-PosX
        uchar LenY;                     // Number of lines, 0 - VideoLines-PosY


        ushort Data[MaxVideoX*MaxVideoY];
                                        // New character data

} T_ArtVideoData;



/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The following packet is used to upload either firmware or UBEA to the Node. This packet
//  must NOT be used with broadcast address
//
// NB. The file data is assumed to contain the S_ArtFirmwareFile header
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
typedef struct S_ArtFirmwareMaster {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpFirmware
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			  // To match ArtPoll
	uchar Filler2;                  // To match ArtPoll

      uchar Type;                     // 0x00 == First firmware packet
                                      // 0x01 == Firmware packet
                                      // 0x02 == Final firmware packet
						  // 0x03 == First UBEA packet
						  // 0x04 == UBEA packet
						  // 0x05 == Final UBEA packet

      uchar BlockId;                  // Firmware block counter 00 - ##

      uchar FirmwareLength3;         // The total number of words in the firmware plus the
                                      //  firmware header size. Eg: a 32K word upload including 530
                                      //  words of header == 0x8212. This is the file length

      uchar FirmwareLength2;
      uchar FirmwareLength1;
      uchar FirmwareLength0;

      uchar Spare[20];			  // Send as zero, receiver does not test

	ushort Data[512];               // hi / lo order. Meaning of this data is manufacturer specific


} T_ArtFirmwareMaster;

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The following packet is used to upload or download user files to or from the Node. This packet
//  must NOT be used with broadcast address
//
// NB. There is no header embedded in the file
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
typedef struct S_ArtFileMaster {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpFile
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			// To match ArtPoll
	uchar Filler2;                  // To match ArtPoll

        uchar Type;                     // 0x00 == First file packet
                                        // 0x01 == File packet
                                        // 0x02 == Final file packet

        uchar BlockId;                  // Firmware block counter 00 - ##

        uchar FileLength3;              // The total number of words in the file

        uchar FileLength2;
        uchar FileLength1;
        uchar FileLength0;

        uchar Name[14];                 // 8.3 file name null terminated

	uchar  ChecksumHi;
	uchar  ChecksumLo;

        uchar Spare[4];	                // Send as zero, receiver does not test

	ushort Data[512];               // hi / lo order. Meaning of this data is manufacturer specific


} T_ArtFileMaster;


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The following packet is used by Controller to Ack receipt of a file packet from a node. This packet
// must NOT be used with broadcast address
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
typedef struct S_ArtFileReply {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpFileFnReply
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			  // To match ArtPoll
	uchar Filler2;                  // To match ArtPoll

        uchar Type;                     // 0x00 == Last packet received OK
                                        // 0x01 == All firmware received OK
                                        // 0xff == Error in download, Controller instructing Node to abort file send

        uchar Spare[21];		// Send as zero, Node does not test


} T_ArtFileReply;



/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The following packet is used by Node to Ack receipt of a firmware or file packet from the master. This packet
// must NOT be used with broadcast address
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
typedef struct S_ArtFirmwareReply {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpFirmwareReply
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			  // To match ArtPoll
	uchar Filler2;                  // To match ArtPoll

      uchar Type;                     // 0x00 == Last packet received OK
                                      // 0x01 == All firmware received OK
                                      // 0xff == Error in upload, Node aborting transfer

      uchar Spare[21];			  // Send as zero, Server does not test


} T_ArtFirmwareReply;


// The following structure defines the header for an Art-Net node firmware upload file (.alf or .alu)

typedef struct S_ArtFirmwareFile {
	uchar  ChecksumHi;
	uchar  ChecksumLo;
	uchar  VersionInfoHi;           // not used for UBEA files
	uchar  VersionInfoLo;           // Version no of encoded file
	char   UserName[30];
	ushort OemNames[256];
	ushort Spare[254];
	uchar  Length3;                 //
	uchar  Length2;
	uchar  Length1;
	uchar  Length0;


} T_ArtFirmwareFile;

// The following structure defines the header for any RDM device firmware upload file (.ald)

typedef struct S_ArtRdmFirmwareFile {
	uchar  ChecksumHi;
	uchar  ChecksumLo;
	uchar  VersionInfoHi;
	uchar  VersionInfoLo;
	char   UserName[30];
	ushort RdmModelId[256]; 	// list of Rdm ModelId's for which this firmware is valid
	uchar  RdmManufacturerIdHi;	// Rdm Manufacturer which this firmware is valid
	uchar  RdmManufacturerIdLo;
	ushort Spare[252];
	uchar  Length3;
	uchar  Length2;
	uchar  Length1;
	uchar  Length0;


} T_ArtRdmFirmwareFile;



/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Status packet from Four-Play
//
// This is broadcast by Four-Play, Unicast by controller to program the product
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//

#define FpStatAutoRun   0       // Autobackup active - awaiting failure
#define FpStatAutoWait  1       // Autobackup active - failed
#define FpStatLoop      2       // Loop mode
#define FpStatPlay      3       // Play
#define FpStatPlayPause 4       // Play pause
#define FpStatPlayTrig  5       // Play awaiting trigger
#define FpStatRec       6       // Record
#define FpStatRecPause  7       // Record pause
#define FpStatRecTrig   8       // Record awaiting trigger
#define FpMax           9


typedef struct S_FpMac {
	uchar Name[10];
        uchar Mode;                     // trigger mode
        uchar Data1;
        uchar Data2;
} T_FpMac;


typedef struct S_ArtPollFpReply {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpPollFpReply
	T_Addr BoxAddr;                 // 0 if not yet configured

	uchar VersionInfoH;             // The node's current FIRMWARE VERS hi
	uchar VersionInfo;              // The node's current FIRMWARE VERS lo

	uchar Filler1;			// 0
	uchar Filler2;                  // 0

	uchar OemH;
        uchar Oem;			// Manufacturer code, bit 15 set if
                                        // extended features avail


        ushort EstaMan;			  // Reserved for ESTA manufacturer id lo, zero for now

	uchar ShortName[ShortNameLength];       // short name defaults to IP
	uchar LongName[LongNameLength];         // long name
	uchar StatusReport[LongNameLength];     // same as vdu status line

        uchar Flags;                    // bit 7 = 1 for master time sync
        uchar Prog;                     // == 0x81 to program, == 0x00 for information.

        uchar NumPorts;                  // number of ports implemented

	uchar PortStat[MaxExNumPorts];    // As per Stat defines

	uchar RxSource[MaxExNumPorts];    // bit 1 = 0 for DMX, = 1 for Art-Net

	uchar RxUniverse[MaxExNumPorts];  // sub-net and universe for each input

	uchar TxDest[MaxExNumPorts];      // bit 1 = 1 to enable network output

	uchar TxUniverse[MaxExNumPorts];  // sub-net and universe for each output

        uchar TrigChanHi;

        uchar TrigChanLo;

        uchar ElapsedTime[4];           // Hi byte first. Elapsed time in mS

        uchar CurrentFileTime[4];       // Hi byte first. Length of selected file in mS

        uchar TimeCode[4];              // Hi byte first. Received timecode

        uchar CurrentFile;

        uchar BootMode;

        uchar BootFile;

        uchar PanelLock;                // Bit 0=1 to lock out front panel

        uchar MidiChannel;

        uchar MidiNote;

        uchar RemoteMode;               // Bit 0=1 to disable remote trigger until file playback completed

	T_FpMac Macro[16];              // macro and remotes

        uchar Filler[32];               // Filler bytes, currently zero.

} T_ArtPollFpReply;



/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Time & Data Sync
//
// This is used to time sync all Art-Net nodes on network
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//

typedef struct S_ArtTimeSync {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpInput
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			// 0
	uchar Filler2;                  // 0

        uchar Prog;             /* ==0xaa to program settings, ==0x00 for information */

        uchar tm_sec;           /* Seconds */
        uchar tm_min;           /* Minutes */
        uchar tm_hour;          /* Hour (0--23) */
        uchar tm_mday;          /* Day of month (1--31) */
        uchar tm_mon;           /* Month (0--11) */
        uchar tm_year_hi;       /* Year (calendar year minus 1900) */
        uchar tm_year_lo;
        uchar tm_wday;          /* Weekday (0--6; Sunday = 0) */
        uchar tm_isdst;         /* 0 if daylight savings time is not in effect) */


} T_ArtTimeSync;


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Keyboard / Trigger Info
//
// This is used to send remote key presses and triggers to network devices
//
// Transmit unicast for a specific node, or broadcast for all nodes.
//
// Receiving node must parse OemType. The data definitions of the key information
//  are product type specific based on this code.
//
// OemCode == OemGlobal defines the global command set
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//

// The following defines relate to the Key field when OemCode==OemGlobal

#define KeyAscii        0       //SubKey is an ASCII code to be processed. Data not used
#define KeyMacro        1       //SubKey is an Macro to be processed. Data not used
#define KeySoft         2       //SubKey is a product soft key
#define KeyAbsolute     3       //SubKey is an absolute show number to run

#define KeyPlay         10      //Device start playing current file
#define KeyPause        11      //Device pause current playback or record
#define KeyContinue     12      //Device continue current playback or record from current location
#define KeyPlayTrig     13      //Device enter play pause and await trigger
#define KeyRec          14      //Device start recording to current file
#define KeyRecTrig      15      //Device enter record pause and await trigger
#define KeyScanRev      16      //Scan reverse by number of frames defined in Data[0], enter play pause
#define KeyScanFwd      17      //Scan forward by number of frames defined in Data[0], enter play pause
#define KeyLocate       18      //Locate frames defined in Data[0]msb->Data[7]lsb, enter play pause
#define KeyEtoE         19      //Switch inputs to outputs configuration. Data[x] defines loop through state
                                //where first array entry is first DMX universe. 1= loop, 0=playback
#define KeyStop         20      //Device stop playback or record
#define KeyAutoRun      21      //Run in automatic mode


#define KeyCursUp        100     //Cursor Up
#define KeyCursDown      101     //Cursor Down
#define KeyCursLeft      102     //Cursor Left
#define KeyCursRight     103     //Cursor Right

typedef struct S_ArtTrigger {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpTrigger
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			// 0
	uchar Filler2;                  // 0

	uchar OemH;
        uchar Oem;			// == 0xffff for the global command set


        uchar Key;                      // Key code
        uchar SubKey;
        uchar Data[64];

} T_ArtTrigger;


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Directory data
//
// This is used to retrieve or set file information from a node
//
// Transmit unicast for a specific node, or broadcast for all nodes.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//

#define AdComDir        0       //request directory info
#define AdComGet        1       //request file download

typedef struct S_ArtDirectory {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpDirectory
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			// 0
	uchar Filler2;                  // 0

        uchar Command;
	uchar FileHi;
        uchar FileLo;			// File number requested or == 0xffff for all file data

} T_ArtDirectory;

typedef struct S_ArtDirectoryReply {
	uchar ID[8];                    // protocol ID = "Art-Net"
	ushort OpCode;                  // == OpDirectoryReply
	uchar VersionH;                 // 0
	uchar Version;                  // protocol version, set to ProtocolVersion
	uchar Filler1;			// 0
	uchar Filler2;                  // 0
        uchar Flags;                    // bit 7 = 1 if files exists

	uchar FileHi;
        uchar FileLo;			// File number: number consecutively from zero

        char Name83[16];               // 8.3 file name. Null if not used. Null terminated
        char Description[64];          // File description. Null if not used. Null terminated
        uchar Length[8];                // File length. Hi byte first. This is the byte count.
        uchar Data[64];                 // User data

} T_ArtDirectoryReply;


typedef struct S_ArtDirectoryList {
	T_ArtDirectoryReply  Dir[MaxUserFiles];   // List of all user files available from node

} T_ArtDirectoryList;




/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Strand Show-Net Deinitions
//
// Note: This information is taken from a combination of:
//
//       A) The original ShowNet document that was released by Strand.
//       B) Empirical tests on actual product.
//
// It should be noted that these two sources of information conflict!
// As Strand has decided not to publish the latest specification, information
//  is limited.
//
// Artistic Licence acknowledge all trade marks
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
//
typedef struct S_ShowDmx {
	uchar SignatureHi;              // protocol ID = 0x80
        uchar SignatureLo;              // protocol ID = 0x8f
	uchar IP[4];	                // IP address, first entry in array is MSB.
        ushort NetSlot[4];              // The packet can contain up to 4 non contiguous blocks, this is the start channel of each.
        ushort SlotSize[4];             // The number of channels in each block, 0 if block not used
        ushort IndexBlock[5];           // Index into Data for each block of data
        uchar PacketCountHi;            // Packet count
        uchar PacketCountLo;            // Packet count
        uchar BlockD[4];                // No idea!
        uchar Name[9];                  // name of console
        uchar Data[MaxDataLength];
} T_ShowDmx;



typedef struct S_Udp {
	ushort  SourcePort;
        ushort  DestPort;
        ushort  Length;
        ushort  Checksum;
        unsigned char UdpData[1500];
        }T_Udp;


typedef struct {
  AnsiString SourceAddress;
  int SourcePort;
  AnsiString DestinationAddress;
  int DestinationPort;
  int IPVersion;
  int TOS;
  int Id;
  int Flags;
  int Offset;
  int TTL;
  int Checksum;
  int IPProtocol;
  AnsiString Payload;
} T_NetPacket;



#endif


