#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include "ns_dpmi.h"
#include "ns_task.h"
#include "ns_cards.h"
#include "ns_user.h"
#include "ns_cms.h"
#include "ns_muldf.h"
#include "ns_inter.h"
#include "ns_multi.h"

#include "m_misc.h"
#include "options.h"
#include "doomstat.h"

static int CMS_Installed = 0;

static short *CMS_BufferStart;
static short *CMS_CurrentBuffer;
static int CMS_BufferNum = 0;
static int CMS_NumBuffers = 0;
static int CMS_TransferLength = 0;
static int CMS_CurrentLength = 0;

unsigned short CMS_Port = 0x220;

static short *CMS_SoundPtr;
volatile int CMS_SoundPlaying;

static task *CMS_Timer;

void (*CMS_CallBack)(void);

/*---------------------------------------------------------------------
   Function: CMS_ServiceInterrupt

   Handles interrupt generated by sound card at the end of a voice
   transfer.  Calls the user supplied callback function.
---------------------------------------------------------------------*/

static void CMS_SetRegister(unsigned short regAddr, unsigned char reg, unsigned char val)
{
    outp(regAddr + 1, reg); /* Select the register */
    outp(regAddr, val);     /* Set the value of the register */
}

static void CMS_Reset(void)
{
    int i;
    unsigned short port = CMS_Port;

    // Null all 32 registers
    for (i = 0; i < 32; i++){
        CMS_SetRegister(port, i, 0x00);
        CMS_SetRegister(port + 2, i, 0x00);
    }

    // Reset chip
    CMS_SetRegister(port, 0x1C, 0x02);
    CMS_SetRegister(port + 2, 0x1C, 0x02);

    // Enable chip
    CMS_SetRegister(port, 0x1C, 0x01);
    CMS_SetRegister(port + 2, 0x1C, 0x01);
}

static void CMS_ServiceInterrupt(task *Task)
{   
    char *ptr = CMS_SoundPtr;

    unsigned char value1 = (unsigned char)*(ptr);
    unsigned char value2 = (unsigned char)*(ptr + MV_RightChannelOffset);
    
    value1 = value1 >> 4;
    value2 = value2 & 0xF0;

    outp(CMS_Port, value1);
    outp(CMS_Port + 2, value2);

    CMS_SoundPtr++;

    CMS_CurrentLength--;
    if (CMS_CurrentLength == 0)
    {
        // Keep track of current buffer
        CMS_CurrentBuffer += CMS_TransferLength;
        CMS_BufferNum++;
        if (CMS_BufferNum >= CMS_NumBuffers)
        {
            CMS_BufferNum = 0;
            CMS_CurrentBuffer = CMS_BufferStart;
        }

        CMS_CurrentLength = CMS_TransferLength;
        CMS_SoundPtr = CMS_CurrentBuffer;

        // Call the caller's callback function
        if (CMS_CallBack != NULL)
        {
            MV_ServiceVoc();
        }
    }
}

/*---------------------------------------------------------------------
   Function: CMS_StopPlayback

   Ends the transfer of digitized sound to the Sound Source.
---------------------------------------------------------------------*/

void CMS_StopPlayback(void)
{
    if (CMS_SoundPlaying)
    {
        TS_Terminate(CMS_Timer);
        CMS_SoundPlaying = 0;
        CMS_BufferStart = NULL;
    }
}

/*---------------------------------------------------------------------
   Function: CMS_BeginBufferedPlayback

   Begins multibuffered playback of digitized sound on the Sound Source.
---------------------------------------------------------------------*/

int CMS_BeginBufferedPlayback(
    char *BufferStart,
    int BufferSize,
    int NumDivisions,
    void (*CallBackFunc)(void))
{
    if (CMS_SoundPlaying)
    {
        CMS_StopPlayback();
    }

    CMS_CallBack = CallBackFunc;

    CMS_BufferStart = BufferStart;
    CMS_CurrentBuffer = BufferStart;
    CMS_SoundPtr = BufferStart;
    // VITI95: OPTIMIZE
    CMS_TransferLength = (BufferSize / NumDivisions) / 2;
    CMS_CurrentLength = CMS_TransferLength;
    CMS_BufferNum = 0;
    CMS_NumBuffers = NumDivisions;

    CMS_SoundPlaying = 1;

    CMS_Timer = TS_ScheduleTask(CMS_ServiceInterrupt, CMS_SampleRate, 1, NULL);
    TS_Dispatch();

    return (CMS_Ok);
}

/*---------------------------------------------------------------------
   Function: CMS_Init

   Initializes the Sound Source prepares the module to play digitized
   sounds.
---------------------------------------------------------------------*/

int CMS_Init(int soundcard, int port)
{
    if (CMS_Installed)
    {
        CMS_Shutdown();
    }

    CMS_Reset();
    CMS_SetRegister(CMS_Port, 0x18, 0x82);
    CMS_SetRegister(CMS_Port + 2, 0x18, 0x82);

    outp(CMS_Port + 1, 0x02);
    outp(CMS_Port + 3, 0x02);

    CMS_SoundPlaying = 0;

    CMS_CallBack = NULL;

    CMS_BufferStart = NULL;

    if (port != -1)
        CMS_Port = port;

    CMS_Installed = 1;

    return (CMS_Ok);
}

/*---------------------------------------------------------------------
   Function: CMS_Shutdown

   Ends transfer of sound data to the Sound Source.
---------------------------------------------------------------------*/

void CMS_Shutdown(void)
{
    CMS_StopPlayback();

    CMS_SoundPlaying = 0;

    CMS_BufferStart = NULL;

    CMS_CallBack = NULL;

    CMS_Installed = 0;
}
