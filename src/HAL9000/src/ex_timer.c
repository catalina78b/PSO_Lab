#include "HAL9000.h"
#include "ex_timer.h"
#include "iomu.h"
#include "thread_internal.h"
#include "ex_event.h"
#include "lock_common.h"

static struct _GLOBAL_TIMER_LIST m_globalTimerList;
 
void 
_No_competing_thread_
ExTimerSystemPreinit(void)
{
    LOG("preinit ok\n");

    memzero(&m_globalTimerList, sizeof(struct _GLOBAL_TIMER_LIST));

    LockInit(&m_globalTimerList.TimerListLock);
    InitializeListHead(&m_globalTimerList.TimerListHead);
    LOG("preinit ok\n");

}

STATUS
ExTimerInit(
    OUT     PEX_TIMER       Timer,
    IN      EX_TIMER_TYPE   Type,
    IN      QWORD           Time
    )
{
    STATUS status;

    if (NULL == Timer)
    {
        return STATUS_INVALID_PARAMETER1;
    }

    if (Type > ExTimerTypeMax)
    {
        return STATUS_INVALID_PARAMETER2;
    }

    status = STATUS_SUCCESS;

    memzero(Timer, sizeof(EX_TIMER));

    Timer->Type = Type;
    if (Timer->Type != ExTimerTypeAbsolute)
    {
        // relative time

        // if the time trigger time has already passed the timer will
        // be signaled after the first scheduler tick
        Timer->TriggerTimeUs = IomuGetSystemTimeUs() + Time;
        Timer->ReloadTimeUs = Time;
    }
    else
    {
        // absolute
        Timer->TriggerTimeUs = Time;
    }
    INTR_STATE state;


    ExEventInit(&Timer->TimerEvent, ExEventTypeNotification, FALSE);

    LockAcquire(&m_globalTimerList.TimerListLock, &state);
    InsertOrderedList(&m_globalTimerList.TimerListHead, &Timer->TimerListElem, ExTimerCompareListElems, NULL);
    LockRelease(&m_globalTimerList.TimerListLock, state);

    LOG("init ok\n");


    return status;
}

void
ExTimerStart(
    IN      PEX_TIMER       Timer
    )
{
    ASSERT(Timer != NULL);

    if (Timer->TimerUninited)
    {
        return;
    }

    Timer->TimerStarted = TRUE;
}

void
ExTimerStop(
    IN      PEX_TIMER       Timer
    )
{
    ASSERT(Timer != NULL);

    if (Timer->TimerUninited)
    {
        return;
    }

    // Set TimerStarted to FALSE to indicate that the timer has stopped evolving
    Timer->TimerStarted = FALSE;

    // Signal waiting threads associated with the timer
    ExEventSignal(&Timer->TimerEvent);
}

void
ExTimerWait(
    INOUT   PEX_TIMER   Timer
)
{
    ASSERT(Timer != NULL);

    if (Timer->TimerUninited)
    {
        return;
    }

    ExEventWaitForSignal(&Timer->TimerEvent);
    LOG("wait ok\n");
}
void
ExTimerUninit(
    INOUT   PEX_TIMER       Timer
    )
{
    ASSERT(Timer != NULL);

    ExTimerStop(Timer);

    Timer->TimerUninited = TRUE;

    ExEventClearSignal(&Timer->TimerEvent);

    // Acquire the lock that protects the global timer list
    INTR_STATE state;
    LockAcquire(&m_globalTimerList.TimerListLock, &state);

    // Remove the timer's structure from the global timer list
    RemoveEntryList(&Timer->TimerListElem);

    // Release the global timer list's lock
    LockRelease(&m_globalTimerList.TimerListLock, state);




}

STATUS
ExTimerCheck(
    IN PLIST_ENTRY ListEntry,
    IN_OPT PVOID FunctionContext
)
{
    UNREFERENCED_PARAMETER(FunctionContext);

    ASSERT(NULL != ListEntry);

    PEX_TIMER timer = CONTAINING_RECORD(ListEntry, EX_TIMER, TimerListElem);



    if (IomuGetSystemTimeUs() >= timer->TriggerTimeUs)
    {
        ExEventSignal(&timer->TimerEvent);

        LOG("check ok\n");
    }

    return STATUS_SUCCESS;
}


void 
ExTimerCheckAll()
{
    INTR_STATE state;
    LockAcquire(&m_globalTimerList.TimerListLock, &state);
    ForEachElementExecute(&m_globalTimerList.TimerListHead, ExTimerCheck, NULL, TRUE);
    LockRelease(&m_globalTimerList.TimerListLock, state);
}


INT64
ExTimerCompareTimers(
    IN      PEX_TIMER     FirstElem,
    IN      PEX_TIMER     SecondElem
)
{
    return FirstElem->TriggerTimeUs - SecondElem->TriggerTimeUs;
}

INT64
ExTimerCompareListElems(
    PLIST_ENTRY t1,
    PLIST_ENTRY t2,
    PVOID context)
{
    UNREFERENCED_PARAMETER(context); 

    ASSERT(t1 != NULL);
    ASSERT(t2 != NULL);
    PEX_TIMER timer1 = CONTAINING_RECORD(t1, EX_TIMER, TimerListElem);
    PEX_TIMER timer2 = CONTAINING_RECORD(t2, EX_TIMER, TimerListElem);

    return ExTimerCompareTimers(timer1, timer2);
}
