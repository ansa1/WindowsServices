#include "countActivity.h"

ULONGLONG IO_process(DWORD PID)
{
  HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);
  
  if (NULL != hProcess)
  {
    PIO_COUNTERS io = (PIO_COUNTERS)malloc(48);
    ULONGLONG r = 0, w = 0, o = 0;
    if (GetProcessIoCounters(hProcess, io))
    {
      r = (io)->ReadTransferCount;
      w = (io)->WriteTransferCount;
      o = (io)->OtherTransferCount;
      free(io);
      CloseHandle(hProcess);
      return r + w;
    }
  }
  else {
  }

  CloseHandle(hProcess);
  return 0;
}

ULONGLONG IO_total()
{
  DWORD total = 0;
  DWORD aProcesses[1024], cbNeeded, cProcesses;
  if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
  {
    return 0;
  }
  cProcesses = cbNeeded / sizeof(DWORD);
  UINT i = 0;

  for (i = 0; i < cProcesses; i += 1)
  {
    if (aProcesses[i] != 0)
    {
      total += IO_process(aProcesses[i]);
    }
  }
  return total;
}

DWORD findBandwidth(DWORD PID)
{
  double current_bandwidth_fraction;
  ULONGLONG total = IO_total();
  ULONGLONG current_proccess = IO_process(PID);
  if (current_proccess > 0) {
    current_bandwidth_fraction = (double)current_proccess / (double)total;
  }
  else
    current_bandwidth_fraction = 0;
  return (DWORD)(current_proccess / 1000);
}

