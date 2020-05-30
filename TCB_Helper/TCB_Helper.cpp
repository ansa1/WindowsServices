#include "stdafx.h"

#include "ServiceSample.h"

//#pragma comment(lib, "cmcfg32.lib")
#pragma comment(lib, "advapi32.lib")

#define SVCNAME TEXT("TCB_Helper_Service")

SERVICE_STATUS          gSvcStatus;
SERVICE_STATUS_HANDLE   gSvcStatusHandle;
HANDLE                  ghSvcStopEvent = NULL;

VOID WINAPI SvcCtrlHandler(DWORD);
VOID WINAPI SvcMain(DWORD, LPTSTR *);

VOID ReportSvcStatus(DWORD, DWORD, DWORD);
VOID SvcInit(DWORD, LPTSTR *);
VOID LogError(LPTSTR);

HANDLE hThread = nullptr;
HANDLE hThreadStartedEvent = nullptr;
HANDLE hStopThreadEvent = nullptr;

FILE* logger;

BOOL SetPrivilege(
  HANDLE hToken,          // access token handle
  LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
  BOOL bEnablePrivilege   // to enable or disable privilege
  )
{
  TOKEN_PRIVILEGES tp;
  LUID luid;

  if (!LookupPrivilegeValue(
    NULL,            // lookup privilege on local system
    lpszPrivilege,   // privilege to lookup 
    &luid))        // receives LUID of privilege
  {
    printf("LookupPrivilegeValue error: %u\n", GetLastError());
    return FALSE;
  }

  tp.PrivilegeCount = 1;
  tp.Privileges[0].Luid = luid;
  if (bEnablePrivilege)
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  else
    tp.Privileges[0].Attributes = 0;

  // Enable the privilege or disable all privileges.

  if (!AdjustTokenPrivileges(
    hToken,
    FALSE,
    &tp,
    sizeof(TOKEN_PRIVILEGES),
    (PTOKEN_PRIVILEGES)NULL,
    (PDWORD)NULL))
  {
    printf("AdjustTokenPrivileges error: %u\n", GetLastError());
    return FALSE;
  }

  if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

  {
    printf("The token does not have the specified privilege. \n");
    return FALSE;
  }

  return TRUE;
}

//
// Purpose: 
//   Entry point for the process
//
// Parameters:
//   None
// 
// Return value:
//   None
//
void __cdecl _tmain(int argc, TCHAR *argv[])
{
  LogError(TEXT("Entry to my service\n"));

  fopen_s(&logger, "C:/Users/ansat/Documents/Visual Studio 2013/Projects/TCB_Helper/Debug/log.txt", "a+");

  // TODO: Add any additional services for the process to this table.
  SERVICE_TABLE_ENTRY DispatchTable[] =
  {
    { SVCNAME, (LPSERVICE_MAIN_FUNCTION)SvcMain },
    { NULL, NULL }
  };

  // This call returns when the service has stopped. 
  // The process should simply terminate when the call returns.

  if (!StartServiceCtrlDispatcher(DispatchTable))
  {
    LogError(TEXT("StartServiceCtrlDispatcher fail"));
  }
  else {

    LogError(TEXT("StartServiceCtrlDispatcher ok"));
  }
}

//
// Purpose: 
//   Installs a service in the SCM database
//
// Parameters:
//   None
// 
// Return value:
//   None
//
VOID SvcInstall()
{
  SC_HANDLE schSCManager;
  SC_HANDLE schService;
  TCHAR szPath[MAX_PATH];

  if (!GetModuleFileName(nullptr, szPath, MAX_PATH))
  {
    printf("Cannot install service (%d)\n", GetLastError());
    return;
  }

  // Get a handle to the SCM database. 

  schSCManager = OpenSCManager(
    NULL,                    // local computer
    NULL,                    // ServicesActive database 
    SC_MANAGER_ALL_ACCESS);  // full access rights 

  if (NULL == schSCManager)
  {
    printf("OpenSCManager failed (%d)\n", GetLastError());
    return;
  }

  // Create the service

  schService = CreateService(
    schSCManager,              // SCM database 
    SVCNAME,                   // name of service 
    SVCNAME,                   // service name to display 
    SERVICE_ALL_ACCESS,        // desired access 
    SERVICE_WIN32_OWN_PROCESS, // service type 
    SERVICE_DEMAND_START,      // start type 
    SERVICE_ERROR_NORMAL,      // error control type 
    szPath,                    // path to service's binary 
    NULL,                      // no load ordering group 
    NULL,                      // no tag identifier 
    NULL,                      // no dependencies 
    NULL,                      // LocalSystem account 
    NULL);                     // no password 

  if (schService == NULL)
  {
    printf("CreateService failed (%d)\n", GetLastError());
    CloseServiceHandle(schSCManager);
    return;
  }
  else printf("Service installed successfully\n");

  CloseServiceHandle(schService);
  CloseServiceHandle(schSCManager);
}

//
// Purpose: 
//   Entry point for the service
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
// 
// Return value:
//   None.
//
VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
  // Register the handler function for the service

  gSvcStatusHandle = RegisterServiceCtrlHandler(
    SVCNAME,
    SvcCtrlHandler);

  if (!gSvcStatusHandle)
  {
    //(TEXT("RegisterServiceCtrlHandler fail"));
    return;
  }
  else {
    //SvcReportEvent(TEXT("RegisterServiceCtrlHandler success"));
    //TODO: report to event log where the log file is located
    // or use eventlog
  }

  // These SERVICE_STATUS members remain as set here

  gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  gSvcStatus.dwServiceSpecificExitCode = 0;

  // Report initial status to the SCM

  ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

#if DEBUG
  for (;;)
  {
    Sleep(1000);
  }
#endif

  // Perform service-specific initialization and work.

  SvcInit(dwArgc, lpszArgv);
}

//
// Purpose: 
//   The service code
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
// 
// Return value:
//   None
//
VOID SvcInit(DWORD dwArgc, LPTSTR *lpszArgv)
{
  // TO_DO: Declare and set any required variables.
  //   Be sure to periodically call ReportSvcStatus() with 
  //   SERVICE_START_PENDING. If initialization fails, call
  //   ReportSvcStatus with SERVICE_STOPPED.

  // Create an event. The control handler function, SvcCtrlHandler,
  // signals this event when it receives the stop control code.


  ghSvcStopEvent = CreateEvent(
    NULL,    // default security attributes
    TRUE,    // manual reset event
    FALSE,   // not signaled
    NULL);   // no name

  if (ghSvcStopEvent == NULL)
  {
    ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
    return;
  }

  // Report running status when initialization is complete.

  ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

  // TODO: Perform work until service stops.
  HANDLE hProcess;
  HANDLE hToken;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken))
  {
    LogError(TEXT("unknown problem in openProcessToken"));
  }
  //TODO: open process token [+]
  //TODO: dublicate token ex [+]
  //TODO: set token information [+]
  //TODO: create process as user [+]

  if (!SetPrivilege(hToken, SE_DEBUG_NAME, TRUE)) // TODO: SE_TCB_NAME
  {
    LogError(TEXT("SetPrivilege"));
  }
  if ((hProcess = OpenProcess(
    PROCESS_ALL_ACCESS,
    FALSE,
    GetCurrentProcessId() // PID
    )) == NULL)
  {
    LogError(TEXT("OpenProcess"));
  }

  HANDLE newToken;
  if (DuplicateTokenEx(hToken,
    MAXIMUM_ALLOWED,
    nullptr,
    SecurityIdentification,
    TokenPrimary,
    &newToken))
  {
    LogError(TEXT("Token Dublicated Successfully"));
  }

  DWORD id = 1;
  if (SetTokenInformation(newToken,
    TokenSessionId,
    &id,
    sizeof(id)))
  {
    LogError(TEXT("Set Token Information ok"));
  }
  else
  {
    LogError(TEXT("Set Token Information fail"));
  }

  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  si.lpTitle = "LookAtMe";
  memset(&pi, 0, sizeof(pi));

  if (!CreateProcessAsUserA(
    newToken,
    NULL,
    "cmd.exe",
    NULL, NULL, FALSE, 0,
    NULL, NULL, &si, &pi))
  {
    //LogError(TEXT("CreateProcessAsUser"));
  }
  while (1)
  {
    // Check whether to stop the service.

    WaitForSingleObject(ghSvcStopEvent, INFINITE);

    ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
    return;
  }
}

//
// Purpose: 
//   Sets the current service status and reports it to the SCM.
//
// Parameters:
//   dwCurrentState - The current state (see SERVICE_STATUS)
//   dwWin32ExitCode - The system error code
//   dwWaitHint - Estimated time for pending operation, 
//     in milliseconds
// 
// Return value:
//   None
//
VOID ReportSvcStatus(DWORD dwCurrentState,
  DWORD dwWin32ExitCode,
  DWORD dwWaitHint)
{
  static DWORD dwCheckPoint = 1;

  // Fill in the SERVICE_STATUS structure.

  gSvcStatus.dwCurrentState = dwCurrentState;
  gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
  gSvcStatus.dwWaitHint = dwWaitHint;

  if (dwCurrentState == SERVICE_START_PENDING)
    gSvcStatus.dwControlsAccepted = 0;
  else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

  if ((dwCurrentState == SERVICE_RUNNING) ||
    (dwCurrentState == SERVICE_STOPPED))
    gSvcStatus.dwCheckPoint = 0;
  else gSvcStatus.dwCheckPoint = dwCheckPoint++;

  // Report the status of the service to the SCM.
  SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

//
// Purpose: 
//   Called by SCM whenever a control code is sent to the service
//   using the ControlService function.
//
// Parameters:
//   dwCtrl - control code
// 
// Return value:
//   None
//
VOID WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
  // Handle the requested control code. 

  switch (dwCtrl)
  {
  case SERVICE_CONTROL_STOP:
    ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

    // Signal the service to stop.
    //TODO: 
    // wait for all threads to stop (with timeout)
    // if timeout - terminate non-stopped threads

    //    myDone();

    SetEvent(ghSvcStopEvent);
    ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

    return;

  case SERVICE_CONTROL_INTERROGATE:
    break;

  default:
    break;
  }

}

//
// Purpose: 
//   Logs messages to the event log
//
// Parameters:
//   szFunction - name of function that failed
// 
// Return value:
//   None
//
// Remarks:
//   The service must have an entry in the Application event log.
//
VOID LogError(LPTSTR szFunction)
{
#define buffsize 160
  wchar_t buffer[buffsize];
  StringCchPrintf(buffer, buffsize, L"%s failed with %d", szFunction, GetLastError());
  if (logger)
  {
    fprintf(logger, "%ls \n", buffer);
  }
}