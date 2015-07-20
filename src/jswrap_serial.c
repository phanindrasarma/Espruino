/*
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * This file is designed to be parsed during the build process
 *
 * JavaScript Serial Port Functions
 * ----------------------------------------------------------------------------
 */
#include "jswrap_serial.h"
#include "jsdevices.h"
#include "jsinteractive.h"
#include "jstimer.h"

/*JSON{
  "type" : "class",
  "class" : "Serial"
}
This class allows use of the built-in USARTs

Methods may be called on the USB, Serial1, Serial2, Serial3, Serial4, Serial5 and Serial6 objects. While different processors provide different numbers of USARTs, you can always rely on at least Serial1 and Serial2
*/

/*JSON{
  "type" : "event",
  "class" : "Serial",
  "name" : "data",
  "params" : [
    ["data","JsVar","A string containing one or more characters of received data"]
  ]
}
The 'data' event is called when data is received. If a handler is defined with `X.on('data', function(data) { ... })` then it will be called, otherwise data will be stored in an internal buffer, where it can be retrieved with `X.read()`
*/

/*JSON{
  "type" : "constructor",
  "class" : "Serial",
  "name" : "Serial",
  "generate" : "jswrap_serial_constructor"
}
Create a software Serial port. This has limited functionality (low baud rates only), but it can work on any pins.

Use `Serial.setup` to configure this port.
*/
JsVar *jswrap_serial_constructor() {
  return jsvNewWithFlags(JSV_OBJECT);
}

/*JSON{
  "type" : "object",
  "name" : "USB",
  "instanceof" : "Serial",
  "#if" : "defined(USB)"
}
The USB Serial port
*/
/*JSON{
  "type" : "object",
  "name" : "Serial1",
  "instanceof" : "Serial",
  "#if" : "USARTS>=1"
}
The first Serial (USART) port
*/
/*JSON{
  "type" : "object",
  "name" : "Serial2",
  "instanceof" : "Serial",
  "#if" : "USARTS>=2"
}
The second Serial (USART) port
*/
/*JSON{
  "type" : "object",
  "name" : "Serial3",
  "instanceof" : "Serial",
  "#if" : "USARTS>=3"
}
The third Serial (USART) port
*/
/*JSON{
  "type" : "object",
  "name" : "Serial4",
  "instanceof" : "Serial",
  "#if" : "USARTS>=4"
}
The fourth Serial (USART) port
*/
/*JSON{
  "type" : "object",
  "name" : "Serial5",
  "instanceof" : "Serial",
  "#if" : "USARTS>=5"
}
The fifth Serial (USART) port
*/
/*JSON{
  "type" : "object",
  "name" : "Serial6",
  "instanceof" : "Serial",
  "#if" : "USARTS>=6"
}
The sixth Serial (USART) port
*/

/*JSON{
  "type" : "object",
  "name" : "LoopbackA",
  "instanceof" : "Serial"
}
A loopback serial device. Data sent to LoopbackA comes out of LoopbackB and vice versa
*/
/*JSON{
  "type" : "object",
  "name" : "LoopbackB",
  "instanceof" : "Serial"
}
A loopback serial device. Data sent to LoopbackA comes out of LoopbackB and vice versa
*/



/*JSON{
  "type" : "method",
  "class" : "Serial",
  "name" : "setConsole",
  "generate_full" : "jsiSetConsoleDevice(jsiGetDeviceFromClass(parent))"
}
Set this Serial port as the port for the console
*/

/*JSON{
  "type" : "method",
  "class" : "Serial",
  "name" : "setup",
  "generate" : "jswrap_serial_setup",
  "params" : [
    ["baudrate","JsVar","The baud rate - the default is 9600"],
    ["options","JsVar",["An optional structure containing extra information on initialising the serial port.","```{rx:pin,tx:pin,bytesize:8,parity:null/'none'/'o'/'odd'/'e'/'even',stopbits:1,flow:null/undefined/'none'/'xon'}```","You can find out which pins to use by looking at [your board's reference page](#boards) and searching for pins with the `UART`/`USART` markers.","Note that even after changing the RX and TX pins, if you have called setup before then the previous RX and TX pins will still be connected to the Serial port as well - until you set them to something else using digitalWrite"]]
  ]
}
Setup this Serial port with the given baud rate and options.

If not specified in options, the default pins are used (usually the lowest numbered pins on the lowest port that supports this peripheral)
*/
void _jswrap_serial_getUsartInfo(JshUSARTInfo *inf, JsVar *baud, JsVar *options) {
  jshUSARTInitInfo(inf);

  if (!jsvIsUndefined(baud)) {
    int b = (int)jsvGetInteger(baud);
    if (b<=100 || b > 10000000)
      jsExceptionHere(JSET_ERROR, "Invalid baud rate specified");
    else
      inf->baudRate = b;
  }


  if (jsvIsObject(options)) {
    inf->pinRX = jshGetPinFromVarAndUnLock(jsvObjectGetChild(options, "rx", 0));
    inf->pinTX = jshGetPinFromVarAndUnLock(jsvObjectGetChild(options, "tx", 0));
    inf->pinCK = jshGetPinFromVarAndUnLock(jsvObjectGetChild(options, "ck", 0));

    JsVar *v;
    v = jsvObjectGetChild(options, "bytesize", 0);
    if (jsvIsInt(v))
      inf->bytesize = (unsigned char)jsvGetInteger(v);
    jsvUnLock(v);

    inf->parity = 0;
    v = jsvObjectGetChild(options, "parity", 0);
    if(jsvIsString(v)) {
      if(jsvIsStringEqual(v, "o") || jsvIsStringEqual(v, "odd"))
        inf->parity = 1;
      else if(jsvIsStringEqual(v, "e") || jsvIsStringEqual(v, "even"))
        inf->parity = 2;
    } else if(jsvIsInt(v)) {
      inf->parity = (unsigned char)jsvGetInteger(v);
    }
    jsvUnLock(v);
    if (inf->parity>2) {
      jsExceptionHere(JSET_ERROR, "Invalid parity %d", inf->parity);
      return;
    }

    v = jsvObjectGetChild(options, "stopbits", 0);
    if (jsvIsInt(v))
      inf->stopbits = (unsigned char)jsvGetInteger(v);
    jsvUnLock(v);

    v = jsvObjectGetChild(options, "flow", 0);
    if(jsvIsUndefined(v) || jsvIsNull(v) || jsvIsStringEqual(v, "none"))
      inf->xOnXOff = false;
    else if(jsvIsStringEqual(v, "xon"))
      inf->xOnXOff = true;
    else jsExceptionHere(JSET_ERROR, "Invalid flow control: %q", v);
    jsvUnLock(v);
  }
}

void jswrap_serial_setup(JsVar *parent, JsVar *baud, JsVar *options) {
  IOEventFlags device = jsiGetDeviceFromClass(parent);
  JshUSARTInfo inf;
  _jswrap_serial_getUsartInfo(&inf, baud, options);

  if (DEVICE_IS_USART(device)) {
#ifdef LINUX
    if (jsvIsObject(options)) {
      jsvUnLock(jsvObjectSetChild(parent, "path", jsvObjectGetChild(options, "path", 0)));
    }
#endif
    jshUSARTSetup(device, &inf);
  }
  // Set baud rate in object, so we can initialise it on startup
  jsvUnLock(jsvObjectSetChild(parent, USART_BAUDRATE_NAME, jsvNewFromInteger(inf.baudRate)));
  // Do the same for options
  if (options)
    jsvObjectSetChild(parent, DEVICE_OPTIONS_NAME, options);
  else
    jsvRemoveNamedChild(parent, DEVICE_OPTIONS_NAME);
}


static void _jswrap_serial_print_hw(int data, void *userData) {
  IOEventFlags device = *(IOEventFlags*)userData;
  jshTransmit(device, (unsigned char)data);
}

typedef struct SWSerialStruct {
  JshUSARTInfo inf;
  JsSysTime time;
  JsSysTime bitLength;
} SWSerialStruct;

static void _jswrap_serial_print_sw(int data, void *userData) {
  SWSerialStruct *s = (SWSerialStruct*)userData;
  // Start bit and data, LSB first
  s->inf.bytesize = 8;
  int bitData = (data & ((1<<s->inf.bytesize)-1)) << 1;
  int bitCount = 1 + s->inf.bytesize;
  // parity?
  // stop bits
  bitData  |= ((1<<s->inf.stopbits)-1) << bitCount;
  bitCount += s->inf.stopbits;

  jsiConsolePrintf("\n%d %d\n",(int)s->bitLength);
  // now scan out
  int bVal=-1,bCnt=0;
  while (bitCount--) {
    int v = bitData&1;
    bitData = bitData>>1;
    if (bCnt && bVal!=v) {
      s->time = s->time + (s->bitLength*bCnt);
      jstPinOutputAtTime(s->time, &s->inf.pinTX, 1, bVal);
      bCnt = 0;
    }
    bCnt++;
    bVal = v;
  }
  s->time = s->time + (bCnt*s->bitLength);
  jstPinOutputAtTime(s->time, &s->inf.pinTX, 1, bVal);
}


void _jswrap_serial_print(JsVar *parent, JsVar *arg, bool isPrint, bool newLine) {
  NOT_USED(parent);
  IOEventFlags device = jsiGetDeviceFromClass(parent);
  void (*cb)(int item, void *callbackData);
  void *cbdata;
  SWSerialStruct s;
  if (DEVICE_IS_USART(device)) {
    cb = _jswrap_serial_print_hw;
    cbdata = (void*)&device;
  } else {
    JsVar *baud = jsvObjectGetChild(parent, USART_BAUDRATE_NAME, 0);
    JsVar *options = jsvObjectGetChild(parent, DEVICE_OPTIONS_NAME, 0);
    _jswrap_serial_getUsartInfo(&s.inf, baud, options);
    jsvUnLock(baud);
    jsvUnLock(options);
    if (!jshIsPinValid(s.inf.pinTX)) return; // not set up!
    jshPinOutput(s.inf.pinTX, 1);
    cb = _jswrap_serial_print_sw;
    cbdata = &s;
    s.bitLength = jshGetTimeFromMilliseconds(1000/(JsVarFloat)s.inf.baudRate);
    s.time = jshGetSystemTime()+jshGetTimeFromMilliseconds(1000);
  }

  if (isPrint) arg = jsvAsString(arg, false);
  jsvIterateCallback(arg, cb, cbdata);
  if (isPrint) jsvUnLock(arg);
  if (newLine) {
    cb((unsigned char)'\r', cbdata);
    cb((unsigned char)'\n', cbdata);
  }
  if (cb==_jswrap_serial_print_sw) {
    jstPinOutputAtTime(s.time, &s.inf.pinTX, 1, 1);
  }
}

/*JSON{
  "type" : "method",
  "class" : "Serial",
  "name" : "print",
  "generate" : "jswrap_serial_print",
  "params" : [
    ["string","JsVar","A String to print"]
  ]
}
Print a string to the serial port - without a line feed

**Note:** This function replaces any occurances of `\n` in the string with `\r\n`. To avoid this, use `Serial.write`.
*/
/*JSON{
  "type" : "method",
  "class" : "Serial",
  "name" : "println",
  "generate" : "jswrap_serial_println",
  "params" : [
    ["string","JsVar","A String to print"]
  ]
}
Print a line to the serial port with a newline (`\r\n`) at the end of it.

**Note:** This function converts data to a string first, eg `Serial.print([1,2,3])` is equivalent to `Serial.print("1,2,3"). If you'd like to write raw bytes, use `Serial.write`.
*/
void jswrap_serial_print(JsVar *parent, JsVar *str) {
  _jswrap_serial_print(parent, str, true, false);
}
void jswrap_serial_println(JsVar *parent,  JsVar *str) {
  _jswrap_serial_print(parent, str, true, true);
}
/*JSON{
  "type" : "method",
  "class" : "Serial",
  "name" : "write",
  "generate" : "jswrap_serial_write",
  "params" : [
    ["data","JsVarArray","One or more items to write. May be ints, strings, arrays, or objects of the form `{data: ..., count:#}`."]
  ]
}
Write a character or array of data to the serial port

This method writes unmodified data, eg `Serial.write([1,2,3])` is equivalent to `Serial.write("\1\2\3")`. If you'd like data converted to a string first, use `Serial.print`.
*/
void jswrap_serial_write(JsVar *parent, JsVar *args) {
  _jswrap_serial_print(parent, args, false, false);
}

/*JSON{
  "type" : "method",
  "class" : "Serial",
  "name" : "onData",
  "generate" : "jswrap_serial_onData",
  "params" : [
    ["function","JsVar",""]
  ]
}
Serial.onData(func) has now been replaced with the event Serial.on(`data`, func)
*/
void jswrap_serial_onData(JsVar *parent, JsVar *func) {
  NOT_USED(parent);
  NOT_USED(func);
  jsWarn("Serial.onData(func) has now been replaced with Serial.on(`data`, func).");
}

/*JSON{
  "type" : "method",
  "class" : "Serial",
  "name" : "available",
  "generate" : "jswrap_stream_available",
  "return" : ["int","How many bytes are available"]
}
Return how many bytes are available to read. If there is already a listener for data, this will always return 0.
*/

/*JSON{
  "type" : "method",
  "class" : "Serial",
  "name" : "read",
  "generate" : "jswrap_stream_read",
  "params" : [
    ["chars","int","The number of characters to read, or undefined/0 for all available"]
  ],
  "return" : ["JsVar","A string containing the required bytes."]
}
Return a string containing characters that have been received
*/

/*JSON{
  "type" : "method",
  "class" : "Serial",
  "name" : "pipe",
  "ifndef" : "SAVE_ON_FLASH",
  "generate" : "jswrap_pipe",
  "params" : [
    ["destination","JsVar","The destination file/stream that will receive content from the source."],
    ["options","JsVar",["An optional object `{ chunkSize : int=32, end : bool=true, complete : function }`","chunkSize : The amount of data to pipe from source to destination at a time","complete : a function to call when the pipe activity is complete","end : call the 'end' function on the destination when the source is finished"]]
  ]
}
Pipe this USART to a stream (an object with a 'write' method)
*/
