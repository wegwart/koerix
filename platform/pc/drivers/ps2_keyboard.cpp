// SPDX-FileCopyrightText: 2017-2021 Alexander Ulmer
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <mm.h>
#include <lib/errno.h>
#include <x86/ports.h>
#include <dev/uart.h>
#include <interrupt.h>
#include <debug.h>
#include <dev/devices.h>
#include <scheduler.h>

class PS2Keyboard;

class PS2Controller
{
    friend class PS2Keyboard;

  public:
    enum Registers
    {
      DATA_PORT   = 0x60,
      CMD_PORT    = 0x64,
      STATUS_PORT = 0x64
    };

    struct ConfigByte
    {
      uint8_t irqPort1      : 1;
      uint8_t irqPort2      : 1;
      uint8_t systemState   : 1;
      uint8_t zero          : 1;
      uint8_t clkPort1      : 1;
      uint8_t clkPort2      : 1;
      uint8_t translPort1   : 1;
      uint8_t zero2         : 1;
    } _PACKED;

    struct StatusRegister
    {
      uint8_t outBufferFull : 1;
      uint8_t inBufferFull  : 1;
      uint8_t systemState   : 1;
      uint8_t toController  : 1;
      uint8_t specific      : 2;
      uint8_t errTimeout    : 1;
      uint8_t errParity     : 1;
    } _PACKED;

    PS2Controller();

  private:
    bool selfTest() {
      outb(CMD_PORT, 0xaa);

      size_t attempts = 5;
      while (attempts-- && (inb(STATUS_PORT) & 1) == 0)
        sched::yield();

      /* some PS/2 controllers apparently return 0x65 instead
       * of 0x55, so allow that, too. */
      uint8_t result = inb(DATA_PORT);
      return result == 0x55 || result == 0x65;
    }

    ConfigByte readConfig() {
      outb(CMD_PORT, 0x20);
      uint8_t config = inb(DATA_PORT);
      return *(ConfigByte*)&config;
    }

    void writeConfig(ConfigByte config) {
      outb(CMD_PORT, 0x60);
      outb(DATA_PORT, *(uint8_t*)&config);
    }

    StatusRegister readStatus() {
      uint8_t status = inb(STATUS_PORT);
      return *(StatusRegister*)&status;
    }

    void flushOutBuffer() {
      /* don't poll the status byte here, because if
       * there's no controller, we could get stuck
       * in an endless loop. */
      inb(DATA_PORT);
    }

    void setPort1IrqEnable(bool enable = true) {
      auto config = readConfig();
      config.irqPort1 = enable;
      writeConfig(config);
    }

    void setPort1Enable(bool enable = true) {
      outb(CMD_PORT, enable ? 0xae : 0xad);
    }

    void setPort2Enable(bool enable = true) {
      outb(CMD_PORT, enable ? 0xa8 : 0xa7);
    }

    uint8_t readData() {
      return inb(DATA_PORT);
    }

  public:
    static PS2Controller s_instance;
};

class PS2Keyboard : public devices::UartDevice
{
  public:
    PS2Keyboard(PS2Controller& controller);

    ssize_t write(char*, size_t) {
      return -ENOTSUP;
    }

    void irq() {
      char c = m_controller.readData();
      receive(&c, 1);
    }

  private:
    PS2Controller& m_controller;
};

static void irqHandler(PS2Keyboard* kbd)
{
  kbd->irq();
}

PS2Keyboard::PS2Keyboard(PS2Controller& controller)
  : m_controller(controller)
{
  m_controller.setPort1Enable();
  m_controller.setPort1IrqEnable();
  registerIrq(1, (void*)irqHandler, this);
}

PS2Controller::PS2Controller()
{
  assert(sizeof(ConfigByte) == 1);

  /* disable both ports during init */
  setPort1Enable(false);
  setPort2Enable(false);

  flushOutBuffer();

  /* performing a configuration update */
  auto config = readConfig();
  bool couldBeDualChannel = config.clkPort2;
  config.irqPort1 = 0;
  config.irqPort2 = 0;
  config.translPort1 = 0;
  writeConfig(config);

  /* perform the PS2 controller self test. by now, this is the only
   * way to know whether there actually is a controller or not.
   * better: detect via ACPI */
  if (!selfTest())
  {
    debug(DEVICES) << "ps/2: self test failed. probably not present\n";
    return;
  }

  /* check if the controller supports two channels */
  bool isDualChannel = false;
  if (couldBeDualChannel)
  {
    setPort2Enable();
    config = readConfig();
    if (config.clkPort2 == 0)
      isDualChannel = true;
    setPort2Enable(false);
  }
  debug(DEVICES) << "ps/2 channels: "
                 << (isDualChannel ? "2" : "1") << "\n";

  auto kbd = new PS2Keyboard(*this);
  dev::registerDevice("keyboard", kbd);
}

PS2Controller PS2Controller::s_instance;

void ps2kbd_init()
{
  new (&PS2Controller::s_instance) PS2Controller();
}
