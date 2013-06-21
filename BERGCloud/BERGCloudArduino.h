/*

BERGCloud library for Arduino

Copyright (c) 2013 BERG Ltd. http://bergcloud.com/

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#ifndef BERGCLOUDARDUINO_H
#define BERGCLOUDARDUINO_H

#include <Arduino.h>
#include <SPI.h>

#include "BERGCloudBase.h"

class CBERGCloudArduino : public CBERGCloudBase
{
public:
  void begin(SPIClass *_pSPI, uint8_t _nSSELPin);
  void end();
private:
  uint16_t SPITransaction(uint8_t *pDataOut, uint8_t *pDataIn, uint16_t dataSize, bool finalCS);
  void timerReset(void);
  uint32_t timerRead_mS(void);
  uint8_t m_nSSELPin;
  SPIClass *m_pSPI;
  uint32_t m_resetTime;

#ifdef _BC_LOG

protected:
  void logPrintf(const char *format, ...);
  char m_logText[_BC_LOG_LINE_LENGTH + 1]; /* +1 for null terminator */

#endif // #ifdef _BC_LOG

};

extern CBERGCloudArduino BERGCloud;

#endif // #ifndef BERGCLOUDARDUINO_H
