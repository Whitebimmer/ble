
#ifndef _BT16_
#define _BT16_

#define hs_base     0x78000
#define ls_base     0x70000


#define SFR(sfr, start, len, dat) (sfr = (sfr & ~((~(0xffffffff << len)) << start)) | ((dat & (~(0xffffffff << len))) << start))

//===============================================================================//
//
//      high speed sfr address define
//
//===============================================================================//
//........ 0x000 - 0x03f........
//

//........ 0x040 - 0x07f........
#define DSPCON                  (*(volatile unsigned long  *)(hs_base + 0x10*4))    //
#define DSP_ILAT                (*(volatile unsigned long  *)(hs_base + 0x11*4))    //
#define DSP_SET_ILAT            (*(volatile unsigned long  *)(hs_base + 0x12*4))    // Write only
#define DSP_CLR_ILAT            (*(volatile unsigned long  *)(hs_base + 0x13*4))    // Write only
#define IPCON0                  (*(volatile unsigned long  *)(hs_base + 0x14*4))    //
#define IPCON1                  (*(volatile unsigned long  *)(hs_base + 0x15*4))    //
#define IPCON2                  (*(volatile unsigned long  *)(hs_base + 0x16*4))    //
//

//........ 0x080 - 0x0bf........
#define SFC_CON                 (*(volatile unsigned long  *)(hs_base + 0x20*4))    //
#define SFC_BAUD                (*(volatile unsigned char  *)(hs_base + 0x21*4))    // Write only
//
#define SFC_BASE_ADR            (*(volatile unsigned long  *)(hs_base + 0x23*4))    // Write only
#define ENC_CON                 (*(volatile unsigned char  *)(hs_base + 0x24*4))    //
#define ENC_KEY                 (*(volatile unsigned short *)(hs_base + 0x25*4))    // Write only
#define ENC_ADR                 (*(volatile unsigned short *)(hs_base + 0x26*4))    // Write only

//........ 0x0c0 - 0x0ff........ 
#define SRC_CON0                (*(volatile unsigned long  *)(hs_base + 0x30*4))    // 
#define SRC_CON1                (*(volatile unsigned long  *)(hs_base + 0x31*4))    // 
#define SRC_CON2                (*(volatile unsigned long  *)(hs_base + 0x32*4))    // 
#define SRC_CON3                (*(volatile unsigned long  *)(hs_base + 0x33*4))    // 
#define SRC_IDAT_ADR            (*(volatile unsigned long  *)(hs_base + 0x34*4))    // 
#define SRC_IDAT_LEN            (*(volatile unsigned long  *)(hs_base + 0x35*4))    // 
#define SRC_ODAT_ADR            (*(volatile unsigned long  *)(hs_base + 0x36*4))    // 
#define SRC_ODAT_LEN            (*(volatile unsigned long  *)(hs_base + 0x37*4))    // 
#define SRC_FLTB_ADR            (*(volatile unsigned long  *)(hs_base + 0x38*4))    // 
 
//........ 0x100 - 0x13f........

//........ 0x140 - 0x17f........
#define RFI_CON                 (*(volatile unsigned char  *)(hs_base + 0x50*4))    //
#define PD_CON                  (*(volatile unsigned short *)(hs_base + 0x51*4))    //
 
//........ 0x180 - 0x1bf........ 
#define AES_CON                 (*(volatile unsigned long  *)(hs_base + 0x60*4))    
#define AES_DAT0                (*(volatile unsigned long  *)(hs_base + 0x61*4)) 
#define AES_DAT1                (*(volatile unsigned long  *)(hs_base + 0x62*4)) 
#define AES_DAT2                (*(volatile unsigned long  *)(hs_base + 0x63*4)) 
#define AES_DAT3                (*(volatile unsigned long  *)(hs_base + 0x64*4)) 
#define AES_KEY                 (*(volatile unsigned long  *)(hs_base + 0x65*4))    // write only 

//........ 0x300 - 0x33f........
#define DSP_PC_LIMH             (*(volatile unsigned long  *)(hs_base + 0xc0*4))    // for debug only
#define DSP_PC_LIML             (*(volatile unsigned long  *)(hs_base + 0xc1*4))    // for debug only
#define DSP_BF_CON              (*(volatile unsigned long  *)(hs_base + 0xc2*4))    // for debug only
#define DSP_EX_LIMH             (*(volatile unsigned long  *)(hs_base + 0xc3*4))    // for debug only
#define DSP_EX_LIML             (*(volatile unsigned long  *)(hs_base + 0xc4*4))    // for debug only
#define DEBUG_MSG               (*(volatile unsigned long  *)(hs_base + 0xc5*4))    // for debug only
#define DEBUG_MSG_CLR           (*(volatile unsigned long  *)(hs_base + 0xc6*4))    // for debug only
#define DEBUG_WR_EN             (*(volatile unsigned long  *)(hs_base + 0xc7*4))    // for debug only
#define PRP_EX_LIMH             (*(volatile unsigned long  *)(hs_base + 0xc8*4))    // for debug only
#define PRP_EX_LIML             (*(volatile unsigned long  *)(hs_base + 0xc9*4))    // for debug only
//


//===============================================================================//
//
//      low speed sfr address define
//
//===============================================================================//
//........ 0x000 - 0x03f........
#define PORTA_OUT               (*(volatile unsigned short *)(ls_base + 0x00*4))    //
#define PORTA_IN                (*(volatile unsigned short *)(ls_base + 0x01*4))    // Read Only
#define PORTA_DIR               (*(volatile unsigned short *)(ls_base + 0x02*4))    //
#define PORTA_DIE               (*(volatile unsigned short *)(ls_base + 0x03*4))    //
#define PORTA_PU                (*(volatile unsigned short *)(ls_base + 0x04*4))    //
#define PORTA_PD                (*(volatile unsigned short *)(ls_base + 0x05*4))    //
#define PORTA_HD                (*(volatile unsigned short *)(ls_base + 0x06*4))    //
#define IOMC0                   (*(volatile unsigned short *)(ls_base + 0x07*4))    //
#define PORTB_OUT               (*(volatile unsigned short *)(ls_base + 0x08*4))    //
#define PORTB_IN                (*(volatile unsigned short *)(ls_base + 0x09*4))    // Read Only
#define PORTB_DIR               (*(volatile unsigned short *)(ls_base + 0x0a*4))    //
#define PORTB_DIE               (*(volatile unsigned short *)(ls_base + 0x0b*4))    //
#define PORTB_PU                (*(volatile unsigned short *)(ls_base + 0x0c*4))    //
#define PORTB_PD                (*(volatile unsigned short *)(ls_base + 0x0d*4))    //
#define PORTB_HD                (*(volatile unsigned short *)(ls_base + 0x0e*4))    //
#define IOMC1                   (*(volatile unsigned short *)(ls_base + 0x0f*4))    //

//........ 0x040 - 0x07f........
#define PORTC_OUT               (*(volatile unsigned short *)(ls_base + 0x10*4))    //
#define PORTC_IN                (*(volatile unsigned short *)(ls_base + 0x11*4))    // Read Only
#define PORTC_DIR               (*(volatile unsigned short *)(ls_base + 0x12*4))    //
#define PORTC_DIE               (*(volatile unsigned short *)(ls_base + 0x13*4))    //
#define PORTC_PU                (*(volatile unsigned short *)(ls_base + 0x14*4))    //
#define PORTC_PD                (*(volatile unsigned short *)(ls_base + 0x15*4))    //
#define PORTC_HD                (*(volatile unsigned short *)(ls_base + 0x16*4))    //
#define WKUP_CON0               (*(volatile unsigned short *)(ls_base + 0x17*4))    //
#define PORTD_OUT               (*(volatile unsigned char  *)(ls_base + 0x18*4))    //
#define PORTD_IN                (*(volatile unsigned char  *)(ls_base + 0x19*4))    // Read Only
#define PORTD_DIR               (*(volatile unsigned char  *)(ls_base + 0x1a*4))    //
#define PORTD_DIE               (*(volatile unsigned char  *)(ls_base + 0x1b*4))    //
#define PORTD_PU                (*(volatile unsigned char  *)(ls_base + 0x1c*4))    //
#define PORTD_PD                (*(volatile unsigned char  *)(ls_base + 0x1d*4))    //
#define PORTD_HD                (*(volatile unsigned char  *)(ls_base + 0x1e*4))    //
#define WKUP_CON1               (*(volatile unsigned short *)(ls_base + 0x1f*4))    //

//........ 0x080 - 0x0bf........
#define TMR0_CON                (*(volatile unsigned short *)(ls_base + 0x20*4))    //
#define TMR0_CNT                (*(volatile unsigned short *)(ls_base + 0x21*4))    //
#define TMR0_PRD                (*(volatile unsigned short *)(ls_base + 0x22*4))    //
#define TMR0_PWM                (*(volatile unsigned short *)(ls_base + 0x23*4))    //
#define TMR1_CON                (*(volatile unsigned short *)(ls_base + 0x24*4))    //
#define TMR1_CNT                (*(volatile unsigned short *)(ls_base + 0x25*4))    //
#define TMR1_PRD                (*(volatile unsigned short *)(ls_base + 0x26*4))    //
#define TMR1_PWM                (*(volatile unsigned short *)(ls_base + 0x27*4))    //
#define TMR2_CON                (*(volatile unsigned short *)(ls_base + 0x28*4))    //
#define TMR2_CNT                (*(volatile unsigned short *)(ls_base + 0x29*4))    //
#define TMR2_PRD                (*(volatile unsigned short *)(ls_base + 0x2a*4))    //
#define TMR2_PWM                (*(volatile unsigned short *)(ls_base + 0x2b*4))    //
#define TMR3_CON                (*(volatile unsigned short *)(ls_base + 0x2c*4))    //
#define TMR3_CNT                (*(volatile unsigned short *)(ls_base + 0x2d*4))    //
#define TMR3_PRD                (*(volatile unsigned short *)(ls_base + 0x2e*4))    //
#define TMR3_PWM                (*(volatile unsigned short *)(ls_base + 0x2f*4))    //

//........ 0x0c0 - 0x0ff........
#define SD0_CON0                (*(volatile unsigned short *)(ls_base + 0x30*4))    //
#define SD0_CON1                (*(volatile unsigned short *)(ls_base + 0x31*4))    //
#define SD0_CON2                (*(volatile unsigned short *)(ls_base + 0x32*4))    //
#define SD0_CPTR                (*(volatile unsigned long  *)(ls_base + 0x33*4))    // Write Only
#define SD0_DPTR                (*(volatile unsigned long  *)(ls_base + 0x34*4))    // Write Only
#define SD1_CON0                (*(volatile unsigned short *)(ls_base + 0x35*4))    //
#define SD1_CON1                (*(volatile unsigned short *)(ls_base + 0x36*4))    //
#define SD1_CON2                (*(volatile unsigned short *)(ls_base + 0x37*4))    //
#define SD1_CPTR                (*(volatile unsigned long  *)(ls_base + 0x38*4))    // Write Only
#define SD1_DPTR                (*(volatile unsigned long  *)(ls_base + 0x39*4))    // Write Only
#define IIC_CON                 (*(volatile unsigned short *)(ls_base + 0x3a*4))    //
#define IIC_BUF                 (*(volatile unsigned char  *)(ls_base + 0x3b*4))    //
#define IIC_BAUD                (*(volatile unsigned char  *)(ls_base + 0x3c*4))    // Write Only
#define IRFLT_CON               (*(volatile unsigned char  *)(ls_base + 0x3d*4))    //
#define WDT_CON                 (*(volatile unsigned char  *)(ls_base + 0x3e*4))    //
#define OSA_CON                 (*(volatile unsigned char  *)(ls_base + 0x3f*4))    //

//........ 0x100 - 0x13f........
#define SPI0_CON                (*(volatile unsigned short *)(ls_base + 0x40*4))    //
#define SPI0_BAUD               (*(volatile unsigned short *)(ls_base + 0x41*4))    // Write Only
#define SPI0_BUF                (*(volatile unsigned char  *)(ls_base + 0x42*4))    //
#define SPI0_ADR                (*(volatile unsigned long  *)(ls_base + 0x43*4))    // Write Only
#define SPI0_CNT                (*(volatile unsigned long  *)(ls_base + 0x44*4))    // Write Only
#define SPI1_CON                (*(volatile unsigned short *)(ls_base + 0x45*4))    //
#define SPI1_BAUD               (*(volatile unsigned short *)(ls_base + 0x46*4))    // Write Only
#define SPI1_BUF                (*(volatile unsigned char  *)(ls_base + 0x47*4))    //
#define SPI1_ADR                (*(volatile unsigned long  *)(ls_base + 0x48*4))    // Write Only
#define SPI1_CNT                (*(volatile unsigned long  *)(ls_base + 0x49*4))    // Write Only
#define PAP_CON                 (*(volatile unsigned long  *)(ls_base + 0x4a*4))    //
#define PAP_DAT0                (*(volatile unsigned short *)(ls_base + 0x4b*4))    // Write Only
#define PAP_DAT1                (*(volatile unsigned short *)(ls_base + 0x4c*4))    // Write Only
#define PAP_BUF                 (*(volatile unsigned short *)(ls_base + 0x4d*4))    //
#define PAP_BUF8                (*(volatile unsigned char  *)(ls_base + 0x4d*4))    //
#define PAP_ADR                 (*(volatile unsigned long  *)(ls_base + 0x4e*4))    // Write Only
#define PAP_CNT                 (*(volatile unsigned long  *)(ls_base + 0x4f*4))    // Write Only

//........ 0x140 - 0x17f........
#define UT0_CON                 (*(volatile unsigned short *)(ls_base + 0x50*4))    //
#define UT0_BAUD                (*(volatile unsigned short *)(ls_base + 0x51*4))    // Write Only
#define UT0_BUF                 (*(volatile unsigned char  *)(ls_base + 0x52*4))    //
#define UT0_OT                  (*(volatile unsigned long  *)(ls_base + 0x53*4))    //
#define UT0_DMA_RD_ADR          (*(volatile unsigned long  *)(ls_base + 0x54*4))    //
#define UT0_DMA_RD_CNT          (*(volatile unsigned short *)(ls_base + 0x55*4))    //
#define UT0_DMA_WR_SADR         (*(volatile unsigned long  *)(ls_base + 0x56*4))    //
#define UT0_DMA_WR_EADR         (*(volatile unsigned long  *)(ls_base + 0x57*4))    //
#define UT0_DMA_WR_CNT          (*(volatile unsigned long  *)(ls_base + 0x58*4))    //
#define EFUSE_CON               (*(volatile unsigned short *)(ls_base + 0x59*4))    //
#define ADC_CON                 (*(volatile unsigned short *)(ls_base + 0x5a*4))    //
#define ADC_RES                 (*(volatile unsigned short *)(ls_base + 0x5b*4))    // Read Only
#define CRC_FIFO                (*(volatile unsigned char  *)(ls_base + 0x5c*4))    // Write Only
#define CRC_REG                 (*(volatile unsigned short *)(ls_base + 0x5d*4))    //
#define WKUP_CON2               (*(volatile unsigned short *)(ls_base + 0x5e*4))    //
#define WKUP_CON3               (*(volatile unsigned short *)(ls_base + 0x5f*4))    //
 
//........ 0x180 - 0x1bf........ 
#define DAA_CON0                (*(volatile unsigned short *)(ls_base + 0x60*4))    // 
#define DAA_CON1                (*(volatile unsigned short *)(ls_base + 0x61*4))    // 
#define DAA_CON2                (*(volatile unsigned short *)(ls_base + 0x62*4))    // 
#define DAA_CON3                (*(volatile unsigned short *)(ls_base + 0x63*4))    // 
#define DAA_CON4                (*(volatile unsigned char  *)(ls_base + 0x64*4))    // 
#define DAC_LEN                 (*(volatile unsigned short *)(ls_base + 0x65*4))    // Write Only 
#define LADC_LEN                (*(volatile unsigned short *)(ls_base + 0x66*4))    // Write Only 
#define ADC12_RES               (*(volatile unsigned short *)(ls_base + 0x67*4))    // Read Only 
#define DAC_CON                 (*(volatile unsigned short *)(ls_base + 0x68*4))    //
#define DAC_ADR                 (*(volatile unsigned long  *)(ls_base + 0x69*4))    // Write Only
#define LADC_CON                (*(volatile unsigned short *)(ls_base + 0x6a*4))    //
#define LADC_CON1               (*(volatile unsigned short *)(ls_base + 0x6b*4))    //
#define LADC_CON2               (*(volatile unsigned short *)(ls_base + 0x6c*4))    //
#define LADC_ADR                (*(volatile unsigned long  *)(ls_base + 0x6d*4))    // Write Only
#define DAC_TRML                (*(volatile unsigned char  *)(ls_base + 0x6e*4))    // Write Only
#define DAC_TRMR                (*(volatile unsigned char  *)(ls_base + 0x6f*4))    // Write Only

//........ 0x1c0 - 0x1ff........
#define USB_CON0                (*(volatile unsigned long  *)(ls_base + 0x70*4))    //
#define USB_CON1                (*(volatile unsigned long  *)(ls_base + 0x71*4))    //
#define USB_EP0_CNT             (*(volatile unsigned short *)(ls_base + 0x72*4))    // Write Only
#define USB_EP1_CNT             (*(volatile unsigned short *)(ls_base + 0x73*4))    // Write Only
#define USB_EP2_CNT             (*(volatile unsigned short *)(ls_base + 0x74*4))    // Write Only
#define USB_EP3_CNT             (*(volatile unsigned short *)(ls_base + 0x75*4))    // Write Only
#define USB_EP0_ADR             (*(volatile unsigned long  *)(ls_base + 0x76*4))    // Write Only
#define USB_EP1_TADR            (*(volatile unsigned long  *)(ls_base + 0x77*4))    // Write Only
#define USB_EP1_RADR            (*(volatile unsigned long  *)(ls_base + 0x78*4))    // Write Only
#define USB_EP2_TADR            (*(volatile unsigned long  *)(ls_base + 0x79*4))    // Write Only
#define USB_EP2_RADR            (*(volatile unsigned long  *)(ls_base + 0x7a*4))    // Write Only
#define USB_EP3_TADR            (*(volatile unsigned long  *)(ls_base + 0x7b*4))    // Write Only
#define USB_EP3_RADR            (*(volatile unsigned long  *)(ls_base + 0x7c*4))    // Write Only
#define PWM4_CON                (*(volatile unsigned char  *)(ls_base + 0x7d*4))    // Write Only
#define IRTC_CON                (*(volatile unsigned short *)(ls_base + 0x7e*4))    //
 
//........ 0x200 - 0x23f........ 
#define USB_IO_CON              (*(volatile unsigned short *)(ls_base + 0x80*4))    //
#define NFC_CON0                (*(volatile unsigned long  *)(ls_base + 0x81*4))    //
#define NFC_CON1                (*(volatile unsigned long  *)(ls_base + 0x82*4))    //
#define NFC_CON2                (*(volatile unsigned long  *)(ls_base + 0x83*4))    //
#define NFC_BUF0                (*(volatile unsigned long  *)(ls_base + 0x84*4))    //
#define NFC_BUF1                (*(volatile unsigned long  *)(ls_base + 0x85*4))    //
#define NFC_BUF2                (*(volatile unsigned long  *)(ls_base + 0x86*4))    // Write Only 
#define NFC_BUF3                (*(volatile unsigned long  *)(ls_base + 0x87*4))    // Write Only 
// 
#define IOMC2                   (*(volatile unsigned long  *)(ls_base + 0x8a*4))    //
#define IOMC3                   (*(volatile unsigned short *)(ls_base + 0x8b*4))    //
#define PLCNTCON                (*(volatile unsigned char  *)(ls_base + 0x8c*4))    //
#define PLCNTVL                 (*(volatile unsigned short *)(ls_base + 0x8d*4))    //

//........ 0x240 - 0x27f........
#define UT1_CON                 (*(volatile unsigned short *)(ls_base + 0x90*4))    //
#define UT1_BAUD                (*(volatile unsigned short *)(ls_base + 0x91*4))    // Write Only
#define UT1_BUF                 (*(volatile unsigned char  *)(ls_base + 0x92*4))    //
#define UT1_OT                  (*(volatile unsigned long  *)(ls_base + 0x93*4))    //
#define UT1_DMA_RD_ADR          (*(volatile unsigned long  *)(ls_base + 0x94*4))    // Write Only
#define UT1_DMA_RD_CNT          (*(volatile unsigned short *)(ls_base + 0x95*4))    //
#define UT1_DMA_WR_SADR         (*(volatile unsigned long  *)(ls_base + 0x96*4))    // Write Only
#define UT1_DMA_WR_EADR         (*(volatile unsigned long  *)(ls_base + 0x97*4))    // Write Only
#define UT1_DMA_WR_CNT          (*(volatile unsigned long  *)(ls_base + 0x98*4))    //
#define UT2_CON                 (*(volatile unsigned short *)(ls_base + 0x99*4))    //
#define UT2_BUF                 (*(volatile unsigned char  *)(ls_base + 0x9a*4))    //
#define UT2_BAUD                (*(volatile unsigned short *)(ls_base + 0x9b*4))    // Write Only
//

//........ 0x280 - 0x2bf........
#define PWR_CON                 (*(volatile unsigned char  *)(ls_base + 0xa0*4))    //
#define CLK_CON0                (*(volatile unsigned short *)(ls_base + 0xa1*4))    //
#define CLK_CON1                (*(volatile unsigned long  *)(ls_base + 0xa2*4))    //
#define CLK_CON2                (*(volatile unsigned long  *)(ls_base + 0xa3*4))    //
#define SYS_DIV                 (*(volatile unsigned short *)(ls_base + 0xa4*4))    //
//
#define PLL_CON                 (*(volatile unsigned long  *)(ls_base + 0xa7*4))    //
#define CHIP_ID                 (*(volatile unsigned short *)(ls_base + 0xa8*4))    // Read Only
#define LDO_CON                 (*(volatile unsigned short *)(ls_base + 0xa9*4))    //
//
#define LVD_CON                 (*(volatile unsigned short *)(ls_base + 0xab*4))    // 
#define MODE_CON                (*(volatile unsigned char  *)(ls_base + 0xac*4))    // 
// 
#define RAND64L                 (*(volatile unsigned char  *)(ls_base + 0xae*4))    // Read Only
#define RAND64H                 (*(volatile unsigned char  *)(ls_base + 0xaf*4))    // Read Only
 
//........ 0x2c0 - 0x2ff........ 
#define ALNK_CON0               (*(volatile unsigned short *)(ls_base + 0xb0*4))    //
#define ALNK_CON1               (*(volatile unsigned short *)(ls_base + 0xb1*4))    //
#define ALNK_CON2               (*(volatile unsigned short *)(ls_base + 0xb2*4))    //
#define ALNK_ADR0               (*(volatile unsigned long  *)(ls_base + 0xb3*4))    // Write Only 
#define ALNK_ADR1               (*(volatile unsigned long  *)(ls_base + 0xb4*4))    // Write Only 
#define ALNK_ADR2               (*(volatile unsigned long  *)(ls_base + 0xb5*4))    // Write Only 
#define ALNK_ADR3               (*(volatile unsigned long  *)(ls_base + 0xb6*4))    // Write Only 
#define ALNK_LEN                (*(volatile unsigned short *)(ls_base + 0xb7*4))    // Write Only 
 
//........ 0x300 - 0x33f........ 
#define LCDC_CON0               (*(volatile unsigned short *)(ls_base + 0xc0*4))    // 
//
#define SEG_IOEN0               (*(volatile unsigned short *)(ls_base + 0xc2*4))    //
#define SEG_IOEN1               (*(volatile unsigned short *)(ls_base + 0xc3*4))    //

//........ 0x340 - 0x37f........

//........ 0x380 - 0x3bf........

//........ 0x3c0 - 0x3ff........


//...........  Full Speed USB .....................
#define FADDR                   0x00
#define POWER                   0x01
#define INTRTX1                 0x02
#define INTRTX2                 0x03
#define INTRRX1                 0x04
#define INTRRX2                 0x05
#define INTRUSB                 0x06
#define INTRTX1E                0x07
#define INTRTX2E                0x08
#define INTRRX1E                0x09
#define INTRRX2E                0x0a
#define INTRUSBE                0x0b
#define FRAME1                  0x0c
#define FRAME2                  0x0d
#define INDEX                   0x0e
#define DEVCTL                  0x0f
#define TXMAXP                  0x10
#define CSR0                    0x11
#define TXCSR1                  0x11
#define TXCSR2                  0x12
#define RXMAXP                  0x13
#define RXCSR1                  0x14
#define RXCSR2                  0x15
#define COUNT0                  0x16
#define RXCOUNT1                0x16
#define RXCOUNT2                0x17
#define TXTYPE                  0x18
#define TXINTERVAL              0x19
#define RXTYPE                  0x1a
#define RXINTERVAL              0x1b


#endif



