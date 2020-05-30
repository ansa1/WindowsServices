#include "PID_finder.h"

TCHAR* getProcessName(DWORD processID)
{
  TCHAR* szProcessName = TEXT("<unknown>");

  HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);

  if (NULL != hProcess)
  {
    HMODULE hMod;
    DWORD cbNeeded;
    
    if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
    {
      GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR) * 260);
    }
  }
  CloseHandle(hProcess);
  return szProcessName;
}


DWORD getPID(TCHAR *processName)
{
  // check all processes
  DWORD aProcesses[1024], cbNeeded, cProcesses;
  if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
  {
    return 0;
  }
  

  cProcesses = cbNeeded / sizeof(DWORD);
  for (UINT i = 0; i < cProcesses; i++)
  {
    if (aProcesses[i] != 0)
    {
      // find process name and compare
      TCHAR *currentProcessName = getProcessName(aProcesses[i]);
      if (0 == _tcscmp(currentProcessName, processName)) {
        return aProcesses[i];
      }
    }
  }
  return 0;
}

