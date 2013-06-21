/*

BERGCloud library common API

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
#include <string.h> /* For memcpy() */

#include "BERGCloudBase.h"

#define SPI_PROTOCOL_HEADER_SIZE (5) // Data length, CRC16 and command/status
#define MAX_DATA_SIZE (MAX_SERIAL_DATA + SPI_PROTOCOL_HEADER_SIZE)

#define SPI_PROTOCOL_PAD    (0xff)
#define SPI_PROTOCOL_RESET  (0xf5)

#define POLL_TIMEOUT_MS (1000)
#define SYNC_TIMEOUT_MS (1000)

uint8_t CBERGCloudBase::nullProductID[16] = {0};

uint16_t CBERGCloudBase::Crc16(uint8_t data, uint16_t crc)
{
  /* From Ember's code */
  crc = (crc >> 8) | (crc << 8);
  crc ^= data;
  crc ^= (crc & 0xff) >> 4;
  crc ^= (crc << 8) << 4;

  crc ^= ( (uint8_t) ( (uint8_t) ( (uint8_t) (crc & 0xff) ) << 5)) |
    ((uint16_t) ( (uint8_t) ( (uint8_t) (crc & 0xff)) >> 3) << 8);

  return crc;
}

bool CBERGCloudBase::transaction(_BC_TRANSACTION *pTr)
{
  uint16_t i;
  uint8_t rxByte;
  bool timeout;
  uint16_t dataSize;
  uint16_t dataCRC;
  uint16_t calcCRC;
  uint8_t header[SPI_PROTOCOL_HEADER_SIZE];
  uint16_t commandSize;
  uint8_t response;

  /* Validate parameters */
  if (  ((pTr->pTx == NULL) && (pTr->txSize != 0)) ||
        (pTr->txSize > MAX_SERIAL_DATA) )
  {
    _LOG_ERROR("Invalid parameter (CBERGCloudBase::transaction)\r\n");
    return false;
  }

  /* Check synchronisation */
  if (!m_synced)
  {
    timerReset();
    timeout = false;

    do {
      rxByte = SPITransaction(SPI_PROTOCOL_PAD, true);
      timeout = timerRead_mS() > SYNC_TIMEOUT_MS;

    } while ((rxByte != SPI_PROTOCOL_RESET) && !timeout);

    if (timeout)
    {
      _LOG_ERROR("Timeout, sync (CBERGCloudBase::transaction)\r\n");
      return false;
    }

    /* Resynchronisation successful */
    m_synced = true;
  }

  /* Command size is header plus data */
  commandSize = SPI_PROTOCOL_HEADER_SIZE + pTr->txSize;

  /* Set command size in header */
  header[0] = commandSize >> 8;    /* MSByte */
  header[1] = commandSize & 0xff;  /* LSByte */

  /* Zero CRC in header */
  header[2] = 0;
  header[3] = 0;

  /* Set command */
  header[4] = pTr->command;

  /* Calculate CRC (header and data) */
  calcCRC = 0xffff;
  
  for (i=0; i<SPI_PROTOCOL_HEADER_SIZE; i++)
  {
    calcCRC = Crc16(header[i], calcCRC);
  }

  for (i=0; i<pTr->txSize; i++)
  {
    calcCRC = Crc16(pTr->pTx[i], calcCRC);
  }

  /* Set CRC in header */
  header[2] = calcCRC >> 8;    /* MSByte */
  header[3] = calcCRC & 0xff;  /* LSByte */

  /* Send header */
  for (i=0; i<SPI_PROTOCOL_HEADER_SIZE; i++)
  {
    rxByte = SPITransaction(header[i], false);

    if (rxByte == SPI_PROTOCOL_RESET)
    {
      _LOG_ERROR("Reset, send header (CBERGCloudBase::transaction)\r\n");
      return false;
    }

    if (rxByte != SPI_PROTOCOL_PAD)
    {
      _LOG_ERROR("SyncErr, send header (CBERGCloudBase::transaction)\r\n");
      m_synced = false;
      return false;
    }
  }

  /* Send data */
  for (i=0; i<pTr->txSize; i++)
  {
    rxByte = SPITransaction(pTr->pTx[i], false);

    if (rxByte == SPI_PROTOCOL_RESET)
    {
      _LOG_ERROR("Reset, send data (CBERGCloudBase::transaction)\r\n");
      return false;
    }

    if (rxByte != SPI_PROTOCOL_PAD)
    {
      _LOG_ERROR("SyncErr, send data (CBERGCloudBase::transaction)\r\n");
      m_synced = false;
      return false;
    }
  }

  /* Poll for response */
  timerReset();
  timeout = false;

  do {
    rxByte = SPITransaction(SPI_PROTOCOL_PAD, false);

    if (rxByte == SPI_PROTOCOL_RESET)
    {
      _LOG_ERROR("Reset, poll (CBERGCloudBase::transaction)\r\n");
      return false;
    }

    // PW TODO: Check if != PAD -> synched=FALSE
    timeout = timerRead_mS() > POLL_TIMEOUT_MS;
    
  } while ((rxByte == SPI_PROTOCOL_PAD) && !timeout);

  if (timeout)
  {
    _LOG_ERROR("Timeout, poll (CBERGCloudBase::transaction)\r\n");
    m_synced = false;
    return false;
  }

  // PW TODO: Should we 'escape' 0xf5?

  /* Read header, we already have the first byte */
  header[0] = rxByte;

  for (i=1; i < SPI_PROTOCOL_HEADER_SIZE; i++)
  {
    header[i] = SPITransaction(SPI_PROTOCOL_PAD, false);
  }

  /* Read command size (header plus data) */
  commandSize = header[0]; /* MSByte */
  commandSize <<= 8;
  commandSize |= header[1]; /* LSByte */

  /* Validate command size */
  if (commandSize > MAX_DATA_SIZE)
  {
    /* Too big */
    _LOG_ERROR("SizeErr, read header (CBERGCloudBase::transaction)\r\n");
    m_synced = false;
    return false;
  }

  if (commandSize < SPI_PROTOCOL_HEADER_SIZE)
  {
    /* Too small */
    _LOG_ERROR("SizeErr, read header (CBERGCloudBase::transaction)\r\n");
    m_synced = false;
    return false;
  }

  /* Calculate data size */
  dataSize = commandSize - SPI_PROTOCOL_HEADER_SIZE;

  /* Read the remaining data */
  for (i = 0; i < dataSize; i++)
  {
    rxByte = SPITransaction(SPI_PROTOCOL_PAD, i < dataSize ? false : true);
    
    if (pTr->pRx != NULL)
    {
      pTr->pRx[i] = rxByte;
    }
  }

  /* Read CRC */
  dataCRC = header[2]; /* MSByte */
  dataCRC <<= 8;
  dataCRC |= header[3]; /* LSByte */

  /* Clear CRC bytes */
  header[2] = 0;
  header[3] = 0;

  /* Calculate CRC (header and data) */
  calcCRC = 0xffff;

  for (i=0; i<SPI_PROTOCOL_HEADER_SIZE; i++)
  {
    calcCRC = Crc16(header[i], calcCRC);
  }

  for (i=0; i<dataSize; i++)
  {
    calcCRC = Crc16(pTr->pRx[i], calcCRC);
  }

  if (calcCRC != dataCRC)
  {
    /* Invalid CRC */
    _LOG_ERROR("CRCErr, read data (CBERGCloudBase::transaction)\r\n");
    m_synced = false;
    return false;
  }

  /* Check response */
  response = header[4];

  if (pTr->pResponse != NULL)
  {
    *pTr->pResponse = response;
  }

  if (pTr->pRxSize != NULL)
  {
    *pTr->pRxSize = dataSize;
  }

  return true;
}

bool CBERGCloudBase::pollForCommand(uint8_t *pCommandBuffer, uint16_t commandBufferSize, uint16_t *pCommandSize, uint8_t *pCommandID)
{
  /* Returns TRUE if a command has been received */

  _BC_TRANSACTION tr;
  uint16_t commandSize;
  uint8_t rxDataBuffer[MAX_SERIAL_DATA+2];
  int i;

  tr.command = SPI_CMD_POLL_FOR_COMMAND;
  tr.pTx = NULL;
  tr.txSize = 0;
  tr.pResponse = &m_lastResponse;
  tr.pRx = rxDataBuffer;
  tr.rxMaxSize = sizeof(rxDataBuffer);
  tr.pRxSize = &commandSize;

  if (!transaction(&tr))
  {
    return false;
  }

  if (m_lastResponse != SPI_RSP_SUCCESS)
  {
    return false;
  }

  if (commandSize < 2)
  {
    return false;
  }

  if (pCommandSize != NULL)
  {
    *pCommandSize = commandSize -2;
  }

  if (pCommandID != NULL)
  {
    *pCommandID = rxDataBuffer[1];
  }

  if (m_lastResponse == SPI_RSP_SUCCESS)
  {
    if (pCommandBuffer != NULL)
    {
      memcpy(pCommandBuffer, &rxDataBuffer[2], commandSize -2);
    }
  }

  return (m_lastResponse == SPI_RSP_SUCCESS);
}

bool CBERGCloudBase::sendEvent(uint8_t eventCode, uint8_t *pEventBuffer, uint16_t eventSize)
{
  /* Returns TRUE if the event is sent successfully */
  uint8_t txDataBuffer[MAX_SERIAL_DATA];
  uint16_t rxDataSize;

  _BC_TRANSACTION tr;

  if ((eventSize + 2) > sizeof(txDataBuffer))
  {
    return false;
  }

  txDataBuffer[0] = BC_EVENT_START_BINARY >> 8;
  txDataBuffer[1] = eventCode;
  memcpy(&txDataBuffer[2], pEventBuffer, eventSize);

  tr.command = SPI_CMD_SEND_EVENT;
  tr.pTx = txDataBuffer;
  tr.txSize = eventSize + 2;
  tr.pResponse = &m_lastResponse;
  tr.pRx = NULL;
  tr.rxMaxSize = 0;
  tr.pRxSize = &rxDataSize;

  if (!transaction(&tr))
  {
    return false;
  }

  return (m_lastResponse == SPI_RSP_SUCCESS);
}

bool CBERGCloudBase::getNetworkState(uint8_t *pState)
{
  uint16_t rxDataSize;
  _BC_TRANSACTION tr;

  tr.command = SPI_CMD_GET_NETWORK_STATE;
  tr.pTx = NULL;
  tr.txSize = 0;
  tr.pResponse = &m_lastResponse;
  tr.pRx = pState;
  tr.rxMaxSize = sizeof(uint8_t);
  tr.pRxSize = &rxDataSize;

  if (!transaction(&tr))
  {
    return false;
  }

  return (m_lastResponse == SPI_RSP_SUCCESS);
}

bool CBERGCloudBase::joinNetwork(const uint8_t productID[16], uint32_t version)
{
  uint8_t tmp[16 + sizeof(version)];
  int i;

  memcpy(&tmp[0], &productID[0], 16);
  tmp[16] = version >> 24;
  tmp[17] = version >> 16;
  tmp[18] = version >> 8;
  tmp[19] = version;

  uint16_t rxDataSize;
  _BC_TRANSACTION tr;

  tr.command = SPI_CMD_SEND_PRODUCT_ANNOUNCE;
  tr.pTx = tmp;
  tr.txSize = 16 + sizeof(version);
  tr.pResponse = &m_lastResponse;
  tr.pRx = NULL;
  tr.rxMaxSize = 0;
  tr.pRxSize = &rxDataSize;

  if (!transaction(&tr))
  {
    return false;
  }

  return (m_lastResponse == SPI_RSP_SUCCESS);
}

bool CBERGCloudBase::getClaimingState(uint8_t *pState)
{
  uint16_t rxDataSize;
  _BC_TRANSACTION tr;

  tr.command = SPI_CMD_GET_CLAIM_STATE;
  tr.pTx = NULL;
  tr.txSize = 0;
  tr.pResponse = &m_lastResponse;
  tr.pRx = pState;
  tr.rxMaxSize = sizeof(uint8_t);
  tr.pRxSize = &rxDataSize;

  if (!transaction(&tr))
  {
    return false;
  }

  return (m_lastResponse == SPI_RSP_SUCCESS);
}

bool CBERGCloudBase::getClaimcode(char *pBuffer, uint32_t bufferSize)
{
  uint16_t rxDataSize;
  _BC_TRANSACTION tr;

  tr.command = SPI_CMD_GET_CLAIMCODE;
  tr.pTx = NULL;
  tr.txSize = 0;
  tr.pResponse = &m_lastResponse;
  tr.pRx = (uint8_t *)pBuffer;
  tr.rxMaxSize = bufferSize;
  tr.pRxSize = &rxDataSize;

  if (!transaction(&tr))
  {
    return false;
  }

  return (m_lastResponse == SPI_RSP_SUCCESS);
}

bool CBERGCloudBase::getEUI64(uint8_t type, uint8_t *pBuffer, uint32_t bufferSize)
{
  uint16_t rxDataSize;
  _BC_TRANSACTION tr;

  tr.command = SPI_CMD_GET_EUI64;
  tr.pTx = &type;
  tr.txSize = sizeof(uint8_t);
  tr.pResponse = &m_lastResponse;
  tr.pRx = pBuffer;
  tr.rxMaxSize = bufferSize;
  tr.pRxSize = &rxDataSize;

  if (!transaction(&tr))
  {
    return false;
  }

  return (m_lastResponse == SPI_RSP_SUCCESS);
}

bool CBERGCloudBase::setDisplayStyle(uint8_t style)
{
  uint16_t rxDataSize;
  _BC_TRANSACTION tr;

  tr.command = SPI_CMD_SET_DISPLAY_STYLE;
  tr.pTx = &style;
  tr.txSize = sizeof(uint8_t);
  tr.pResponse = &m_lastResponse;
  tr.pRx = NULL;
  tr.rxMaxSize = 0;
  tr.pRxSize = &rxDataSize;

  if (!transaction(&tr))
  {
    return false;
  }

  return (m_lastResponse == SPI_RSP_SUCCESS);
}

bool CBERGCloudBase::print(const char *pString)
{
  uint8_t strLen = 0;
  const char *pTmp = pString;

  /* Get string length excluding terminator */
  while ((*pTmp++ != '\0') && (strLen < UINT8_MAX))
  {
    strLen++;
  }

  uint16_t rxDataSize;
  _BC_TRANSACTION tr;

  tr.command = SPI_CMD_DISPLAY_PRINT;
  tr.pTx = (uint8_t *)pString;
  tr.txSize = strLen;
  tr.pResponse = &m_lastResponse;
  tr.pRx = NULL;
  tr.rxMaxSize = 0;
  tr.pRxSize = &rxDataSize;

  if (!transaction(&tr))
  {
    return false;
  }

  return (m_lastResponse == SPI_RSP_SUCCESS);
}

uint8_t CBERGCloudBase::SPITransaction(uint8_t dataOut, bool finalCS)
{
  uint8_t dataIn = 0;

  SPITransaction(&dataOut, &dataIn, (uint16_t)1, finalCS);

  return dataIn;
}

void CBERGCloudBase::begin(void)
{
  m_synced = false;
  m_lastResponse = SPI_RSP_SUCCESS;

#ifdef _BC_LOG

  m_logError = true;
  m_logData = false;

#endif // #ifdef _BC_LOG
}

void CBERGCloudBase::end(void)
{
}

#ifdef _BC_LOG

const _BC_STATUS_MAP CBERGCloudBase::statusTextMap[] = {
  {SPI_RSP_SUCCESS,           "SPI_RSP_SUCCESS"},
  {SPI_RSP_INVALID_COMMAND,   "SPI_RSP_INVALID_COMMAND"},
  {SPI_RSP_BUSY,              "SPI_RSP_BUSY"},
  {SPI_RSP_NO_DATA,           "SPI_RSP_NO_DATA"},
  {SPI_RSP_SEND_FAILED,       "SPI_RSP_SEND_FAILED"},
};

const char *CBERGCloudBase::getStatusText(uint8_t value)
{
  uint8_t i;

  for (i=0; i<(sizeof(statusTextMap)/sizeof(_BC_STATUS_MAP)); i++)
  {
    if (statusTextMap[i].value == value)
    {
      return statusTextMap[i].text;
    }
  }

  /* Not found */
  return NULL;
}

bool CBERGCloudBase::setLogOutput(bool logError = true, bool logData = false)
{
  m_logError = logError;
  m_logData = logData;
  return true;
}

#else // #ifdef _BC_LOG

bool CBERGCloudBase::setLogOutput(bool logError, bool logData)
{
  return false;
}

#endif // #ifdef _BC_LOG
