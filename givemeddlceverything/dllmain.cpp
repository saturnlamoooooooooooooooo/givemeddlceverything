#include "complete.h"
#include "mono.h"
#include <Windows.h>
#include <string>

namespace
{
	constexpr int kCompleteButtonId = 1001;
	constexpr int kSaveButtonId = 1002;
	constexpr int kLockButtonId = 1003;

	HMODULE g_module = nullptr;
	HWND g_status = nullptr;

	void Log(const std::string& line)
	{
		OutputDebugStringA(("[givemeddlceverything] " + line + "\n").c_str());

		char temp[MAX_PATH]{};
		if (GetTempPathA(MAX_PATH, temp) == 0)
		{
			return;
		}

		HANDLE file = CreateFileA((std::string(temp) + "givemeddlceverything.log").c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
		{
			return;
		}

		const std::string text = line + "\r\n";
		DWORD written = 0;
		WriteFile(file, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
		CloseHandle(file);
	}

	void SetStatus(const std::string& text)
	{
		if (g_status != nullptr)
		{
			SetWindowTextA(g_status, text.c_str());
		}
		Log(text);
	}

	void RunComplete()
	{
		SetStatus("Working...");
		const ddlc::Result result = ddlc::CompleteEverything();
		SetStatus(result.text);
	}

	void RunLock()
	{
		SetStatus("Clearing...");
		const ddlc::Result result = ddlc::LockEverything();
		SetStatus(result.text);
	}

	void RunSave()
	{
		const ddlc::Result result = ddlc::SaveNow();
		SetStatus(result.text);
	}

	LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
	{
		switch (message)
		{
		case WM_COMMAND:
			if (HIWORD(wparam) == BN_CLICKED)
			{
				if (LOWORD(wparam) == kCompleteButtonId)
				{
					RunComplete();
					return 0;
				}
				if (LOWORD(wparam) == kLockButtonId)
				{
					RunLock();
					return 0;
				}
				if (LOWORD(wparam) == kSaveButtonId)
				{
					RunSave();
					return 0;
				}
			}
			break;

		case WM_CLOSE:
			DestroyWindow(window);
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		default:
			break;
		}

		return DefWindowProcA(window, message, wparam, lparam);
	}

	HWND CreateChild(HWND parent, const char* type, const char* text, int x, int y, int width, int height, int id, DWORD style)
	{
		HWND child = CreateWindowExA(0, type, text, WS_CHILD | WS_VISIBLE | style,
			x, y, width, height, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), g_module, nullptr);
		if (child != nullptr)
		{
			SendMessageA(child, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
		}
		return child;
	}

	HWND CreateMenuWindow()
	{
		WNDCLASSEXA windowClass{};
		windowClass.cbSize = sizeof(windowClass);
		windowClass.lpfnWndProc = WindowProc;
		windowClass.hInstance = g_module;
		windowClass.hCursor = LoadCursorA(nullptr, IDC_ARROW);
		windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
		windowClass.lpszClassName = "givemeddlceverythingWindow";
		RegisterClassExA(&windowClass);

		HWND window = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, windowClass.lpszClassName, "givemeddlceverything", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 40, 40, 340, 254, nullptr, nullptr, g_module, nullptr);
		if (window == nullptr)
		{
			return nullptr;
		}

		CreateChild(window, "BUTTON", "COMPLETE EVERYTHING  (F9)", 14, 14, 300, 46, kCompleteButtonId, BS_PUSHBUTTON);
		CreateChild(window, "BUTTON", "Save now  (F10)", 14, 68, 300, 30, kSaveButtonId, BS_PUSHBUTTON);
		CreateChild(window, "BUTTON", "Lock everything  (F11)  - for testing", 14, 104, 300, 30, kLockButtonId, BS_PUSHBUTTON);
		g_status = CreateChild(window, "STATIC", "Ready.", 14, 142, 300, 60, 0, SS_LEFT);

		ShowWindow(window, SW_SHOWNOACTIVATE);
		return window;
	}

	bool WaitForRuntime()
	{
		for (int attempt = 0; attempt < 120; attempt++)
		{
			std::string error;
			if (mono::Load(error) && mono::get_root_domain() != nullptr)
			{
				return true;
			}
			Sleep(500);
		}
		return false;
	}

	DWORD WINAPI MenuThread(LPVOID)
	{
		if (!WaitForRuntime())
		{
			Log("Gave up waiting for the Mono runtime.");
			FreeLibraryAndExitThread(g_module, 0);
		}

		mono::thread_attach(mono::get_root_domain());
		Log("Attached to Mono.");

		HWND window = CreateMenuWindow();
		if (window == nullptr)
		{
			Log("Couldn't create the menu window.");
			FreeLibraryAndExitThread(g_module, 0);
		}

		bool completeHeld = false;
		bool saveHeld = false;
		bool lockHeld = false;
		bool running = true;

		while (running)
		{
			MSG message;
			while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE))
			{
				if (message.message == WM_QUIT)
				{
					running = false;
					break;
				}
				TranslateMessage(&message);
				DispatchMessageA(&message);
			}

			const bool complete = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
			if (complete && !completeHeld)
			{
				RunComplete();
			}
			completeHeld = complete;

			const bool save = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;
			if (save && !saveHeld)
			{
				RunSave();
			}
			saveHeld = save;

			const bool lock = (GetAsyncKeyState(VK_F11) & 0x8000) != 0;
			if (lock && !lockHeld)
			{
				RunLock();
			}
			lockHeld = lock;

			Sleep(16);
		}

		FreeLibraryAndExitThread(g_module, 0);
	}
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		g_module = module;
		DisableThreadLibraryCalls(module);
		CreateThread(nullptr, 0, MenuThread, nullptr, 0, nullptr);
	}
	return TRUE;
}