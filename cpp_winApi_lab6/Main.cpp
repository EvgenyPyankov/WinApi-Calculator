#include <windows.h>
#include <tchar.h>
#include <math.h>
#include "resource.h"


static TCHAR szWindowClass[] = _T("win32app");
static TCHAR szTitle[] = _T("Calculator");
HINSTANCE hInst;

struct Op {         //operand
	int nom;        //numerator
	int denom;      //denominator
	int sign;
};

const int WIDTH = 320;
const int HEIGHT = 510;
const int BTN_WIDTH = 70;
const int BTN_HEIGHT = 40;
const int EDIT_HEIGHT = 50;
const int MARGIN = 10;
const int KEYBOARD_MARGIN_HEIGHT = 100;
const int MAX_ENTER_LENGTH = 3;                //max number of digits you can enter
const int MAX_LENGTH = MAX_ENTER_LENGTH*2;     // max number of digits in numerator/donominator

HWND hWnd;
HWND hEdit;
HWND hMEdit, hErrorEdit;
HWND hBtns[10];
HWND hMc, hMs, hMr, hMplus;
HWND hCe, hC, hBs;
HWND hSqr, hRev, hDiv, hMult, hSub, hAdd, hEqual, hDivE, hMultE, hSubE, hAddE, hSign;

Op op1;
Op curOp;
int curOperator; //current operator
			/*
			1:/
			2:*
			3:-
			4:+
			*/
WCHAR curOpNomText[MAX_LENGTH+1], curOpDenomText[MAX_LENGTH + 1];
WCHAR editText[MAX_LENGTH*2];

int state;    // 1 - operand1, 2 - operator, 3 - operand2, 4 - =
bool curOpState;   // 0 - numerator, 1 - denominator

Op memOp;
bool memIsEmpty = true;

bool showDenom;
bool isError;

void SetStr(WCHAR *str, WCHAR *s) {
	wsprintf(str, TEXT("%s"), s);
}

void SetStr(WCHAR *str, int i) {
	wsprintf(str, TEXT("%d"), i);
}

void AddStr(WCHAR *str, WCHAR *c) {
	wsprintf(str, TEXT("%s%s"), str, c);
}

void AddDigitToStr(int digit) {
	if (!curOpState) {
		int i = wcslen(curOpNomText);
		if (i == 1 && curOpNomText[0]=='0')
			SetStr(curOpNomText, L"");
		wsprintf(curOpNomText, TEXT("%s%d"), curOpNomText, digit);
	}
	else 
		wsprintf(curOpDenomText, TEXT("%s%d"), curOpDenomText, digit);
}

void ChangeErrorStatus() {
	if (isError) {
		isError = false;
		SetWindowText(hErrorEdit, L"");
	}
	else {
		isError = true;
		SetWindowText(hErrorEdit, L"E");
	}
	UpdateWindow(hWnd);
}

void SetEditText() {
	if (curOp.sign == 1)
		SetStr(editText, L"");
	else
		SetStr(editText, L"-");

	if (showDenom) {
		if (!curOpState && wcslen(curOpDenomText) == 0) {
			AddStr(editText, curOpNomText);
		}
		else {
			wsprintf(editText, TEXT("%s%s/%s"), editText, curOpNomText, curOpDenomText);
		}
	}
	else {
		if (((state == 1 || state == 3) && (curOpState)) || ((state == 2 || state == 4) && (wcslen(curOpDenomText) > 0) && (curOpDenomText[0] != '1')))
			wsprintf(editText, TEXT("%s%s/%s"), editText, curOpNomText, curOpDenomText);
		else
			AddStr(editText, curOpNomText);
	}
	SetWindowText(hEdit, editText);
	UpdateWindow(hWnd);
}

bool canAddDigit(int digit) {
	if (!curOpState) {
		int i = wcslen(curOpNomText);
		if (i < MAX_ENTER_LENGTH)
			return true;
		return false;
	}
	else {
		int i = wcslen(curOpDenomText);
		if (i < MAX_ENTER_LENGTH && (i>0 || digit!=0))
			return true;
		return false;
	}
}


void ClearOp(Op &op) {
	op.nom = 0;
	op.denom = 1;
	op.sign = 1;
}

void ClearCurOp() {
	ClearOp(curOp);
	SetStr(curOpNomText, L"0");
	SetStr(curOpDenomText, L"");
	curOpState = false;
	SetEditText();
	if (isError) ChangeErrorStatus();
}

void Clear() {
	ClearCurOp();
	ClearOp(op1);
	state = 1;
}

void Init() {
	Clear();
	memIsEmpty = true;
	showDenom = true;
	isError = false;
}

void AddDigit(int digit) {
	if (isError) ChangeErrorStatus();
	switch (state)
	{
	case 1:
		if (canAddDigit(digit))
			AddDigitToStr(digit);
		break;
	case 2:
		if (canAddDigit(digit))
			AddDigitToStr(digit);
		state = 3;
		break;
	case 3:
		if (canAddDigit(digit))
			AddDigitToStr(digit);
		break;
	case 4:
		state = 1;
		Clear();
		AddDigit(digit);
		break;
	default:
		break;
	}
	SetEditText();
}

void Copy(Op &source, Op &target) {
	target.nom = source.nom;
	target.denom = source.denom;
	target.sign = source.sign;
}

int GCD(int a, int b) {     // greatest common divisor
	while (b != 0) {
		int tmp = a%b;
		a = b;
		b = tmp;
	}
	return a;
}

void TryToCancel(Op &op) {                
	int i = GCD(op.nom, op.denom);
	op.nom /= i;
	op.denom /= i;
}

int ICM(int a, int b) {               // least common multiple
	return (a*b) / GCD(a, b);
}

bool Div(Op& op1, Op& op2) {
	if (op2.nom == 0) return false;
	op1.nom *= op2.denom;
	op1.denom *= op2.nom;
	op1.sign *= op2.sign;
	return true;
}

bool Mult(Op& op1, Op& op2) {
	op1.nom *= op2.nom;
	op1.denom *= op2.denom;
	op1.sign *= op2.sign;
	return true;
}

bool Add(Op& op1, Op& op2) {
	int i = ICM(op1.denom, op2.denom);
	int buf1 = i / op1.denom;
	int buf2 = i / op2.denom;
	op1.nom = op1.nom*buf1*op1.sign + op2.nom*buf2*op2.sign;
	if (op1.nom >= 0) 
		op1.sign = 1;
	else  {
		op1.sign = -1;
		op1.nom = abs(op1.nom);
	}
	op1.denom = i;
	return true;
}

bool Sub(Op& op1, Op& op2) {
	op2.sign *= -1;
	Add(op1, op2);
	return true;
}

void SetCurOp() {
	curOp.nom = _ttoi(curOpNomText);
	int i = _ttoi(curOpDenomText);
	if (i == 0)
		curOp.denom = 1;
	else curOp.denom = i;
}

void SetTextCurOp() {
	SetStr(curOpNomText, curOp.nom);
	SetStr(curOpDenomText, curOp.denom);
}

void Equal() {
	if (state!=1) {
		state = 4;
		SetCurOp();
		bool check;
		switch (curOperator)
		{
		case 1:
			check = Div(op1, curOp);
			break;
		case 2:
			check = Mult(op1, curOp);
			break;
		case 3:
			check =  Sub(op1, curOp);
			break;
		case 4:
			check = Add(op1, curOp);
			break;
		default:
			break;
		}
		if (check) {
			TryToCancel(op1);
			Copy(op1, curOp);
			SetTextCurOp();
			SetEditText();
		}
		else { 
			Clear();
			ChangeErrorStatus();
		}
	}
}

void SetOperator(int i) {
	switch (state)
	{
	case 1:
		state = 2;
		SetCurOp();
		Copy(curOp,op1);
		ClearCurOp();
		curOperator = i;
		break;
	case 2:
		curOperator = i;
		break;
	case 3:
		Equal();
		state = 1;
		SetOperator(i);
		break;
	case 4:
		state = 1;
		SetOperator(i);
		break;
	default:
		break;
	}
}



void SetComponents(HWND hWnd) {
	hEdit = CreateWindow(L"edit", L"", WS_CHILD | ES_READONLY |WS_VISIBLE | ES_RIGHT, MARGIN + 30, KEYBOARD_MARGIN_HEIGHT - MARGIN - EDIT_HEIGHT, WIDTH - 20 - 5 * MARGIN, EDIT_HEIGHT, hWnd, 0, hInst, NULL);
	HFONT hFont = CreateFont(50, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
	SendMessage(hEdit, WM_SETFONT, WPARAM(hFont), TRUE);

	hMEdit = CreateWindow(L"edit", L"", WS_CHILD | ES_READONLY | WS_VISIBLE, MARGIN, KEYBOARD_MARGIN_HEIGHT - MARGIN - EDIT_HEIGHT, 20, 20, hWnd, 0, hInst, NULL);
	hErrorEdit = CreateWindow(L"edit", L"", WS_CHILD | ES_READONLY | WS_VISIBLE, MARGIN, KEYBOARD_MARGIN_HEIGHT - MARGIN - EDIT_HEIGHT+20, 20, 20, hWnd, 0, hInst, NULL);

	//memory keys

	hMc = CreateWindow(L"button", L"MC", WS_CHILD | WS_VISIBLE, MARGIN, MARGIN + KEYBOARD_MARGIN_HEIGHT, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);
	hMs = CreateWindow(L"button", L"MS", WS_CHILD | WS_VISIBLE, MARGIN + BTN_WIDTH, MARGIN + KEYBOARD_MARGIN_HEIGHT, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);
	hMr = CreateWindow(L"button", L"MR", WS_CHILD | WS_VISIBLE, MARGIN + BTN_WIDTH * 2, MARGIN + KEYBOARD_MARGIN_HEIGHT, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);
	hMplus = CreateWindow(L"button", L"M+", WS_CHILD | WS_VISIBLE, MARGIN + BTN_WIDTH * 3, MARGIN + KEYBOARD_MARGIN_HEIGHT, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);

	//operators keys

	hDiv = CreateWindow(L"button", L"/", WS_CHILD | WS_VISIBLE, MARGIN + BTN_WIDTH * 3, MARGIN * 2 + KEYBOARD_MARGIN_HEIGHT + BTN_HEIGHT, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);
	hMult = CreateWindow(L"button", L"*", WS_CHILD | WS_VISIBLE, MARGIN + BTN_WIDTH * 3, MARGIN * 2 + KEYBOARD_MARGIN_HEIGHT + BTN_HEIGHT * 2, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);
	hSub = CreateWindow(L"button", L"-", WS_CHILD | WS_VISIBLE, MARGIN + BTN_WIDTH * 3, MARGIN * 2 + KEYBOARD_MARGIN_HEIGHT + BTN_HEIGHT * 3, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);
	hAdd = CreateWindow(L"button", L"+", WS_CHILD | WS_VISIBLE, MARGIN + BTN_WIDTH * 3, MARGIN * 2 + KEYBOARD_MARGIN_HEIGHT + BTN_HEIGHT * 4, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);
	hEqual = CreateWindow(L"button", L"=", WS_CHILD | WS_VISIBLE, MARGIN + BTN_WIDTH * 3, MARGIN * 2 + KEYBOARD_MARGIN_HEIGHT + BTN_HEIGHT * 5, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);
	hSqr = CreateWindow(L"button", L"SQR", WS_CHILD | WS_VISIBLE, MARGIN, MARGIN * 2 + KEYBOARD_MARGIN_HEIGHT + BTN_HEIGHT * 5, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);
	hRev = CreateWindow(L"button", L"REV", WS_CHILD | WS_VISIBLE, MARGIN + BTN_WIDTH, MARGIN * 2 + KEYBOARD_MARGIN_HEIGHT + BTN_HEIGHT * 5, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);
	hDivE = CreateWindow(L"button", L"/=", WS_CHILD | WS_VISIBLE, MARGIN , MARGIN * 2 + KEYBOARD_MARGIN_HEIGHT + BTN_HEIGHT * 6, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);
	hMultE = CreateWindow(L"button", L"*=", WS_CHILD | WS_VISIBLE, MARGIN+BTN_WIDTH, MARGIN * 2 + KEYBOARD_MARGIN_HEIGHT + BTN_HEIGHT * 6, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);
	hSubE = CreateWindow(L"button", L"-=", WS_CHILD | WS_VISIBLE, MARGIN + BTN_WIDTH*2, MARGIN * 2 + KEYBOARD_MARGIN_HEIGHT + BTN_HEIGHT * 6, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);
	hAddE = CreateWindow(L"button", L"+=", WS_CHILD | WS_VISIBLE, MARGIN + BTN_WIDTH*3, MARGIN * 2 + KEYBOARD_MARGIN_HEIGHT + BTN_HEIGHT * 6, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);
	hSign = CreateWindow(L"button", L"- / +", WS_CHILD | WS_VISIBLE, MARGIN , MARGIN * 2 + KEYBOARD_MARGIN_HEIGHT + BTN_HEIGHT * 7, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);

	//delete keys

	hCe = CreateWindow(L"button", L"CE", WS_CHILD | WS_VISIBLE, MARGIN, MARGIN * 2 + KEYBOARD_MARGIN_HEIGHT + BTN_HEIGHT, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);
	hC = CreateWindow(L"button", L"C", WS_CHILD | WS_VISIBLE, MARGIN + BTN_WIDTH, MARGIN * 2 + KEYBOARD_MARGIN_HEIGHT + BTN_HEIGHT, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);
	hBs = CreateWindow(L"button", L"BS", WS_CHILD | WS_VISIBLE, MARGIN + BTN_WIDTH * 2, MARGIN * 2 + KEYBOARD_MARGIN_HEIGHT + BTN_HEIGHT, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);

	//number keys

	int counter = 1;
	WCHAR buffer[10];
	for (int i = 3; i >0; i--)
		for (int j = 0; j < 3; j++) {
			wsprintf(buffer, TEXT("%d"), counter);
			hBtns[counter] = CreateWindow(L"button", buffer, WS_CHILD | WS_VISIBLE, MARGIN + j*BTN_WIDTH, MARGIN * 2 + KEYBOARD_MARGIN_HEIGHT + i*BTN_HEIGHT + BTN_HEIGHT, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);
			counter++;
		}
	hBtns[0] = CreateWindow(L"button", L"0", WS_CHILD | WS_VISIBLE, MARGIN + 2 * BTN_WIDTH, MARGIN * 2 + KEYBOARD_MARGIN_HEIGHT + 5 * BTN_HEIGHT, BTN_WIDTH, BTN_HEIGHT, hWnd, 0, hInst, NULL);

}


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL,
			_T("Call to RegisterClassEx failed!"),
			_T("Win32"),
			NULL);

		return 1;
	}

	hInst = hInstance;
    hWnd = CreateWindow(
		szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		WIDTH, HEIGHT,
		NULL,
		NULL,
		hInstance,
		NULL
		);

	if (!hWnd)
	{
		MessageBox(NULL,
			_T("Call to CreateWindow failed!"),
			_T("Failure!"),
			NULL);
		return 1;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

// dialog window "about" callback function

BOOL CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	case WM_CLOSE:
		EndDialog(hWnd, 0);
		return FALSE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hWnd, 0);
			return FALSE;
		}
	default:
		break;
	}
	
	return FALSE;
}



LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hDc;
	PAINTSTRUCT ps;
	HMENU hMenu = GetMenu(hWnd);
	WORD code;

	switch (message)
	{
	case WM_GETMINMAXINFO:
	{
		//disable resize

		MINMAXINFO *pInfo = (MINMAXINFO *)lParam;
		POINT Min = { WIDTH,HEIGHT };
		POINT Max = { WIDTH,HEIGHT };
		pInfo->ptMinTrackSize = Min;
		pInfo->ptMaxTrackSize = Max;
		return 0;
	}
	break;
	case WM_CREATE: {
		SetComponents(hWnd);
		Init();
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN: {
		code = LOWORD(wParam);
		if (code == 0x30 || code == 0x60)
			AddDigit(0);
		if (code == 0x31 || code == 0x61)
			AddDigit(1);
		if (code == 0x32 || code == 0x62)
			AddDigit(2);
		if (code == 0x33 || code == 0x63)
			AddDigit(3);
		if (code == 0x34 || code == 0x64)
			AddDigit(4);
		if (code == 0x35 || code == 0x65)
			AddDigit(5);
		if (code == 0x36 || code == 0x66)
			AddDigit(6);
		if (code == 0x37 || code == 0x67)
			AddDigit(7);
		if (code == 0x38 || code == 0x68)
			AddDigit(8);
		if (code == 0x39 || code == 0x69)
			AddDigit(9);
		if (code == VK_DIVIDE) {
			if ((state == 1 || state == 3) && (!curOpState)) {
				curOpState = true;
				SetEditText();
			}
			else SetOperator(1);
		}
		if (code == VK_MULTIPLY)
			SetOperator(2);
		if (code == VK_SUBTRACT)
			SetOperator(3);
		if (code == VK_ADD)
			SetOperator(4);
		if (code == VK_RETURN)
			Equal();
		if (code == VK_BACK)
		{
			if (!curOpState) {
				int i = wcslen(curOpNomText);
				if (i > 0) curOpNomText[i - 1] = 0;
				if (i == 1) curOpNomText[0] = '0';
				SetEditText();
			}
			else {
				int i = wcslen(curOpDenomText);
				if (i > 0) curOpDenomText[i - 1] = 0;
				if (i == 1) curOpNomText[0] = '0';
				SetEditText();
			}
		}
		if (code == VK_DELETE)
			Clear();
	}
	 break;
	case WM_COMMAND: {

		//menu items clicks

		switch (LOWORD(wParam))
		{
		case ID_FILE_EXIT:
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		case ID_OPTIONS_DON:
			if (showDenom) {
				showDenom = false;
				CheckMenuItem(GetSubMenu(hMenu, 1), ID_OPTIONS_DON, MF_UNCHECKED);
			}
			else {
				showDenom = true;
				CheckMenuItem(GetSubMenu(hMenu, 1), ID_OPTIONS_DON, MF_CHECKED);
			}
			break;
		case ID_INFO_ABOUT:
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG1), 0, (DlgProc), 0);
			break;
		}
		//number button clicks

		if (lParam == (LPARAM)hBtns[0]) {
			AddDigit(0);
		}
		if (lParam == (LPARAM)hBtns[1]) {
			AddDigit(1);
		}
		if (lParam == (LPARAM)hBtns[2]) {
			AddDigit(2);
		}
		if (lParam == (LPARAM)hBtns[3]) {
			AddDigit(3);
		}
		if (lParam == (LPARAM)hBtns[4]) {
			AddDigit(4);
		}
		if (lParam == (LPARAM)hBtns[5]) {
			AddDigit(5);
		}
		if (lParam == (LPARAM)hBtns[6]) {
			AddDigit(6);
		}
		if (lParam == (LPARAM)hBtns[7]) {
			AddDigit(7);
		}
		if (lParam == (LPARAM)hBtns[8]) {
			AddDigit(8);
		}
		if (lParam == (LPARAM)hBtns[9]) {
			AddDigit(9);
		}

		//operators buttons clicks

		if (lParam == (LPARAM)hSign) {
			if (_ttoi(curOpNomText) != 0)
				curOp.sign *= -1;
			SetEditText();
		}

		if (lParam == (LPARAM)hDiv) {
			if ((state == 1 || state == 3) && (!curOpState)) {
				curOpState = true;
				SetEditText();
			}
			else SetOperator(1);
		}

		if (lParam == (LPARAM)hMult) {
			SetOperator(2);
		}
		if (lParam == (LPARAM)hSub) {
			SetOperator(3);
		}
		if (lParam == (LPARAM)hAdd) {
			SetOperator(4);
		}
		if (lParam == (LPARAM)hEqual) {
			Equal();
		}
		if (lParam == (LPARAM)hSqr) {
			SetCurOp();
			curOp.nom *= curOp.nom;
			curOp.denom *= curOp.denom;
			curOp.sign *= curOp.sign;
			SetTextCurOp();
			SetEditText();
		}
		if (lParam == (LPARAM)hRev) {
			if (wcslen(curOpDenomText) == 0) SetStr(curOpDenomText, L"1");
			WCHAR buf[MAX_LENGTH * 2];
			SetStr(buf, curOpNomText);
			SetStr(curOpNomText, curOpDenomText);
			SetStr(curOpDenomText, buf);
			SetEditText();
		}
		if (lParam == (LPARAM)hDivE) {
			if (curOpNomText[0] != '0') {
				SetStr(curOpNomText, L"1");
				SetStr(curOpDenomText, L"1");
				SetEditText();
			}
			else ChangeErrorStatus();
		}

		if (lParam == (LPARAM)hMultE) {
			SetCurOp();
			Mult(curOp, curOp);
			TryToCancel(curOp);
			SetTextCurOp();
			SetEditText();
		}
		if (lParam == (LPARAM)hSubE) {
			SetStr(curOpNomText, L"0");
			SetStr(curOpDenomText, L"1");
			SetEditText();
		}
		if (lParam == (LPARAM)hAddE) {
			SetCurOp();
			Add(curOp, curOp);
			TryToCancel(curOp);
			SetTextCurOp();
			SetEditText();
		}

		//delete buttons clicks

		if (lParam == (LPARAM)hCe) {
			ClearCurOp();
		}
		if (lParam == (LPARAM)hC) {
			Clear();
		}
		if (lParam == (LPARAM)hBs) {
			if (!curOpState) {
				int i = wcslen(curOpNomText);
				if (i > 0) curOpNomText[i - 1] = 0;
				if (i == 1) curOpNomText[0] = '0';
				SetEditText();
			}
			else {
				int i = wcslen(curOpDenomText);
				if (i > 0) curOpDenomText[i - 1] = 0;
				if (i == 1) curOpNomText[0] = '0';
				SetEditText();
			}

		}

		//mem buttons clicks

		if (lParam == (LPARAM)hMc) {
			memIsEmpty = true;
			ClearOp(memOp);
			SetWindowText(hMEdit, L"");
			UpdateWindow(hWnd);
		}
		if (lParam == (LPARAM)hMs) {
			memIsEmpty = false;
			SetCurOp();
			Copy(curOp, memOp);
			SetWindowText(hMEdit, L"M");
			UpdateWindow(hWnd);
		}
		if (lParam == (LPARAM)hMr) {
			if (!memIsEmpty) {
				Copy(memOp, curOp);
				SetTextCurOp();
				SetEditText();
			}
		}
		if (lParam == (LPARAM)hMplus) {
			if (!memIsEmpty) {
				SetCurOp();
				Add(memOp, curOp);
				TryToCancel(memOp);
			}
		}
	}
	break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;

	}
	SetFocus(hWnd);
	return 0;
}









