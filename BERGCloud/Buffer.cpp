/*

Simple buffer

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


#include <stdint.h>


#include <stdio.h> // TODO TEMP

#include "Buffer.h"

CBuffer::CBuffer(void)
{
  clearBuffer();
}

CBuffer::~CBuffer(void)
{
}

bool CBuffer::addToBuffer(uint8_t data)
{
  /* Add byte to buffer */
  if (m_written < sizeof(m_data))
  {
    m_data[m_written++] = data;
    return true;
  }

  /* Buffer is full */
  return false;
}

uint16_t CBuffer::getBufferFreeSpace(void)
{
  /* Return space available in buffer in bytes */
  return sizeof(m_data) - m_written;
}

uint16_t CBuffer::getBufferDataRemaining(void)
{
  /* Return unused data remaining in bytes */
  return m_written - m_read;
}

bool CBuffer::removeFromBuffer(uint8_t *data)
{
  /* Remove byte from buffer */
  if (m_read < m_written)
  {
    *data = m_data[m_read++];
    printf("0x%02x\n", *data);
    return true;
  }

  /* No more data */
  return false;
}

void CBuffer::clearBuffer(void)
{
  /* Empty buffer */
  m_read = 0;
  m_written = 0;
}
