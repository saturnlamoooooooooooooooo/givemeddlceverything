#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include <string>

namespace
{
	std::string DirectoryOfThisExe()
	{
		char path[MAX_PATH]{};
		GetModuleFileNameA(nullptr, path, MAX_PATH);
		std::string full(path);
		const size_t slash = full.find_last_of('\\');
		return (slash == std::string::npos) ? std::string() : full.substr(0, slash + 1);
	}

	DWORD FindProcessId(const std::string& executableName)
	{
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (snapshot == INVALID_HANDLE_VALUE)
		{
			return 0;
		}

		PROCESSENTRY32 entry{};
		entry.dwSize = sizeof(entry);

		DWORD processId = 0;
		if (Process32First(snapshot, &entry))
		{
			do
			{
				if (_stricmp(entry.szExeFile, executableName.c_str()) == 0)
				{
					processId = entry.th32ProcessID;
					break;
				}
			} while (Process32Next(snapshot, &entry));
		}

		CloseHandle(snapshot);
		return processId;
	}
}

int main(int argc, char** argv)
{
	const std::string dllPath = (argc > 1) ? argv[1] : DirectoryOfThisExe() + "givemeddlceverything.dll";
	const std::string processName = (argc > 2) ? argv[2] : "Doki Doki Literature Club Plus.exe";

	if (GetFileAttributesA(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		std::printf("Can't find the DLL at:\n  %s\n", dllPath.c_str());
		return 1;
	}

	const DWORD processId = FindProcessId(processName);
	if (processId == 0)
	{
		std::printf("'%s' isn't running.\n", processName.c_str());
		return 1;
	}

	HANDLE process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, processId);
	if (process == nullptr)
	{
		std::printf("OpenProcess failed (%lu). Try running this as administrator.\n", GetLastError());
		return 1;
	}

	int exitCode = 1;
	const SIZE_T pathBytes = dllPath.size() + 1;
	LPVOID remotePath = VirtualAllocEx(process, nullptr, pathBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (remotePath == nullptr)
	{
		std::printf("VirtualAllocEx failed (%lu).\n", GetLastError());
	}
	else if (!WriteProcessMemory(process, remotePath, dllPath.c_str(), pathBytes, nullptr))
	{
		std::printf("WriteProcessMemory failed (%lu).\n", GetLastError());
	}
	else
	{
		auto loadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"));

		HANDLE remoteThread = CreateRemoteThread(process, nullptr, 0, loadLibrary, remotePath, 0, nullptr);
		if (remoteThread == nullptr)
		{
			std::printf("CreateRemoteThread failed (%lu).\n", GetLastError());
		}
		else
		{
			WaitForSingleObject(remoteThread, 10000);

			DWORD loadResult = 0;
			GetExitCodeThread(remoteThread, &loadResult);
			CloseHandle(remoteThread);

			if (loadResult == 0)
			{
				std::printf("LoadLibrary returned null inside the game - the DLL failed to load.\n");
			}
			else
			{
				std::printf("Injected. Look for the givemeddlceverything window (or press F9).\n");
				exitCode = 0;
			}
		}
	}

	if (remotePath != nullptr)
	{
		VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
	}
	CloseHandle(process);
	return exitCode;
}