#include <windows.h>
#include <string>
#include <vector>
#include <chrono>
#include <random>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

constexpr int ID_TOP_EDIT = 100;
constexpr int ID_BOTTOM_EDIT = 101;
constexpr int ID_BTN_SHOWANSWER = 200;
constexpr int ID_BTN_GOOD = 201;
constexpr int ID_BTN_MEH = 202;
constexpr int ID_BTN_BAD = 203;

constexpr int HOTKEY_ID_SHOW_SPACE = 300;
constexpr int HOTKEY_ID_SHOW_ENTER = 301;
constexpr int HOTKEY_ID_RATE_BAD = 302;
constexpr int HOTKEY_ID_RATE_MEH = 303;
constexpr int HOTKEY_ID_RATE_GOOD = 304;
constexpr int HOTKEY_ID_EXIT = 305;

struct Card {
    int lhs;
    int rhs;
    std::wstring question;
    std::wstring answer;
};

enum class Rating { Bad, Meh, Good };

struct AppState {
    std::vector<Card> cards;
    size_t currentCardIndex = 0;
    bool answerVisible = false;
    HWND hTopEdit = nullptr;
    HWND hBottomEdit = nullptr;
    HWND hBtnShowAnswer = nullptr;
    HWND hBtnGood = nullptr;
    HWND hBtnMeh = nullptr;
    HWND hBtnBad = nullptr;
    HFONT hFont = nullptr;
};

AppState g_state;

std::wstring BuildQuestionText(int lhs, int rhs) {
    return std::to_wstring(lhs) + L" \u00D7 " + std::to_wstring(rhs);
}

std::wstring BuildAnswerText(int lhs, int rhs) {
    return std::to_wstring(lhs * rhs);
}

char RatingToChar(Rating rating) {
    switch (rating) {
    case Rating::Bad: return 'B';
    case Rating::Meh: return 'M';
    case Rating::Good: return 'G';
    }
    return '?';
}

std::string FormatTimestamp(std::chrono::system_clock::time_point tp) {
    std::time_t time = std::chrono::system_clock::to_time_t(tp);
    std::tm localTime{};
    localtime_s(&localTime, &time);
    std::ostringstream ss;
    ss << std::put_time(&localTime, "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

void AppendRatingToLog(size_t cardIndex, Rating rating) {
    std::ofstream logFile("ratings.log", std::ios::app);
    if (!logFile.is_open()) {
        return;
    }
    auto now = std::chrono::system_clock::now();
    logFile << FormatTimestamp(now) << '|' << cardIndex << '|' << RatingToChar(rating) << "\n";
}

Card& CurrentCard() {
    return g_state.cards[g_state.currentCardIndex % g_state.cards.size()];
}

void EnableRatingButtons(BOOL enabled) {
    EnableWindow(g_state.hBtnGood, enabled);
    EnableWindow(g_state.hBtnMeh, enabled);
    EnableWindow(g_state.hBtnBad, enabled);
}

void ShowCurrentQuestion() {
    if (g_state.cards.empty()) {
        return;
    }

    Card& card = CurrentCard();
    SetWindowTextW(g_state.hTopEdit, card.question.c_str());
    SetWindowTextW(g_state.hBottomEdit, L"");
    EnableWindow(g_state.hBottomEdit, FALSE);
    g_state.answerVisible = false;
    EnableRatingButtons(FALSE);
}

void RevealAnswer() {
    if (g_state.answerVisible || g_state.cards.empty()) {
        return;
    }

    Card& card = CurrentCard();
    SetWindowTextW(g_state.hBottomEdit, card.answer.c_str());
    EnableWindow(g_state.hBottomEdit, TRUE);
    g_state.answerVisible = true;
    EnableRatingButtons(TRUE);
    SetFocus(g_state.hBtnGood);
}

void AdvanceToNextCard() {
    if (g_state.cards.empty()) {
        return;
    }
    ++g_state.currentCardIndex;
    if (g_state.currentCardIndex >= g_state.cards.size()) {
        MessageBoxW(nullptr, L"Reached end of deck. Restarting from the beginning.", L"Deck", MB_OK | MB_ICONINFORMATION);
        g_state.currentCardIndex = 0;
    }
    ShowCurrentQuestion();
}

void RateCurrentCard(Rating rating) {
    if (!g_state.answerVisible) {
        return;
    }
    AppendRatingToLog(g_state.currentCardIndex, rating);
    AdvanceToNextCard();
}

void InitializeCards() {
    g_state.cards.clear();
    g_state.cards.reserve(13 * 13);

    for (int lhs = 0; lhs <= 12; ++lhs) {
        for (int rhs = 0; rhs <= 12; ++rhs) {
            Card card{};
            card.lhs = lhs;
            card.rhs = rhs;
            card.question = BuildQuestionText(lhs, rhs);
            card.answer = BuildAnswerText(lhs, rhs);
            g_state.cards.push_back(card);
        }
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(g_state.cards.begin(), g_state.cards.end(), gen);
    g_state.currentCardIndex = 0;
    g_state.answerVisible = false;
}

void ApplyFont(HWND hwnd) {
    if (g_state.hFont) {
        SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(g_state.hFont), TRUE);
    }
}

void LayoutControls(HWND hwnd, int clientWidth, int clientHeight) {
    const int margin = 10;
    const int buttonBarHeight = 44;
    const int revealHeight = 40;

    if (clientWidth <= 0 || clientHeight <= 0) {
        return;
    }

    int availableHeight = clientHeight - buttonBarHeight - revealHeight - (4 * margin);
    if (availableHeight < 0) {
        availableHeight = 0;
    }

    int questionHeight = static_cast<int>(availableHeight * 0.3);
    int answerHeight = static_cast<int>(availableHeight * 0.3);

    int y = margin;
    MoveWindow(g_state.hTopEdit, margin, y, clientWidth - (2 * margin), questionHeight, TRUE);
    y += questionHeight + margin;

    MoveWindow(g_state.hBtnShowAnswer, margin, y, clientWidth - (2 * margin), revealHeight, TRUE);
    y += revealHeight + margin;

    int remainingForAnswer = clientHeight - buttonBarHeight - margin - y;
    if (remainingForAnswer < answerHeight) {
        remainingForAnswer = answerHeight;
    }
    MoveWindow(g_state.hBottomEdit, margin, y, clientWidth - (2 * margin), remainingForAnswer, TRUE);

    int buttonY = clientHeight - buttonBarHeight + (buttonBarHeight - 32) / 2;
    int buttonWidth = (clientWidth - (4 * margin)) / 3;
    if (buttonWidth < 60) {
        buttonWidth = 60;
    }

    MoveWindow(g_state.hBtnGood, margin, buttonY, buttonWidth, 32, TRUE);
    MoveWindow(g_state.hBtnMeh, margin + buttonWidth + margin, buttonY, buttonWidth, 32, TRUE);
    MoveWindow(g_state.hBtnBad, margin + 2 * (buttonWidth + margin), buttonY, buttonWidth, 32, TRUE);
}

void RegisterHotkeys(HWND hwnd) {
    RegisterHotKey(hwnd, HOTKEY_ID_SHOW_SPACE, MOD_NOREPEAT, VK_SPACE);
    RegisterHotKey(hwnd, HOTKEY_ID_SHOW_ENTER, MOD_NOREPEAT, VK_RETURN);
    RegisterHotKey(hwnd, HOTKEY_ID_RATE_BAD, MOD_NOREPEAT, '1');
    RegisterHotKey(hwnd, HOTKEY_ID_RATE_MEH, MOD_NOREPEAT, '2');
    RegisterHotKey(hwnd, HOTKEY_ID_RATE_GOOD, MOD_NOREPEAT, '3');
    RegisterHotKey(hwnd, HOTKEY_ID_EXIT, MOD_NOREPEAT, VK_ESCAPE);
}

void UnregisterHotkeys(HWND hwnd) {
    UnregisterHotKey(hwnd, HOTKEY_ID_SHOW_SPACE);
    UnregisterHotKey(hwnd, HOTKEY_ID_SHOW_ENTER);
    UnregisterHotKey(hwnd, HOTKEY_ID_RATE_BAD);
    UnregisterHotKey(hwnd, HOTKEY_ID_RATE_MEH);
    UnregisterHotKey(hwnd, HOTKEY_ID_RATE_GOOD);
    UnregisterHotKey(hwnd, HOTKEY_ID_EXIT);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_state.hTopEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(ID_TOP_EDIT), nullptr, nullptr);

        g_state.hBtnShowAnswer = CreateWindowExW(0, L"BUTTON", L"Show Answer",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(ID_BTN_SHOWANSWER), nullptr, nullptr);

        g_state.hBottomEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(ID_BOTTOM_EDIT), nullptr, nullptr);

        g_state.hBtnGood = CreateWindowExW(0, L"BUTTON", L"Good",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(ID_BTN_GOOD), nullptr, nullptr);
        g_state.hBtnMeh = CreateWindowExW(0, L"BUTTON", L"Meh",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(ID_BTN_MEH), nullptr, nullptr);
        g_state.hBtnBad = CreateWindowExW(0, L"BUTTON", L"Bad",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(ID_BTN_BAD), nullptr, nullptr);

        HDC hdc = GetDC(hwnd);
        int fontSize = -MulDiv(16, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        g_state.hFont = CreateFontW(fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        ReleaseDC(hwnd, hdc);

        ApplyFont(g_state.hTopEdit);
        ApplyFont(g_state.hBottomEdit);
        ApplyFont(g_state.hBtnShowAnswer);
        ApplyFont(g_state.hBtnGood);
        ApplyFont(g_state.hBtnMeh);
        ApplyFont(g_state.hBtnBad);

        InitializeCards();
        ShowCurrentQuestion();
        RegisterHotkeys(hwnd);
        break;
    }
    case WM_SIZE: {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        LayoutControls(hwnd, width, height);
        break;
    }
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize.x = 640;
        info->ptMinTrackSize.y = 480;
        break;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case ID_BTN_SHOWANSWER:
            RevealAnswer();
            break;
        case ID_BTN_GOOD:
            RateCurrentCard(Rating::Good);
            break;
        case ID_BTN_MEH:
            RateCurrentCard(Rating::Meh);
            break;
        case ID_BTN_BAD:
            RateCurrentCard(Rating::Bad);
            break;
        }
        break;
    }
    case WM_HOTKEY: {
        switch (wParam) {
        case HOTKEY_ID_SHOW_SPACE:
        case HOTKEY_ID_SHOW_ENTER:
            RevealAnswer();
            break;
        case HOTKEY_ID_RATE_BAD:
            RateCurrentCard(Rating::Bad);
            break;
        case HOTKEY_ID_RATE_MEH:
            RateCurrentCard(Rating::Meh);
            break;
        case HOTKEY_ID_RATE_GOOD:
            RateCurrentCard(Rating::Good);
            break;
        case HOTKEY_ID_EXIT:
            PostQuitMessage(0);
            break;
        }
        break;
    }
    case WM_DESTROY:
        UnregisterHotkeys(hwnd);
        if (g_state.hFont) {
            DeleteObject(g_state.hFont);
            g_state.hFont = nullptr;
        }
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    SetProcessDPIAware();

    const wchar_t CLASS_NAME[] = L"QATrainerMainWindow";

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = wc.hIcon;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClassExW(&wc)) {
        return 0;
    }

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Multiplication Facts Trainer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 700,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!hwnd) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}

