#ifndef __linux__
#error This file should not be compiled outside of Linux
#endif // __linux__

#include "cppsrc/U8g2lib.h"

extern "C" uint8_t u8x8_byte_linux_i2c(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr);
extern "C" uint8_t u8x8_linux_i2c_delay (u8x8_t * u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

typedef void (*u8g2_Setup_Func)(u8g2_t *u8g2, const u8g2_cb_t *rotation, u8x8_msg_cb byte_cb, u8x8_msg_cb gpio_and_delay_cb);

class U8G2LinuxI2C : public U8G2 {
  public: U8G2LinuxI2C(const u8g2_cb_t *rotation, uint8_t bus, uint8_t address, u8g2_Setup_Func setupFunc) {
    setupFunc(&u8g2, rotation, u8x8_byte_linux_i2c, u8x8_linux_i2c_delay);
    setI2CBus(bus);
    setI2CAddress(address);
  }
};

class U8G2_SH1106_128X64_NONAME_F_HW_I2C_LINUX : public U8G2LinuxI2C {
  public: U8G2_SH1106_128X64_NONAME_F_HW_I2C_LINUX(const u8g2_cb_t *rotation, uint8_t bus, uint8_t address) :
    U8G2LinuxI2C(rotation, bus, address, u8g2_Setup_sh1106_i2c_128x64_noname_f)
  {}
};

class U8G2_SSD1306_128X64_NONAME_F_HW_I2C_LINUX : public U8G2LinuxI2C {
  public: U8G2_SSD1306_128X64_NONAME_F_HW_I2C_LINUX(const u8g2_cb_t *rotation, uint8_t bus, uint8_t address) :
    U8G2LinuxI2C(rotation, bus, address, u8g2_Setup_ssd1306_i2c_128x64_noname_f)
  { }
};
