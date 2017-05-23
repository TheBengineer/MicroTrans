  MEMORY
  {
    page0   (rwx) : ORIGIN = 0x0, LENGTH = 0x3000
    data    (rw)  : ORIGIN = 0x3000, LENGTH = 0x1000
    eeprom  (rx)  : ORIGIN = 0x4000, LENGTH = 0x800
    lookup  (rx)  : ORIGIN = 0x4800, LENGTH = 0x1C00
    text3   (rx)  : ORIGIN = 0x6400, LENGTH = 0x1c00
    text    (rx)  : ORIGIN = 0xC000, LENGTH = 0x3780
    text38  (rx)  : ORIGIN = 0x0F0000, LENGTH = 0x4000
    text39  (rx)  : ORIGIN = 0x0F4000, LENGTH = 0x4000
    text3a  (rx)  : ORIGIN = 0x0F8000, LENGTH = 0x4000
    text3b  (rx)  : ORIGIN = 0x0FC000, LENGTH = 0x4000
    text3c  (rx)  : ORIGIN = 0x100000, LENGTH = 0x4000
    text3d  (rx)  : ORIGIN = 0x104000, LENGTH = 0x4000

  }
  PROVIDE (_stack = 0x3fff);
