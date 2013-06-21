/*

BERGCloud message pack/unpack

Based on MessagePack http://msgpack.org/

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


#define __STDC_LIMIT_MACROS /* Include C99 stdint defines in C++ code */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h> /* For memcpy() */

#include "Message.h"

#define IN_RANGE(value, min, max) ((value >= min) && (value <= max))

CMessage::CMessage(void)
{
  clearCachedType();
}

CMessage::~CMessage(void)
{
}

#define _MP_FIXNUM_POS_MIN  0x00
#define _MP_FIXNUM_POS_MAX  0x7f
#define _MP_FIXMAP_MIN      0x80
#define _MP_FIXMAP_MAX      0x8f
#define _MP_FIXARRAY_MIN    0x90
#define _MP_FIXARRAY_MAX    0x9f
#define _MP_FIXRAW_MIN      0xa0
#define _MP_FIXRAW_MAX      0xbf
#define _MP_NIL             0xc0
#define _MP_BOOL_FALSE      0xc2
#define _MP_BOOL_TRUE       0xc3
#define _MP_FLOAT           0xca
#define _MP_DOUBLE          0xcb
#define _MP_UINT8           0xcc
#define _MP_UINT16          0xcd
#define _MP_UINT32          0xce
#define _MP_UNIT64          0xcf
#define _MP_INT8            0xd0
#define _MP_INT16           0xd1
#define _MP_INT32           0xd2
#define _MP_INT64           0xd3
#define _MP_RAW16           0xda
#define _MP_RAW32           0xdb
#define _MP_ARRAY16         0xdc
#define _MP_ARRAY32         0xdd
#define _MP_MAP16           0xde
#define _MP_MAP32           0xdf
#define _MP_FIXNUM_NEG_MIN  0xe0
#define _MP_FIXNUM_NEG_MAX  0xff

/*
    Pack methods
*/

bool CMessage::pack(uint8_t n)
{
  _LOG_PACK("pack uint8_t\n");

  if (getBufferFreeSpace() < (sizeof(n) + 1))
  {
    /* Not enough space remaining in the buffer */
    return false;
  }

  addToBuffer(_MP_UINT8);
  addToBuffer(n);
  return true;
}

bool CMessage::pack(uint16_t n)
{
  _LOG_PACK("pack uint16_t\n");

  if (getBufferFreeSpace() < (sizeof(n) + 1))
  {
    /* Not enough space remaining in the buffer */
    return false;
  }

  addToBuffer(_MP_UINT16);
  addToBuffer((uint8_t)(n >> 8));
  addToBuffer((uint8_t)n);
  return true;
}

bool CMessage::pack(uint32_t n)
{
  _LOG_PACK("pack uint32_t\n");

  if (getBufferFreeSpace() < (sizeof(n) + 1))
  {
    /* Not enough space remaining in the buffer */
    return false;
  }

  addToBuffer(_MP_UINT32);
  addToBuffer((uint8_t)(n >> 24));
  addToBuffer((uint8_t)(n >> 16));
  addToBuffer((uint8_t)(n >> 8));
  addToBuffer((uint8_t)n);
  return true;
}

bool CMessage::pack(int8_t n)
{
  _LOG_PACK("pack int8_t\n");

  if (getBufferFreeSpace() < (sizeof(n) + 1))
  {
    /* Not enough space remaining in the buffer */
    return false;
  }

  addToBuffer(_MP_INT8);
  addToBuffer((uint8_t)n);
  return true;
}

bool CMessage::pack(int16_t n)
{
  _LOG_PACK("pack int16_t\n");

  if (getBufferFreeSpace() < (sizeof(n) + 1))
  {
    /* Not enough space remaining in the buffer */
    return false;
  }

  addToBuffer(_MP_INT16);
  addToBuffer((uint8_t)(n >> 8));
  addToBuffer((uint8_t)n);
  return true;
}

bool CMessage::pack(int32_t n)
{
  _LOG_PACK("pack int32_t\n");

  if (getBufferFreeSpace() < (sizeof(n) + 1))
  {
    /* Not enough space remaining in the buffer */
    return false;
  }

  addToBuffer(_MP_INT32);
  addToBuffer((uint8_t)(n >> 24));
  addToBuffer((uint8_t)(n >> 16));
  addToBuffer((uint8_t)(n >> 8));
  addToBuffer((uint8_t)n);
  return true;
}

bool CMessage::pack(float n)
{
  uint32_t data;

  _LOG_PACK("pack float\n");

  if (getBufferFreeSpace() < (sizeof(data) + 1))
  {
    /* Not enough space remaining in the buffer */
    return false;
  }

  /* Convert to binary data */
  memcpy(&data, &n, sizeof(float));

  addToBuffer(_MP_FLOAT);
  addToBuffer((uint8_t)(data >> 24));
  addToBuffer((uint8_t)(data >> 16));
  addToBuffer((uint8_t)(data >> 8));
  addToBuffer((uint8_t)data);
  return true;
}

bool CMessage::pack(bool n)
{
  /*
      Note that Arduino redefines 'true' and 'false' in Arduino.h.
      You can undefine them in your code to make them type 'bool' again:
      #undef true
      #undef false
  */

  _LOG_PACK("pack bool\n");

  if (getBufferFreeSpace() < 1)
  {
    /* Not enough space remaining in the buffer */
    return false;
  }

  addToBuffer(n ? _MP_BOOL_TRUE : _MP_BOOL_FALSE);
  return true;
}

bool CMessage::pack(char *pString)
{
  uint32_t strLen = 0; /* uint32 here as calculations may exceed UINT16_MAX */
  char *pTmp = pString;

  _LOG_PACK("pack string\n");

  /* Get string length excluding terminator */
  while ((*pTmp++ != '\0') && (strLen < UINT16_MAX))
  {
    strLen++;
  }

  if (getBufferFreeSpace() < (strLen + 1 + sizeof(uint16_t)))
  {
    /* Not enough space remaining in the buffer */
    return false;
  }

  addToBuffer(_MP_RAW16);
  addToBuffer((uint8_t)(strLen >> 8));
  addToBuffer((uint8_t)strLen);

  while (strLen-- > 0)
  {
    addToBuffer((uint8_t)*pString++);
  }

  return true;
}

bool CMessage::pack(uint8_t *pData, uint32_t sizeInBytes)
{
  _LOG_PACK("pack data\n");

  if (sizeInBytes > UINT16_MAX)
  {
    /* Too big to encode as RAW16 */
    return false;
  }

  if (getBufferFreeSpace() < (sizeInBytes + 1 + sizeof(uint16_t)))
  {
    /* Not enough space remaining in the buffer */
    return false;
  }

  addToBuffer(_MP_RAW16);
  addToBuffer((uint8_t)(sizeInBytes >> 8));
  addToBuffer((uint8_t)sizeInBytes);

  while (sizeInBytes-- > 0)
  {
    addToBuffer((uint8_t)*pData++);
  }

  return true;
}

#ifdef ARDUINO
//bool CMessage::pack(string s)
//{
//  _LOG_PACK("pack string class\n");
//  return false;
//}
#else
//bool CMessage::pack(CString& s)
//{
//  _LOG_PACK("pack CString\n");
//  return false;
//}
#endif

/* 
    Unpack methods
*/

bool CMessage::getNextType(uint8_t *pMessagePackType)
{
  /* To allow for 'peek'ing at the next type we read it */
  /* from the buffer and cache it in a temporary variable */
  if (!m_cached)
  {
    /* Update stored values */
    if (!removeFromBuffer(&m_messagePack_t))
    {
      /* No more data */
      return false;
    }

    m_cached = true;
  }

  /* Return cached value */
  *pMessagePackType = m_messagePack_t;
  return true;
}

void CMessage::clearCachedType(void)
{
  m_cached = false;
}

bool CMessage::getUnsignedInteger(uint32_t *pValue, uint8_t maxBytes)
{
  uint8_t type;
  uint8_t temp;

  if (maxBytes == 0) /* Just in case... */
  {
    return false;
  }

  if (!getNextType(&type))
  {
    /* Not enough data remaining in the buffer */
    return false;
  }

  if (IN_RANGE(type, _MP_FIXNUM_POS_MIN, _MP_FIXNUM_POS_MAX))
  {
    /* FixedNum value */
    *pValue = type;
    return true;
  }

  if (type == _MP_UINT8)
  {
    if (getBufferDataRemaining() < sizeof(uint8_t))
    {
      /* Not enough data remaining in the buffer */
      return false;
    }

    /* Get 8-bit unsigned integer */
    removeFromBuffer(&temp);
    *pValue = temp;
    return true;
  }

  if ((type == _MP_UINT16) && (maxBytes >= sizeof(uint16_t)))
  {
    if (getBufferDataRemaining() < sizeof(uint16_t))
    {
      /* Not enough data remaining in the buffer */
      return false;
    }

    /* Get 16-bit unsigned integer */
    removeFromBuffer(&temp);
    *pValue = temp;
    *pValue = *pValue << 8;
    removeFromBuffer(&temp);
    *pValue |= temp;
    return true;
  }

  if ((type == _MP_UINT32) && (maxBytes >= sizeof(uint32_t)))
  {
    if (getBufferDataRemaining() < sizeof(uint32_t))
    {
      /* Not enough data remaining in the buffer */
      return false;
    }

    /* Get 32-bit unsigned integer */
    removeFromBuffer(&temp);
    *pValue = temp;
    *pValue = *pValue << 8;
    removeFromBuffer(&temp);
    *pValue |= temp;
    *pValue = *pValue << 8;
    removeFromBuffer(&temp);
    *pValue |= temp;
    *pValue = *pValue << 8;
    removeFromBuffer(&temp);
    *pValue |= temp;
    return true;
  }

  /* Can't convert this type */
  return false;
}

bool CMessage::getSignedInteger(int32_t *pValue, uint8_t maxBytes)
{
  uint8_t type;
  uint8_t temp;

  if (maxBytes == 0) /* Just in case... */
  {
    return false;
  }

  if (!getNextType(&type))
  {
    /* Not enough data remaining in the buffer */
    return false;
  }

  if (IN_RANGE(type, _MP_FIXNUM_POS_MIN, _MP_FIXNUM_POS_MAX))
  {
    /* FixedNum value */
    *pValue = (int32_t)type;
    return true;
  }

  if (IN_RANGE(type, _MP_FIXNUM_NEG_MIN, _MP_FIXNUM_NEG_MAX))
  {
    /* FixedNum value */
    *pValue = (int32_t)type;
    return true;
  }

  if (type == _MP_INT8)
  {
    if (getBufferDataRemaining() < sizeof(int8_t))
    {
      /* Not enough data remaining in the buffer */
      return false;
    }

    /* Get 8-bit signed integer */
    removeFromBuffer(&temp);
    *pValue = temp;
    return true;
  }

  if ((type == _MP_INT16) && (maxBytes >= sizeof(int16_t)))
  {
    if (getBufferDataRemaining() < sizeof(int16_t))
    {
      /* Not enough data remaining in the buffer */
      return false;
    }

    /* Get 16-bit signed integer */
    removeFromBuffer(&temp);
    *pValue = temp;
    *pValue = *pValue << 8;
    removeFromBuffer(&temp);
    *pValue |= temp;
    return true;
  }

  if ((type == _MP_INT32) && (maxBytes >= sizeof(int32_t)))
  {
    if (getBufferDataRemaining() < sizeof(int32_t))
    {
      /* Not enough data remaining in the buffer */
      return false;
    }

    /* Get 32-bit signed integer */
    removeFromBuffer(&temp);
    *pValue = temp;
    *pValue = *pValue << 8;
    removeFromBuffer(&temp);
    *pValue |= temp;
    *pValue = *pValue << 8;
    removeFromBuffer(&temp);
    *pValue |= temp;
    *pValue = *pValue << 8;
    removeFromBuffer(&temp);
    *pValue |= temp;
    return true;
  }

  /* Can't convert this type */
  return false;
}

bool CMessage::unpack(void)
{
  _LOG_PACK("unpack skip\n");
  return false;
}

bool CMessage::unpack(uint8_t& n)
{
  uint32_t temp;
  _LOG_PACK("unpack uint8_t\n");

  if (!getUnsignedInteger(&temp, sizeof(uint8_t)))
  {
    return false;
  }

  clearCachedType();
  n = (uint8_t)temp;
  return true;
}

bool CMessage::unpack(uint16_t& n)
{
  uint32_t temp;
  _LOG_PACK("unpack uint16_t\n");

  if (!getUnsignedInteger(&temp, sizeof(uint16_t)))
  {
    return false;
  }

  clearCachedType();
  n = (uint16_t)temp;
  return true;
}

bool CMessage::unpack(uint32_t& n)
{
  uint32_t temp;
  _LOG_PACK("unpack uint32_t\n");

  if (!getUnsignedInteger(&temp, sizeof(uint32_t)))
  {
    return false;
  }

  clearCachedType();
  n = temp;
  return true;
}

bool CMessage::unpack(int8_t& n)
{
  int32_t temp;
  _LOG_PACK("unpack int8_t\n");

  if (!getSignedInteger(&temp, sizeof(int8_t)))
  {
    return false;
  }

  clearCachedType();
  n = (int8_t)temp;
  return true;
}

bool CMessage::unpack(int16_t& n)
{
  int32_t temp;
  _LOG_PACK("unpack int16_t\n");

  if (!getSignedInteger(&temp, sizeof(int16_t)))
  {
    return false;
  }

  clearCachedType();
  n = (uint16_t)temp;
  return true;
}

bool CMessage::unpack(int32_t& n)
{
  int32_t temp;
  _LOG_PACK("unpack int32_t\n");

  if (!getSignedInteger(&temp, sizeof(int32_t)))
  {
    return false;
  }

  clearCachedType();
  n = temp;
  return true;
}

bool CMessage::unpack(float& n)
{
  uint32_t data;
  uint8_t temp;
  uint8_t type;

  _LOG_PACK("unpack float\n");

  if (!getNextType(&type))
  {
    /* Not enough data remaining in the buffer */
    return false;
  }

  if (type == _MP_FLOAT)
  {
    if (getBufferDataRemaining() < sizeof(float))
    {
      /* Not enough data remaining in the buffer */
      return false;
    }

    removeFromBuffer(&temp);
    data = temp;
    data = data << 8;
    removeFromBuffer(&temp);
    data |= temp;
    data = data << 8;
    removeFromBuffer(&temp);
    data |= temp;
    data = data << 8;
    removeFromBuffer(&temp);
    data |= temp;

    /* Convert to float */
    memcpy(&n, &data, sizeof(float));

    clearCachedType();
    return true;
  }

  return false;
}

bool CMessage::unpack(bool& n)
{
  uint8_t type;
  _LOG_PACK("unpack bool\n");

  if (!getNextType(&type))
  {
    /* Not enough data remaining in the buffer */
    return false;
  }

  if ((type == _MP_BOOL_FALSE) || (type == _MP_BOOL_TRUE))
  {
    n = (type == _MP_BOOL_TRUE);

    clearCachedType();
    return true;
  }

  return false;
}

bool CMessage::getContainerSize(uint32_t *size, /* Returned size */
                                uint8_t mp_min, /* MessagePack fixed type minimum */
                                uint8_t mp_max, /* MessagePack fixed type maximum */
                                uint8_t mp_16,  /* MessagePack 16-bit type */
                                uint8_t mp_32)  /* MessagePack 32-bit type */
{
  /* Get the size of a RAW, ARRAY or MAP type */
  uint8_t type;
  uint8_t temp;

  if (!getNextType(&type))
  {
    /* Not enough data remaining in the buffer */
    return false;
  }

  if (IN_RANGE(type, mp_min, mp_max))
  {
    /* FixedNum value */
    *size = type - mp_min;
    return true;
  }

  if (type == mp_16)
  {
    if (getBufferDataRemaining() < sizeof(uint16_t))
    {
      /* Not enough data remaining in the buffer */
      return false;
    }

    /* Get 16-bit unsigned integer */
    removeFromBuffer(&temp);
    *size = temp;
    *size = *size << 8;
    removeFromBuffer(&temp);
    *size |= temp;
    return true;
  }

  if (type == mp_32)
  {
    if (getBufferDataRemaining() < sizeof(uint32_t))
    {
      /* Not enough data remaining in the buffer */
      return false;
    }

    /* Get 32-bit unsigned integer */
    removeFromBuffer(&temp);
    *size = temp;
    *size = *size << 8;
    removeFromBuffer(&temp);
    *size |= temp;
    *size = *size << 8;
    removeFromBuffer(&temp);
    *size |= temp;
    *size = *size << 8;
    removeFromBuffer(&temp);
    *size |= temp;
  }

  return false;
}

bool CMessage::unpack(char *pString, uint32_t maxSizeInBytes)
{
  uint32_t size;
  uint8_t temp;

  _LOG_PACK("unpack string\n");

  /* Buffer must be a minimum size, at least one character plus terminator */
  if (maxSizeInBytes < (1 + 1))
  {
    return false;
  }

  if (!getContainerSize(&size, _MP_FIXRAW_MIN, _MP_FIXRAW_MAX, _MP_RAW16, _MP_RAW32))
  {
    return false;
  }

  /* Must read all of the data but only copy up to maxSizeInBytes-1 */
  while (size-- > 0)
  {
    if (!removeFromBuffer(&temp))
    {
      /* Not enough data remaining in the buffer */
      return false;
    }

    if (maxSizeInBytes > 1) /* Reserve one byte for null terminator */
    {
      maxSizeInBytes--;
      *pString++ = temp;
    }
  }

  /* Add terminator */
  *pString = '\0';

  clearCachedType();
  return true;
}

bool CMessage::unpack(uint8_t *pData, uint32_t maxSizeInBytes)
{
  uint32_t size;
  uint8_t temp;

  _LOG_PACK("unpack data\n");

  if (!getContainerSize(&size, _MP_FIXRAW_MIN, _MP_FIXRAW_MAX, _MP_RAW16, _MP_RAW32))
  {
    return false;
  }

  /* Must read all of the data but only copy up to maxSizeInBytes */
  while (size-- > 0)
  {
    if (!removeFromBuffer(&temp))
    {
      /* Not enough data remaining in the buffer */
      return false;
    }

    if (maxSizeInBytes > 0)
    {
      maxSizeInBytes--;
      *pData++ = temp;
    }
  }

  clearCachedType();
  return true;
}

#ifdef ARDUINO
//bool CMessage::unpack(string& s)
//{
//  _LOG_PACK("unpack string class\n");
//  return false;
//}
#else
//  bool CMessage::unpack(CString& s)
//  {
//    _LOG_PACK("unpack CString\n");
//    return false;
//  }
#endif
