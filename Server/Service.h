#pragma once
#include <WinSock2.h>
#define serviceName TEXT("CHAT_SERVER")


SERVICE_STATUS serviceStatus;
SERVICE_STATUS_HANDLE serviceStatusHandle;
LPCWSTR servicePath=L"G:\\project\\WindowsService\\Debug\\WindowsService.exe";

int addLogMessage(const char* text)
{
	return printf(text);
}


void ControlHandler(DWORD request) {
	switch (request)
	{
	case SERVICE_CONTROL_STOP:
		addLogMessage("Stopped.\n");

		serviceStatus.dwWin32ExitCode = 0;
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);
		return;

	case SERVICE_CONTROL_SHUTDOWN:
		addLogMessage("Shutdown.\n");

		serviceStatus.dwWin32ExitCode = 0;
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);
		return;

	default:
		break;
	}

	SetServiceStatus(serviceStatusHandle, &serviceStatus);

	return;
}


void ServiceMain(int argc, char** argv) {

	int i = 0;

	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	serviceStatus.dwCurrentState = SERVICE_START_PENDING;
	serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	serviceStatus.dwWin32ExitCode = 0;
	serviceStatus.dwServiceSpecificExitCode = 0;
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwWaitHint = 0;

	serviceStatusHandle = RegisterServiceCtrlHandler(serviceName, (LPHANDLER_FUNCTION)ControlHandler);
	if (serviceStatusHandle == (SERVICE_STATUS_HANDLE)0) {
		return;
	}


	serviceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(serviceStatusHandle, &serviceStatus);

	while (serviceStatus.dwCurrentState == SERVICE_RUNNING)
	{
		char buffer[255];
		sprintf_s(buffer, "%u", i);
		int result = addLogMessage(buffer);
		if (result) {
			serviceStatus.dwCurrentState = SERVICE_STOPPED;
			serviceStatus.dwWin32ExitCode = -1;
			SetServiceStatus(serviceStatusHandle, &serviceStatus);
			return;
		}
		i++;
	}

	return;
}

int InstallService() {
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (!hSCManager) {
		addLogMessage("Error: Can't open Service Control Manager\n");
		return -1;
	}

	SC_HANDLE hService = CreateService(
		hSCManager,
		serviceName,
		serviceName,
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		L"G:\\project\\WindowsService\\Debug\\WindowsService.exe",
		NULL, NULL, NULL, NULL, NULL
	);

	if (!hService) {
		int err = GetLastError();
		switch (err) {
		case ERROR_ACCESS_DENIED:
			addLogMessage("Error: ERROR_ACCESS_DENIED\n");
			break;
		case ERROR_CIRCULAR_DEPENDENCY:
			addLogMessage("Error: ERROR_CIRCULAR_DEPENDENCY\n");
			break;
		case ERROR_DUPLICATE_SERVICE_NAME:
			addLogMessage("Error: ERROR_DUPLICATE_SERVICE_NAME\n");
			break;
		case ERROR_INVALID_HANDLE:
			addLogMessage("Error: ERROR_INVALID_HANDLE\n");
			break;
		case ERROR_INVALID_NAME:
			addLogMessage("Error: ERROR_INVALID_NAME\n");
			break;
		case ERROR_INVALID_PARAMETER:
			addLogMessage("Error: ERROR_INVALID_PARAMETER\n");
			break;
		case ERROR_INVALID_SERVICE_ACCOUNT:
			addLogMessage("Error: ERROR_INVALID_SERVICE_ACCOUNT\n");
			break;
		case ERROR_SERVICE_EXISTS:
			addLogMessage("Error: ERROR_SERVICE_EXISTS\n");
			break;
		default:
			addLogMessage("Error: Undefined\n");
		}
		CloseServiceHandle(hSCManager);
		return -1;
	}
	CloseServiceHandle(hService);

	CloseServiceHandle(hSCManager);
	addLogMessage("Success install service!\n");
	return 0;
}

int RemoveService() {
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager) {
		addLogMessage("Error: Can't open Service Control Manager\n");
		return -1;
	}
	SC_HANDLE hService = OpenService(hSCManager, serviceName, SERVICE_STOP | DELETE);
	if (!hService) {
		addLogMessage("Error: Can't remove service\n");
		CloseServiceHandle(hSCManager);
		return -1;
	}

	DeleteService(hService);
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	addLogMessage("Success remove service!\n");
	return 0;
}

int StartService() 
{
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	SC_HANDLE hService = OpenService(hSCManager, serviceName, SERVICE_START);
	if (!StartService(hService, 0, NULL)) {
		CloseServiceHandle(hSCManager);
		addLogMessage("Error: Can't start service\n");
		return -1;
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	addLogMessage("Ey Start\n");				
	return 0;
}

void Service(int argc, char* argv[])
{
	servicePath = LPTSTR(argv[0]);
	addLogMessage("Ey\n");

	if (argc - 1 == 0) {
		addLogMessage("Ey1\n");
		SERVICE_TABLE_ENTRY ServiceTable[1];
		ServiceTable[0].lpServiceName = (LPWSTR)serviceName;
		ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;

		if (!StartServiceCtrlDispatcher(ServiceTable)) {
			addLogMessage("Error: StartServiceCtrlDispatcher\n");
		}
	}
	else if (strcmp(argv[argc - 1], "install") == 0) {
		addLogMessage("Ey Install\n");
		InstallService();
	}
	else if (strcmp(argv[argc - 1], "remove") == 0) {
		addLogMessage("Ey Remove\n");
		RemoveService();
	}
	else if (strcmp(argv[argc - 1], "start") == 0) {
		
		StartService();
	}
}