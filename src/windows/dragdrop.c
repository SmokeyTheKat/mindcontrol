#include <mindcontrol/windows/wininfo.h>
#include <mindcontrol/device_control.h>
#include <mindcontrol/list.h>

#ifdef __WIN64

#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <shellapi.h>


static struct list file_paths;

bool running = false;
ATOM window_class;
HWND drop_window;

static void free_file_paths(void)
{
	for (list_iterate(&file_paths, i, char*))
		free(*i);
	list_clear(&file_paths, char*);
}

LRESULT CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_CREATE:
	{
		running = true;
	}
	break;
	case WM_DESTROY:
	case WM_CLOSE:
	case WM_QUIT:
		running = false;
		break;
	case WM_ERASEBKGND:
	{
		return 1;
	} break;
	case WM_NCHITTEST:
	{
		return HTTRANSPARENT;
	} break;
	case WM_DROPFILES:
	{
		free_file_paths();

		HDROP drop =  (HDROP)wParam;
		UINT filePathesCount = DragQueryFileW(drop, 0xFFFFFFFF, NULL, 512);
		wchar_t* fileName = NULL;
		UINT longestFileNameLength = 0;
		for(UINT i = 0; i < filePathesCount; ++i)
		{
			UINT fileNameLength = DragQueryFileW(drop, i, NULL, 512) + 1;
			if(fileNameLength > longestFileNameLength)
			{
				longestFileNameLength = fileNameLength;
				fileName = (wchar_t*)realloc(fileName, longestFileNameLength * sizeof(*fileName));
			}
			DragQueryFileW(drop, i, fileName, fileNameLength);

			int file_name_length = wcslen(fileName);

			char* cstr_file_name = malloc(file_name_length);
			wcstombs(cstr_file_name, fileName, file_name_length);

			list_push_back(&file_paths, cstr_file_name, char*);
		}
		free(fileName);
		DragFinish(drop);
		running = false;
		break;
	}

	default:
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

ATOM create_window_class(void)
{
	WNDCLASSEX class_info;
	class_info.cbSize = sizeof(class_info);
	class_info.style = CS_DBLCLKS | CS_NOCLOSE;
	class_info.lpfnWndProc = wndProc;
	class_info.cbClsExtra = 0;
	class_info.cbWndExtra      = 0;
	class_info.hInstance       = hinstance;
	class_info.hIcon           = NULL;
	class_info.hCursor         = NULL;
	class_info.hbrBackground   = NULL;
	class_info.lpszMenuName    = NULL;
	class_info.lpszClassName   = (LPCSTR)L"DragnDropClass";
	class_info.hIconSm         = NULL;
	return RegisterClassEx(&class_info);
}

HWND create_drop_window(ATOM window_class, char* name)
{
	return CreateWindowEx(
		WS_EX_TOPMOST | /*WS_EX_TRANSPARENT | */WS_EX_ACCEPTFILES,
		MAKEINTATOM(window_class),
		name,
		WS_POPUP,
		0, 0, 1, 1,
		NULL, NULL,
		hinstance,
		NULL);
}


bool processEvents(HWND window)
{
	MSG msg;

	while(PeekMessage(&msg, window, 0, 0, PM_REMOVE))
	{
		printf("------------\n");
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return running;
}

void cursor_left_down(void)
{
	INPUT ip = {0};
	ip.type = INPUT_MOUSE;
	ip.mi.mouseData = 0;
	ip.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	ip.mi.time = 0;
	ip.ki.dwExtraInfo = (ULONG_PTR)&(long){0};
	SendInput(1, &ip, sizeof(ip));
}

void cursor_left_up(void)
{
	INPUT ip = {0};
	ip.type = INPUT_MOUSE;
	ip.mi.mouseData = 0;
	ip.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	ip.mi.time = 0;
	ip.ki.dwExtraInfo = (ULONG_PTR)&(long){0};
	SendInput(1, &ip, sizeof(ip));
}

struct list* get_dragdrop_files(void)
{
	POINT pos;
	GetCursorPos(&pos);
	SetWindowPos(drop_window, HWND_TOPMOST, pos.x - 10, pos.y - 10, 20, 20, SWP_SHOWWINDOW);

	cursor_left_up();

	running = true;
	while (processEvents(drop_window));

	ShowWindow(drop_window, SW_HIDE);

	for (list_iterate(&file_paths, i, char*))
		printf("%s\n", *i);

	return &file_paths;
}

void init_dragdrop(void)
{
	window_class = create_window_class();
	drop_window = create_drop_window(window_class, "YO");
	file_paths = make_list(10, char*);
}

#endif