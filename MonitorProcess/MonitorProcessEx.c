#include <ntifs.h>
#include <ntddk.h>
#include <ntstrsafe.h>

#define MAX 200

UCHAR* PsGetProcessImageFileName(PEPROCESS Process);

NTSYSAPI NTSTATUS NTAPI PsSuspendProcess(_In_ PEPROCESS Process);
NTSYSAPI NTSTATUS NTAPI PsResumeProcess(_In_ PEPROCESS Process);

VOID SleepMilliseconds(LONGLONG ms)
{
    LARGE_INTEGER interval;
    interval.QuadPart = -(ms * 10000); // Negative for relative time
    KeDelayExecutionThread(KernelMode, FALSE, &interval);
}

VOID PrintLongUnicodeString(PCWSTR str, USHORT lengthInBytes)
{
    USHORT totalChars = lengthInBytes / sizeof(WCHAR);
    USHORT offset = 0;

    while (offset < totalChars)
    {
        USHORT cmdlen = (totalChars - offset > MAX)
            ? MAX
            : (totalChars - offset);

        WCHAR cmd[MAX + 1];
        RtlCopyMemory(cmd, str + offset, cmdlen * sizeof(WCHAR));
        cmd[cmdlen] = L'\0';

        DbgPrint("%ws", cmd);

        offset += cmdlen;
    }
    DbgPrint("\n");
}

VOID ProcessNotifyCallbackEx(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _In_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    if (CreateInfo == NULL) // Process exit
        return;

    const char* imageName = (const char*)PsGetProcessImageFileName(Process);

    // Chỉ xử lý khi là cmd.exe hoặc powershell.exe
    if (_stricmp(imageName, "cmd.exe") == 0 || _stricmp(imageName, "powershell.exe") == 0)
    {
        ULONG parentPID = (ULONG)(ULONG_PTR)CreateInfo->CreatingThreadId.UniqueProcess;



        // Suspend 5 giây rồi resume
        NTSTATUS status = PsSuspendProcess(Process);
        if (NT_SUCCESS(status))
        {
            DbgPrint("[Monitor] Suspended PID=%u for 5 seconds\n", (ULONG)(ULONG_PTR)ProcessId);
            SleepMilliseconds(5000);
            DbgPrint("[Monitor] Target Process: %s | PID=%u | ParentPID=%u | CommandLine=",
                imageName,
                (ULONG)(ULONG_PTR)ProcessId,
                parentPID);

            if (CreateInfo->CommandLine && CreateInfo->CommandLine->Buffer)
            {
                PrintLongUnicodeString(CreateInfo->CommandLine->Buffer,
                    CreateInfo->CommandLine->Length);
            }
            else {
                DbgPrint("(null)\n");
            }
            status = PsResumeProcess(Process);
            if (NT_SUCCESS(status))
            {
                DbgPrint("[Monitor] Resumed PID=%u\n", (ULONG)(ULONG_PTR)ProcessId);
            }
            else
            {
                DbgPrint("[Monitor] Failed to resume PID=%u: 0x%X\n", (ULONG)(ULONG_PTR)ProcessId, status);
            }
        }
        else
        {
            DbgPrint("[Monitor] Failed to suspend PID=%u: 0x%X\n", (ULONG)(ULONG_PTR)ProcessId, status);
        }
    }
}

VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    PsSetCreateProcessNotifyRoutineEx(ProcessNotifyCallbackEx, TRUE);
    DbgPrint("[Monitor] Driver Unloaded\n");
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    DbgPrint("[Monitor] Driver Loaded\n");

    NTSTATUS status = PsSetCreateProcessNotifyRoutineEx(ProcessNotifyCallbackEx, FALSE);
    if (!NT_SUCCESS(status))
    {
        DbgPrint("[Monitor] Failed to register callback: 0x%X\n", status);
        return status;
    }

    DriverObject->DriverUnload = DriverUnload;
    return STATUS_SUCCESS;
}


