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


#ifndef MESSAGE_H
#define MESSAGE_H

#include "Buffer.h"
//#include "BERGCloudConfig.h"

#define _LOG_PACK(...) printf(__VA_ARGS__);

#ifndef _LOG_PACK
#define _LOG_PACK(...)
#endif

/* MessagePack types */
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

class CMessage : public CBuffer
{
public:
  CMessage(void);
  ~CMessage(void);

  /* Pack methods */
  bool pack(uint8_t n);
  bool pack(uint16_t n);
  bool pack(uint32_t n);
  bool pack(int8_t n);
  bool pack(int16_t n);
  bool pack(int32_t n);
  bool pack(float n);
  bool pack(bool n);
  bool pack(char *pString);
  bool pack(uint8_t *pData, uint32_t sizeInBytes);
  #ifdef ARDUINO
  //bool pack(string s);
  #else
//  bool pack(CString& s);
  #endif

  /* Unpack methods */
  bool getNextType(uint8_t *pMessagePackType);
  bool unpack(void); /* Skip item */
  bool unpack(uint8_t& n);
  bool unpack(uint16_t& n);
  bool unpack(uint32_t& n);
  bool unpack(int8_t& n);
  bool unpack(int16_t& n);
  bool unpack(int32_t& n);
  bool unpack(float& n);
  bool unpack(bool& n);
  bool unpack(char *pString, uint32_t maxSizeInBytes);
  bool unpack(uint8_t *pData, uint32_t maxSizeInBytes);
  #ifdef ARDUINO
//  bool unpack(string& s);
  #else
//  bool unpack(CString& s);
  #endif

private:
  void clearCachedType(void);
  bool getUnsignedInteger(uint32_t *pValue, uint8_t maxBytes);
  bool getSignedInteger(int32_t *pValue, uint8_t maxBytes);
  bool getContainerSize(uint32_t *size, uint8_t mp_min, uint8_t mp_max, uint8_t mp_16, uint8_t mp_32);
  bool m_cached;
  uint8_t m_messagePack_t;

};

#endif // #ifndef MESSAGE_H
