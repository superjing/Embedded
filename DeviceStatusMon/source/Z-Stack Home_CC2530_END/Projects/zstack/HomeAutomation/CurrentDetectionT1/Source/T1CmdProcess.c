#include <string.h>
#include <stdio.h>

#include "hal_types.h"
#include "T1CmdProcess.h"
#include "nv.h"
#include "CurrentDetectionT1.h"
#include "AF.h"

/*
//For Test
#define SET_T1_DELTA_CMD_LEN    (13)
#define SET_T1_RATE_CMD_LEN     (13)
#define SET_UINT16_CMD_LEN      (13)
#define CMD_VALUE_OFFSET        (9)
*/

#define SET_T1_DELTA_CMD_LEN    (3)
#define SET_T1_RATE_CMD_LEN     (3)
#define SET_UINT16_CMD_LEN      (3)
#define CMD_VALUE_OFFSET        (1)

#define RESPONSE_ORG_VALUE_INDEX     (HEART_BIT_AD1_INDEX)
#define RESPONSE_NEW_VALUE_INDEX     (HEART_BIT_AD1_INDEX + 2)

extern uint8 aExtendedAddress[];
extern uint16 delta;
extern uint16 heartbitRate;
extern afAddrType_t CurrentDetectionT1_Periodic_DstAddr;
extern endPointDesc_t CurrentDetectionT1_epDesc;
extern uint8 CurrentDetectionT1_TransID;

static void CurrentDetectionT1_SetUint16Variable(uint16 cmdLen, uint8 * cmd, uint16 * variable);

enum
{
   kSetDelta = 0x75,
   kSetRate  = 0x76
};

void CurrentDetectionT1_RfCMD(uint16 cmdLen, uint8 *cmd)
{
  switch (cmd[0])
  {
     case kSetDelta:
       CurrentDetectionT1_SetUint16Variable(cmdLen, cmd, &delta);
       break;

     case kSetRate:
       CurrentDetectionT1_SetUint16Variable(cmdLen, cmd, &heartbitRate);
       break;

     default:
       break;
  }
}

static void CurrentDetectionT1_SetUint16Variable(uint16 cmdLen, uint8 * cmd, uint16 * variable)
{
    if ((SET_UINT16_CMD_LEN != cmdLen) || (NULL == cmd))
    {
      return;
    }

    uint8 responseToG1Data[HEART_BIT_MSG_LEN];
    memcpy(responseToG1Data, serialNumber, SN_LEN);
    uint32 curTime = lastNvTime + osal_GetSystemClock();
    memcpy(responseToG1Data + SN_LEN, &curTime, sizeof(curTime));

    //Set original value in reponse data.
    responseToG1Data[RESPONSE_ORG_VALUE_INDEX] = (*variable) >> 8;
    responseToG1Data[RESPONSE_ORG_VALUE_INDEX + 1] = (uint8)((*variable) & 0x00FF);

    //Set new value in reponse data.
    responseToG1Data[RESPONSE_NEW_VALUE_INDEX] = cmd[CMD_VALUE_OFFSET];
    responseToG1Data[RESPONSE_NEW_VALUE_INDEX + 1] = cmd[CMD_VALUE_OFFSET + 1];

    //Set status byte: bit7:1  bit6-bit0: cmd[0]
    responseToG1Data[HEART_BIT_STATUS_INDEX] = (cmd[0]) | (0x80);

    if (AF_DataRequest(
          &CurrentDetectionT1_Periodic_DstAddr,
          &CurrentDetectionT1_epDesc,
          DTCT_PERIODIC_CLUSTERID,
          HEART_BIT_MSG_LEN,
          responseToG1Data,
          &CurrentDetectionT1_TransID,
          AF_DISCV_ROUTE,
          AF_DEFAULT_RADIUS) == afStatus_SUCCESS)
    {
       (*variable) = 0;
       (*variable) = ((*variable) | (uint16)cmd[CMD_VALUE_OFFSET]) << 8;
       (*variable) |= cmd[CMD_VALUE_OFFSET + 1];

       //Write new variable value to nv
       nv_write_config();
    }
}
