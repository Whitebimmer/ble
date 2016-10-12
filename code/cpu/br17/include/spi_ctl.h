#ifndef SPI_CTL_H_INCLUDED
#define SPI_CTL_H_INCLUDED

extern void spi_int(void);
extern void spi_wr(unsigned char adr , unsigned long dat);
extern unsigned long spi_rd(unsigned char adr);
extern void spi_wr_rg(unsigned char adr , unsigned char bit, unsigned char len, unsigned long dat);
extern char spi_agc_inc(char sel, char inc);
extern char spi_pwr_inc(char sel, char inc);
#endif
