/*
  ConfigurableFirmata.h - ConfigurableFirmata library v2.10.1 - 2018-6-2
  Copyright (c) 2006-2008 Hans-Christoph Steiner.  All rights reserved.
  Copyright (c) 2013 Norbert Truchsess. All rights reserved.
  Copyright (c) 2013-2017 Jeff Hoefs. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  See file LICENSE.txt for further informations on licensing terms.
*/

#ifndef Configurable_Firmata_h
#define Configurable_Firmata_h

#include "utility/Boards.h"  /* Hardware Abstraction Layer + Wiring/Arduino */

/* Version numbers for the protocol.  The protocol is still changing, so these
 * version numbers are important.
 * Query using the REPORT_VERSION message.
 */
#define FIRMATA_PROTOCOL_MAJOR_VERSION  2 // for non-compatible changes
#define FIRMATA_PROTOCOL_MINOR_VERSION  7 // for backwards compatible changes
#define FIRMATA_PROTOCOL_BUGFIX_VERSION 0 // for bugfix releases

/*
 * Version numbers for the Firmata library.
 * ConfigurableFirmata 3.1 implements version 2.7.0 of the Firmata protocol.
 * The firmware version will not always equal the protocol version going forward.
 * Query using the REPORT_FIRMWARE message.
 */
#define FIRMATA_FIRMWARE_MAJOR_VERSION  3 // for non-compatible changes
#define FIRMATA_FIRMWARE_MINOR_VERSION  1 // for backwards compatible changes
#define FIRMATA_FIRMWARE_BUGFIX_VERSION 0 // for bugfix releases

#ifdef LARGE_MEM_DEVICE
#define MAX_DATA_BYTES         252 // The ESP32 has enough RAM so we can reduce the number of packets, but the value must not exceed 2^8 - 1, because many methods use byte-indexing only
#else
#define MAX_DATA_BYTES          64 // max number of data bytes in incoming messages
#endif
#define LARGE_MEM_RCV_BUF_SIZE 4096 // Size of the wifi receive buffer for large mem devices. If this is smaller than 1024, heavy transactions are significantly slower

// Arduino 101 also defines SET_PIN_MODE as a macro in scss_registers.h
#ifdef SET_PIN_MODE
#undef SET_PIN_MODE
#endif

// message command bytes (128-255/0x80-0xFF)
#define DIGITAL_MESSAGE         0x90 // send data for a digital port (8 bits)
#define ANALOG_MESSAGE          0xE0 // send data for an analog pin (or PWM)
#define REPORT_ANALOG           0xC0 // enable analog input by pin #
#define REPORT_DIGITAL          0xD0 // enable digital input by port pair
//
#define SET_PIN_MODE            0xF4 // set a pin to INPUT/OUTPUT/PWM/etc
#define SET_DIGITAL_PIN_VALUE   0xF5 // set value of an individual digital pin
//
#define REPORT_VERSION          0xF9 // report protocol version
#define SYSTEM_RESET            0xFF // reset from MIDI
//
#define START_SYSEX             0xF0 // start a MIDI Sysex message
#define END_SYSEX               0xF7 // end a MIDI Sysex message

// extended command set using sysex (0-127/0x00-0x7F)
/* 0x00-0x0F reserved for user-defined commands */
#define SERIAL_MESSAGE          0x60 // communicate with serial devices, including other boards
#define ENCODER_DATA            0x61 // reply with encoders current positions
#define ACCELSTEPPER_DATA       0x62 // control a stepper motor
#define REPORT_DIGITAL_PIN      0x63 // (reserved)
#define EXTENDED_REPORT_ANALOG  0x64 // Enable reporting analog channels > 15. Supported with v3.1 or later.
#define REPORT_FEATURES         0x65 // (reserved)
#define SPI_DATA                0x68 // SPI Commands start with this byte
#define ANALOG_MAPPING_QUERY    0x69 // ask for mapping of analog to pin numbers
#define ANALOG_MAPPING_RESPONSE 0x6A // reply with mapping info
#define CAPABILITY_QUERY        0x6B // ask for supported modes and resolution of all pins
#define CAPABILITY_RESPONSE     0x6C // reply with supported modes and resolution
#define PIN_STATE_QUERY         0x6D // ask for a pin's current mode and value
#define PIN_STATE_RESPONSE      0x6E // reply with pin's current mode and value
#define EXTENDED_ANALOG         0x6F // analog write (PWM, Servo, etc) to any pin or analog input from a pin > 15
#define SERVO_CONFIG            0x70 // set max angle, minPulse, maxPulse, freq
#define STRING_DATA             0x71 // a string message with 14-bits per char
#define STEPPER_DATA            0x72 // control a stepper motor
#define ONEWIRE_DATA            0x73 // send an OneWire read/write/reset/select/skip/search request
#define DHTSENSOR_DATA          0x74 // Used by DhtFirmata
#define SHIFT_DATA              0x75 // a bitstream to/from a shift register
#define I2C_REQUEST             0x76 // send an I2C read/write request
#define I2C_REPLY               0x77 // a reply to an I2C read request
#define I2C_CONFIG              0x78 // config I2C settings such as delay times and power pins
#define REPORT_FIRMWARE         0x79 // report name and version of the firmware
#define SAMPLING_INTERVAL       0x7A // set the poll rate of the main loop
#define SCHEDULER_DATA          0x7B // send a createtask/deletetask/addtotask/schedule/querytasks/querytask request to the scheduler
#define ANALOG_CONFIG           0x7C // (reserved)
#define FREQUENCY_COMMAND       0x7D // Command for the Frequency module
#define SYSEX_NON_REALTIME      0x7E // MIDI Reserved for non-realtime messages
#define SYSEX_REALTIME          0x7F // MIDI Reserved for realtime messages

// these are DEPRECATED to make the naming more consistent
#define FIRMATA_STRING          0x71 // same as STRING_DATA
#define SYSEX_I2C_REQUEST       0x76 // same as I2C_REQUEST
#define SYSEX_I2C_REPLY         0x77 // same as I2C_REPLY
#define SYSEX_SAMPLING_INTERVAL 0x7A // same as SAMPLING_INTERVAL

// pin modes
#define PIN_MODE_INPUT          0x00 // INPUT is defined in Arduino.h, but may not be the same as this one
#define PIN_MODE_OUTPUT         0x01 // OUTPUT is defined in Arduino.h. Careful: OUTPUT is defined as 2 on ESP32!
                                     // therefore OUTPUT and PIN_MODE_OUTPUT are not the same!
#define PIN_MODE_ANALOG         0x02 // analog pin in analogInput mode
#define PIN_MODE_PWM            0x03 // digital pin in PWM output mode
#define PIN_MODE_SERVO          0x04 // digital pin in Servo output mode
#define PIN_MODE_SHIFT          0x05 // shiftIn/shiftOut mode
#define PIN_MODE_I2C            0x06 // pin included in I2C setup
#define PIN_MODE_ONEWIRE        0x07 // pin configured for 1-wire
#define PIN_MODE_STEPPER        0x08 // pin configured for stepper motor
#define PIN_MODE_ENCODER        0x09 // pin configured for rotary encoders
#define PIN_MODE_SERIAL         0x0A // pin configured for serial communication
#define PIN_MODE_PULLUP         0x0B // enable internal pull-up resistor for pin
// Extensions under development
#define PIN_MODE_SPI            0x0C // pin configured for SPI
#define PIN_MODE_SONAR =        0x0D // pin configured for HC-SR04
#define PIN_MODE_TONE =         0x0E // pin configured for tone
#define PIN_MODE_DHT            0x0F // pin configured for DHT
#define PIN_MODE_FREQUENCY      0x10 // pin configured for frequency measurement

#define PIN_MODE_IGNORE         0x7F // pin configured to be ignored by digitalWrite and capabilityResponse
#define TOTAL_PIN_MODES         16

extern "C" {
  // callback function types
  typedef void (*callbackFunction)(byte, int);
  typedef void (*systemResetCallbackFunction)(void);
  typedef void (*stringCallbackFunction)(char *);
  typedef void (*sysexCallbackFunction)(byte command, byte argc, byte *argv);
  typedef void (*delayTaskCallbackFunction)(long delay);
}

typedef const __FlashStringHelper FlashString;

// TODO make it a subclass of a generic Serial/Stream base class
class FirmataClass
{
  public:
    FirmataClass();
    /* Arduino constructors */
    void begin();
    void begin(long);
    void begin(Stream &s, bool isConsole = true);
    /* querying functions */
    void printVersion(void);
    void blinkVersion(void);
    void printFirmwareVersion(void);
    //void setFirmwareVersion(byte major, byte minor);  // see macro below
    void setFirmwareNameAndVersion(const char *name, byte major, byte minor);
    void disableBlinkVersion();
    /* serial receive handling */
    int available(void);
    void processInput(void);
    void parse(byte inputData);
    void resetParser();
    boolean isParsingMessage(void);
    boolean isResetting(void);
    /* serial send handling */
    void sendAnalog(byte pin, int value);
    void sendDigital(byte pin, int value); // TODO implement this
    void sendDigitalPort(byte portNumber, int portData);
    void sendString(const FlashString* flashString);
    void sendString(const FlashString* flashString, uint32_t errorData);
    void sendStringf(const FlashString* fmt, ...);
    void sendString(byte command, const char *string);
    void sendSysex(byte command, byte bytec, byte *bytev);
    void write(byte c);

    size_t write(byte* buf, size_t length);

    void sendPackedUInt14(uint16_t value);
    void sendPackedUInt32(uint32_t value);
    void sendPackedUInt64(uint64_t value);
    uint16_t decodePackedUInt14(byte* argv);
    uint32_t decodePackedUInt32(byte* argv);
    uint64_t decodePackedUInt64(byte* argv);
    /* attach & detach callback functions to messages */
    void attach(byte command, callbackFunction newFunction);
    void attach(byte command, systemResetCallbackFunction newFunction);
    void attach(byte command, stringCallbackFunction newFunction);
    void attach(byte command, sysexCallbackFunction newFunction);
    void detach(byte command);
    /* delegate to Scheduler (if any) */
    void attachDelayTask(delayTaskCallbackFunction newFunction);
    void delayTask(long delay);
    /* access pin config */
    byte getPinMode(byte pin);
    void setPinMode(byte pin, byte config);
    /* access pin state */
    int getPinState(byte pin);
    void setPinState(byte pin, byte state);

    /* utility methods */
    void sendValueAsTwo7bitBytes(int value);
    void startSysex(void);
    void endSysex(void);

  private:
    Stream *FirmataStream;
    /* firmware name and version */
    const char *firmwareVersionName;
    byte firmwareVersionMajor;
    byte firmwareVersionMinor;
    /* input message handling */
    byte waitForData; // this flag says the next serial input will be data
    byte executeMultiByteCommand; // execute this after getting multi-byte data
    byte multiByteChannel; // channel data for multiByteCommands
    byte storedInputData[MAX_DATA_BYTES]; // multi-byte data
    /* sysex */
    boolean parsingSysex;
    int sysexBytesRead;
    /* pins configuration */
    byte pinConfig[TOTAL_PINS];         // configuration of every pin
    byte pinState[TOTAL_PINS];           // any value that has been written

    boolean resetting;

    // True if the current stream is also the console, 
    // if false, we log information messages separately to the console
    boolean outputIsConsole;

    /* callback functions */
    callbackFunction currentDigitalCallback;
    callbackFunction currentReportAnalogCallback;
    callbackFunction currentReportDigitalCallback;
    callbackFunction currentPinModeCallback;
    callbackFunction currentPinValueCallback;
    systemResetCallbackFunction currentSystemResetCallback;
    stringCallbackFunction currentStringCallback;
    sysexCallbackFunction currentSysexCallback;
    delayTaskCallbackFunction delayTaskCallback;

    boolean blinkVersionDisabled;

    /* private methods ------------------------------ */
    void processSysexMessage(void);
    void systemReset(void);
    void strobeBlinkPin(byte pin, int count, int onInterval, int offInterval);
#ifdef LARGE_MEM_DEVICE
    byte readCache[LARGE_MEM_RCV_BUF_SIZE];
#endif
};

extern FirmataClass Firmata;

/*==============================================================================
 * MACROS
 *============================================================================*/

/* shortcut for setFirmwareNameAndVersion() that uses __FILE__ to set the
 * firmware name.  It needs to be a macro so that __FILE__ is included in the
 * firmware source file rather than the library source file.
 */
#define setFirmwareVersion(x, y)   setFirmwareNameAndVersion(__FILE__, x, y)

#endif /* Configurable_Firmata_h */
