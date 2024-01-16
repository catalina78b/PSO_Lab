#include "HAL9000.h"
#include "cmd_fs_helper.h"
#include "display.h"
#include "dmp_io.h"
#include "print.h"
#include "iomu.h"
#include "test_common.h"
#include "strutils.h"

void
CmdPrintVolumeInformation(
    IN      QWORD           NumberOfParameters
    )
{
    ASSERT(NumberOfParameters == 0);

    printColor(MAGENTA_COLOR, "%7s", "Letter|");
    printColor(MAGENTA_COLOR, "%17s", "Type|");
    printColor(MAGENTA_COLOR, "%10s", "Mounted|");
    printColor(MAGENTA_COLOR, "%10s", "Bootable|");
    printColor(MAGENTA_COLOR, "%17s", "Offset|");
    printColor(MAGENTA_COLOR, "%17s", "Size|");
    printColor(MAGENTA_COLOR, "\n");

    IomuExecuteForEachVpb(DumpVpb, NULL, FALSE);
}

#pragma warning(push)

// warning C4717: '_CmdInfiniteRecursion': recursive on all control paths, function will cause runtime stack overflow
#pragma warning(disable:4717)
void
CmdInfiniteRecursion(
    IN      QWORD           NumberOfParameters
    )
{
    ASSERT(NumberOfParameters == 0);

    CmdInfiniteRecursion(NumberOfParameters);
}
#pragma warning(pop)

void
CmdRtcFail(
    IN      QWORD           NumberOfParameters
    )
{
    char buffer[] = "Alex is a smart boy!\n";

    ASSERT(NumberOfParameters == 0);

    strcpy(buffer, "Alex is a very dumb boy!\n");
}

void
CmdRangeFail(
    IN      QWORD           NumberOfParameters
    )
{
    ASSERT(NumberOfParameters == 0);

    perror("Cannot implement! :(\n");
}

void
(__cdecl CmdBiteCookie)(
    IN      QWORD           NumberOfParameters
    )
{
    char buffer[] = "Alex is a smart boy!\n";

    ASSERT(NumberOfParameters == 0);

    strcpy(buffer + sizeof(buffer) + sizeof(PVOID), "Alex is a very dumb boy!\n");
}

void
(__cdecl CmdLogSetState)(
    IN      QWORD           NumberOfParameters,
    IN      char*           LogState
    )
{
    ASSERT(NumberOfParameters == 1);

    LogSetState(stricmp(LogState, "ON") == 0);
}

void
(__cdecl CmdSetLogLevel)(
    IN      QWORD           NumberOfParameters,
    IN      char*           LogLevelString
    )
{
    LOG_LEVEL logLevel;

    ASSERT(NumberOfParameters == 1);

    atoi32(&logLevel, LogLevelString, BASE_TEN);

    if (logLevel > LogLevelError)
    {
        perror("Invalid log level %u specified!\n", logLevel);
        return;
    }

    printf("Will set logging level to %u\n", logLevel);
    LogSetLevel(logLevel);
}

void
(__cdecl CmdSetLogComponents)(
    IN      QWORD           NumberOfParameters,
    IN      char*           LogComponentsString
    )
{
    LOG_COMPONENT logComponents;

    ASSERT(NumberOfParameters == 1);

    atoi32(&logComponents, LogComponentsString, BASE_HEXA);

    printf("Will set logging components to 0x%x\n", logComponents);

    LogSetTracedComponents(logComponents);
}

void
(__cdecl CmdClearScreen)(
    IN          QWORD       NumberOfParameters
    )
{
    ASSERT(NumberOfParameters == 0);

    DispClearScreen();
}

void
(__cdecl CmdRunAllFunctionalTests)(
    IN          QWORD       NumberOfParameters
    )
{
    ASSERT(NumberOfParameters == 0);

    TestRunAllFunctional();
}

void
(__cdecl CmdRunAllPerformanceTests)(
    IN          QWORD       NumberOfParameters
    )
{
    ASSERT(NumberOfParameters == 0);

    TestRunAllPerformance();
}
void
(__cdecl CmdMyTest)(
    IN          QWORD       NumberOfParameters,
    IN          char*       Param1,
    IN          char*       Param2
    )
{
    if (NumberOfParameters == 1)
    {
        LOG("Nb of param is 1. First param is %s.\n", Param1);
    }
    else if (NumberOfParameters == 2)
    {
        LOG("Nb of param is 2.  Param are %s and %s.\n", Param1, Param2);
    }

}
typedef struct _MY_ENTRY {
    LIST_ENTRY ListEntry;
    DWORD Value;
}MY_ENTRY, * PMY_ENTRY;


static
STATUS
(__cdecl _MyListFunc) (
    IN      PLIST_ENTRY     ListEntry,
    IN_OPT  PVOID           FunctionContext
    ) {
    UNREFERENCED_PARAMETER(FunctionContext);
    PMY_ENTRY pMyEntry = CONTAINING_RECORD(ListEntry, MY_ENTRY, ListEntry);

    LOG("%d ", pMyEntry->Value);

    return STATUS_SUCCESS;
}


void
(__cdecl CmdMyTest2)(
    IN          QWORD       NumberOfParameters,
    IN          char* Param1,
    IN          char* Param2
    )
{
    DWORD dwMin, dwMax;

    dwMin = dwMax = 0;

    LIST_ENTRY head;

    PMY_ENTRY pMyEntry;

    pMyEntry = NULL;

    LIST_ITERATOR iterator;

    PLIST_ENTRY pListEntry;

    if (NumberOfParameters == 1)
    {
        atoi(&dwMax, Param1, 10, FALSE);
        dwMin = 1;
    }
    else if (NumberOfParameters == 2)
    {
        atoi(&dwMin, Param1, 10, FALSE);
        atoi(&dwMax, Param2, 10, FALSE);
    }
    if (dwMin > dwMax)
    {
        dwMax ^= dwMin;
        dwMin ^= dwMax;
        dwMax ^= dwMin;
    }

    InitializeListHead(&head);

    for (DWORD index = dwMin; index <= dwMax; index++)
    {
        pMyEntry = ExAllocatePoolWithTag(PoolAllocateZeroMemory, sizeof(MY_ENTRY), HEAP_TEST_TAG, PAGE_SIZE);
        if (pMyEntry == NULL)
        {
            LOG_FUNC_ERROR("ExAllocatePoolWithTag", STATUS_HEAP_INSUFFICIENT_RESOURCES);
            return;
        }

        pMyEntry->Value = index;

        if (pMyEntry->Value % 2 == 0)
        {
            InsertHeadList(&head, &pMyEntry->ListEntry);
        }
        else
        {
            InsertTailList(&head, &pMyEntry->ListEntry);

        }
    }
    LOG(" Method 1 (DLL) : ");
    for (PLIST_ENTRY pEntry = head.Flink; pEntry != &head; pEntry = pEntry->Flink)
    {
        pMyEntry = CONTAINING_RECORD(pEntry, MY_ENTRY, ListEntry);
        LOG("%d ", pMyEntry->Value);
        
    }
    LOG("\n");


    LOG(" Method 2 (iterator) : ");

    ListIteratorInit(&head, &iterator);

    while ((pListEntry = ListIteratorNext(&iterator)) != NULL)
    {
        pMyEntry = CONTAINING_RECORD(pListEntry, MY_ENTRY, ListEntry);
        LOG("%d ", pMyEntry->Value);
    }
    LOG("\n");

    LOG(" Method 3 (for each elem) : ");

    ForEachElementExecute(&head, _MyListFunc, NULL, FALSE);

    LOG("\n");

    while (!IsListEmpty(&head))
    {
        pListEntry = RemoveHeadList(&head);
        pMyEntry = CONTAINING_RECORD(pListEntry, MY_ENTRY, ListEntry);
        ExFreePoolWithTag(pMyEntry, HEAP_TEST_TAG);
    }
}
