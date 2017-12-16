#pragma comment (lib, "comctl32")

#include <windows.h>

#include "BeatLine.h"
#include "Chrd.h"
#include "Fragment.h"
#include "Note.h"
#include "Element.h"
#include "Pause.h"

#define ID_BUTTON_LOAD_FROM_FILE 1000
// Транспонирование
#define ID_Transpose 1002  
// Тональность
#define ID_Tonality 1003
// Длинна интервала
#define ID_IntervalLength 1004
// Добавить ноту/интервал 
#define ID_AddNoteAndInterval 1005
// Вывести такт
#define ID_PrintTact 1006
#define ID_setBeatLines 1007
#define ID_FragmentBox 1008
#define RUN_BUTTON_ID 1009
#define ID_CHRD_ADD 1020
// MENU
#define ABOUT_MENU_ID 103
#define EXIT_MENU_ID 104
#define FILE_MENU_ID 106
#define CHRD_MENU_ID 107
#define CHRD_COUNT_NOTE_ID 108
// Скрол
#define ID_SCROLL 1010
#define ID_COMBO 1011
// Кнопки
#define ID_ADD_NOTE_FRAGMENT 1012
#define ID_ADD_PAUSE 1013

const int buttonStyle = BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_MULTILINE,
buttonWidth = 100, buttonHeight = 50;

// Тональность установлена
static bool FLAG_TONALITY = FALSE;

HANDLE hwndThread1, hwndThread2, hTimer;
HWND hStatusBar;
HWND scrollBarFrag;
HWND Apply, editblock;
HWND hGrBox, upPrior;
HWND Scroll1;
HWND hDlg, hChildWindow1, hChildWindow2, hChildWindow4, hMainWnd;
WNDCLASSEX wc;
HANDLE hCOM; // хендл сом порта
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LONG WINAPI ChildWndProc(HWND, UINT, WPARAM, LPARAM);

Fragment frag1;
Element *lastClick, *prelastClick;

bool suspend;
char szClassName[] = "MyClass";
TCHAR szChildClassName[] = L"SettingsWindow";
int childwindNum;
static HWND Combo1;
int nItem;

typedef struct paintParam
{
	Fragment *frag;
} beatSTRUCT, *beatSTRUCT_P;

//Поток расстановки тактовых черт
DWORD WINAPI Thread1Proc(CONST LPVOID lpParam)
{
	frag1.setBeatLines();
	SetWindowText(hStatusBar, L"Расстановка завершена");
	ExitThread(0);
}

// Поток определяет тональность
DWORD WINAPI Thread2Proc(CONST LPVOID lpParam)
{
	string tonality;
	wchar_t wtext[1024];
	LPWSTR ptr;
	tonality = frag1.getTonality();
	mbstowcs(wtext, tonality.c_str(), strlen(tonality.c_str()) + 1);
	ptr = wtext;
	MessageBox(hMainWnd, ptr, L"Определение тональности", MB_OK);

	ExitThread(0);
}

// Метод перевода строки в формат LPWSTR
LPWSTR toLPWSTR(string alters)
{
	wchar_t wtext[1024];
	mbstowcs(wtext, alters.c_str(), strlen(alters.c_str()) + 1);
	LPWSTR ptr = wtext;
	return ptr;
}

// Создаем статус бар
HWND DoCreateStatusBar(HWND hwndParent, int idStatus, HINSTANCE hinst, int cParts)
{
	HWND hwndStatus;
	RECT rcClient;
	HLOCAL hloc;
	PINT paParts;
	int i, nWidth;

	InitCommonControls();

	// Дескриптор статус бара
	hwndStatus = CreateWindowEx(0, STATUSCLASSNAME, (PCTSTR)NULL, SBARS_SIZEGRIP |
		WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwndParent, (HMENU)idStatus, hinst, NULL);

	GetClientRect(hwndParent, &rcClient);
	hloc = LocalAlloc(LHND, sizeof(int) * cParts);
	paParts = (PINT)LocalLock(hloc);
	nWidth = rcClient.right / cParts;
	int rightEdge = nWidth;

	for (i = 0; i < cParts; i++)
	{
		paParts[i] = rightEdge;
		rightEdge += nWidth;
	}

	SendMessage(hwndStatus, SB_SETPARTS, (WPARAM)cParts, (LPARAM)paParts);
	LocalUnlock(hloc);
	LocalFree(hloc);

	return hwndStatus;
}

// ComboBox для нот.
// HWND hwndParent - Дескриптор окна.
HWND CreateComboBoxAddNote(HWND hwndParent, int width, int height, int x, int y)
{
	HWND hComboBox = CreateWindow(L"combobox", L" ",
		WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | WS_VSCROLL,
		width, height, x, y, hwndParent, NULL, wc.hInstance, NULL);

	TCHAR a_notes[7][10] = { TEXT("F"), TEXT("C"), TEXT("G"), TEXT("D"),
		TEXT("A"), TEXT("E"), TEXT("B") };
	TCHAR A[7];

	memset(&A, 0, sizeof(A));

	for (int k = 0; k < 7; k += 1)
	{
		wcscpy_s(A, sizeof(A) / sizeof(TCHAR), (TCHAR*)a_notes[k]);
		SendMessage(hComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)A);
	}
	
	SendMessage(hComboBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

	return hComboBox; 	// Создать hComboBox
}

// ComboBox для Альтерации.
// HWND hwndParent - Дескриптор окна.
HWND CreateComboBoxAddNoteAlter(HWND hwndParent, int width, int height, int x, int y)
{
	HWND hComboBox = CreateWindow(L"combobox", L" ", WS_CHILD | WS_VISIBLE
		| CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | WS_VSCROLL, width, height, x, y, hwndParent, NULL, wc.hInstance, NULL);

	TCHAR a_alter[3][10] = { TEXT("_"), TEXT("b"), TEXT("#") };
	TCHAR AA[3];
	memset(&AA, 0, sizeof(AA));

	for (int k = 0; k < 3; k += 1)
	{
		wcscpy_s(AA, sizeof(AA) / sizeof(TCHAR), (TCHAR*)a_alter[k]);
		SendMessage(hComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)AA);
	}
	SendMessage(hComboBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

	return hComboBox; 	// Создать hComboBox
}

// Fragment - окно с нотами и аккордами (Рабочая область)
HWND CreateFragmentBox(HWND hMainWnd, int x, int y, int width, int height)
{
	HWND hFragmentBox = CreateWindow(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_OVERLAPPEDWINDOW, x, y,
		width, height, hMainWnd, (HMENU)ID_FragmentBox, wc.hInstance, NULL);

	return hFragmentBox;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	lastClick = nullptr; prelastClick = nullptr;

	// Заполняем структуру класса окна
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc; //WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"MyClass";
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	// Регистрируем класс окна
	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Не удается зарегистрировать класс.", L"Ошибка", MB_OK);
		return 0;
	}

	// Создаем основное окно приложения
	hMainWnd = CreateWindowW(wc.lpszClassName, L"WndProc", WS_OVERLAPPEDWINDOW | WS_VSCROLL, CW_USEDEFAULT, 
		0, 900, 500, NULL, NULL, hInstance, NULL);

	if (!hMainWnd)
	{
		MessageBox(NULL, L"Не удалось создать главное окно.", L"Ошибка", MB_OK);
		return 0;
	}

	SetWindowText(hMainWnd, L"Музыкальный фрагмент");

	// Показываем окно
	ShowWindow(hMainWnd, nCmdShow);
	UpdateWindow(hMainWnd);

	// Выполняем цикл обработки сообщений до закрытия приложения
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

BOOL CALLBACK DlgProc(HWND hwnd, UINT Message, WPARAM wparam, LPARAM lparam)
{
	switch (Message)
	{
		case WM_CLOSE:
			EndDialog(hwnd, 0);
			break;
			break;
		default:
			break;
	}
	return 0;
}

LRESULT CALLBACK WndProc(HWND hMainWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HDC hDC;
	PAINTSTRUCT ps;
	RECT rect;
	HWND filedit;
	ifstream in;
	wstring arr_w;
	int fragLength;
	HMENU main_menu, menu_view, menu_file, main_change;

	vector <HWND> elementFrNote;
	vector <HWND> elementFrLabel;

	char buffer = '1';
	static HWND combo_add_note, combo_add_note_alter, edit_add_note_dur;
	char pos[5];

	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	OPENFILENAME ofn = { 0 };
	char szDirect[260];
	char szFileName[260];

	wchar_t wtext[1024];
	LPWSTR ptr;

	string alters = "", alterswork;
	string *str = new string[4];

	HWND tootip;
	WNDCLASS w;
	Element *elemnt;
	beatSTRUCT_P param;
	static int nScrollPos;

	switch (msg)
	{
		int x, y, cxClient, cyClient; //координаты

	case WM_CREATE:
	{
		// Начальное значение позиции
		nScrollPos = 0;
		// Задаем диапазон изменения значений
		SetScrollRange(hMainWnd, SB_VERT, 0, 1000, FALSE);
		// Устанавливаем ползунок в начальную позицию
		SetScrollPos(hMainWnd, SB_VERT, nScrollPos, TRUE);

		main_menu = CreateMenu();
		menu_file = CreatePopupMenu();
		menu_view = CreatePopupMenu();
		AppendMenu(main_menu, MF_STRING | MF_POPUP, (UINT)menu_file, L"&Файл");
		AppendMenu(menu_file, MF_STRING, FILE_MENU_ID, L"Открыть");
		AppendMenu(menu_file, MF_STRING, ID_CHRD_ADD, L"Добавить Ноту/Аккорд");
		AppendMenu(menu_file, MF_STRING, ID_ADD_PAUSE, L"Добавить паузу");

		AppendMenu(main_menu, MF_STRING | MF_POPUP, (UINT)menu_view, L"&Правка");
		AppendMenu(menu_view, MF_STRING, ID_Transpose, L"Перемещение по октавам");
		AppendMenu(menu_view, MF_STRING, ID_setBeatLines, L"Расстановка тактов");
		AppendMenu(menu_view, MF_STRING, ID_Tonality, L"Определение тональности");
		AppendMenu(menu_view, MF_STRING, ID_PrintTact, L"Вывести такт");
		AppendMenu(menu_view, MF_STRING, ID_IntervalLength, L"Длина интервала");
		AppendMenu(menu_view, MF_STRING, ID_AddNoteAndInterval, L"Сложение ноты с интервалов");

		AppendMenu(main_menu, MF_STRING, ABOUT_MENU_ID, L"О программе");
		AppendMenu(main_menu, MF_STRING, EXIT_MENU_ID, L"Выход");
		SetMenu(hMainWnd, main_menu);

		DestroyMenu(main_menu);
		UpdateWindow(hMainWnd);
	}
	break;
	// Сообщение от вертикальной полосы просмотра
	case WM_VSCROLL:
	{
		switch (wParam)
		{
		case SB_TOP:
		{
			nScrollPos = 0;
			break;
		}
		case SB_BOTTOM:
		{
			nScrollPos = 500;
			break;
		}
		case SB_LINEUP:
		{
			nScrollPos += 10;
			break;
		}
		case SB_LINEDOWN:
		{
			nScrollPos -= 10;
			break;
		}
		case SB_PAGEUP:
		{
			nScrollPos += cyClient;

			break;
		}
		case SB_PAGEDOWN:
		{
			nScrollPos -= cyClient;
			break;
		}
		case SB_THUMBPOSITION:
		{
			nScrollPos = LOWORD(lParam);
			break;
		}
		// Блокируем для того чтобы избежать
		// мерцания содержимого окна при
		// перемещении ползунка
		case SB_THUMBTRACK:
		{
			nScrollPos = LOWORD(lParam);
			break;
		}
		default:
			break;
		}

		// Ограничиваем диапазон изменения значений
		//if (nScrollPos > 1000) nScrollPos = 1000;
		//if (nScrollPos < 0) nScrollPos = 0;

		// Устанавливаем ползунок в новое положение
		SetScrollPos(hMainWnd, SB_VERT, nScrollPos, TRUE);
		// HWND лейблов которые будем передвигать
		int yyNum = 0;
		elementFrLabel = frag1.getListElementsFragment();
		for (int i = 0; i < 2; i++)
		{
			HWND hElementFrLabel = reinterpret_cast<HWND>(elementFrLabel[i]);
			MoveWindow((HWND)hElementFrLabel, 10, (30 + yyNum) + nScrollPos, 160, 20, TRUE);
			yyNum = 30;
		}

		// HWND нот и аккордов которые будем передвигать
		int xNum = 0, yNum = 0;
		elementFrNote = frag1.getListElementsFragment();
		unsigned frLenNote = elementFrNote.size();
		unsigned j = 0;
		for (unsigned i = 2; i < frLenNote; i++)
		{
			HWND hElementFr = reinterpret_cast<HWND>(elementFrNote[i]);
			MoveWindow((HWND)hElementFr, 10 + xNum, (90 + yNum) + nScrollPos, 70, 60, TRUE);
			if ((j + 1) % 9 == 0)
			{
				xNum = 0;
				yNum += 70;
			}
			else
			{
				xNum += 90;
			}
			j++;
		}

		elementFrNote.empty();
		// Обновляем окно
		UpdateWindow(hMainWnd);
		InvalidateRect(hMainWnd, NULL, TRUE);

		return 0;
	}
	case WM_PAINT:
	{
		hDC = BeginPaint(hMainWnd, &ps);
		GetClientRect(hMainWnd, &rect);
		EndPaint(hMainWnd, &ps);
	}
	break;
	case WM_CLOSE:
		DestroyWindow(hMainWnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_COMMAND:
	{
		switch (wParam)
		{
		case FILE_MENU_ID:
		{
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = NULL;

			mbstowcs(wtext, szDirect, strlen(szDirect) + 1);
			ptr = wtext;
			ofn.lpstrFile = ptr;

			*(ofn.lpstrFile) = 0;
			ofn.nMaxFile = sizeof(szDirect);
			ofn.lpstrFilter = NULL;
			ofn.nFilterIndex = 1;

			mbstowcs(wtext, szFileName, strlen(szDirect) + 1);
			ptr = wtext;
			ofn.lpstrFileTitle = ptr;
			*(ofn.lpstrFileTitle) = 0;
			ofn.nMaxFileTitle = sizeof(szFileName);
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = OFN_EXPLORER;
			GetOpenFileName(&ofn);

			arr_w = wstring(ptr);
			frag1.readFragment(string(arr_w.begin(), arr_w.end()));

			frag1.printFragment(hMainWnd, wc.hInstance);

			UpdateWindow(hMainWnd);
		}
		break;
		case ID_Transpose:
		{
			if (frag1.getLength() == 0) {
				MessageBox(NULL, L"Не выбран фрагмент!", L"Ошибка", MB_OK);
				return 0;
			}

			childwindNum = 1;
			SendMessage(hChildWindow1, WM_CLOSE, NULL, NULL);
			memset(&w, 0, sizeof(WNDCLASS));
			w.style = CS_HREDRAW | CS_VREDRAW;
			w.lpfnWndProc = ChildWndProc;
			w.hInstance = HINSTANCE(wc.hInstance);
			w.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
			w.lpszClassName = szChildClassName;
			RegisterClass(&w);

			hChildWindow1 = CreateWindow(szChildClassName, L"Выбор способа транспонирования", WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, NULL, 300, 200, hMainWnd, NULL, HINSTANCE(wc.hInstance), NULL);
			ShowWindow(hChildWindow1, SW_NORMAL);
			UpdateWindow(hChildWindow1);
			UpdateWindow(hMainWnd);
		}
		break;
		case ID_setBeatLines:
		{
			if (frag1.getLength() == 0)
			{
				MessageBox(NULL, L"Не выбран фрагмент!", L"Ошибка", MB_OK);
				return 0;
			}

			hStatusBar = DoCreateStatusBar(hMainWnd, 0, wc.hInstance, 1);
			SetWindowText(hStatusBar, L"Идет расстановка тактовых черт..");
			hwndThread1 = CreateThread(NULL, 0, &Thread1Proc, NULL, NULL, NULL);
			Sleep(3000);
			frag1.printFragment(hMainWnd, wc.hInstance);
			UpdateWindow(hMainWnd);
		}
		break;
		case ID_Tonality:
		{
			if (frag1.getLength() == 0) {
				MessageBox(NULL, L"Не выбран фрагмент!", L"Ошибка", MB_OK);
				return 0;
			}

			// Определение тональности.
			hwndThread2 = CreateThread(NULL, 0, &Thread2Proc, NULL, NULL, NULL);
		}
		break;
		case ID_PrintTact:
		{
			if (frag1.getLength() == 0) {
				MessageBox(NULL, L"Не выбран фрагмент!", L"Ошибка", MB_OK);
				return 0;
			}

			childwindNum = 2;
			SendMessage(hChildWindow2, WM_CLOSE, NULL, NULL);
			memset(&w, 0, sizeof(WNDCLASS));
			w.style = CS_HREDRAW | CS_VREDRAW;
			w.lpfnWndProc = ChildWndProc;
			w.hInstance = HINSTANCE(wc.hInstance);
			w.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
			w.lpszClassName = szChildClassName;
			RegisterClass(&w);

			hChildWindow2 = CreateWindow(szChildClassName, L"Выбор номера такта", WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, NULL, 300, 200, hMainWnd, NULL, HINSTANCE(wc.hInstance), NULL);

			ShowWindow(hChildWindow2, SW_NORMAL);
			UpdateWindow(hChildWindow2);
			UpdateWindow(hMainWnd);
		}
		break;
		case ID_IntervalLength:
		{
			if (frag1.getLength() == 0) {
				MessageBox(NULL, L"Не выбран фрагмент!", L"Ошибка", MB_OK);
				return 0;
			}

			if (lastClick == nullptr || prelastClick == nullptr)
				MessageBox(NULL, L"Выберите две ноты!", L"Ошибка", MB_OK);
			else
			{
				str[2] = to_string(frag1.getType());
				str[3] = to_string(frag1.getNumber());

				prelastClick->Print(str);
				alters = str[0];

				lastClick->Print(str);
				alterswork = "You chose the notes " + alters + " and " + str[0] + ". Continue?";
				wchar_t wtext[1024];
				mbstowcs(wtext, alterswork.c_str(), strlen(alterswork.c_str()) + 1);
				ptr = wtext;

				if (MessageBox(NULL, ptr, L"", MB_OKCANCEL) == TRUE)
				{
					alterswork = "The interval between " + alters + " and " + str[0] + " = "
						+ to_string(abs(frag1.getInterval(reinterpret_cast<Note*>(lastClick),
							reinterpret_cast<Note*>(prelastClick))) / 2) + " tones.";
					wchar_t wtext[1024];
					mbstowcs(wtext, alterswork.c_str(), strlen(alterswork.c_str()) + 1);
					ptr = wtext;
					MessageBox(NULL, ptr, L"", MB_OK);
				}
			}

			frag1.printFragment(hMainWnd, wc.hInstance);
			UpdateWindow(hMainWnd);
		}
		break;
		case ID_AddNoteAndInterval:
		{
			if (frag1.getLength() == 0) {
				MessageBox(NULL, L"Не выбран фрагмент!", L"Ошибка", MB_OK);
				return 0;
			}

			if (lastClick == nullptr)
				MessageBox(NULL, L"Выберите ноту!", L"Ошибка", MB_OK);
			else
			{
				str[2] = to_string(frag1.getType());
				str[3] = to_string(frag1.getNumber());

				lastClick->Print(str);
				alters = str[0];

				lastClick->Print(str);
				alterswork = "You chose the note " + alters + ". Continue?";
				wchar_t wtext[1024];
				mbstowcs(wtext, alterswork.c_str(), strlen(alterswork.c_str()) + 1);
				ptr = wtext;

				if (MessageBox(NULL, ptr, L"", MB_OKCANCEL) == TRUE)
				{
					childwindNum = 3;
					SendMessage(hChildWindow2, WM_CLOSE, NULL, NULL);
					memset(&w, 0, sizeof(WNDCLASS));
					w.style = CS_HREDRAW | CS_VREDRAW;
					w.lpfnWndProc = ChildWndProc;
					w.hInstance = HINSTANCE(wc.hInstance);
					w.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
					w.lpszClassName = szChildClassName;
					RegisterClass(&w);
					hChildWindow2 = CreateWindow(szChildClassName, L"Задайте интервал", WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT, NULL, 300, 200, hMainWnd, NULL, HINSTANCE(wc.hInstance), NULL);
					ShowWindow(hChildWindow2, SW_NORMAL);
					UpdateWindow(hChildWindow2);
					UpdateWindow(hMainWnd);
				}
			}
		}
		break;
		case ID_ADD_NOTE_FRAGMENT:
		{
			// Получить содержимое из комбобокса (Ноту)
			int noteIndex = SendMessage(combo_add_note, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			TCHAR  name_note[5];
			(TCHAR)SendMessage(combo_add_note, (UINT)CB_GETLBTEXT, (WPARAM)noteIndex, (LPARAM)name_note);
			// Получить содержимое из комбобокса (Альтерацию)
			int alterIndex = SendMessage(combo_add_note_alter, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			TCHAR  alter_note[5];
			(TCHAR)SendMessage(combo_add_note_alter, (UINT)CB_GETLBTEXT, (WPARAM)alterIndex, (LPARAM)alter_note);
			// Получить содержимое из эдита (Длительность)
			TCHAR buff[10];
			GetWindowText(edit_add_note_dur, buff, 10);
			// Дабавляем ноту во фрагмент
			int dur_note = _ttoi(buff);
			frag1.addNoteFragment(name_note[0], alter_note[0], 2, dur_note);
			// Перерисовываем и выводим
			frag1.printFragment(hMainWnd, wc.hInstance);

			// Устанавливаю тональность					
			if (!FLAG_TONALITY)
			{
				FLAG_TONALITY = TRUE;
				Note note;
				int alter_n, name_n;

				if (note.getAlterative() == '#' || note.getAlterative() == '_')
					alter_n = 0;
				else
					alter_n = 1;

				name_n = note.getSharpOrder(note.getName());
				frag1.setTonality(alter_n, name_n);
			}
			UpdateWindow(hMainWnd);
		}
		break;
		case ID_ADD_PAUSE:
		{
			frag1.addElement(new Pause(2));
			frag1.printFragment(hMainWnd, wc.hInstance);
			UpdateWindow(hMainWnd);
		}
		break;
		case ID_CHRD_ADD:
		{
			childwindNum = 4;
			SendMessage(hChildWindow4, WM_CLOSE, NULL, NULL);
			memset(&w, 0, sizeof(WNDCLASS));
			w.style = CS_HREDRAW | CS_VREDRAW;
			w.lpfnWndProc = ChildWndProc;
			w.hInstance = HINSTANCE(wc.hInstance);
			w.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
			w.lpszClassName = szChildClassName;
			RegisterClass(&w);

			hChildWindow4 = CreateWindow(szChildClassName, L"Добавить Ноту/Аккорд", WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, NULL, 600, 300, hMainWnd, NULL, HINSTANCE(wc.hInstance), NULL);
			ShowWindow(hChildWindow4, SW_NORMAL);
			UpdateWindow(hChildWindow4);
			UpdateWindow(hMainWnd);
		}
		break;
		case ABOUT_MENU_ID:
			DialogBoxParam(wc.hInstance, MAKEINTRESOURCE(IDD_DIALOG1), nullptr, DlgProc, 0);
			break;
		case EXIT_MENU_ID:
			DestroyWindow(hMainWnd);
			break;
		default:
			return 0;
		}
	}
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			SendMessage(hMainWnd, WM_CLOSE, 0, 0);
			return 0;
		}
	case WM_LBUTTONDOWN:
	{
		//Узнаем какой фрагмент был выбран.
		hDC = GetDC(hMainWnd);
		x = LOWORD(lParam) - 20; //узнаём координаты
		y = HIWORD(lParam) - 10;

		elemnt = frag1.findElement(x, y);

		if ((elemnt != nullptr) && (elemnt->getType() == 1))
		{
			if (lastClick != nullptr)
				prelastClick = lastClick;

			lastClick = elemnt;
		}
	}
	break;
	case WM_SIZE:
	{
		cxClient = LOWORD(lParam);
		cyClient = HIWORD(lParam);
		return 0;
	}
	break;
	case WM_CTLCOLORSTATIC:
	{
		switch (wParam)
		{
		case 5001:
			HDC hdcStatic = (HDC)wParam;
			SetBkColor(hdcStatic, TRANSPARENT);
			return (INT_PTR)GetStockObject(NULL_BRUSH);
		}
	}
	default:
		return DefWindowProc(hMainWnd, msg, wParam, lParam);
	}
	return 0;
}

LONG WINAPI ChildWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lparam)
{
	static HWND c_add_note1, c_add_note_alter1, e_add_note_dur,
		c_add_note2, c_add_note_alter2,
		c_add_note3, c_add_note_alter3;
	static HWND hEditcount_note, labelChrd, btnChrd;

	TCHAR buf[10];
	LPWSTR item;
	string str = "   ";
	int intervNum;
	wchar_t wtext[1024];
	LPWSTR ptr;
	string alters = "", alterswork;
	string *str1 = new string[4];
	wstring arr_w;
	Note newnote;
	vector <HWND> elemsFrg;

	switch (Message)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case 102:
		{
			if (Button_GetCheck(upPrior) == BST_CHECKED)
			{
				frag1.transpose(-1);
				frag1.printFragment(hMainWnd, wc.hInstance);
			}
			else
			{
				frag1.transpose(1);
				frag1.printFragment(hMainWnd, wc.hInstance);
			}

			InvalidateRect(hWnd, NULL, true);
		}
		break;
		case 103:
		{
			hGrBox = CreateFragmentBox(hMainWnd, 50, 50, 350, 200);
			GetWindowText(editblock, buf, 10);
			frag1.printTact(hGrBox, wc.hInstance, atoi((char*)buf));
			InvalidateRect(hWnd, NULL, true);
		}
		break;
		case 104:
		{
			if (nItem > 12)
				nItem = -((nItem - 1) % 10);

			newnote = *(reinterpret_cast<Note*>(lastClick)) + nItem;
			GetWindowTextW(Combo1, buf, sizeof(buf));
			str1[2] = to_string(frag1.getType());
			str1[3] = to_string(frag1.getNumber());
			lastClick->Print(str1);
			alters = str1[0];
			newnote.Print(str1);

			arr_w = wstring(buf);
			alterswork = alters + " + (" + string(arr_w.begin(), arr_w.end()) + ") = " + str1[0];
			mbstowcs(wtext, alterswork.c_str(), strlen(alterswork.c_str()) + 1);
			ptr = wtext;
			MessageBox(NULL, ptr, L"", MB_OK);

			// Нота полученная в результате сложения с интервалом
			frag1.updateNote((reinterpret_cast<Note*>(lastClick)), newnote.getName(), newnote.getAlterative(), 2);
			frag1.printFragment(hMainWnd, wc.hInstance);
			UpdateWindow(hMainWnd);
		}
		break;
		case CHRD_MENU_ID:
		{
			TCHAR buf[10];
			GetWindowText(hEditcount_note, buf, 10);
			int c_note = _ttoi(buf);

			int noteIndex = SendMessage(c_add_note1, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			TCHAR  name_note[5];
			(TCHAR)SendMessage(c_add_note1, (UINT)CB_GETLBTEXT, (WPARAM)noteIndex, (LPARAM)name_note);

			int alterIndex = SendMessage(c_add_note_alter1, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			TCHAR  alter_note[5];
			(TCHAR)SendMessage(c_add_note_alter1, (UINT)CB_GETLBTEXT, (WPARAM)alterIndex, (LPARAM)alter_note);

			TCHAR buff[10];
			GetWindowText(e_add_note_dur, buff, 10);
			int dur_note = _ttoi(buff);

			Chrd *chrd = new Chrd(dur_note);

			if (c_note == 1)
			{
				frag1.addNoteFragment(name_note[0], alter_note[0], 2, dur_note);

				// Устанавливаю тональность					
				if (!FLAG_TONALITY)
				{
					FLAG_TONALITY = TRUE;
					Note note;
					int alter_n, name_n;

					if (note.getAlterative() == '#' || note.getAlterative() == 'b')
						alter_n = 0;
					else
						alter_n = 1;

					name_n = note.getSharpOrder(note.getName());
					frag1.setTonality(alter_n, name_n);
				}
			}
			else
				chrd->addNote(new Note(name_note[0], alter_note[0] == '_' ? ' ' : alter_note[0], 2, dur_note));

			if (c_note == 2 || c_note == 3)
			{
				int noteIndex2 = SendMessage(c_add_note2, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				TCHAR  name_note2[5];
				(TCHAR)SendMessage(c_add_note2, (UINT)CB_GETLBTEXT, (WPARAM)noteIndex2, (LPARAM)name_note2);

				int alterIndex2 = SendMessage(c_add_note_alter2, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				TCHAR  alter_note2[5];
				(TCHAR)SendMessage(c_add_note_alter2, (UINT)CB_GETLBTEXT, (WPARAM)alterIndex2, (LPARAM)alter_note2);

				chrd->addNote(new Note(name_note2[0], alter_note2[0] == '_' ? ' ' : alter_note2[0], 2, dur_note));

				if (c_note == 3)
				{
					int noteIndex3 = SendMessage(c_add_note3, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
					TCHAR  name_note3[5];
					(TCHAR)SendMessage(c_add_note3, (UINT)CB_GETLBTEXT, (WPARAM)noteIndex3, (LPARAM)name_note3);

					int alterIndex3 = SendMessage(c_add_note_alter3, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
					TCHAR  alter_note3[5];
					(TCHAR)SendMessage(c_add_note_alter3, (UINT)CB_GETLBTEXT, (WPARAM)alterIndex3, (LPARAM)alter_note3);

					chrd->addNote(new Note(name_note3[0], alter_note3[0] == '_' ? ' ' : alter_note3[0], 2, dur_note));
				}
				frag1.addElement(chrd);
			}

			// Перерисовываем и выводим
			frag1.printFragment(hMainWnd, wc.hInstance);
		}
		break;
		case CHRD_COUNT_NOTE_ID:
		{
			// Получить содержимое из эдита (Длительность)
			TCHAR buff[10];
			GetWindowText(hEditcount_note, buff, 10);
			// Дабавляем ноту во фрагмент
			int c_note = _ttoi(buff);

			if (c_note != 1 && c_note != 2 && c_note != 3)
				return 0;

			CreateWindow(L"STATIC", L"Выберите ноту:", WS_CHILD | WS_VISIBLE,
				10, 30, 150, 20, hWnd, NULL, nullptr, NULL);
			c_add_note1 = CreateComboBoxAddNote(hWnd, 10, 60, 150, 200);
			CreateWindow(L"STATIC", L"Выберите alt:", WS_CHILD | WS_VISIBLE,
				10, 90, 150, 20, hWnd, NULL, nullptr, NULL);
			c_add_note_alter1 = CreateComboBoxAddNoteAlter(hWnd, 10, 120, 150, 200);
			CreateWindow(L"STATIC", L"Укажите dur:", WS_CHILD | WS_VISIBLE,
				10, 150, 150, 20, hWnd, NULL, nullptr, NULL);
			e_add_note_dur = CreateWindow(L"EDIT", L"2", WS_CHILD | WS_VISIBLE | WS_BORDER,
				10, 180, 150, 20, hWnd, NULL, nullptr, NULL);
			//---------------------------------------------------------------------------------------------------
			int x_btn = 0;
			if (c_note == 2 || c_note == 3)
			{
				CreateWindow(L"STATIC", L"Выберите ноту:", WS_CHILD | WS_VISIBLE,
					210, 30, 150, 20, hWnd, NULL, nullptr, NULL);
				c_add_note2 = CreateComboBoxAddNote(hWnd, 210, 60, 150, 200);
				CreateWindow(L"STATIC", L"Выберите alt:", WS_CHILD | WS_VISIBLE,
					210, 90, 150, 20, hWnd, NULL, nullptr, NULL);
				c_add_note_alter2 = CreateComboBoxAddNoteAlter(hWnd, 210, 120, 150, 200);
				//---------------------------------------------------------------------------------------------------
				x_btn = 200;
				if (c_note == 3)
				{
					CreateWindow(L"STATIC", L"Выберите ноту:", WS_CHILD | WS_VISIBLE,
						410, 30, 150, 20, hWnd, NULL, nullptr, NULL);
					c_add_note3 = CreateComboBoxAddNote(hWnd, 410, 60, 150, 200);
					CreateWindow(L"STATIC", L"Выберите alt:", WS_CHILD | WS_VISIBLE,
						410, 90, 150, 20, hWnd, NULL, nullptr, NULL);
					c_add_note_alter3 = CreateComboBoxAddNoteAlter(hWnd, 410, 120, 150, 200);
					x_btn = 400;
				}
			}

			CreateWindow(L"BUTTON", L"Добавить", BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP,
				10 + x_btn, 210, 150, 24, hWnd, (HMENU)CHRD_MENU_ID, wc.hInstance, NULL);

			ShowWindow((HWND)labelChrd, SW_HIDE);
			ShowWindow((HWND)btnChrd, SW_HIDE);
			ShowWindow((HWND)hEditcount_note, SW_HIDE);
		}
		break;
		case ID_COMBO:
			if (HIWORD(wParam) == CBN_EDITCHANGE)
				GetWindowTextW(Combo1, item, sizeof(item));

			if (HIWORD(wParam) == CBN_SELCHANGE)
				nItem = ComboBox_GetCurSel(Combo1);
		}
		break;
	case WM_CREATE:
		switch (childwindNum)
		{
		case 1:
		{
			CreateWindow(L"STATIC", L"Транспонировать вверх", WS_CHILD | WS_VISIBLE,
				10, 10, 200, 20, hWnd, (HMENU)150, nullptr, NULL);
			CreateWindowEx(WS_EX_TRANSPARENT, L"BUTTON", L"Вверх", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
				220, 10, 10, 10, hWnd, (HMENU)151, GetModuleHandle(NULL), 0);
			CreateWindow(L"STATIC", L"Транспонировать вниз", WS_CHILD | WS_VISIBLE,
				10, 40, 200, 20, hWnd, (HMENU)152, nullptr, NULL);
			upPrior = CreateWindowEx(WS_EX_TRANSPARENT, L"BUTTON", L"Вниз", WS_CHILD | WS_VISIBLE
				| BS_AUTORADIOBUTTON, 220, 40, 10, 10, hWnd, (HMENU)153, GetModuleHandle(NULL), 0);
			Button_SetCheck(upPrior, 1);

			Apply = CreateWindow(L"BUTTON", L"Применить", BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP,
				95, 80, 110, 20, hWnd, (HMENU)102, wc.hInstance, NULL);
		}
		break;
		case 2:
		{
			CreateWindow(L"STATIC", L"Введите номер такта", WS_CHILD | WS_VISIBLE,
				10, 10, 200, 20, hWnd, NULL, nullptr, NULL);
			editblock = CreateWindow(L"EDIT", L"1", WS_CHILD | WS_VISIBLE | WS_BORDER,
				220, 10, 30, 20, hWnd, (HMENU)155, nullptr, NULL);
			Apply = CreateWindow(L"BUTTON", L"Применить", BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP,
				95, 80, 110, 20, hWnd, (HMENU)103, wc.hInstance, NULL);
		}
		break;
		case 3:
		{
			CreateWindow(L"STATIC", L"Выберите интервал из списка:", WS_CHILD | WS_VISIBLE,
				10, 10, 250, 20, hWnd, NULL, nullptr, NULL);
			Combo1 = CreateWindow(L"combobox", L"combo", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST
				| CBS_AUTOHSCROLL | WS_VSCROLL, 10, 40, 100, 200, hWnd, (HMENU)ID_COMBO, wc.hInstance, NULL);
			nItem = 0;

			TCHAR Intervals[25][10] =
			{
				TEXT("P1"), TEXT("m2"), TEXT("M2"), TEXT("m3"),
				TEXT("M3"), TEXT("P4"), TEXT("A4/d5"), TEXT("P5"), TEXT("m6"),
				TEXT("M6"), TEXT("m7"), TEXT("M7"), TEXT("P8"),
				TEXT("-m2"), TEXT("-M2"), TEXT("-m3"),TEXT("-M3"),
				TEXT("-P4"), TEXT("-A4/d5"), TEXT("-P5"), TEXT("-m6"), TEXT("-M6"),
				TEXT("-m7"), TEXT("-M7"), TEXT("-P8")
			};

			TCHAR A[12];
			int  k = 0;
			memset(&A, 0, sizeof(A));

			for (k = 0; k <= 22; k += 1)
			{
				wcscpy_s(A, sizeof(A) / sizeof(TCHAR), (TCHAR*)Intervals[k]);
				SendMessage(Combo1, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)A);
			}

			SendMessage(Combo1, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
			SendMessage(Combo1, CB_SETEXTENDEDUI, 1, 0);
			Apply = CreateWindow(L"BUTTON", L"Применить", BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD
				| WS_TABSTOP, 95, 80, 110, 20, hWnd, (HMENU)104, wc.hInstance, NULL);
		}
		break;
		case 4:
		{
			labelChrd = CreateWindow(L"STATIC", L"Добавить ноту (1) или аккорд (2-3 ноты):", WS_CHILD | WS_VISIBLE,
				10, 10, 280, 20, hWnd, NULL, nullptr, NULL);
			hEditcount_note = CreateWindow(L"EDIT", L"1", WS_CHILD | WS_VISIBLE | WS_BORDER,
				10, 40, 150, 20, hWnd, NULL, nullptr, NULL);
			btnChrd = CreateWindow(L"BUTTON", L"Добавить", BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP,
				10, 70, 150, 24, hWnd, (HMENU)CHRD_COUNT_NOTE_ID, wc.hInstance, NULL);
		}
		break;
		}
	case WM_MOVE: case WM_DESTROY:
		InvalidateRect(hWnd, NULL, true);
		break;
	default:
		return DefWindowProc(hWnd, Message, wParam, lparam);
	}
	return 0;
}