/*
  ConfigurableFirmata.pp - ConfigurableFirmata library v2.10.1 - 2017-6-2
  Copyright (c) 2006-2008 Hans-Christoph Steiner.  All rights reserved.
  Copyright (c) 2013 Norbert Truchsess. All rights reserved.
  Copyright (c) 2013-2017 Jeff Hoefs. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  See file LICENSE.txt for further informations on licensing terms.
*/

//******************************************************************************
//* Includes
//******************************************************************************

#include "ConfigurableFirmata.h"
#include "HardwareSerial.h"

extern "C" {
#include <string.h>
#include <stdlib.h>
}

//******************************************************************************
//* Support Functions
//******************************************************************************

/**
 * Split a 14-bit byte into two 7-bit values and write each value.
 * @param value The 14-bit value to be split and written separately.
 */
void FirmataClass::sendValueAsTwo7bitBytes(int value)
{
  FirmataStream->write(value & B01111111); // LSB
  FirmataStream->write(value >> 7 & B01111111); // MSB
}

/**
 * A helper method to write the beginning of a Sysex message transmission.
 */
void FirmataClass::startSysex(void)
{
  FirmataStream->write(START_SYSEX);
}

/**
 * A helper method to write the end of a Sysex message transmission.
 */
void FirmataClass::endSysex(void)
{
  FirmataStream->write(END_SYSEX);
  FirmataStream->flush();
}

//******************************************************************************
//* Constructors
//******************************************************************************

/**
 * The Firmata class.
 * An instance named "Firmata" is created automatically for the user.
 */
FirmataClass::FirmataClass()
{
  firmwareVersionMinor = 0;
  firmwareVersionMajor = 0;
  firmwareVersionName = "";
  blinkVersionDisabled = false;
  systemReset();
}

//******************************************************************************
//* Public Methods
//******************************************************************************

/**
 * Initialize the default Serial transport at the default baud of 57600.
 */
void FirmataClass::begin(void)
{
    begin(57600);
    outputIsConsole = true;
}

/**
 * Initialize the default Serial transport and override the default baud.
 * Sends the protocol version to the host application followed by the firmware version and name.
 * blinkVersion is also called. To skip the call to blinkVersion, call Firmata.disableBlinkVersion()
 * before calling Firmata.begin(baud).
 * @param speed The baud to use. 57600 baud is the default value.
 */
void FirmataClass::begin(long speed)
{
    Serial.begin(speed);
    Serial.setTimeout(0);
    FirmataStream = &Serial;
    outputIsConsole = true;
    blinkVersion();
    printVersion();         // send the protocol version
    printFirmwareVersion(); // send the firmware name and version
}

/**
 * Reassign the Firmata stream transport.
 * @param s A reference to the Stream transport object. This can be any type of
 * transport that implements the Stream interface. Some examples include Ethernet, WiFi
 * and other UARTs on the board (Serial1, Serial2, etc).
 */
void FirmataClass::begin(Stream& s, bool isConsole)
{
    FirmataStream = &s;
    outputIsConsole = isConsole;
    // do not call blinkVersion() here because some hardware such as the
    // Ethernet shield use pin 13
    printVersion();         // send the protocol version
    printFirmwareVersion(); // send the firmware name and version
}

/**
 * Send the Firmata protocol version to the Firmata host application.
 */
void FirmataClass::printVersion(void)
{
  FirmataStream->write(REPORT_VERSION);
  FirmataStream->write(FIRMATA_PROTOCOL_MAJOR_VERSION);
  FirmataStream->write(FIRMATA_PROTOCOL_MINOR_VERSION);
}

/**
 * Blink the Firmata protocol version to the onboard LEDs (if the board has an onboard LED).
 * If VERSION_BLINK_PIN is not defined in Boards.h for a particular board, then this method
 * does nothing.
 * The first series of flashes indicates the firmware major version (2 flashes = 2).
 * The second series of flashes indicates the firmware minor version (5 flashes = 5).
 */
void FirmataClass::blinkVersion(void)
{
#if defined(VERSION_BLINK_PIN)
  if (blinkVersionDisabled) return;
  // flash the pin with the protocol version
  pinMode(VERSION_BLINK_PIN, OUTPUT);
  strobeBlinkPin(VERSION_BLINK_PIN, FIRMATA_FIRMWARE_MAJOR_VERSION, 40, 210);
  delay(250);
  strobeBlinkPin(VERSION_BLINK_PIN, FIRMATA_FIRMWARE_MINOR_VERSION, 40, 210);
  delay(125);
#endif
}

/**
 * Provides a means to disable the version blink sequence on the onboard LED, trimming startup
 * time by a couple of seconds.
 * Call this before Firmata.begin(). It only applies when using the default Serial transport.
 */
void FirmataClass::disableBlinkVersion()
{
    blinkVersionDisabled = true;
}

/**
 * Sends the firmware name and version to the Firmata host application. The major and minor version
 * numbers are the first 2 bytes in the message. The following bytes are the characters of the
 * firmware name.
 */
void FirmataClass::printFirmwareVersion(void)
{
    if (firmwareVersionMajor != 0 && FirmataStream != nullptr) { // make sure that the name has been set before reporting
        startSysex();
        FirmataStream->write(REPORT_FIRMWARE);
        FirmataStream->write(firmwareVersionMajor); // major version number
        FirmataStream->write(firmwareVersionMinor); // minor version number
        size_t len = strlen(firmwareVersionName);
        for (size_t i = 0; i < len; ++i)
        {
            sendValueAsTwo7bitBytes(firmwareVersionName[i]);
        }
        endSysex();
    }
}

/**
 * Sets the name and version of the firmware. This is not the same version as the Firmata protocol
 * (although at times the firmware version and protocol version may be the same number).
 * @param name A pointer to the name char array
 * @param major The major version number
 * @param minor The minor version number
 */
void FirmataClass::setFirmwareNameAndVersion(const char *name, byte major, byte minor)
{
  firmwareVersionName = name;
  firmwareVersionMajor = major;
  firmwareVersionMinor = minor;
}

//------------------------------------------------------------------------------
// Input Stream Handling

/**
 * A wrapper for Stream::available()
 * @return The number of bytes remaining in the input stream buffer.
 */
int FirmataClass::available(void)
{
  return FirmataStream->available();
}

/**
 * Process incoming sysex messages. Handles REPORT_FIRMWARE and STRING_DATA internally.
 * Calls callback function for STRING_DATA and all other sysex messages.
 * @private
 */
void FirmataClass::processSysexMessage(void)
{
  switch (storedInputData[0]) { //first byte in buffer is command
    case REPORT_FIRMWARE:
      printFirmwareVersion();
      break;
    case STRING_DATA:
      if (currentStringCallback) {
        byte bufferLength = (sysexBytesRead - 1) / 2;
        if (bufferLength <= 0)
        {
          break;
        }
        byte i = 1;
        byte j = 0;
        while (j < bufferLength) {
          // The string length will only be at most half the size of the
          // stored input buffer so we can decode the string within the buffer.
          storedInputData[j] = storedInputData[i];
          i++;
          storedInputData[j] += (storedInputData[i] << 7);
          i++;
          j++;
        }
        // Make sure string is null terminated. This may be the case for data
        // coming from client libraries in languages that don't null terminate
        // strings.
        if (storedInputData[j - 1] != '\0') {
          storedInputData[j] = '\0';
        }
        (*currentStringCallback)((char *)&storedInputData[0]);
      }
      break;
    default:
      if (currentSysexCallback)
        (*currentSysexCallback)(storedInputData[0], sysexBytesRead - 1, storedInputData + 1);
  }
}


/**
 * Read a single int from the input stream. If the value is not = -1, pass it on to parse(byte)
 */
void FirmataClass::processInput(void)
{
#ifdef LARGE_MEM_DEVICE
    int readCachePos = 0;
    const int writeCachePos = FirmataStream->readBytes(readCache, LARGE_MEM_RCV_BUF_SIZE);
    
    while (parsingSysex && writeCachePos > readCachePos + 4)
    {
        uint32_t nextWord = *(uint32_t*)(readCache + readCachePos);
        // Any special character in the current word?
        if (nextWord & 0x80808080)
        {
            break; // And don't increment (we read the bytes again)
        }
        readCachePos += 4;
        *(uint32_t*)(storedInputData + sysexBytesRead) = nextWord;
        sysexBytesRead += 4;
    }

    while (writeCachePos > readCachePos)
    {
        const byte inputData = readCache[readCachePos];
        readCachePos++;
        parse(inputData);
    }

#else
    int inputData = FirmataStream->read();
    if (inputData != -1)
    {
        parse(inputData);
    }
#endif
}

void FirmataClass::resetParser()
{
    parsingSysex = false;
    sysexBytesRead = 0;
    waitForData = 0;
    executeMultiByteCommand = 0;
}

/**
 * Parse data from the input stream.
 * @param inputData A single byte to be added to the parser.
 */
void FirmataClass::parse(byte inputData)
{
  int command;

  // TODO make sure it handles -1 properly

  // Firmata.sendStringf(F("Received byte 0x%x"), (int)inputData);

  if (inputData == SYSTEM_RESET)
  {
      // A system reset shall always be done, regardless of the state of the parser.
      parsingSysex = false;
      sysexBytesRead = 0;
      waitForData = 0;
      systemReset();
      return;
  }
  if (parsingSysex) {
    if (inputData == END_SYSEX) {
		//stop sysex byte
      parsingSysex = false;
      //fire off handler function
      processSysexMessage();
    } else {
      //normal data byte - add to buffer
      storedInputData[sysexBytesRead] = inputData;
      sysexBytesRead++;
	  if (sysexBytesRead == MAX_DATA_BYTES)
	  {
		  Firmata.sendString(F("Discarding input message, out of buffer"));
		  parsingSysex = false;
		  sysexBytesRead = 0;
        waitForData = 0;
      }
	  }
  } else if ( (waitForData > 0) && (inputData < 128) ) {
    waitForData--;
    storedInputData[waitForData] = inputData; // this inverses the order: element 0 is the MSB of the argument!
    if ( (waitForData == 0) && executeMultiByteCommand ) { // got the whole message
      switch (executeMultiByteCommand) {
        case ANALOG_MESSAGE:
        {
            // Repack analog message as EXTENDED_ANALOG sysex message
            byte b0 = storedInputData[0];
            byte b1 = storedInputData[1];
            storedInputData[0] = EXTENDED_ANALOG;
            storedInputData[1] = multiByteChannel;
            storedInputData[2] = b1;
            storedInputData[3] = b0;
            storedInputData[4] = END_SYSEX;
            sysexBytesRead = 4; // Not including the END_SYSEX byte
            processSysexMessage();
        }
          break;
        case DIGITAL_MESSAGE:
          if (currentDigitalCallback) {
            (*currentDigitalCallback)(multiByteChannel,
                                      (storedInputData[0] << 7)
                                      + storedInputData[1]);
          }
          break;
        case SET_PIN_MODE:
          setPinMode(storedInputData[1], storedInputData[0]);
          break;
        case SET_DIGITAL_PIN_VALUE:
          if (currentPinValueCallback)
            (*currentPinValueCallback)(storedInputData[1], storedInputData[0]);
          break;
        case REPORT_ANALOG:
          if (currentReportAnalogCallback)
            (*currentReportAnalogCallback)(multiByteChannel, storedInputData[0]);
          break;
        case REPORT_DIGITAL:
          if (currentReportDigitalCallback)
            (*currentReportDigitalCallback)(multiByteChannel, storedInputData[0]);
          break;
      }
      executeMultiByteCommand = 0;
    }
  } else {
    // remove channel info from command byte if less than 0xF0
    if (inputData < 0xF0) {
      command = inputData & 0xF0;
      multiByteChannel = inputData & 0x0F;
    } else {
      command = inputData;
      // commands in the 0xF* range don't use channel data
    }
    switch (command) {
      case ANALOG_MESSAGE:
      case DIGITAL_MESSAGE:
      case SET_PIN_MODE:
      case SET_DIGITAL_PIN_VALUE:
        waitForData = 2; // two data bytes needed
        executeMultiByteCommand = command;
        break;
      case REPORT_ANALOG:
      case REPORT_DIGITAL:
        waitForData = 1; // one data byte needed
        executeMultiByteCommand = command;
        break;
      case START_SYSEX:
        parsingSysex = true;
        sysexBytesRead = 0;
        break;
      case SYSTEM_RESET:
        systemReset();
        break;
      case REPORT_VERSION:
        Firmata.printVersion();
        break;
    }
  }
}

/**
 * @return Returns true if the parser is actively parsing data.
 */
boolean FirmataClass::isParsingMessage(void)
{
  return (waitForData > 0 || parsingSysex);
}

/**
 * @return Returns true if the SYSTEM_RESET message is being executed
 */
boolean FirmataClass::isResetting(void)
{
  return resetting;
}

//------------------------------------------------------------------------------
// Output Stream Handling

/**
 * Send an analog message to the Firmata host application. The range of pins is limited to [0..15]
 * when using the ANALOG_MESSAGE. The maximum value of the ANALOG_MESSAGE is limited to 14 bits
 * (16384). To increase the pin range or value, see the documentation for the EXTENDED_ANALOG
 * message.
 * @param analogChannel The analog pin to send the value of (limited to pins 0 - 15).
 * @param value The value of the analog pin (0 - 1024 for 10-bit analog, 0 - 4096 for 12-bit, etc).
 * The maximum value is 14-bits (16384).
 */
void FirmataClass::sendAnalog(byte analogPin, int value)
{
    if (analogPin <= 15)
    {
        // pin can only be 0-15, so chop higher bits
        FirmataStream->write(ANALOG_MESSAGE | (analogPin & 0xF));
        sendValueAsTwo7bitBytes(value);
    }
    else
    {
        startSysex();
        FirmataStream->write(EXTENDED_ANALOG);
        FirmataStream->write(analogPin);
        sendValueAsTwo7bitBytes(value);
        endSysex();
    }
}

/* (intentionally left out asterix here)
 * STUB - NOT IMPLEMENTED
 * Send a single digital pin value to the Firmata host application.
 * @param pin The digital pin to send the value of.
 * @param value The value of the pin.
 */
void FirmataClass::sendDigital(byte pin, int value)
{
  /* TODO add single pin digital messages to the protocol, this needs to
   * track the last digital data sent so that it can be sure to change just
   * one bit in the packet.  This is complicated by the fact that the
   * numbering of the pins will probably differ on Arduino, Wiring, and
   * other boards.  The DIGITAL_MESSAGE sends 14 bits at a time, but it is
   * probably easier to send 8 bit ports for any board with more than 14
   * digital pins.
   */

  // TODO: the digital message should not be sent on the serial port every
  // time sendDigital() is called.  Instead, it should add it to an int
  // which will be sent on a schedule.  If a pin changes more than once
  // before the digital message is sent on the serial port, it should send a
  // digital message for each change.

  //    if(value == 0)
  //        sendDigitalPortPair();
}


/**
 * Send an 8-bit port in a single digital message (protocol v2 and later).
 * Send 14-bits in a single digital message (protocol v1).
 * @param portNumber The port number to send. Note that this is not the same as a "port" on the
 * physical microcontroller. Ports are defined in order per every 8 pins in ascending order
 * of the Arduino digital pin numbering scheme. Port 0 = pins D0 - D7, port 1 = pins D8 - D15, etc.
 * @param portData The value of the port. The value of each pin in the port is represented by a bit.
 */
void FirmataClass::sendDigitalPort(byte portNumber, int portData)
{
    byte msg[3];
    msg[0] = (DIGITAL_MESSAGE | (portNumber & 0xF));
    msg[1] = ((byte)portData % 128); // Tx bits 0-6
    msg[2] = (portData >> 7);  // Tx bits 7-13
    FirmataStream->write(msg, 3);
}

/**
 * Send a sysex message where all values after the command byte are packet as 2 7-bit bytes
 * (this is not always the case so this function is not always used to send sysex messages).
 * @param command The sysex command byte.
 * @param bytec The number of data bytes in the message (excludes start, command and end bytes).
 * @param bytev A pointer to the array of data bytes to send in the message.
 */
void FirmataClass::sendSysex(byte command, byte bytec, byte *bytev)
{
  byte i;
  startSysex();
  FirmataStream->write(command);
  for (i = 0; i < bytec; i++) {
    sendValueAsTwo7bitBytes(bytev[i]);
  }
  endSysex();
}

/**
 * Send a string to the Firmata host application.
 * @param command Must be STRING_DATA
 * @param string A pointer to the char string
 */
void FirmataClass::sendString(byte command, const char *string)
{
  if (string == nullptr)
  {
    return;
  }
  sendSysex(command, (byte)strlen(string), (byte *)string);
}

/**
 * Send a string to the Firmata host application.
 * @param flashString A pointer to the char string
 */
void FirmataClass::sendStringf(const FlashString* flashString, ...) 
{
	// The parameter "sizeOfArgs" is currently unused.
	// 16 bit board?
#ifdef ARDUINO_ARCH_AVR
    const int maxSize = 32;
#else
    const int maxSize = 255;
#endif
	int len = strlen_P((const char*)flashString);
    if (len >= maxSize)
    {
        return;
    }
	va_list va;
    va_start (va, flashString);
	char bytesInput[maxSize];
	char bytesOutput[maxSize];
	startSysex();
	FirmataStream->write(STRING_DATA);
	for (int i = 0; i < len; i++) 
	{
		bytesInput[i] = (pgm_read_byte(((const char*)flashString) + i));
    }
	bytesInput[len] = 0;
	memset(bytesOutput, 0, sizeof(char) * maxSize);
	
	vsnprintf(bytesOutput, maxSize, bytesInput, va);
    if (!outputIsConsole)
    {
        Serial.println(bytesOutput);
    }

	len = strlen(bytesOutput);
	for (int i = 0; i < len; i++) 
	{
		sendValueAsTwo7bitBytes(bytesOutput[i]);
    }
	
	endSysex();
    va_end (va);
}

/**
 * Send a constant string to the Firmata host application.
 * @param flashString A pointer to the string in flash memory
 */
void FirmataClass::sendString(const FlashString* flashString)
{
    int len = strlen_P((const char*)flashString);
    if (!outputIsConsole)
    {
        Serial.println(flashString);
    }
    startSysex();
    FirmataStream->write(STRING_DATA);
    for (int i = 0; i < len; i++) 
    {
        sendValueAsTwo7bitBytes(pgm_read_byte(((const char*)flashString) + i));
    }
    endSysex();
}

/**
 * Send a constant string to the Firmata host application.
 * @param flashString A pointer to the string in flash memory
 * @param errorData A number that is sent out with the string (i.e. error code, unrecognized command number)
 */
void FirmataClass::sendString(const FlashString* flashString, uint32_t errorData)
{
    int len = strlen_P((const char*)flashString);
#ifndef SIM
    if (!outputIsConsole)
    {
        Serial.print(flashString);
        Serial.println(errorData);
    }
#endif
    startSysex();
    FirmataStream->write(STRING_DATA);
    for (int i = 0; i < len; i++) {
        sendValueAsTwo7bitBytes(pgm_read_byte(((const char*)flashString) + i));
    }
    String error = String(errorData, HEX);
    for (unsigned int i = 0; i < error.length(); i++) {
        sendValueAsTwo7bitBytes((byte)error.charAt(i));
    }

    endSysex();
}


/**
 * A wrapper for Stream::available().
 * Write a single byte to the output stream.
 * @param c The byte to be written.
 */
void FirmataClass::write(byte c)
{
  FirmataStream->write(c);
}

size_t FirmataClass::write(byte* buf, size_t length)
{
    return FirmataStream->write(buf, length);
}


/**
 * Attach a generic sysex callback function to a command (options are: ANALOG_MESSAGE,
 * DIGITAL_MESSAGE, REPORT_ANALOG, REPORT DIGITAL, SET_PIN_MODE and SET_DIGITAL_PIN_VALUE).
 * @param command The ID of the command to attach a callback function to.
 * @param newFunction A reference to the callback function to attach.
 */
void FirmataClass::attach(byte command, callbackFunction newFunction)
{
  switch (command) {
    case DIGITAL_MESSAGE: currentDigitalCallback = newFunction; break;
    case REPORT_ANALOG: currentReportAnalogCallback = newFunction; break;
    case REPORT_DIGITAL: currentReportDigitalCallback = newFunction; break;
    case SET_PIN_MODE: currentPinModeCallback = newFunction; break;
    case SET_DIGITAL_PIN_VALUE: currentPinValueCallback = newFunction; break;
  }
}

/**
 * Attach a callback function for the SYSTEM_RESET command.
 * @param command Must be set to SYSTEM_RESET or it will be ignored.
 * @param newFunction A reference to the system reset callback function to attach.
 */
void FirmataClass::attach(byte command, systemResetCallbackFunction newFunction)
{
  switch (command) {
    case SYSTEM_RESET: currentSystemResetCallback = newFunction; break;
  }
}

/**
 * Attach a callback function for the STRING_DATA command.
 * @param command Must be set to STRING_DATA or it will be ignored.
 * @param newFunction A reference to the string callback function to attach.
 */
void FirmataClass::attach(byte command, stringCallbackFunction newFunction)
{
  switch (command) {
    case STRING_DATA: currentStringCallback = newFunction; break;
  }
}

/**
 * Attach a generic sysex callback function to sysex command.
 * @param command The ID of the command to attach a callback function to.
 * @param newFunction A reference to the sysex callback function to attach.
 */
void FirmataClass::attach(byte command, sysexCallbackFunction newFunction)
{
  currentSysexCallback = newFunction;
}

/**
 * Detach a callback function for a specified command (such as SYSTEM_RESET, STRING_DATA,
 * ANALOG_MESSAGE, DIGITAL_MESSAGE, etc).
 * @param command The ID of the command to detatch the callback function from.
 */
void FirmataClass::detach(byte command)
{
  switch (command) {
    case SYSTEM_RESET: currentSystemResetCallback = NULL; break;
    case STRING_DATA: currentStringCallback = NULL; break;
    case START_SYSEX: currentSysexCallback = NULL; break;
    default:
      attach(command, (callbackFunction)NULL);
  }
}

/**
 * Detach a callback function for a delayed task when using FirmataScheduler
 * @see FirmataScheduler
 * @param newFunction A reference to the delay task callback function to attach.
 */
void FirmataClass::attachDelayTask(delayTaskCallbackFunction newFunction)
{
  delayTaskCallback = newFunction;
}

/**
 * Call the delayTask callback function when using FirmataScheduler. Must first attach a callback
 * using attachDelayTask.
 * @see FirmataScheduler
 * @param delay The amount of time to delay in milliseconds.
 */
void FirmataClass::delayTask(long delay)
{
  if (delayTaskCallback) {
    (*delayTaskCallback)(delay);
  }
}

/**
 * @param pin The pin to get the configuration of.
 * @return The configuration of the specified pin.
 */
byte FirmataClass::getPinMode(byte pin)
{
  return pinConfig[pin];
}

/**
 * Set the pin mode/configuration. The pin configuration (or mode) in Firmata represents the
 * current function of the pin. Examples are digital input or output, analog input, pwm, i2c,
 * serial (uart), etc.
 * @param pin The pin to configure.
 * @param config The configuration value for the specified pin.
 */
void FirmataClass::setPinMode(byte pin, byte config)
{
  if (pinConfig[pin] == PIN_MODE_IGNORE)
    return;
  pinState[pin] = 0;
  pinConfig[pin] = config;
  if (currentPinModeCallback)
    (*currentPinModeCallback)(pin, config);
}

/**
 * @param pin The pin to get the state of.
 * @return The state of the specified pin.
 */
int FirmataClass::getPinState(byte pin)
{
  return pinState[pin];
}

/// <summary>
/// Decodes an uint 32 from 5 bytes
/// </summary>
uint32_t FirmataClass::decodePackedUInt32(byte* argv)
{
  uint32_t result = 0;
  result = argv[0];
  result |= ((uint32_t)argv[1]) << 7;
  result |= ((uint32_t)argv[2]) << 14;
  result |= ((uint32_t)argv[3]) << 21;
  result |= ((uint32_t)argv[4]) << 28;
  return result;
}

uint64_t FirmataClass::decodePackedUInt64(byte* argv)
{
  uint64_t result = 0;
  result += decodePackedUInt32(argv);
  result += static_cast<uint64_t>(decodePackedUInt32(argv + 5)) << 32;
  return result;
}

/// <summary>
/// Decode a uint14 from 2 x 7 bit
/// </summary>
uint16_t FirmataClass::decodePackedUInt14(byte* argv)
{
  uint32_t result = 0;
  result = argv[0];
  result |= ((uint32_t)argv[1]) << 7;
  return (uint16_t)result;
}

void FirmataClass::sendPackedUInt32(uint32_t value)
{
  Firmata.write((byte)(value & 0x7F));
  Firmata.write((byte)((value >> 7) & 0x7F));
  Firmata.write((byte)((value >> 14) & 0x7F));
  Firmata.write((byte)((value >> 21) & 0x7F));
  Firmata.write((byte)((value >> 28) & 0x0F)); // only 4 bits left, and we don't care about the sign here
}

void FirmataClass::sendPackedUInt64(uint64_t value)
{
  sendPackedUInt32(value & 0xFFFFFFFF);
  sendPackedUInt32(value >> 32);
}

void FirmataClass::sendPackedUInt14(uint16_t value)
{
  Firmata.write((byte)(value & 0x7F));
  Firmata.write((byte)((value >> 7) & 0x7F));
}


/**
 * Set the pin state. The pin state of an output pin is the pin value. The state of an
 * input pin is 0, unless the pin has it's internal pull up resistor enabled, then the value is 1.
 * @param pin The pin to set the state of
 * @param state Set the state of the specified pin
 */
void FirmataClass::setPinState(byte pin, byte state)
{
  pinState[pin] = state;
}


// sysex callbacks
/*
 * this is too complicated for analogReceive, but maybe for Sysex?
 void FirmataClass::attachSysex(sysexFunction newFunction)
 {
 byte i;
 byte tmpCount = analogReceiveFunctionCount;
 analogReceiveFunction* tmpArray = analogReceiveFunctionArray;
 analogReceiveFunctionCount++;
 analogReceiveFunctionArray = (analogReceiveFunction*) calloc(analogReceiveFunctionCount, sizeof(analogReceiveFunction));
 for(i = 0; i < tmpCount; i++) {
 analogReceiveFunctionArray[i] = tmpArray[i];
 }
 analogReceiveFunctionArray[tmpCount] = newFunction;
 free(tmpArray);
 }
*/

//******************************************************************************
//* Private Methods
//******************************************************************************

/**
 * Resets the system state upon a SYSTEM_RESET message from the host software.
 * @private
 */
void FirmataClass::systemReset(void)
{
  resetting = true;
  int i;

  waitForData = 0; // this flag says the next serial input will be data
  executeMultiByteCommand = 0; // execute this after getting multi-byte data
  multiByteChannel = 0; // channel data for multiByteCommands

  for (i = 0; i < MAX_DATA_BYTES; i++) {
    storedInputData[i] = 0;
  }

  parsingSysex = false;
  sysexBytesRead = 0;

  if (currentSystemResetCallback)
    (*currentSystemResetCallback)();

  resetting = false;
}

/**
 * Flashing the pin for the version number
 * @private
 * @param pin The pin the LED is attached to.
 * @param count The number of times to flash the LED.
 * @param onInterval The number of milliseconds for the LED to be ON during each interval.
 * @param offInterval The number of milliseconds for the LED to be OFF during each interval.
 */
void FirmataClass::strobeBlinkPin(byte pin, int count, int onInterval, int offInterval)
{
  byte i;
  for (i = 0; i < count; i++) {
    delay(offInterval);
    digitalWrite(pin, HIGH);
    delay(onInterval);
    digitalWrite(pin, LOW);
  }
}

// make one instance for the user to use
FirmataClass Firmata;
