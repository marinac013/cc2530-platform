#include "OSAL.h"
#include "PCApp.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"

#include "Debug.h"
#include "DHT11.h"
#include "GenericApp.h"
#include "GenericType.h"

/*prototype function*/

void PowerSaving(void);
void GetIeeeAddr(uint8 *pIeeeAddr, uint8 *pStr);
afAddrType_t SampleApp_Periodic_DstAddr;
afAddrType_t SampleApp_Flash_DstAddr;

uint8 SampleAppPeriodicCounter = 0;
uint8 SampleAppFlashCounter = 0;
//@fn rt_str_reverse
//@brief message received is reserved, so we need to reserve again using this function
//@para *tr the first byte in a string that needs reserved
//@para * length the total length that needs to be reserved, 0 included
// e.g a[]={1,2,3,4} then rt_str_reserve(&a[0], 3) result in {4,3,2,1}
void reserve_string(char *str, int length)
{
    char temp, *end_ptr;
    end_ptr = str + length;

    while (end_ptr > str)
    {
        temp = *str;
        *str = *end_ptr;
        *end_ptr = temp;
        str++;
        end_ptr--;
    }
}

void SampleApp_SendPeriodicMessage(void)
{
    SampleApp_Periodic_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;
    SampleApp_Periodic_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
    SampleApp_Periodic_DstAddr.addr.shortAddr = 0xFFFF;

    uint8 data[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}; //自定义数据
    if (AF_DataRequest(&SampleApp_Periodic_DstAddr, &GenericApp_epDesc,
                       GENERICAPP_CLUSTERID,
                       10,   //字节数
                       data, //指针头
                       &GenericApp_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS) == afStatus_SUCCESS)
    {
    }
    else
    {
        // Error occurred in request to send.
    }
}
void SampleApp_SendFlashMessage(uint16 flashTime)
{
    // Setup for the flash command's destination address - Group 1
    SampleApp_Flash_DstAddr.addrMode = (afAddrMode_t)afAddrGroup;
    SampleApp_Flash_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
    SampleApp_Flash_DstAddr.addr.shortAddr = SAMPLEAPP_FLASH_GROUP;
    uint8 buffer[3];
    buffer[0] = (uint8)(SampleAppFlashCounter++);
    buffer[1] = LO_UINT16(flashTime);
    buffer[2] = HI_UINT16(flashTime);

    if (AF_DataRequest(&SampleApp_Flash_DstAddr, &GenericApp_epDesc,
                       GENERICAPP_CLUSTERID,
                       3,
                       buffer,
                       &GenericApp_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS) == afStatus_SUCCESS)
    {
    }
    else
    {
        // Error occurred in request to send.
    }
}

void PCApp_SendTheMessage(unsigned char dest_endID, unsigned char cmd, unsigned int data)
{
    unsigned char theMessageData[6] = "";
    theMessageData[0] = 0xCC; // beginning check byte
    theMessageData[1] = EndDeviceInfos[dest_endID].compressed_addr;
    theMessageData[2] = cmd;

    osal_buffer_uint16(&theMessageData[3], data);
    theMessageData[5] = 0x33; // end check byte;
    // set the destination data;

    GenericApp_DstAddr.addrMode = (afAddrMode_t)Addr16Bit; // (afAddrMode_t)Addr64Bit;
    GenericApp_DstAddr.endPoint = 10;                      //EndDeviceInfos[dest_endID].endPoint;
    GenericApp_DstAddr.addr.shortAddr = 0;
    //osal_memcpy(GenericApp_DstAddr.addr.extAddr, EndDeviceInfos[dest_endID].extAddr,8);
    if (AF_DataRequest(&GenericApp_DstAddr, &GenericApp_epDesc,
                       GENERICAPP_CLUSTERID,
                       (byte)osal_strlen(theMessageData) + 1, // send one more char or the last char might be missing.
                       theMessageData,
                       &GenericApp_TransID,
                       AF_DISCV_ROUTE, AF_DEFAULT_RADIUS) == afStatus_SUCCESS)
    {
        LREPMaster("\r\n AF_DataRequest: Request to send!. \r\n");
    }
    else
    {
        LREPMaster("\r\n AF_DataRequest:Send failure!. \r\n");
    }
}
/*
@fn: SHApp_SerialMSGCB
@brief: process data received by UART, used to execute cmds from slave machine
@param: none
@return: none
*/
void PCApp_SerialMSGCB(void)
{
    unsigned char dest_endID = 0;
    unsigned int data = 0;
    unsigned char buff[7] = "";
    HalUARTRead(0, buff, 6);
    if ((buff[1] & GENERICAPP_ENDPOINT) && (buff[0] == 0xCC) && (buff[5] == 0x33))
    {
        dest_endID = buff[1] & 0x1F; // get destination endPoint from UART message;
        data = osal_build_uint16(&buff[3]);
        HalUARTWrite(0, buff, 6);

        if (buff[2] & 0x80) // cmd for coordinator
        {
            // add cmds here, e.g case 0x81 ...
            switch (buff[2])
            {
            // add cmds here e.g case 0x81
            default:
                break;
            }
        }
        else
        {
            PCApp_SendTheMessage(dest_endID, buff[2], data);
        }
    }
}
void PCApp_SendTemperature()
{
    uint8_t state[2];    // state of node
    uint8_t temp[3];     // temperature value;
    uint8_t humidity[3]; // humidity value;
    uint8_t adc[4];      // adc value;
    uint8_t buffer[29];
    uint8_t sht11_data[4];

    // Read temperature and humidity
    DHT11_ReadData(sht11_data, 4);
    temp[0] = sht11_data[0] + 0x30;
    temp[1] = sht11_data[1] + 0x30;
    temp[2] = '\0';
    humidity[0] = sht11_data[2] + 0x30;
    humidity[1] = sht11_data[3] + 0x30;
    humidity[2] = '\0';
    // Read adc value
    uint16_t value;
    osal_memset(adc, 0, 4);
    value = ReadADC();
    sprintf(adc, "%03d", value);
    if (H_PIN == 1)
    {
        DelayMs(10);
        if (H_PIN == 1)
        {
            state[0] = 0x31;
        }
        else
        {
            state[0] = 0x30;
        }
    }
    else
    {
        state[0] = 0x30;
    }
    state[1] = '\0';

    osal_memcpy(buffer, temp, 2);
    osal_memcpy(&buffer[2], "#", 1);
    osal_memcpy(&buffer[3], humidity, 2);
    osal_memcpy(&buffer[5], "#", 1);
    osal_memcpy(&buffer[6], state, 1);
    osal_memcpy(&buffer[7], "#", 1);
    osal_memcpy(&buffer[8], adc, 3);
    osal_memcpy(&buffer[11], "#", 1);

    uint8_t strIeeeAddr[17] = {0};
    GetIeeeAddr(aExtendedAddress + Z_EXTADDR_LEN - 1, strIeeeAddr);
    osal_memcpy(&buffer[12], strIeeeAddr, 16);
    buffer[28] = '\0';

    AF_DataRequest(&GenericApp_DstAddr, &GenericApp_epDesc,
                   GENERICAPP_CLUSTERID,
                   (byte)osal_strlen(buffer) + 1,
                   (byte *)buffer,
                   &GenericApp_TaskID,
                   AF_DISCV_ROUTE,
                   AF_DEFAULT_RADIUS);
}
void GetIeeeAddr(uint8 *pIeeeAddr, uint8 *pStr)
{
    uint8 i;
    uint8 *xad = pIeeeAddr;

    for (i = 0; i < Z_EXTADDR_LEN * 2; xad--)
    {
        uint8 ch;
        ch = (*xad >> 4) & 0x0F;
        *pStr++ = ch + ((ch < 10) ? '0' : '7');
        i++;
        ch = *xad & 0x0F;
        *pStr++ = ch + ((ch < 10) ? '0' : '7');
        i++;
    }
}

void PowerSaving(void)
{
    NLME_SetPollRate(0);
    NLME_SetQueuedPollRate(0);
    NLME_SetPollRate(0);
}
