#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_


#define B7_ALTO1	0x4000
#define B7_ALTO2  0x8000
#define B7_ALTO3	0xc000
#define B7_MASK	0x3fff

#define B6_ALTO1	0x1000
#define B6_ALTO2	0x2000
#define B6_ALTO3	0x3000
#define B6_MASK	0xcfff

#define B5_ALTO1	0x0400
#define B5_ALTO2	0x0800
#define B5_ALTO3	0x0c00
#define B5_MASK	0xf3ff

#define B4_ALTO1	0x0100
#define B4_ALTO2	0x0200
#define B4_ALTO3	0x0300
#define B4_MASK	0xfcff

#define B3_ALTO1	0x0040
#define B3_ALTO2	0x0080
#define B3_ALTO3	0x00c0
#define B3_MASK	0xff3f

#define B2_ALTO1	0x0010
#define B2_ALTO2	0x0020
#define B2_ALTO3	0x0030
#define B2_MASK	0xffcf

#define B1_ALTO1	0x0004
#define B1_ALTO2	0x0008
#define B1_ALTO3	0x000c
#define B1_MASK	0xfff3
	
#define B0_ALTO1	0x0001
#define B0_ALTO2	0x0002
#define B0_ALTO3	0x0003
#define B0_MASK	0xfffc

#define B7_ALTI 0x80
#define B6_ALTI 0x40
#define B5_ALTI 0x20
#define B4_ALTI 0x10
#define B3_ALTI 0x08
#define B2_ALTI 0x04
#define B1_ALTI 0x02
#define B0_ALTI 0x01


#define PORT_BIT0	0x004
#define PORT_BIT1	0x008
#define PORT_BIT2 0x010
#define PORT_BIT3	0x020
#define PORT_BIT4	0x040
#define PORT_BIT5	0x080
#define PORT_BIT6	0x100
#define PORT_BIT7	0x200
#define ALL_PORT_BITS 0x3fc


#define BIT_0	0x01
#define BIT_1	0x02
#define BIT_2	0x04
#define BIT_3	0x08
#define BIT_4	0x10
#define BIT_5	0x20
#define BIT_6	0x40
#define BIT_7	0x80


#define ON	1
#define OFF 0


#endif /*TYPEDEFS_H_*/
