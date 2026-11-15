
#ifndef volH
#define volH

#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <iostream>
#include <string.h>

extern int NumberOfSoundSessions;

#define STATUS_SUCCESS               ((NTSTATUS)0x00000000L)
#define STATUS_INFO_LENGTH_MISMATCH  ((NTSTATUS)0xC0000004L)

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemProcessInformation = 5
} SYSTEM_INFORMATION_CLASS;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;

typedef LONG KPRIORITY; // Thread priority

typedef struct _SYSTEM_PROCESS_INFORMATION_DETAILD {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER SpareLi1;
    LARGE_INTEGER SpareLi2;
    LARGE_INTEGER SpareLi3;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    ULONG InheritedFromUniqueProcessId;
    ULONG HandleCount;
    BYTE Reserved4[4];
    PVOID Reserved5[11];
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER Reserved6[6];
} SYSTEM_PROCESS_INFORMATION_DETAILD, *PSYSTEM_PROCESS_INFORMATION_DETAILD;

typedef NTSTATUS (WINAPI *PFN_NT_QUERY_SYSTEM_INFORMATION)(
  IN       SYSTEM_INFORMATION_CLASS SystemInformationClass,
  IN OUT   PVOID SystemInformation,
  IN       ULONG SystemInformationLength,
  OUT OPTIONAL  PULONG ReturnLength
);

String GetNameByPid(int pid) {
	size_t bufferSize = 102400;
	String ans="";
    PSYSTEM_PROCESS_INFORMATION_DETAILD pspid=
        (PSYSTEM_PROCESS_INFORMATION_DETAILD) malloc (bufferSize);
    ULONG ReturnLength;
    PFN_NT_QUERY_SYSTEM_INFORMATION pfnNtQuerySystemInformation = (PFN_NT_QUERY_SYSTEM_INFORMATION)
        GetProcAddress (GetModuleHandle(TEXT("ntdll.dll")), "NtQuerySystemInformation");
    NTSTATUS status;

    while (TRUE) {
        status = pfnNtQuerySystemInformation (SystemProcessInformation, (PVOID)pspid,
                                              bufferSize, &ReturnLength);
        if (status == STATUS_SUCCESS)
            break;
        else if (status != STATUS_INFO_LENGTH_MISMATCH) { // 0xC0000004L
            _tprintf (TEXT("ERROR 0x%X\n"), status);
            return 1;   // error
        }

        bufferSize *= 2;
        pspid = (PSYSTEM_PROCESS_INFORMATION_DETAILD) realloc ((PVOID)pspid, bufferSize);
    }

    for (;;
		 pspid=(PSYSTEM_PROCESS_INFORMATION_DETAILD)(pspid->NextEntryOffset + (PBYTE)pspid)) {

		if((int)(pspid->UniqueProcessId)==pid)
		ans=(pspid->ImageName.Length && pspid->ImageName.Buffer)? pspid->ImageName.Buffer : L"";

		if (pspid->NextEntryOffset == 0) break;
    }

	return ans;
}

template <class T> void SafeRelease(T *ppT) {
	if (ppT) {
		(ppT)->Release();
		ppT = NULL;
	}
}

class SoundVolume {
public:
	HRESULT hr=NULL;
	IAudioMeterInformation *pMeterInfo = NULL;
	double newVolume;

	SoundVolume(void) {
		hr=NULL;
		CoInitialize(NULL);
	}

	~SoundVolume(void)	{
		CoUninitialize();
	}

void * GetInfo(void) {
	return pMeterInfo;
}

int GetNumberOfSoundSessions(void) {
	IAudioSessionManager2 *pSessionManager;

	IMMDeviceEnumerator *deviceEnumerator = NULL;
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER,
						  __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);

	IMMDevice *defaultDevice = NULL;

	hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
	hr = defaultDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&pSessionManager);

//	deviceEnumerator->Release();
//	deviceEnumerator = NULL;
	SafeRelease(deviceEnumerator);

	if (!pSessionManager) {
		return 0;
	}

	int cbSessionCount = 0;

	IAudioSessionEnumerator* pSessionList = NULL;
	hr = pSessionManager->GetSessionEnumerator(&pSessionList);
	hr = pSessionList->GetCount(&cbSessionCount);

	SafeRelease(pSessionList);

	return cbSessionCount;
}

void SetPositionOfSoundSessions(float volume) {
	IAudioSessionManager2 *pSessionManager;

	IMMDeviceEnumerator *deviceEnumerator = NULL;
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER,
						  __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);

	IMMDevice *defaultDevice = NULL;

	hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
	hr = defaultDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&pSessionManager);

//	deviceEnumerator->Release();
//	deviceEnumerator = NULL;
	SafeRelease(deviceEnumerator);

	if (!pSessionManager) {
		return;
	}

	int cbSessionCount = 0;
	LPWSTR pswSession = NULL;

	IAudioSessionEnumerator* pSessionList = NULL;
	IAudioSessionControl* pSessionControl = NULL;

	hr = pSessionManager->GetSessionEnumerator(&pSessionList);
	hr = pSessionList->GetCount(&cbSessionCount);

	for (int index = 0 ; index < cbSessionCount ; index++) {
		CoTaskMemFree(pswSession);
		SafeRelease(pSessionControl);

		hr = pSessionList->GetSession(index, &pSessionControl);

		IAudioSessionControl2* pSessionControl2 = NULL;
		pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);

		ISimpleAudioVolume* pSimpleAudioVolume = NULL;
		hr = pSessionControl2->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pSimpleAudioVolume);
		DWORD n = 0;
		hr = pSessionControl2->GetProcessId(&n);

		if ( wcscmp(GetNameByPid(n).c_str(), L"Player_c.exe")==0) {
			pSimpleAudioVolume->SetMute(false, NULL);
			pSimpleAudioVolume->SetMasterVolume(volume, NULL );
			pSimpleAudioVolume->Release();
		}
		pSessionControl->Release();
		pSessionControl2->Release();
	}

	CoTaskMemFree(pswSession);
	SafeRelease(pSessionControl);
	CoTaskMemFree(pSessionList);
}

void SetMuteOfSysSounds(float volume) {
	IAudioSessionManager2 *pSessionManager;

	IMMDeviceEnumerator *deviceEnumerator = NULL;
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER,
						  __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);

	IMMDevice *defaultDevice = NULL;

	hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
	hr = defaultDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&pSessionManager);

	SafeRelease(deviceEnumerator);

	if (!pSessionManager) {
		return;
	}

	int cbSessionCount = 0;
	LPWSTR pswSession = NULL;

	IAudioSessionEnumerator* pSessionList = NULL;
	IAudioSessionControl* pSessionControl = NULL;

	hr = pSessionManager->GetSessionEnumerator(&pSessionList);
	hr = pSessionList->GetCount(&cbSessionCount);

	for (int index = 0 ; index < cbSessionCount ; index++) {
		CoTaskMemFree(pswSession);
		SafeRelease(pSessionControl);

		hr = pSessionList->GetSession(index, &pSessionControl);

		IAudioSessionControl2* pSessionControl2 = NULL;
		pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);

		ISimpleAudioVolume* pSimpleAudioVolume = NULL;
		hr = pSessionControl2->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pSimpleAudioVolume);
		DWORD n = 0;
		hr = pSessionControl2->GetProcessId(&n);
		if ( GetNameByPid(n)=="") {
			pSimpleAudioVolume->SetMute(volume, NULL);
			pSimpleAudioVolume->Release();
		}
		pSessionControl->Release();
		pSessionControl2->Release();
	}

	CoTaskMemFree(pswSession);
	SafeRelease(pSessionControl);
	CoTaskMemFree(pSessionList);
}

float GetPosition(int direct, bool bScalar) {

	IMMDeviceEnumerator *deviceEnumerator = NULL;
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER,
						  __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);

	IMMDevice *defaultDevice = NULL;
	IMMDeviceCollection *wdevices;
	switch (direct) {
		case 0:
		hr = deviceEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &defaultDevice);
		hr = defaultDevice->Activate(__uuidof(IAudioMeterInformation), CLSCTX_ALL, NULL, (void**)&pMeterInfo);
		break;
		case 1:
		hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
		hr = defaultDevice->Activate(__uuidof(IAudioMeterInformation), CLSCTX_ALL, NULL, (void**)&pMeterInfo);
		break;
	}
	deviceEnumerator->Release();
	deviceEnumerator = NULL;

	IAudioEndpointVolume *endpointVolume = NULL;
	hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume),
		 CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
	defaultDevice->Release();
	defaultDevice = NULL;

	float currentVolume = 0;
	if (!bScalar)
		endpointVolume->GetMasterVolumeLevel(&currentVolume); // in dB
	else
		endpointVolume->GetMasterVolumeLevelScalar(&currentVolume); // scalar
	endpointVolume->Release();

	return currentVolume;
}

void SetPosition(int direct, double nVolume, bool bScalar) {

	IMMDeviceEnumerator *deviceEnumerator = NULL;
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER,
						  __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);

	IMMDevice *defaultDevice = NULL;
	if (direct==0)
		hr = deviceEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &defaultDevice);
	else
		hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
	deviceEnumerator->Release();
	deviceEnumerator = NULL;

	IAudioEndpointVolume *endpointVolume = NULL;
	hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume),
		 CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
	defaultDevice->Release();
	defaultDevice = NULL;

	newVolume = nVolume;
	if (!bScalar)
		hr = endpointVolume->SetMasterVolumeLevel((float)newVolume, NULL);
	else
		hr = endpointVolume->SetMasterVolumeLevelScalar((float)newVolume, NULL);
	endpointVolume->Release();
}
};

#endif

