#pragma comment (lib, "comctl32")

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

// MENU
#define ABOUT_MENU_ID 103
#define EXIT_MENU_ID 104
#define FILE_MENU_ID 106

#define ID_SCROLL 1010
#define ID_COMBO 1011

#define ID_ADD_NOTE_FRAGMENT 1012

const int buttonStyle = BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_MULTILINE, 
		  buttonWidth = 100,
		  buttonHeight = 50;

HANDLE hwndThread1, hwndThread2, hTimer;

HWND hStatusBar;

HWND scrollBarFrag;
HWND Apply, editblock;

HWND hGrBox, upPrior;
HWND hDlg, hChildWindow1, hChildWindow2, hMainWnd;;
WNDCLASSEX wc;
bool suspend;
	char szClassName[] = "MyClass";
	TCHAR szChildClassName[] = L"SettingsWindow";

HANDLE hCOM; // хендл сом порта
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LONG WINAPI ChildWndProc(HWND, UINT, WPARAM,LPARAM);
int childwindNum;

Fragment frag1;

Element *lastClick, *prelastClick;

static HWND Combo1;
int nItem ;

typedef struct paintParam
{
	Fragment *frag;
} beatSTRUCT, *beatSTRUCT_P;

//Поток расстановки тактовых черт
DWORD WINAPI Thread1Proc(CONST LPVOID lpParam) {

	frag1.setBeatLines();

	Sleep(3000);

	SetWindowText(hStatusBar, L"Расстановка завершена");
	
	frag1.printFragment(hGrBox, wc.hInstance);
					
	UpdateWindow(hMainWnd);

	ExitThread(0);
}

// Поток определяет тональность
DWORD WINAPI Thread2Proc(CONST LPVOID lpParam) {

	string tonality;
	wchar_t wtext[1024];
	LPWSTR ptr;
	tonality = frag1.getTonality();
	mbstowcs(wtext, tonality.c_str(), strlen(tonality.c_str())+1);
	ptr = wtext;
	MessageBox(hMainWnd, ptr, L"Определение тональности", MB_OK);

	ExitThread(0);
}

// Метод перевода строки в формат LPWSTR
LPWSTR toLPWSTR(string alters)
{
	wchar_t wtext[1024];
	mbstowcs(wtext, alters.c_str(), strlen(alters.c_str())+1);
	LPWSTR ptr = wtext;
	return ptr;
}

// Создаем статус бар
//
//
HWND DoCreateStatusBar(HWND hwndParent, int idStatus, HINSTANCE
	hinst, int cParts)
{
	HWND hwndStatus;
	RECT rcClient;
	HLOCAL hloc;
	PINT paParts;
	int i, nWidth;

	InitCommonControls();

	// Дескриптор статус бара
	hwndStatus = CreateWindowEx(
		0,STATUSCLASSNAME,         
		(PCTSTR)NULL,           
		SBARS_SIZEGRIP |         
		WS_CHILD | WS_VISIBLE,   
		0, 0, 0, 0,              
		hwndParent,             
		(HMENU)idStatus,       
		hinst,                   
		NULL);                  

	GetClientRect(hwndParent, &rcClient);

	hloc = LocalAlloc(LHND, sizeof(int) * cParts);
	paParts = (PINT)LocalLock(hloc);
	nWidth = rcClient.right / cParts;
	int rightEdge = nWidth;
	for (i = 0; i < cParts; i++) {
		paParts[i] = rightEdge;
		rightEdge += nWidth;
	}
	SendMessage(hwndStatus, SB_SETPARTS, (WPARAM)cParts, (LPARAM)
		paParts);

	LocalUnlock(hloc);
	LocalFree(hloc);
	return hwndStatus;
}

// Создание вертикального скролла
HWND CreateAVerticalScrollBar(HWND hwndParent, int sbHeight)
{
    RECT rect;

	// Получить размеры клиентской области родительского окна;
    if (!GetClientRect(hwndParent, &rect))
        return NULL;

    // Создать scroll bar
	return (CreateWindowEx(
		0, // расширенных стилей
		L"SCROLLBAR", // класс управления полосой прокрутки
		(PTSTR)NULL, // текста окна
		WS_CHILD | WS_VISIBLE // стили окон
		| SBS_VERT, // стиль горизонтальной полосы прокрутки
		rect.left, // горизонтальное положение
		rect.bottom - sbHeight, // вертикальное положение
		rect.right, // ширина полосы прокрутки
		sbHeight, // высота полосы прокрутки
		hwndParent, // обращение к главному окну
		(HMENU)NULL, // указатель меню
		wc.hInstance, // экземпляр, владеющий этим окном
		(PVOID)NULL // указатель не нужен
	));
}

// Создает всплывающую подсказку для элемента в диалоговом окне.
// Параметры:
// idTool - идентификатор элемента диалогового окна.
// nDlg - дескриптор окна диалогового окна.
// pszText - строка, используемая в качестве текста всплывающей подсказки.
// return:
// Dсплывающую подсказку.
HWND CreateToolTip(int toolID, HWND hDlg, HINSTANCE hInst, PTSTR pszText)
{
	if (!toolID || !hDlg || !pszText)
	{
		return NULL;
	}

	// Get the window of the tool.
	HWND hwndTool = GetDlgItem(hDlg, toolID);
	if (!hwndTool)
	{
		return NULL;
	}

	// Create the tooltip. g_hInst is the global instance handle.
	HWND hwndTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hDlg, NULL,
		hInst, NULL);

	if (!hwndTip)
	{
		return NULL;
	}

	// Associate the tooltip with the tool.
	TOOLINFO toolInfo = { 0 };
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = hDlg;
	toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	toolInfo.uId = (UINT_PTR)hwndTool;
	toolInfo.lpszText = pszText;
	if (!SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo))
	{
		DestroyWindow(hwndTip);
		return NULL;
	}

	return hwndTip;
}

HWND DoCreateTooltip(HWND hwndOwner)
{
	HWND hwndTT;
	int row, col; 
	TOOLINFO ti;
	int id = 0; 
	InitCommonControls();
	hwndTT = CreateWindow(TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, (HMENU)NULL, wc.hInstance, NULL);
	if (hwndTT == (HWND)NULL)
		return (HWND)NULL;

			ti.cbSize = sizeof(TOOLINFO);
			ti.uFlags = 0;
			ti.hwnd = hwndOwner;
			ti.hinst = wc.hInstance;
			ti.uId = (UINT)id;
			ti.lpszText = L"Подсказка";

			if (!SendMessage(hwndTT, TTM_ADDTOOL, 0,
				(LPARAM)(LPTOOLINFO)&ti))
				return NULL;
	return hwndTT;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	lastClick = nullptr; prelastClick = nullptr;

	MSG msg;

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
	if (!RegisterClassEx(&wc)) {
		MessageBox(NULL, L"Не удается зарегистрировать класс.", L"Ошибка", MB_OK);
		return 0;
	}

	// Создаем основное окно приложения
	hMainWnd = CreateWindowW(wc.lpszClassName, L"WndProc", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 1100, 500, NULL, NULL, hInstance, NULL);
	if (!hMainWnd) {
		MessageBox(NULL, L"Не удалось создать главное окно.", L"Ошибка", MB_OK);
		return 0;
	}
	SetWindowText(hMainWnd, L"Музыкальный фрагмент");

	// Показываем окно
	ShowWindow(hMainWnd, nCmdShow);
	UpdateWindow(hMainWnd);

	// Выполняем цикл обработки сообщений до закрытия приложения
	while(GetMessage(&msg, NULL, 0, 0)) {
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

// Debug Pix потом удалить
void drawPix(HWND hWnd, int x, int y)
{
	InvalidateRect(hWnd, NULL, TRUE);
	RECT rectangle;
	RECT clientRect;
	InvalidateRect(hWnd, NULL, TRUE);
	HDC hdc; //создаём контекст устройства
	PAINTSTRUCT paintStruct; //создаём экземпляр структуры графического вывода
	static int size = 5;

	rectangle = { x,y,x + size,y + size };

	hdc = BeginPaint(hWnd, &paintStruct);
	GetClientRect(hWnd, &clientRect);

	//рисуем квадрат
	FillRect(hdc, &rectangle, HBRUSH(CreateSolidBrush(RGB(128, 0, 128))));
	EndPaint(hWnd, &paintStruct);

	ReleaseDC(hWnd, hdc);
	DeleteObject(&rectangle);
}

LRESULT CALLBACK WndProc(HWND hMainWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	char buffer = '1';
	static HWND Scroll1;
	static HWND combo_add_note, combo_add_note_alter, edit_add_note_dur;
	static int nPage, nCurPos, nPosMin, nPosMax;
	char pos[5];

	HDC hDC;
	PAINTSTRUCT ps;
	RECT rect;
	HWND filedit;
	ifstream in;
	wstring arr_w;
	int fragLength;
	HMENU main_menu, menu_view;

	SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    OPENFILENAME ofn={0};
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

	switch (msg) {
		int x,y; //координаты

		case WM_CREATE:
		{
			// Создаем скрол бар
			Scroll1 = CreateWindow(L"scrollbar", NULL, SBS_VERT | SBS_BOTTOMALIGN | WS_CHILD | WS_VISIBLE, 6 * buttonWidth + 250 + 20, 18, 16, 392, hMainWnd, (HMENU)0, NULL, NULL);

			nPage = 10;
			nPosMin = 1;
			nPosMax = 390;
			nCurPos = 0;

			SetScrollRange(Scroll1, SB_CTL, nPosMin, nPosMax, TRUE);
			SetScrollPos(Scroll1, SB_CTL, nCurPos, TRUE);

			/*CreateToolbarEx(hMainWnd, WS_CHILD | WS_VISIBLE | CCS_TOP, 1,
			0, HINST_COMMCTRL, IDB_STD_SMALL_COLOR, tbb, 7, 0, 0, 0, 0, sizeof(TBBUTTON));*/

			// Лейбл  "Фрагмент"
			hGrBox = CreateWindow(L"Button", L"Н О Т Ы", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 20, 10, 6 * buttonWidth + 250, 400, hMainWnd, (HMENU)ID_FragmentBox, wc.hInstance, NULL);

			main_menu = CreateMenu();
			menu_view = CreatePopupMenu();
			AppendMenu(main_menu, MF_STRING | MF_POPUP, (UINT)menu_view, L"&Ноты");
			AppendMenu(menu_view, MF_STRING, FILE_MENU_ID, L"Открыть");
			AppendMenu(menu_view, MF_STRING, ID_Transpose, L"Перемещение по октавам");
			AppendMenu(menu_view, MF_STRING, ID_setBeatLines, L"Расстановка тактов");
			AppendMenu(menu_view, MF_STRING, ID_Tonality, L"Определение тональности");
			AppendMenu(menu_view, MF_STRING, ID_PrintTact, L"Вывести такт");
			AppendMenu(menu_view, MF_STRING, ID_IntervalLength, L"Длина интервала");
			AppendMenu(menu_view, MF_STRING, ID_AddNoteAndInterval, L"Сложение ноты с интервалов");

			AppendMenu(main_menu, MF_STRING, ABOUT_MENU_ID, L"О программе");
			AppendMenu(main_menu, MF_STRING, EXIT_MENU_ID, L"Выход");
			SetMenu(hMainWnd, main_menu);

			//Добавление ноты
			CreateWindow(L"STATIC",
				L"Выберите ноту:", WS_CHILD | WS_VISIBLE,
				900, 10, 150, 20, hMainWnd, NULL, nullptr, NULL);

			combo_add_note = CreateWindow(L"combobox", L"combo_add_note", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | WS_VSCROLL,
				900, 40, 150, 200, hMainWnd, NULL, wc.hInstance, NULL);
			TCHAR a_notes[7][10] =
			{
				TEXT("F"), TEXT("C"), TEXT("G"), TEXT("D"),
				TEXT("A"), TEXT("E"), TEXT("B")
			};

			TCHAR A[7];
			memset(&A, 0, sizeof(A));
			for (int k = 0; k < 7; k += 1)
			{
				wcscpy_s(A, sizeof(A) / sizeof(TCHAR), (TCHAR*)a_notes[k]);
				SendMessage(combo_add_note, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)A);
			}
			SendMessage(combo_add_note, CB_SETCURSEL, (WPARAM)1, (LPARAM)0);
			//---------------------------------------------------------------------------------------------------
			CreateWindow(L"STATIC",
				L"Выберите alt:", WS_CHILD | WS_VISIBLE,
				900, 70, 150, 20, hMainWnd, NULL, nullptr, NULL);

			combo_add_note_alter = CreateWindow(L"combobox", L"combo_add_note_alter", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | WS_VSCROLL,
				900, 100, 150, 200, hMainWnd, NULL, wc.hInstance, NULL);
			TCHAR a_alter[3][10] =
			{
				TEXT("_"), TEXT("b"), TEXT("#")
			};

			TCHAR AA[3];
			memset(&AA, 0, sizeof(AA));
			for (int k = 0; k < 3; k += 1)
			{
				wcscpy_s(AA, sizeof(AA) / sizeof(TCHAR), (TCHAR*)a_alter[k]);
				SendMessage(combo_add_note_alter, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)AA);
			}
			SendMessage(combo_add_note_alter, CB_SETCURSEL, (WPARAM)1, (LPARAM)0);
			//=========================================================================================
			CreateWindow(L"STATIC",
				L"Укажите dur:", WS_CHILD | WS_VISIBLE,
				900, 130, 150, 20, hMainWnd, NULL, nullptr, NULL);

			edit_add_note_dur = CreateWindow(L"EDIT",
				L"2", WS_CHILD | WS_VISIBLE | WS_BORDER,
				900, 160, 150, 20, hMainWnd, NULL, nullptr, NULL);

			CreateWindow(L"BUTTON", L"Добавить", BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP, 900, 190, 150, 24, hMainWnd, (HMENU)ID_ADD_NOTE_FRAGMENT, wc.hInstance, NULL);

			DestroyMenu(main_menu);
			UpdateWindow(hMainWnd);
		}
			break;

		case WM_VSCROLL:
			switch (LOWORD(wParam)) {
				case SB_TOP:
					nCurPos=nPosMin;
					break;
				case SB_LINEUP:
					nCurPos--;
					break;
				case SB_PAGEUP:
					nCurPos-=nPage;
					break;
				case SB_BOTTOM:
					nCurPos=nPosMax;
					break;
				case SB_LINEDOWN:
					nCurPos++;
					break;
				case SB_PAGEDOWN:
					nCurPos+=nPage;
					break;
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
					nCurPos=HIWORD(wParam);
					break;
			}
			if (nCurPos>=nPosMax) {
				nCurPos=nPosMax;
				EnableScrollBar(Scroll1, SB_CTL, ESB_DISABLE_DOWN);
			}
			if (nCurPos<=nPosMin) {
				nCurPos=nPosMin;
				EnableScrollBar(Scroll1, SB_CTL, ESB_DISABLE_UP);
			}

			SetScrollPos(Scroll1, SB_CTL, nCurPos, TRUE);

			break;

		case WM_PAINT:
			hDC = BeginPaint(hMainWnd, &ps);
			GetClientRect(hMainWnd, &rect);
			EndPaint(hMainWnd, &ps);	

			return 0;

		case WM_CLOSE:
			DestroyWindow(hMainWnd);
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_COMMAND:
			switch(wParam) {
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

					DestroyWindow(hGrBox);
					hGrBox = CreateWindow(L"Button", L"Н О Т Ы", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 20, 10, 6 * buttonWidth + 250, 400, hMainWnd, (HMENU)ID_FragmentBox, wc.hInstance, NULL);

					frag1.printFragment(hGrBox, wc.hInstance);
					frag1.setTonality(1, 4);
					UpdateWindow(hGrBox);
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

					hChildWindow1 = CreateWindow(
						szChildClassName,
						L"Выбор способа транспонирования",
						WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT,
						NULL,
						300,
						200,
						hMainWnd,
						NULL,
						HINSTANCE(wc.hInstance),
						NULL);

					ShowWindow(hChildWindow1, SW_NORMAL);
					UpdateWindow(hChildWindow1);
					UpdateWindow(hGrBox);
					UpdateWindow(hMainWnd);
				}			
				break;

				case ID_setBeatLines:
				{
					if (frag1.getLength() == 0) {
						MessageBox(NULL, L"Не выбран фрагмент!", L"Ошибка", MB_OK);
						return 0;
					}
					DestroyWindow(hGrBox);
					hGrBox = CreateWindow(L"Button", L"Н О Т Ы", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 20, 10, 6 * buttonWidth + 250, 400, hMainWnd, (HMENU)ID_FragmentBox, wc.hInstance, NULL);

					hStatusBar = DoCreateStatusBar(hMainWnd, 0, wc.hInstance, 1);
					SetWindowText(hStatusBar, L"Идет расстановка тактовых черт..");

					hwndThread1 = CreateThread(NULL, 0, &Thread1Proc, NULL, NULL, NULL);
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

					hChildWindow2 = CreateWindow(
						szChildClassName,
						L"Выбор номера такта",
						WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT,
						NULL,
						300,
						200,
						hMainWnd,
						NULL,
						HINSTANCE(wc.hInstance),
						NULL);

					ShowWindow(hChildWindow2, SW_NORMAL);
					UpdateWindow(hChildWindow2);
					UpdateWindow(hGrBox);
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
							alterswork = "The interval between " + alters + " and " + str[0] + " = " + to_string(abs(frag1.getInterval(reinterpret_cast<Note*>(lastClick), reinterpret_cast<Note*>(prelastClick))) / 2) + " tones.";
							wchar_t wtext[1024];
							mbstowcs(wtext, alterswork.c_str(), strlen(alterswork.c_str()) + 1);
							ptr = wtext;
							MessageBox(NULL, ptr, L"", MB_OK);
						}

					}
					DestroyWindow(hGrBox);
					hGrBox = CreateWindow(L"Button", L"Н О Т Ы", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 20, 10, 6 * buttonWidth + 250, 400, hMainWnd, (HMENU)ID_FragmentBox, wc.hInstance, NULL);

					frag1.printFragment(hGrBox, wc.hInstance);
					UpdateWindow(hGrBox);
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

							hChildWindow2 = CreateWindow(
								szChildClassName,
								L"Задайте интервал",
								WS_OVERLAPPEDWINDOW,
								CW_USEDEFAULT,
								NULL,
								300,
								200,
								hMainWnd,
								NULL,
								HINSTANCE(wc.hInstance),
								NULL);

							ShowWindow(hChildWindow2, SW_NORMAL);
							UpdateWindow(hChildWindow2);
							UpdateWindow(hGrBox);
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
					frag1.addNoteFragment(name_note[0], alter_note[0], 2, (INT)buff[0]);

					// Перерисовываем и выводим
					DestroyWindow(hGrBox);
					hGrBox = CreateWindow(L"Button", L"Н О Т Ы", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 20, 10, 6 * buttonWidth + 250, 400, hMainWnd, (HMENU)ID_FragmentBox, wc.hInstance, NULL);

					frag1.printFragment(hGrBox, wc.hInstance);
					UpdateWindow(hGrBox);
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

		case WM_KEYDOWN:
			switch(wParam) {
				case VK_ESCAPE:
					SendMessage(hMainWnd, WM_CLOSE, 0, 0);
					return 0;
			}

		case WM_LBUTTONDOWN:
			//Узнаем какой фрагмент был выбран.
			hDC=GetDC(hMainWnd);
			x=LOWORD(lParam) - 20; //узнаём координаты
			y=HIWORD(lParam) - 10;

			// drawPix(hMainWnd, x, y);

			//if(x < 0 || y < 0 || y > 400 || x >  6 * buttonWidth + 370) break;

			elemnt = frag1.findElement(x, y);
			if((elemnt != nullptr) && (elemnt->getType() == 1))
			{
				if(lastClick != nullptr)
					prelastClick = lastClick;
				
				lastClick = elemnt;
			}

		break;

		case WM_CTLCOLORSTATIC:
				switch (wParam)
				{
				case 5001:
					HDC hdcStatic = (HDC) wParam;
					SetBkColor(hdcStatic, TRANSPARENT);
					return (INT_PTR)GetStockObject(NULL_BRUSH);
				}

		
		default:
			return DefWindowProc(hMainWnd, msg, wParam, lParam);
	}
	return 0;
}

LONG WINAPI ChildWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lparam) 
{
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

	switch (Message) 
	{	
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case 102:
			{
				if (Button_GetCheck(upPrior) == BST_CHECKED)
				{
					frag1.transpose(-1);
					frag1.printFragment(hGrBox, wc.hInstance);
				}
				else
				{
					frag1.transpose(1);
					frag1.printFragment(hGrBox, wc.hInstance);
				}
				InvalidateRect(hWnd, NULL, true);
			}
				break;

			case 103:
			{
				DestroyWindow(hGrBox);
				hGrBox = CreateWindow(L"Button", L"Н О Т Ы", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 20, 10, 6 * buttonWidth + 250, 400, hMainWnd, (HMENU)ID_FragmentBox, wc.hInstance, NULL);
				GetWindowText(editblock, buf, 10);

				frag1.printTact(hGrBox, wc.hInstance, atoi((char*)buf));
				InvalidateRect(hWnd, NULL, true);
			}
				break;

			case 104:
			{
				if (nItem > 12)
				{
					nItem = -((nItem - 1) % 10);
				}

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

				DestroyWindow(hGrBox);
				hGrBox = CreateWindow(L"Button", L"Н О Т Ы", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 20, 10, 6 * buttonWidth + 250, 400, hMainWnd, (HMENU)ID_FragmentBox, wc.hInstance, NULL);

				frag1.printFragment(hGrBox, wc.hInstance);
				UpdateWindow(hGrBox);
				UpdateWindow(hMainWnd);
			}
				break;

			case ID_COMBO:
				if (HIWORD(wParam) == CBN_EDITCHANGE) {
					GetWindowTextW(Combo1, item, sizeof(item));
				}
				if(HIWORD(wParam)==CBN_SELCHANGE)
					{
						nItem = ComboBox_GetCurSel(Combo1);
					}
			}
			break;
		case WM_CREATE:
			switch(childwindNum)
			{
			case 1:
				CreateWindow(L"STATIC",
					L"Транспонировать вверх", WS_CHILD | WS_VISIBLE,
					10, 10, 200, 20, hWnd, (HMENU)150, nullptr, NULL);
				CreateWindowEx(WS_EX_TRANSPARENT, L"BUTTON", L"Вверх",
					WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
					220, 10, 10, 10,
					hWnd, (HMENU)151,
					GetModuleHandle(NULL), 0);
				CreateWindow(L"STATIC",
					L"Транспонировать вниз", WS_CHILD | WS_VISIBLE,
					10, 40, 200, 20, hWnd, (HMENU)152, nullptr, NULL);
				upPrior = CreateWindowEx(WS_EX_TRANSPARENT, L"BUTTON", L"Вниз",
					WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
					220, 40, 10, 10,
					hWnd, (HMENU)153,
					GetModuleHandle(NULL), 0);
				Button_SetCheck(upPrior, 1);

				Apply = CreateWindow(L"BUTTON", L"Применить", BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP, 95, 80, 110, 20, hWnd, (HMENU)102, wc.hInstance, NULL);
				break;
			case 2:
				CreateWindow(L"STATIC",
				L"Введите номер такта", WS_CHILD | WS_VISIBLE,
				10, 10, 200, 20, hWnd, (HMENU)154, nullptr, NULL);

				editblock = CreateWindow(L"EDIT",
					L"1", WS_CHILD | WS_VISIBLE | WS_BORDER,
				220, 10, 30, 20, hWnd, (HMENU)155, nullptr, NULL);

			Apply = CreateWindow(L"BUTTON", L"Применить", BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP, 95, 80, 110, 20, hWnd, (HMENU)103, wc.hInstance, NULL);
				break;

			case 3:
				CreateWindow(L"STATIC",
				L"Выберите интервал из списка:", WS_CHILD | WS_VISIBLE,
				10, 10, 250, 20, hWnd, (HMENU)154, nullptr, NULL);

				Combo1 = CreateWindow(L"combobox", L"combo", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | WS_VSCROLL, 10, 40, 100, 200, hWnd, (HMENU)ID_COMBO, wc.hInstance, NULL);
				nItem = 0;
							// load the combobox with item list.  
			// Send a CB_ADDSTRING message to load each item

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

			memset(&A,0,sizeof(A)); 

			for (k = 0; k <= 22; k += 1)
			{
				wcscpy_s(A, sizeof(A)/sizeof(TCHAR),  (TCHAR*)Intervals[k]);

				// Add string to combobox.
				SendMessage(Combo1,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) A); 
			}
  
			// Send the CB_SETCURSEL message to display an initial item 
			//  in the selection field  
			SendMessage(Combo1, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

			SendMessage(Combo1, CB_SETEXTENDEDUI,1, 0);

			Apply = CreateWindow(L"BUTTON", L"Применить", BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP, 95, 80, 110, 20, hWnd, (HMENU)104, wc.hInstance, NULL);
			
				break;

			}
		case WM_MOVE: case WM_DESTROY:
			InvalidateRect(hWnd,NULL,true);
			break;
		default:
			return DefWindowProc(hWnd, Message, wParam, lparam);
	}
	return 0;
}