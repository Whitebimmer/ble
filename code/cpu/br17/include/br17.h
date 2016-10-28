
#ifndef __BR17__
#define __BR17__

//===============================================================================//
//
//      sfr define
//
//===============================================================================//

  #define hs_base   0x70000
  #define ls_base   0x60000
  #define zb_sfr_base   0xF5F00

  #define __RW      volatile       // read write
  #define __RO      volatile       // only read
  #define __WO      volatile       // only write

  #define __u8      unsigned int   // u8  to u32 special for struct
  #define __u16     unsigned int   // u16 to u32 special for struct
  #define __u32     unsigned int

  #define map_adr(grp, adr)  ((64 * grp + adr) * 4)     // grp(0x0-0xff), adr(0x0-0x3f)

#define SFR(sfr, start, len, dat) (sfr = (sfr & ~((~(0xffffffff << len)) << start)) | ((dat & (~(0xffffffff << len))) << start))
//===============================================================================//
//
//      high speed sfr address define
//
//===============================================================================//

//............. 0x0000 - 0x00ff............ for cpu
  #define DSPCON                        (*(__RW __u32 *)(hs_base + map_adr(0x00, 0x00)))
  #define ILAT1                         (*(__RO __u32 *)(hs_base + map_adr(0x00, 0x01)))
  #define ILAT1_SET                     (*(__WO __u32 *)(hs_base + map_adr(0x00, 0x02)))
  #define ILAT1_CLR                     (*(__WO __u32 *)(hs_base + map_adr(0x00, 0x03)))
  #define ILAT0                         (*(__RO __u32 *)(hs_base + map_adr(0x00, 0x04)))
  #define ILAT0_SET                     (*(__WO __u32 *)(hs_base + map_adr(0x00, 0x05)))
  #define ILAT0_CLR                     (*(__WO __u32 *)(hs_base + map_adr(0x00, 0x06)))
  #define IPCON0                        (*(__RW __u32 *)(hs_base + map_adr(0x00, 0x07)))
  #define IPCON1                        (*(__RW __u32 *)(hs_base + map_adr(0x00, 0x08)))
  #define IPCON2                        (*(__RW __u32 *)(hs_base + map_adr(0x00, 0x09)))
  #define IPCON3                        (*(__RW __u32 *)(hs_base + map_adr(0x00, 0x0a)))

  #define TICK_TMR_CON                  (*(__RW __u8  *)(hs_base + map_adr(0x00, 0x10)))
  #define TICK_TMR_CNT                  (*(__RW __u32 *)(hs_base + map_adr(0x00, 0x11)))
  #define TICK_TMR_PRD                  (*(__RW __u32 *)(hs_base + map_adr(0x00, 0x12)))

//............. 0x0100 - 0x01ff............ for debug
  #define DSP_BF_CON                    (*(__RW __u32 *)(hs_base + map_adr(0x01, 0x00)))
  #define DEBUG_WR_EN                   (*(__RW __u32 *)(hs_base + map_adr(0x01, 0x01)))
  #define DEBUG_MSG                     (*(__RO __u32 *)(hs_base + map_adr(0x01, 0x02)))
  #define DEBUG_MSG_CLR                 (*(__WO __u32 *)(hs_base + map_adr(0x01, 0x03)))
  #define DSP_PC_LIMH                   (*(__RW __u32 *)(hs_base + map_adr(0x01, 0x04)))
  #define DSP_PC_LIML                   (*(__RW __u32 *)(hs_base + map_adr(0x01, 0x05)))
  #define DSP_EX_LIMH                   (*(__RW __u32 *)(hs_base + map_adr(0x01, 0x06)))
  #define DSP_EX_LIML                   (*(__RW __u32 *)(hs_base + map_adr(0x01, 0x07)))
  #define PRP_EX_LIMH                   (*(__RW __u32 *)(hs_base + map_adr(0x01, 0x08)))
  #define PRP_EX_LIML                   (*(__RW __u32 *)(hs_base + map_adr(0x01, 0x09)))
  #define PRP_MMU_MSG                   (*(__RO __u32 *)(hs_base + map_adr(0x01, 0x0a)))
  #define LSB_MMU_MSG_CH                (*(__RO __u32 *)(hs_base + map_adr(0x01, 0x0b)))
  #define PRP_WR_LIMIT_MSG              (*(__RO __u32 *)(hs_base + map_adr(0x01, 0x0c)))
  #define LSB_WR_LIMIT_CH               (*(__RO __u32 *)(hs_base + map_adr(0x01, 0x0d)))

//............. 0x0200 - 0x02ff............ for sfc
  #define SFC_CON                       (*(__RW __u32 *)(hs_base + map_adr(0x02, 0x00)))
  #define SFC_BAUD                      (*(__WO __u8  *)(hs_base + map_adr(0x02, 0x01)))
  #define SFC_CODE                      (*(__WO __u16 *)(hs_base + map_adr(0x02, 0x02)))
  #define SFC_BASE_ADR                  (*(__WO __u32 *)(hs_base + map_adr(0x02, 0x03)))

//............. 0x0300 - 0x03ff............ for enc
  #define ENC_CON                       (*(__RW __u8  *)(hs_base + map_adr(0x03, 0x00)))
  #define ENC_KEY                       (*(__WO __u16 *)(hs_base + map_adr(0x03, 0x01)))
  #define ENC_ADR                       (*(__WO __u16 *)(hs_base + map_adr(0x03, 0x02)))
  #define SFC_UNENC_ADRH                (*(__RW __u32 *)(hs_base + map_adr(0x03, 0x03)))
  #define SFC_UNENC_ADRL                (*(__RW __u32 *)(hs_base + map_adr(0x03, 0x04)))

//............. 0x0400 - 0x04ff............ for others
  #define WL_CON0                       (*(__RW __u32 *)(hs_base + map_adr(0x04, 0x00)))
  #define WL_CON1                       (*(__RW __u32 *)(hs_base + map_adr(0x04, 0x01)))
  #define WL_CON2                       (*(__RW __u32 *)(hs_base + map_adr(0x04, 0x02)))
  #define WL_CON3                       (*(__RW __u32 *)(hs_base + map_adr(0x04, 0x03)))

  #define WL_LOFC_CON                   (*(__RW __u32 *)(hs_base + map_adr(0x04, 0x08)))
  #define WL_LOFC_RES                   (*(__RW __u32 *)(hs_base + map_adr(0x04, 0x09)))
//  #define PD_CON                        (*(__RW __u16 *)(hs_base + map_adr(0x04, 0x01)))

//............. 0x0500 - 0x05ff............ for aes
  #define AES_CON                       (*(__RW __u32 *)(hs_base + map_adr(0x05, 0x00)))
  #define AES_DATIN                     (*(__RW __u32 *)(hs_base + map_adr(0x05, 0x01)))
  #define AES_KEY                       (*(__WO __u32 *)(hs_base + map_adr(0x05, 0x02)))
  #define AES_ENCRES0                   (*(__RW __u32 *)(hs_base + map_adr(0x05, 0x03)))
  #define AES_ENCRES1                   (*(__RW __u32 *)(hs_base + map_adr(0x05, 0x04)))
  #define AES_ENCRES2                   (*(__RW __u32 *)(hs_base + map_adr(0x05, 0x05)))
  #define AES_ENCRES3                   (*(__RW __u32 *)(hs_base + map_adr(0x05, 0x06)))
  #define AES_DECRES0                   (*(__RW __u32 *)(hs_base + map_adr(0x05, 0x07)))
  #define AES_DECRES1                   (*(__RW __u32 *)(hs_base + map_adr(0x05, 0x08)))
  #define AES_DECRES2                   (*(__RW __u32 *)(hs_base + map_adr(0x05, 0x09)))
  #define AES_DECRES3                   (*(__RW __u32 *)(hs_base + map_adr(0x05, 0x0a)))

//............. 0x0600 - 0x06ff............ for fft
  #define FFT_CON                       (*(__RW __u32 *)(hs_base + map_adr(0x06, 0x00)))
  #define FFT_ADRI                      (*(__RW __u32 *)(hs_base + map_adr(0x06, 0x01)))
  #define FFT_ADRO                      (*(__RW __u32 *)(hs_base + map_adr(0x06, 0x02)))
  #define FFT_ADRW                      (*(__RW __u32 *)(hs_base + map_adr(0x06, 0x03)))

//............. 0x0700 - 0x07ff............ for eq
  #define EQ_CON                        (*(__RW __u32 *)(hs_base + map_adr(0x07, 0x00)))
  #define EQ_LEN                        (*(__RW __u32 *)(hs_base + map_adr(0x07, 0x01)))
  #define EQ_ADRI                       (*(__RW __u32 *)(hs_base + map_adr(0x07, 0x02)))
  #define EQ_ADRO                       (*(__RW __u32 *)(hs_base + map_adr(0x07, 0x03)))
  #define EQ_CADR                       (*(__RW __u32 *)(hs_base + map_adr(0x07, 0x04)))

//............. 0x0800 - 0x08ff............ for src
  #define SRC_CON0                      (*(__RW __u32 *)(hs_base + map_adr(0x08, 0x00)))
  #define SRC_CON1                      (*(__RW __u32 *)(hs_base + map_adr(0x08, 0x01)))
  #define SRC_CON2                      (*(__RW __u32 *)(hs_base + map_adr(0x08, 0x02)))
  #define SRC_CON3                      (*(__RW __u32 *)(hs_base + map_adr(0x08, 0x03)))
  #define SRC_IDAT_ADR                  (*(__RW __u32 *)(hs_base + map_adr(0x08, 0x04)))
  #define SRC_IDAT_LEN                  (*(__RW __u32 *)(hs_base + map_adr(0x08, 0x05)))
  #define SRC_ODAT_ADR                  (*(__RW __u32 *)(hs_base + map_adr(0x08, 0x06)))
  #define SRC_ODAT_LEN                  (*(__RW __u32 *)(hs_base + map_adr(0x08, 0x07)))
  #define SRC_FLTB_ADR                  (*(__RW __u32 *)(hs_base + map_adr(0x08, 0x08)))

//............. 0x0800 - 0x08ff............ for fm
  #define FM_CON                        (*(__RW __u32 *)(hs_base + map_adr(0x09, 0x00)))
  #define FM_BASE                       (*(__RW __u32 *)(hs_base + map_adr(0x09, 0x01)))
  #define FMADC_CON                     (*(__RW __u32 *)(hs_base + map_adr(0x09, 0x02)))
  #define FMHF_CON0                     (*(__RW __u32 *)(hs_base + map_adr(0x09, 0x03)))
  #define FMHF_CON1                     (*(__RW __u32 *)(hs_base + map_adr(0x09, 0x04)))
  #define FMHBT_RSSI                    (*(__RW __u32 *)(hs_base + map_adr(0x09, 0x05)))
  #define FMADC_RSSI                    (*(__RW __u32 *)(hs_base + map_adr(0x09, 0x06)))
  #define FMHF_CRAM                     (*(__RW __u32 *)(hs_base + map_adr(0x09, 0x07)))
  #define FMHF_DRAM                     (*(__RW __u32 *)(hs_base + map_adr(0x09, 0x08)))
  #define FMLF_CON                      (*(__RW __u32 *)(hs_base + map_adr(0x09, 0x09)))
  #define FMLF_RES                      (*(__RW __u32 *)(hs_base + map_adr(0x09, 0x0a)))
// mc pwm
  #define MCTMR0_CON                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x00)))
  #define MCTMR0_CNT                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x01)))
  #define MCTMR0_PR                     (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x02)))
  #define MCTMR1_CON                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x03)))
  #define MCTMR1_CNT                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x04)))
  #define MCTMR1_PR                     (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x05)))
  #define MCTMR2_CON                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x06)))
  #define MCTMR2_CNT                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x07)))
  #define MCTMR2_PR                     (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x08)))
  #define MCTMR3_CON                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x09)))
  #define MCTMR3_CNT                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x0a)))
  #define MCTMR3_PR                     (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x0b)))
  #define MCTMR4_CON                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x0c)))
  #define MCTMR4_CNT                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x0d)))
  #define MCTMR4_PR                     (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x0e)))
  #define MCTMR5_CON                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x0f)))
  #define MCTMR5_CNT                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x10)))
  #define MCTMR5_PR                     (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x11)))
  #define MCFPIN_CON                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x12)))
  #define MCCH0_CON0                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x13)))
  #define MCCH0_CON1                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x14)))
  #define MCCH0_CMP                     (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x15)))
  #define MCCH1_CON0                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x16)))
  #define MCCH1_CON1                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x17)))
  #define MCCH1_CMP                     (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x18)))
  #define MCCH2_CON0                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x19)))
  #define MCCH2_CON1                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x1a)))
  #define MCCH2_CMP                     (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x1b)))
  #define MCCH3_CON0                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x1c)))
  #define MCCH3_CON1                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x1d)))
  #define MCCH3_CMP                     (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x1e)))
  #define MCCH4_CON0                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x1f)))
  #define MCCH4_CON1                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x20)))
  #define MCCH4_CMP                     (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x21)))
  #define MCCH5_CON0                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x22)))
  #define MCCH5_CON1                    (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x23)))
  #define MCCH5_CMP                     (*(__RW __u32 *)(hs_base + map_adr(0x0a, 0x24)))

//===============================================================================//
//
//      low speed sfr address define
//
//===============================================================================//

//............. 0x0000 - 0x00ff............ for syscfg
  #define CHIP_ID                       (*(__RO __u16 *)(ls_base + map_adr(0x00, 0x00)))
  #define MODE_CON                      (*(__RW __u8  *)(ls_base + map_adr(0x00, 0x01)))
  #define WKUP_CON0                     (*(__RW __u16 *)(ls_base + map_adr(0x00, 0x02)))
  #define WKUP_CON1                     (*(__RW __u16 *)(ls_base + map_adr(0x00, 0x03)))
  #define WKUP_CON2                     (*(__RW __u16 *)(ls_base + map_adr(0x00, 0x04)))
  #define WKUP_CON3                     (*(__RW __u16 *)(ls_base + map_adr(0x00, 0x05)))
  #define IOMC0                         (*(__RW __u16 *)(ls_base + map_adr(0x00, 0x06)))
  #define IOMC1                         (*(__RW __u16 *)(ls_base + map_adr(0x00, 0x07)))
  #define IOMC2                         (*(__RW __u32 *)(ls_base + map_adr(0x00, 0x08)))
  #define IOMC3                         (*(__RW __u16 *)(ls_base + map_adr(0x00, 0x09)))

  #define PWR_CON                       (*(__RW __u8  *)(ls_base + map_adr(0x00, 0x10)))
  #define SYS_DIV                       (*(__RW __u16 *)(ls_base + map_adr(0x00, 0x11)))
  #define CLK_CON0                      (*(__RW __u16 *)(ls_base + map_adr(0x00, 0x12)))
  #define CLK_CON1                      (*(__RW __u32 *)(ls_base + map_adr(0x00, 0x13)))
  #define CLK_CON2                      (*(__RW __u32 *)(ls_base + map_adr(0x00, 0x14)))
//#define CLK_GAT0                      (*(__RW __u16 *)(ls_base + map_adr(0x00, 0x15)))
//#define CLK_GAT1                      (*(__RW __u32 *)(ls_base + map_adr(0x00, 0x16)))

  #define PLL_CON                       (*(__RW __u32 *)(ls_base + map_adr(0x00, 0x20)))
  #define LDO_CON                       (*(__RW __u32 *)(ls_base + map_adr(0x00, 0x21)))
  #define LVD_CON                       (*(__RW __u16 *)(ls_base + map_adr(0x00, 0x22)))
  #define WDT_CON                       (*(__RW __u8  *)(ls_base + map_adr(0x00, 0x23)))
  #define OSA_CON                       (*(__RW __u8  *)(ls_base + map_adr(0x00, 0x24)))
  #define EFUSE_CON                     (*(__RW __u16 *)(ls_base + map_adr(0x00, 0x25)))
//#define HTC_CON                       (*(__RW __u16 *)(ls_base + map_adr(0x00, 0x26)))

//............. 0x0100 - 0x01ff............ for port
  #define PORTA_OUT                     (*(__RW __u16 *)(ls_base + map_adr(0x01, 0x00)))
  #define PORTA_IN                      (*(__RO __u16 *)(ls_base + map_adr(0x01, 0x01)))
  #define PORTA_DIR                     (*(__RW __u16 *)(ls_base + map_adr(0x01, 0x02)))
  #define PORTA_DIE                     (*(__RW __u16 *)(ls_base + map_adr(0x01, 0x03)))
  #define PORTA_PU                      (*(__RW __u16 *)(ls_base + map_adr(0x01, 0x04)))
  #define PORTA_PD                      (*(__RW __u16 *)(ls_base + map_adr(0x01, 0x05)))
  #define PORTA_HD                      (*(__RW __u16 *)(ls_base + map_adr(0x01, 0x06)))

  #define PORTB_OUT                     (*(__RW __u16 *)(ls_base + map_adr(0x01, 0x08)))
  #define PORTB_IN                      (*(__RO __u16 *)(ls_base + map_adr(0x01, 0x09)))
  #define PORTB_DIR                     (*(__RW __u16 *)(ls_base + map_adr(0x01, 0x0a)))
  #define PORTB_DIE                     (*(__RW __u16 *)(ls_base + map_adr(0x01, 0x0b)))
  #define PORTB_PU                      (*(__RW __u16 *)(ls_base + map_adr(0x01, 0x0c)))
  #define PORTB_PD                      (*(__RW __u16 *)(ls_base + map_adr(0x01, 0x0d)))
  #define PORTB_HD                      (*(__RW __u16 *)(ls_base + map_adr(0x01, 0x0e)))

  #define PORTC_OUT                     (*(__RW __u16 *)(ls_base + map_adr(0x01, 0x10)))
  #define PORTC_IN                      (*(__RO __u16 *)(ls_base + map_adr(0x01, 0x11)))
  #define PORTC_DIR                     (*(__RW __u16 *)(ls_base + map_adr(0x01, 0x12)))
  #define PORTC_DIE                     (*(__RW __u16 *)(ls_base + map_adr(0x01, 0x13)))
  #define PORTC_PU                      (*(__RW __u16 *)(ls_base + map_adr(0x01, 0x14)))
  #define PORTC_PD                      (*(__RW __u16 *)(ls_base + map_adr(0x01, 0x15)))
  #define PORTC_HD                      (*(__RW __u16 *)(ls_base + map_adr(0x01, 0x16)))

  #define PORTD_OUT                     (*(__RW __u8  *)(ls_base + map_adr(0x01, 0x18)))
  #define PORTD_IN                      (*(__RO __u8  *)(ls_base + map_adr(0x01, 0x19)))
  #define PORTD_DIR                     (*(__RW __u8  *)(ls_base + map_adr(0x01, 0x1a)))
  #define PORTD_DIE                     (*(__RW __u8  *)(ls_base + map_adr(0x01, 0x1b)))
  #define PORTD_PU                      (*(__RW __u8  *)(ls_base + map_adr(0x01, 0x1c)))
  #define PORTD_PD                      (*(__RW __u8  *)(ls_base + map_adr(0x01, 0x1d)))
  #define PORTD_HD                      (*(__RW __u8  *)(ls_base + map_adr(0x01, 0x1e)))

//............. 0x0200 - 0x02ff............ for timer
  #define TMR0_CON                      (*(__RW __u16 *)(ls_base + map_adr(0x02, 0x00)))
  #define TMR0_CNT                      (*(__RW __u16 *)(ls_base + map_adr(0x02, 0x01)))
  #define TMR0_PRD                      (*(__RW __u16 *)(ls_base + map_adr(0x02, 0x02)))
  #define TMR0_PWM                      (*(__RW __u16 *)(ls_base + map_adr(0x02, 0x03)))
  #define TMR1_CON                      (*(__RW __u16 *)(ls_base + map_adr(0x02, 0x04)))
  #define TMR1_CNT                      (*(__RW __u16 *)(ls_base + map_adr(0x02, 0x05)))
  #define TMR1_PRD                      (*(__RW __u16 *)(ls_base + map_adr(0x02, 0x06)))
  #define TMR1_PWM                      (*(__RW __u16 *)(ls_base + map_adr(0x02, 0x07)))
  #define TMR2_CON                      (*(__RW __u16 *)(ls_base + map_adr(0x02, 0x08)))
  #define TMR2_CNT                      (*(__RW __u16 *)(ls_base + map_adr(0x02, 0x09)))
  #define TMR2_PRD                      (*(__RW __u16 *)(ls_base + map_adr(0x02, 0x0a)))
  #define TMR2_PWM                      (*(__RW __u16 *)(ls_base + map_adr(0x02, 0x0b)))
  #define TMR3_CON                      (*(__RW __u16 *)(ls_base + map_adr(0x02, 0x0c)))
  #define TMR3_CNT                      (*(__RW __u16 *)(ls_base + map_adr(0x02, 0x0d)))
  #define TMR3_PRD                      (*(__RW __u16 *)(ls_base + map_adr(0x02, 0x0e)))
  #define TMR3_PWM                      (*(__RW __u16 *)(ls_base + map_adr(0x02, 0x0f)))

//............. 0x0300 - 0x03ff............ for uart
  #define UT0_CON                       (*(__RW __u16 *)(ls_base + map_adr(0x03, 0x00)))
  #define UT0_BAUD                      (*(__WO __u16 *)(ls_base + map_adr(0x03, 0x01)))
  #define UT0_BUF                       (*(__RW __u8  *)(ls_base + map_adr(0x03, 0x02)))
  #define UT0_OT                        (*(__RW __u32 *)(ls_base + map_adr(0x03, 0x03)))
  #define UT0_DMA_RD_ADR                (*(__RW __u32 *)(ls_base + map_adr(0x03, 0x04)))
  #define UT0_DMA_RD_CNT                (*(__RW __u16 *)(ls_base + map_adr(0x03, 0x05)))
  #define UT0_DMA_WR_SADR               (*(__RW __u32 *)(ls_base + map_adr(0x03, 0x06)))
  #define UT0_DMA_WR_EADR               (*(__RW __u32 *)(ls_base + map_adr(0x03, 0x07)))
  #define UT0_DMA_WR_CNT                (*(__RW __u32 *)(ls_base + map_adr(0x03, 0x08)))
  #define UT1_CON                       (*(__RW __u16 *)(ls_base + map_adr(0x03, 0x09)))
  #define UT1_BAUD                      (*(__WO __u16 *)(ls_base + map_adr(0x03, 0x0a)))
  #define UT1_BUF                       (*(__RW __u8  *)(ls_base + map_adr(0x03, 0x0b)))
  #define UT1_OT                        (*(__RW __u32 *)(ls_base + map_adr(0x03, 0x0c)))
  #define UT1_DMA_RD_ADR                (*(__WO __u32 *)(ls_base + map_adr(0x03, 0x0d)))
  #define UT1_DMA_RD_CNT                (*(__RW __u16 *)(ls_base + map_adr(0x03, 0x0e)))
  #define UT1_DMA_WR_SADR               (*(__WO __u32 *)(ls_base + map_adr(0x03, 0x0f)))
  #define UT1_DMA_WR_EADR               (*(__WO __u32 *)(ls_base + map_adr(0x03, 0x10)))
  #define UT1_DMA_WR_CNT                (*(__RW __u32 *)(ls_base + map_adr(0x03, 0x11)))
  #define UT2_CON                       (*(__RW __u16 *)(ls_base + map_adr(0x03, 0x12)))
  #define UT2_BUF                       (*(__RW __u8  *)(ls_base + map_adr(0x03, 0x13)))
  #define UT2_BAUD                      (*(__WO __u16 *)(ls_base + map_adr(0x03, 0x14)))

//............. 0x0400 - 0x04ff............ for spi
  #define SPI0_CON                      (*(__RW __u16 *)(ls_base + map_adr(0x04, 0x00)))
  #define SPI0_BAUD                     (*(__WO __u16 *)(ls_base + map_adr(0x04, 0x01)))
  #define SPI0_BUF                      (*(__RW __u8  *)(ls_base + map_adr(0x04, 0x02)))
  #define SPI0_ADR                      (*(__WO __u32 *)(ls_base + map_adr(0x04, 0x03)))
  #define SPI0_CNT                      (*(__WO __u32 *)(ls_base + map_adr(0x04, 0x04)))
  #define SPI1_CON                      (*(__RW __u16 *)(ls_base + map_adr(0x04, 0x05)))
  #define SPI1_BAUD                     (*(__WO __u16 *)(ls_base + map_adr(0x04, 0x06)))
  #define SPI1_BUF                      (*(__RW __u8  *)(ls_base + map_adr(0x04, 0x07)))
  #define SPI1_ADR                      (*(__WO __u32 *)(ls_base + map_adr(0x04, 0x08)))
  #define SPI1_CNT                      (*(__WO __u32 *)(ls_base + map_adr(0x04, 0x09)))

//............. 0x0500 - 0x05ff............ for pap
  #define PAP_CON                       (*(__RW __u32 *)(ls_base + map_adr(0x05, 0x00)))
  #define PAP_DAT0                      (*(__WO __u16 *)(ls_base + map_adr(0x05, 0x01)))
  #define PAP_DAT1                      (*(__WO __u16 *)(ls_base + map_adr(0x05, 0x02)))
  #define PAP_BUF                       (*(__RW __u16 *)(ls_base + map_adr(0x05, 0x03)))
//#define PAP_BUF8                      (*(__RW __u8  *)(ls_base + map_adr(0x05, 0x04)))
  #define PAP_ADR                       (*(__WO __u32 *)(ls_base + map_adr(0x05, 0x05)))
  #define PAP_CNT                       (*(__WO __u32 *)(ls_base + map_adr(0x05, 0x06)))

//............. 0x0600 - 0x06ff............ for sd
  #define SD0_CON0                      (*(__RW __u16 *)(ls_base + map_adr(0x06, 0x00)))
  #define SD0_CON1                      (*(__RW __u16 *)(ls_base + map_adr(0x06, 0x01)))
  #define SD0_CON2                      (*(__RW __u16 *)(ls_base + map_adr(0x06, 0x02)))
  #define SD0_CPTR                      (*(__WO __u32 *)(ls_base + map_adr(0x06, 0x03)))
  #define SD0_DPTR                      (*(__WO __u32 *)(ls_base + map_adr(0x06, 0x04)))
  #define SD1_CON0                      (*(__RW __u16 *)(ls_base + map_adr(0x06, 0x05)))
  #define SD1_CON1                      (*(__RW __u16 *)(ls_base + map_adr(0x06, 0x06)))
  #define SD1_CON2                      (*(__RW __u16 *)(ls_base + map_adr(0x06, 0x07)))
  #define SD1_CPTR                      (*(__WO __u32 *)(ls_base + map_adr(0x06, 0x08)))
  #define SD1_DPTR                      (*(__WO __u32 *)(ls_base + map_adr(0x06, 0x09)))

//............. 0x0700 - 0x07ff............ for iic
  #define IIC_CON                       (*(__RW __u16 *)(ls_base + map_adr(0x07, 0x00)))
  #define IIC_BUF                       (*(__RW __u8  *)(ls_base + map_adr(0x07, 0x01)))
  #define IIC_BAUD                      (*(__WO __u8  *)(ls_base + map_adr(0x07, 0x02)))

//............. 0x0800 - 0x08ff............ for lcd
  #define LCDC_CON0                     (*(__RW __u16 *)(ls_base + map_adr(0x08, 0x00)))
//#define LCDC_CON1                     (*(__RW __u16 *)(ls_base + map_adr(0x08, 0x01)))
  #define SEG_IOEN0                     (*(__RW __u16 *)(ls_base + map_adr(0x08, 0x02)))
  #define SEG_IOEN1                     (*(__RW __u16 *)(ls_base + map_adr(0x08, 0x03)))

//............. 0x0900 - 0x09ff............ for others
  #define PWM4_CON                      (*(__WO __u8  *)(ls_base + map_adr(0x09, 0x00)))
  #define IRTC_CON                      (*(__RW __u16 *)(ls_base + map_adr(0x09, 0x01)))
  #define IRFLT_CON                     (*(__RW __u8  *)(ls_base + map_adr(0x09, 0x02)))

//............. 0x0a00 - 0x0aff............ for dac
  #define DAC_LEN                       (*(__WO __u16 *)(ls_base + map_adr(0x0a, 0x00)))
  #define DAC_CON                       (*(__RW __u16 *)(ls_base + map_adr(0x0a, 0x01)))
  #define DAC_ADR                       (*(__WO __u32 *)(ls_base + map_adr(0x0a, 0x02)))
  #define DAC_TRML                      (*(__WO __u8  *)(ls_base + map_adr(0x0a, 0x03)))
  #define DAC_TRMR                      (*(__WO __u8  *)(ls_base + map_adr(0x0a, 0x04)))

  #define ADC_CON                       (*(__RW __u16 *)(ls_base + map_adr(0x0a, 0x08)))
//
  #define ADC_ADR                       (*(__WO __u32 *)(ls_base + map_adr(0x0a, 0x0b)))
  #define ADC_LEN                       (*(__WO __u16 *)(ls_base + map_adr(0x0a, 0x0c)))

  #define DAA_CON0                      (*(__RW __u16 *)(ls_base + map_adr(0x0a, 0x10)))
  #define DAA_CON1                      (*(__RW __u16 *)(ls_base + map_adr(0x0a, 0x11)))
  #define DAA_CON2                      (*(__RW __u16 *)(ls_base + map_adr(0x0a, 0x12)))
  #define DAA_CON3                      (*(__RW __u16 *)(ls_base + map_adr(0x0a, 0x13)))
  #define DAA_CON4                      (*(__RW __u8  *)(ls_base + map_adr(0x0a, 0x14)))

//............. 0x0b00 - 0x0bff............ for alnk
  #define ALNK_CON0                     (*(__RW __u16 *)(ls_base + map_adr(0x0b, 0x00)))
  #define ALNK_CON1                     (*(__RW __u16 *)(ls_base + map_adr(0x0b, 0x01)))
  #define ALNK_CON2                     (*(__RW __u16 *)(ls_base + map_adr(0x0b, 0x02)))
  #define ALNK_ADR0                     (*(__WO __u32 *)(ls_base + map_adr(0x0b, 0x03)))
  #define ALNK_ADR1                     (*(__WO __u32 *)(ls_base + map_adr(0x0b, 0x04)))
  #define ALNK_ADR2                     (*(__WO __u32 *)(ls_base + map_adr(0x0b, 0x05)))
  #define ALNK_ADR3                     (*(__WO __u32 *)(ls_base + map_adr(0x0b, 0x06)))
  #define ALNK_LEN                      (*(__WO __u16 *)(ls_base + map_adr(0x0b, 0x07)))

//............. 0x0c00 - 0x0cff............ for nfc
  #define NFC_CON0                      (*(__RW __u32 *)(ls_base + map_adr(0x0c, 0x01)))
  #define NFC_CON1                      (*(__RW __u32 *)(ls_base + map_adr(0x0c, 0x02)))
  #define NFC_CON2                      (*(__RW __u32 *)(ls_base + map_adr(0x0c, 0x03)))
  #define NFC_BUF0                      (*(__RW __u32 *)(ls_base + map_adr(0x0c, 0x04)))
  #define NFC_BUF1                      (*(__RW __u32 *)(ls_base + map_adr(0x0c, 0x05)))
  #define NFC_BUF2                      (*(__WO __u32 *)(ls_base + map_adr(0x0c, 0x06)))
  #define NFC_BUF3                      (*(__WO __u32 *)(ls_base + map_adr(0x0c, 0x07)))

//............. 0x0d00 - 0x0dff............ for usb
  #define USB_IO_CON                    (*(__RW __u16 *)(ls_base + map_adr(0x0d, 0x00)))
  #define USB_CON0                      (*(__RW __u32 *)(ls_base + map_adr(0x0d, 0x01)))
  #define USB_CON1                      (*(__RW __u32 *)(ls_base + map_adr(0x0d, 0x02)))
  #define USB_EP0_CNT                   (*(__WO __u16 *)(ls_base + map_adr(0x0d, 0x03)))
  #define USB_EP1_CNT                   (*(__WO __u16 *)(ls_base + map_adr(0x0d, 0x04)))
  #define USB_EP2_CNT                   (*(__WO __u16 *)(ls_base + map_adr(0x0d, 0x05)))
  #define USB_EP3_CNT                   (*(__WO __u16 *)(ls_base + map_adr(0x0d, 0x06)))
  #define USB_EP0_ADR                   (*(__WO __u32 *)(ls_base + map_adr(0x0d, 0x07)))
  #define USB_EP1_TADR                  (*(__WO __u32 *)(ls_base + map_adr(0x0d, 0x08)))
  #define USB_EP1_RADR                  (*(__WO __u32 *)(ls_base + map_adr(0x0d, 0x09)))
  #define USB_EP2_TADR                  (*(__WO __u32 *)(ls_base + map_adr(0x0d, 0x0a)))
  #define USB_EP2_RADR                  (*(__WO __u32 *)(ls_base + map_adr(0x0d, 0x0b)))
  #define USB_EP3_TADR                  (*(__WO __u32 *)(ls_base + map_adr(0x0d, 0x0c)))
  #define USB_EP3_RADR                  (*(__WO __u32 *)(ls_base + map_adr(0x0d, 0x0d)))

//............. 0x0e00 - 0x0eff............ for crc
  #define CRC_FIFO                      (*(__WO __u8  *)(ls_base + map_adr(0x0e, 0x00)))
  #define CRC_REG                       (*(__RW __u16 *)(ls_base + map_adr(0x0e, 0x01)))

//............. 0x0f00 - 0x0fff............ for rand64
  #define RAND64L                       (*(__RO __u8  *)(ls_base + map_adr(0x0f, 0x00)))
  #define RAND64H                       (*(__RO __u8  *)(ls_base + map_adr(0x0f, 0x01)))

//............. 0x1000 - 0x10ff............ for adc
  #define GPADC_CON                     (*(__RW __u16 *)(ls_base + map_adr(0x10, 0x00)))
  #define GPADC_RES                     (*(__RO __u16 *)(ls_base + map_adr(0x10, 0x01)))

//............. 0x1100 - 0x11ff............ for pulse counter
  #define PLCNTCON                      (*(__RW __u8  *)(ls_base + map_adr(0x11, 0x00)))
  #define PLCNTVL                       (*(__RW __u16 *)(ls_base + map_adr(0x11, 0x01)))

//............. 0x1200 - 0x12ff............ for power down
  #define PD_CON                        (*(__RW __u16 *)(ls_base + map_adr(0x12, 0x00)))
  #define PD_DAT                        (*(__RW __u8  *)(ls_base + map_adr(0x12, 0x01)))

//............. 0x1300 - 0x13ff............ for ctmu
  #define CTM_CON0                      (*(__RW __u16 *)(ls_base + map_adr(0x13, 0x00)))
  #define CTM_CON1                      (*(__RW __u32 *)(ls_base + map_adr(0x13, 0x01)))
  #define CTM_RES                       (*(__RO __u16 *)(ls_base + map_adr(0x13, 0x02)))

// mc speed det
  #define SPDETCON0                     (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x00)))
  #define SPDETCON1                     (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x01)))
  #define PULSE_CNT0                    (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x02)))
  #define PULSE_CNT1                    (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x03)))
  #define PULSE_CNT2                    (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x04)))
  #define PULSE_CNT3                    (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x05)))
  #define PULSE_CNT4                    (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x06)))
  #define PULSE_CNT5                    (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x07)))
  #define WIDTH_CNT0                    (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x08)))
  #define WIDTH_CNT1                    (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x09)))
  #define WIDTH_CNT2                    (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x0a)))
  #define WIDTH_CNT3                    (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x0b)))
  #define WIDTH_CNT4                    (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x0c)))
  #define WIDTH_CNT5                    (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x0d)))
  #define SPDET0_PR                     (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x0e)))
  #define SPDET1_PR                     (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x0f)))
  #define SPDET2_PR                     (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x10)))
  #define SPDET3_PR                     (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x11)))
  #define SPDET4_PR                     (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x12)))
  #define SPDET5_PR                     (*(__RW __u32 *)(ls_base + map_adr(0x14, 0x13)))


//............. 0x1c00 - 0x1fff............ for anl ctl
  #define WLA_CON0                      (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x00)))  // long
  #define WLA_CON1                      (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x01)))  // long
  #define WLA_CON2                      (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x02)))  // long
  #define WLA_CON3                      (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x03)))  // long
  #define WLA_CON4                      (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x04)))  // long
  #define WLA_CON5                      (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x05)))  // long
  #define WLA_CON6                      (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x06)))  // long
  #define WLA_CON7                      (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x07)))  // long
  #define WLA_CON8                      (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x08)))  // long
  #define WLA_CON9                      (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x09)))  // long
  #define WLA_CON10                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x0a)))  // long
  #define WLA_CON11                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x0b)))  // long
  #define WLA_CON12                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x0c)))  // long
  #define WLA_CON13                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x0d)))  // long
  #define WLA_CON14                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x0e)))  // long
  #define WLA_CON15                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x0f)))  // long
  #define WLA_CON16                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x10)))  // long, read only
  #define WLA_CON17                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x11)))  // long, read only
  #define WLA_CON18                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x12)))  // long, read only
  #define WLA_CON19                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x13)))  // long, read only
  #define WLA_CON20                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x14)))  // long, read only
  #define WLA_CON21                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x15)))  // long, read only

  #define WLA_CON26                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x1a)))  // long, read only
  #define WLA_CON27                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x1b)))  // long, read only
  #define WLA_CON28                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x1c)))  // long, read only
  #define WLA_CON29                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x1d)))  // long, read only
  #define WLA_CON30                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x1e)))  // long, read only
  #define WLA_CON31                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x1f)))  // long, read only
  #define WLA_CON32                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x20)))  // long, read only
  #define WLA_CON33                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x21)))  // long, read only
  #define WLA_CON34                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x22)))  // long, read only
  #define WLA_CON35                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x23)))  // long, read only
  #define WLA_CON36                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x24)))  // long, read only
  #define WLA_CON37                     (*(__RW __u32 *)(ls_base + map_adr(0x1c, 0x25)))  // long, read only

  #define FMA_CON0                      (*(__RW __u32 *)(ls_base + map_adr(0x1d, 0x00)))  // long
  #define FMA_CON1                      (*(__RW __u32 *)(ls_base + map_adr(0x1d, 0x01)))  // long
  #define FMA_CON2                      (*(__RW __u32 *)(ls_base + map_adr(0x1d, 0x02)))  // long
  #define FMA_CON3                      (*(__RW __u32 *)(ls_base + map_adr(0x1d, 0x03)))  // long
  #define FMA_CON4                      (*(__RW __u32 *)(ls_base + map_adr(0x1d, 0x04)))  // long
  #define FMA_CON5                      (*(__RW __u32 *)(ls_base + map_adr(0x1d, 0x05)))  // long
  #define FMA_CON6                      (*(__RW __u32 *)(ls_base + map_adr(0x1d, 0x06)))  // long
  #define FMA_CON7                      (*(__RW __u32 *)(ls_base + map_adr(0x1d, 0x07)))  // long
  #define FMA_CON8                      (*(__RW __u32 *)(ls_base + map_adr(0x1d, 0x08)))  // long

//===============================================================================//
//
//      c51 sfr address define
//
//===============================================================================//

//............. 0x0000 - 0x00ff............ for c51
// 0x8x
#define ZB_SOFTSPND                     (*(__RW u8  *)(zb_sfr_base + 0x80))
#define ZB_SP    		        (*(__RW u8  *)(zb_sfr_base + 0x81))
#define ZB_DP0L   		        (*(__RW u8  *)(zb_sfr_base + 0x82))
#define ZB_DP0H   		        (*(__RW u8  *)(zb_sfr_base + 0x83))
#define ZB_DP1L   		        (*(__RW u8  *)(zb_sfr_base + 0x84))
#define ZB_DP1H   		        (*(__RW u8  *)(zb_sfr_base + 0x85))
#define ZB_DPCON  		        (*(__RW u8  *)(zb_sfr_base + 0x86))
#define ZB_SPH  		        (*(__RW u8  *)(zb_sfr_base + 0x87))
#define ZB_SOFTCPND                     (*(__RW u8  *)(zb_sfr_base + 0x88))
#define ZB_SOFTPND                     (*(__RW u8  *)(zb_sfr_base + 0x89))
//
#define ZB_IPCON0L    	                (*(__RW u8  *)(zb_sfr_base + 0x8a))
#define ZB_IPCON0H    	                (*(__RW u8  *)(zb_sfr_base + 0x8b))
#define ZB_IPCON1L    	                (*(__RW u8  *)(zb_sfr_base + 0x8c))
#define ZB_IPCON1H    	                (*(__RW u8  *)(zb_sfr_base + 0x8d))
#define ZB_IPCON016 	                (*(__RW u16 *)(zb_sfr_base + 0x8a))
#define ZB_IPCON116 	                (*(__RW u16 *)(zb_sfr_base + 0x8c))

// 0x9x
//
#define ZB_PWRCON 		        (*(__RW u8  *)(zb_sfr_base + 0x91))
#define ZB_PDATA  		        (*(__RW u8  *)(zb_sfr_base + 0x94))
#define ZB_IE1  		        (*(__RW u8  *)(zb_sfr_base + 0x95))
//

// 0xax
//
#define ZB_T0CONL 		        (*(__RW u8  *)(zb_sfr_base + 0xa2))
#define ZB_T0CONH 		        (*(__RW u8  *)(zb_sfr_base + 0xa3))
#define ZB_T0CNTL 		        (*(__RW u8  *)(zb_sfr_base + 0xa4))
#define ZB_T0CNTH 		        (*(__RW u8  *)(zb_sfr_base + 0xa5))
#define ZB_T0PRDL 		        (*(__RW u8  *)(zb_sfr_base + 0xa6))
#define ZB_T0PRDH 		        (*(__RW u8  *)(zb_sfr_base + 0xa7))
#define ZB_IE0   		        (*(__RW u8  *)(zb_sfr_base + 0xa8))
//
#define ZB_T1CONL 		        (*(__RW u8  *)(zb_sfr_base + 0xaa))
#define ZB_T1CONH 		        (*(__RW u8  *)(zb_sfr_base + 0xab))
#define ZB_T1CNTL 		        (*(__RW u8  *)(zb_sfr_base + 0xac))
#define ZB_T1CNTH 		        (*(__RW u8  *)(zb_sfr_base + 0xad))
#define ZB_T1PRDL 		        (*(__RW u8  *)(zb_sfr_base + 0xae))
#define ZB_T1PRDH 		        (*(__RW u8  *)(zb_sfr_base + 0xaf))
#define ZB_T0CON16	                (*(__RW u16 *)(zb_sfr_base + 0xa2))
#define ZB_T0CNT16	                (*(__RW u16 *)(zb_sfr_base + 0xa4))
#define ZB_T0PRD16	                (*(__RW u16 *)(zb_sfr_base + 0xa6))
#define ZB_T1CON16       	        (*(__RW u16 *)(zb_sfr_base + 0xaa))
#define ZB_T1CNT16	                (*(__RW u16 *)(zb_sfr_base + 0xac))
#define ZB_T1PRD16	                (*(__RW u16 *)(zb_sfr_base + 0xae))

// 0xbx

// 0xcx

// 0xdx
#define ZB_PSW   		        (*(__RW u8  *)(zb_sfr_base + 0xD0))

// 0xex
#define ZB_ACC   		        (*(__RW u8  *)(zb_sfr_base + 0xE0))

// 0xfx
#define ZB_B     		        (*(__RW u8  *)(zb_sfr_base + 0xF0))
#define ZB_UTBUF     	                (*(__RW u8  *)(zb_sfr_base + 0xf1))
#define ZB_UTCON0    	                (*(__RW u8  *)(zb_sfr_base + 0xf2))
#define ZB_UTCON1    	                (*(__RW u8  *)(zb_sfr_base + 0xf3))
#define ZB_UTBAUD    	                (*(__RW u8  *)(zb_sfr_base + 0xf4))
#define ZB_UTBAUDH   	                (*(__RW u8  *)(zb_sfr_base + 0xf5))
#define ZB_UTBAUD16	                (*(__RW u8  *)(zb_sfr_base + 0xf4))

#define ZB_MCPCNT0   	                (*(__RW u8  *)(zb_sfr_base + 0xf8))
#define ZB_MCPCNT1   	                (*(__RW u8  *)(zb_sfr_base + 0xf9))
#define ZB_MCPMADR0  	                (*(__RW u8  *)(zb_sfr_base + 0xfa))
#define ZB_MCPMADR1  	                (*(__RW u8  *)(zb_sfr_base + 0xfb))
#define ZB_MCPSADR0    	                (*(__RW u8  *)(zb_sfr_base + 0xfc))
#define ZB_MCPSADR1  	                (*(__RW u8  *)(zb_sfr_base + 0xfd))
#define ZB_MCPSADR2  	                (*(__RW u8  *)(zb_sfr_base + 0xfe))
#define ZB_MCPCON    	                (*(__RW u8  *)(zb_sfr_base + 0xff))
#define ZB_MCPCNT16                     (*(__RW u16 *)(zb_sfr_base + 0xf8))
#define ZB_MCPMADR16                    (*(__RW u16 *)(zb_sfr_base + 0xfa))
#define ZB_MCPSADR16                    (*(__RW u16 *)(zb_sfr_base + 0xfc))



//.............  Full Speed USB ...................
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

