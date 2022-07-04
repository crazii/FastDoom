#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include "ns_dpmi.h"
#include "ns_task.h"
#include "ns_cards.h"
#include "ns_user.h"
#include "ns_sbdm.h"
#include "ns_sb.h"
#include "ns_sbdef.h"

#include "m_misc.h"

#include "doomstat.h"

static int SBDM_Installed = 0;

static char *SBDM_BufferStart;
static char *SBDM_BufferEnd;
static char *SBDM_CurrentBuffer;
static int SBDM_BufferNum = 0;
static int SBDM_NumBuffers = 0;
static int SBDM_TotalBufferSize = 0;
static int SBDM_TransferLength = 0;
static int SBDM_CurrentLength = 0;

static char *SBDM_SoundPtr;
volatile int SBDM_SoundPlaying;

static task *SBDM_Timer;

void (*SBDM_CallBack)(void);

/*---------------------------------------------------------------------
   Function: SBDM_ServiceInterrupt

   Handles interrupt generated by sound card at the end of a voice
   transfer.  Calls the user supplied callback function.
---------------------------------------------------------------------*/

static void SBDM_ServiceInterrupt(task *Task)
{
    BLASTER_WriteDSP(DSP_DirectDAC);
    BLASTER_WriteDSP(*SBDM_SoundPtr);

    SBDM_SoundPtr++;

    SBDM_CurrentLength--;
    if (SBDM_CurrentLength == 0)
    {
        // Keep track of current buffer
        SBDM_CurrentBuffer += SBDM_TransferLength;
        SBDM_BufferNum++;
        if (SBDM_BufferNum >= SBDM_NumBuffers)
        {
            SBDM_BufferNum = 0;
            SBDM_CurrentBuffer = SBDM_BufferStart;
        }

        SBDM_CurrentLength = SBDM_TransferLength;
        SBDM_SoundPtr = SBDM_CurrentBuffer;

        // Call the caller's callback function
        if (SBDM_CallBack != NULL)
        {
            SBDM_CallBack();
        }
    }
}

/*---------------------------------------------------------------------
   Function: SBDM_StopPlayback

   Ends the transfer of digitized sound to the Sound Source.
---------------------------------------------------------------------*/

void SBDM_StopPlayback(void)
{
    if (SBDM_SoundPlaying)
    {
        TS_Terminate(SBDM_Timer);
        SBDM_SoundPlaying = 0;
        SBDM_BufferStart = NULL;
    }
}

/*---------------------------------------------------------------------
   Function: SBDM_BeginBufferedPlayback

   Begins multibuffered playback of digitized sound on the Sound Source.
---------------------------------------------------------------------*/

int SBDM_BeginBufferedPlayback(
    char *BufferStart,
    int BufferSize,
    int NumDivisions,
    void (*CallBackFunc)(void))
{
    if (SBDM_SoundPlaying)
    {
        SBDM_StopPlayback();
    }

    SBDM_CallBack = CallBackFunc;

    SBDM_BufferStart = BufferStart;
    SBDM_CurrentBuffer = BufferStart;
    SBDM_SoundPtr = BufferStart;
    SBDM_TotalBufferSize = BufferSize;
    SBDM_BufferEnd = BufferStart + BufferSize;
    // VITI95: OPTIMIZE
    SBDM_TransferLength = BufferSize / NumDivisions;
    SBDM_CurrentLength = SBDM_TransferLength;
    SBDM_BufferNum = 0;
    SBDM_NumBuffers = NumDivisions;

    SBDM_SoundPlaying = 1;

    SBDM_Timer = TS_ScheduleTask(SBDM_ServiceInterrupt, SBDM_SampleRate, 1, NULL);
    TS_Dispatch();

    return (SBDM_Ok);
}

/*---------------------------------------------------------------------
   Function: SBDM_Init

   Initializes the Sound Source prepares the module to play digitized
   sounds.
---------------------------------------------------------------------*/

int SBDM_Init(int soundcard)
{
    int status;

    if (SBDM_Installed)
    {
        SBDM_Shutdown();
    }

    BLASTER_ResetDSP();
    BLASTER_SpeakerOn();

    status = SBDM_Ok;

    SBDM_SoundPlaying = 0;

    SBDM_CallBack = NULL;

    SBDM_BufferStart = NULL;

    SBDM_Installed = 1;

    return (status);
}

int SBDM_SetMixMode(int mode)
{
    mode = MONO_8BIT;
    return (mode);
}

/*---------------------------------------------------------------------
   Function: SBDM_Shutdown

   Ends transfer of sound data to the Sound Source.
---------------------------------------------------------------------*/

void SBDM_Shutdown(void)
{
    SBDM_StopPlayback();

    SBDM_SoundPlaying = 0;

    SBDM_BufferStart = NULL;

    SBDM_CallBack = NULL;

    SBDM_Installed = 0;
}
