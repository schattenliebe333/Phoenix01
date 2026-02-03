// RAEL GUI Cockpit (Windows 11) - code only (WinAPI)
//
// Design goals:
// - Start/stop RAEL core (rael.exe) as a child process.
// - Communicate via STDIN/STDOUT pipes (text protocol).
// - "Browser"-style tabs for metrics pages.
// - Always-visible right-side communication pane (Rael dialog).
// - Default READ-ONLY: GUI only sends safe read commands (node/lane/events/status).
//
// Build (Visual Studio x64 Native Tools Command Prompt):
//   cd build && Build.bat
// Produces: bin\rael.exe (core) and bin\rael_gui.exe (this cockpit).

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <cwchar>

// Machine Binding für Hardware-Lizenzierung
#include "rael/machine_binding.h"

// --------------------------
// Small helpers
// --------------------------

static std::wstring widen(const std::string& s) {
  if (s.empty()) return L"";
  int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
  std::wstring out(len, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), out.data(), len);
  return out;
}

static std::string narrow(const std::wstring& ws) {
  if (ws.empty()) return "";
  int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), nullptr, 0, nullptr, nullptr);
  std::string out(len, '\0');
  WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), out.data(), len, nullptr, nullptr);
  return out;
}

static void edit_append(HWND hEdit, const std::wstring& text) {
  // Append and keep caret at end
  int len = GetWindowTextLengthW(hEdit);
  SendMessageW(hEdit, EM_SETSEL, (WPARAM)len, (LPARAM)len);
  SendMessageW(hEdit, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
  SendMessageW(hEdit, EM_SCROLLCARET, 0, 0);
}

// --------------------------
// Child process (rael.exe)
// --------------------------

struct ChildProc {
  HANDLE hProcess{nullptr};
  HANDLE hThread{nullptr};
  HANDLE hStdinWrite{nullptr};
  HANDLE hStdoutRead{nullptr};

  std::thread reader;
  std::atomic<bool> running{false};

  std::mutex out_mx;
  std::vector<std::string> out_lines; // collected lines (GUI pulls periodically)
};

static void child_close(ChildProc& c) {
  c.running.store(false);
  if (c.reader.joinable()) c.reader.join();

  if (c.hStdinWrite) { CloseHandle(c.hStdinWrite); c.hStdinWrite = nullptr; }
  if (c.hStdoutRead) { CloseHandle(c.hStdoutRead); c.hStdoutRead = nullptr; }
  if (c.hThread) { CloseHandle(c.hThread); c.hThread = nullptr; }
  if (c.hProcess) { CloseHandle(c.hProcess); c.hProcess = nullptr; }
}

static bool child_send(ChildProc& c, const std::string& line) {
  if (!c.hStdinWrite) return false;
  std::string payload = line;
  if (payload.empty() || payload.back() != '\n') payload.push_back('\n');
  DWORD written = 0;
  BOOL ok = WriteFile(c.hStdinWrite, payload.data(), (DWORD)payload.size(), &written, nullptr);
  return ok && written == payload.size();
}

static bool child_spawn(ChildProc& c, const std::wstring& exePath, const std::wstring& workDir) {
  child_close(c);

  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = nullptr;

  HANDLE childStdoutRead = nullptr;
  HANDLE childStdoutWrite = nullptr;
  HANDLE childStdinRead = nullptr;
  HANDLE childStdinWrite = nullptr;

  if (!CreatePipe(&childStdoutRead, &childStdoutWrite, &sa, 0)) return false;
  if (!SetHandleInformation(childStdoutRead, HANDLE_FLAG_INHERIT, 0)) return false;

  if (!CreatePipe(&childStdinRead, &childStdinWrite, &sa, 0)) return false;
  if (!SetHandleInformation(childStdinWrite, HANDLE_FLAG_INHERIT, 0)) return false;

  STARTUPINFOW si{};
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdOutput = childStdoutWrite;
  si.hStdError  = childStdoutWrite;
  si.hStdInput  = childStdinRead;

  PROCESS_INFORMATION pi{};

  std::wstring cmdline = L"\"" + exePath + L"\"";
  BOOL ok = CreateProcessW(
      nullptr,
      cmdline.data(),
      nullptr,
      nullptr,
      TRUE,
      CREATE_NO_WINDOW,
      nullptr,
      workDir.empty() ? nullptr : workDir.c_str(),
      &si,
      &pi);

  // Parent no longer needs these inherited ends
  CloseHandle(childStdoutWrite);
  CloseHandle(childStdinRead);

  if (!ok) {
    CloseHandle(childStdoutRead);
    CloseHandle(childStdinWrite);
    return false;
  }

  c.hProcess = pi.hProcess;
  c.hThread = pi.hThread;
  c.hStdoutRead = childStdoutRead;
  c.hStdinWrite = childStdinWrite;
  c.running.store(true);

  c.reader = std::thread([&c]() {
    std::string buf;
    buf.reserve(4096);

    char tmp[512];
    while (c.running.load()) {
      DWORD avail = 0;
      if (!PeekNamedPipe(c.hStdoutRead, nullptr, 0, nullptr, &avail, nullptr)) {
        break;
      }
      if (avail == 0) {
        Sleep(25);
        continue;
      }
      DWORD read = 0;
      if (!ReadFile(c.hStdoutRead, tmp, (DWORD)sizeof(tmp), &read, nullptr) || read == 0) {
        break;
      }
      buf.append(tmp, tmp + read);

      // extract lines
      for (;;) {
        size_t pos = buf.find('\n');
        if (pos == std::string::npos) break;
        std::string line = buf.substr(0, pos);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        buf.erase(0, pos + 1);
        {
          std::lock_guard<std::mutex> lk(c.out_mx);
          c.out_lines.push_back(line);
          // keep memory bounded
          if (c.out_lines.size() > 2000) {
            c.out_lines.erase(c.out_lines.begin(), c.out_lines.begin() + 500);
          }
        }
      }
    }
    c.running.store(false);
  });

  return true;
}

static std::vector<std::string> child_drain_output(ChildProc& c) {
  std::lock_guard<std::mutex> lk(c.out_mx);
  std::vector<std::string> out;
  out.swap(c.out_lines);
  return out;
}

// --------------------------
// UI state
// --------------------------

enum class Mode { ReadOnly, Armed };

struct UiState {
  Mode mode{Mode::ReadOnly};
  int activeTab{0};
};

// --------------------------
// Improvements (human-in-the-loop)
// --------------------------

enum class ImproveSource {
  FromReflections,
  FromUserTasks,
  FromLiveDefense,
  FromManual
};

enum class ImproveDecision {
  Pending,
  Approved,
  Rejected
};

struct ImprovementItem {
  int id{0};
  ImproveSource source{ImproveSource::FromReflections};
  ImproveDecision decision{ImproveDecision::Pending};
  int importance{5};   // 1..10
  int risk{2};         // 1..10
  std::string title;
  std::string problem;
  std::string rationale;
  std::string code; // optional; for copy/review
};

struct ImprovementsState {
  std::mutex mu;
  std::vector<ImprovementItem> items;
  int nextId{1};
  int selected{-1};
};

static ImprovementsState g_impr;

// Forward declaration of UiHandles (needed by functions below)
struct UiHandles {
  HWND hwnd{nullptr};
  HWND hTab{nullptr};
  HWND hMainLog{nullptr};
  HWND hChatLog{nullptr};
  HWND hChatInput{nullptr};
  HWND hShellInput{nullptr};
  HWND hBtnSendChat{nullptr};
  HWND hBtnSendShell{nullptr};
  HWND hBtnArm{nullptr};
  HWND hFace{nullptr};
  HWND hPalette{nullptr};
  HWND hCanvas{nullptr};
  HWND hBuilderStats{nullptr};
  HWND hImprList{nullptr};
  HWND hImprDetail{nullptr};
  HWND hImprApprove{nullptr};
  HWND hImprReject{nullptr};
  HWND hImprCopy{nullptr};
};

static UiHandles g_ui;

// Persist as JSONL-ish lines without external deps.
// Format per line:
//  ID|SRC|DEC|IMP|RISK|title\tproblem\trationale\tcode
static std::wstring impr_store_path() {
  wchar_t path[MAX_PATH];
  DWORD n = GetEnvironmentVariableW(L"PROGRAMDATA", path, MAX_PATH);
  std::wstring base = (n > 0) ? std::wstring(path) : L".";
  std::wstring dir = base + L"\\RAEL";
  CreateDirectoryW(dir.c_str(), nullptr);
  return dir + L"\\improvements.db";
}

static int to_int_safe(const std::string& s, int defv) {
  try { return std::stoi(s); } catch (...) { return defv; }
}

static const char* src_to_str(ImproveSource s) {
  switch (s) {
    case ImproveSource::FromReflections: return "REFLECT";
    case ImproveSource::FromUserTasks:   return "USER";
    case ImproveSource::FromLiveDefense: return "LIVE";
    case ImproveSource::FromManual:      return "MANUAL";
  }
  return "REFLECT";
}

static ImproveSource str_to_src(const std::string& s) {
  if (s == "USER") return ImproveSource::FromUserTasks;
  if (s == "LIVE") return ImproveSource::FromLiveDefense;
  if (s == "MANUAL") return ImproveSource::FromManual;
  return ImproveSource::FromReflections;
}

static const char* dec_to_str(ImproveDecision d) {
  switch (d) {
    case ImproveDecision::Pending:  return "P";
    case ImproveDecision::Approved: return "A";
    case ImproveDecision::Rejected: return "R";
  }
  return "P";
}

static ImproveDecision str_to_dec(const std::string& s) {
  if (s == "A") return ImproveDecision::Approved;
  if (s == "R") return ImproveDecision::Rejected;
  return ImproveDecision::Pending;
}

static void impr_load() {
  std::wstring wpath = impr_store_path();
  HANDLE h = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (h == INVALID_HANDLE_VALUE) return;
  DWORD size = GetFileSize(h, nullptr);
  if (size == INVALID_FILE_SIZE || size == 0) { CloseHandle(h); return; }
  std::string buf(size, '\0');
  DWORD read = 0;
  if (!ReadFile(h, buf.data(), size, &read, nullptr) || read == 0) { CloseHandle(h); return; }
  CloseHandle(h);

  std::lock_guard<std::mutex> lk(g_impr.mu);
  g_impr.items.clear();
  g_impr.selected = -1;
  g_impr.nextId = 1;

  size_t off = 0;
  while (off < buf.size()) {
    size_t eol = buf.find('\n', off);
    std::string line = (eol == std::string::npos) ? buf.substr(off) : buf.substr(off, eol - off);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    off = (eol == std::string::npos) ? buf.size() : eol + 1;
    if (line.empty()) continue;

    // split header fields by '|'
    std::vector<std::string> parts;
    size_t p = 0;
    for (int i = 0; i < 6; ++i) {
      size_t nx = line.find('|', p);
      if (nx == std::string::npos) { parts.push_back(line.substr(p)); p = line.size(); break; }
      parts.push_back(line.substr(p, nx - p));
      p = nx + 1;
    }
    if (parts.size() < 6) continue;
    std::string rest = (p < line.size()) ? line.substr(p) : std::string();
    // split rest by '\t' into up to 4 fields
    std::vector<std::string> t;
    size_t tp = 0;
    for (int i = 0; i < 4; ++i) {
      size_t nx = rest.find('\t', tp);
      if (nx == std::string::npos) { t.push_back(rest.substr(tp)); break; }
      t.push_back(rest.substr(tp, nx - tp));
      tp = nx + 1;
    }
    while (t.size() < 4) t.push_back("");

    ImprovementItem it;
    it.id = to_int_safe(parts[0], g_impr.nextId);
    it.source = str_to_src(parts[1]);
    it.decision = str_to_dec(parts[2]);
    it.importance = to_int_safe(parts[3], 5);
    it.risk = to_int_safe(parts[4], 2);
    it.title = t[0];
    it.problem = t[1];
    it.rationale = t[2];
    it.code = t[3];
    g_impr.items.push_back(std::move(it));
    if (g_impr.nextId <= g_impr.items.back().id) g_impr.nextId = g_impr.items.back().id + 1;
  }
}

static void impr_save() {
  std::lock_guard<std::mutex> lk(g_impr.mu);
  std::string out;
  out.reserve(g_impr.items.size() * 256);
  for (auto& it : g_impr.items) {
    out += std::to_string(it.id);
    out += "|";
    out += src_to_str(it.source);
    out += "|";
    out += dec_to_str(it.decision);
    out += "|";
    out += std::to_string(it.importance);
    out += "|";
    out += std::to_string(it.risk);
    out += "|";
    out += it.title; out += "\t";
    out += it.problem; out += "\t";
    out += it.rationale; out += "\t";
    out += it.code;
    out += "\n";
  }

  std::wstring wpath = impr_store_path();
  HANDLE h = CreateFileW(wpath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (h == INVALID_HANDLE_VALUE) return;
  DWORD written = 0;
  WriteFile(h, out.data(), (DWORD)out.size(), &written, nullptr);
  CloseHandle(h);
}

static void impr_add(const ImprovementItem& in) {
  std::lock_guard<std::mutex> lk(g_impr.mu);
  ImprovementItem it = in;
  it.id = g_impr.nextId++;
  g_impr.items.push_back(std::move(it));
  impr_save();
}

static std::wstring impr_pretty_source(ImproveSource s) {
  switch (s) {
    case ImproveSource::FromReflections: return L"Reflections";
    case ImproveSource::FromUserTasks:   return L"User Tasks";
    case ImproveSource::FromLiveDefense: return L"Live Defense";
    case ImproveSource::FromManual:      return L"Manual";
  }
  return L"Reflections";
}

static std::wstring impr_pretty_decision(ImproveDecision d) {
  switch (d) {
    case ImproveDecision::Pending:  return L"PENDING";
    case ImproveDecision::Approved: return L"APPROVED";
    case ImproveDecision::Rejected: return L"REJECTED";
  }
  return L"PENDING";
}

static void impr_refresh_listbox() {
  if (!g_ui.hImprList) return;
  SendMessageW(g_ui.hImprList, LB_RESETCONTENT, 0, 0);
  std::lock_guard<std::mutex> lk(g_impr.mu);
  for (size_t i = 0; i < g_impr.items.size(); ++i) {
    const auto& it = g_impr.items[i];
    std::wstring line = L"[" + impr_pretty_decision(it.decision) + L"] " +
                        L"(" + impr_pretty_source(it.source) + L") " +
                        widen(it.title);
    int idx = (int)SendMessageW(g_ui.hImprList, LB_ADDSTRING, 0, (LPARAM)line.c_str());
    // store vector index as item data (safe as long as we refresh when mutating)
    SendMessageW(g_ui.hImprList, LB_SETITEMDATA, idx, (LPARAM)i);
  }
}

static void impr_show_selected() {
  if (!g_ui.hImprDetail) return;
  std::lock_guard<std::mutex> lk(g_impr.mu);
  if (g_impr.selected < 0 || g_impr.selected >= (int)g_impr.items.size()) {
    SetWindowTextW(g_ui.hImprDetail, L"Select an improvement to see details.");
    return;
  }
  const auto& it = g_impr.items[(size_t)g_impr.selected];
  std::wstring txt;
  txt += L"Title: " + widen(it.title) + L"\r\n";
  txt += L"Source: " + impr_pretty_source(it.source) + L"\r\n";
  txt += L"Decision: " + impr_pretty_decision(it.decision) + L"\r\n";
  wchar_t buf[128];
  swprintf(buf, 128, L"Importance: %d/10 | Risk: %d/10\r\n", it.importance, it.risk);
  txt += buf;
  txt += L"\r\nProblem:\r\n" + widen(it.problem) + L"\r\n";
  txt += L"\r\nRationale:\r\n" + widen(it.rationale) + L"\r\n";
  if (!it.code.empty()) {
    txt += L"\r\n--- CODE (review/copy) ---\r\n";
    txt += widen(it.code);
    txt += L"\r\n";
  }
  SetWindowTextW(g_ui.hImprDetail, txt.c_str());
}

static bool clipboard_set_text(const std::wstring& text) {
  if (!OpenClipboard(g_ui.hwnd)) return false;
  EmptyClipboard();
  size_t bytes = (text.size() + 1) * sizeof(wchar_t);
  HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, bytes);
  if (!hg) { CloseClipboard(); return false; }
  void* p = GlobalLock(hg);
  if (!p) { GlobalFree(hg); CloseClipboard(); return false; }
  memcpy(p, text.c_str(), bytes);
  GlobalUnlock(hg);
  SetClipboardData(CF_UNICODETEXT, hg);
  CloseClipboard();
  return true;
}

// --------------------------
// Face (Rael) speaking visualization
// --------------------------

enum class Viseme { Neutral, A, E, I, O, U, M, F, S, T, R, L };

struct FaceState {
  std::mutex mu;
  bool speaking{false};
  float mouthOpen{0.0f}; // 0..1
  Viseme viseme{Viseme::Neutral};
  std::string sayText{};
  size_t sayIdx{0};
  DWORD sayEndMs{0};
  DWORD lastUpdateMs{0};
};

static FaceState g_face;
static constexpr UINT_PTR IDT_FACE = 2001;


// --------------------------
// BUILDER model (GUI-only simulation)
// --------------------------

enum class BlockType {
  Spiral,
  Nozzle,
  Divider,
  Star8,
  QuintMemory,
  Shield,
  Sensor
};

struct BlockSpec {
  const wchar_t* name;
  double heat_add;   // heat contribution
  double cool_add;   // cooling contribution
  double perf_mul;   // performance multiplier
};

static BlockSpec spec(BlockType t) {
  switch (t) {
    case BlockType::Spiral:      return {L"Spirale",        0.3, 1.2, 0.98};
    case BlockType::Nozzle:      return {L"Performance-D\xFCse", 1.4, 0.1, 1.12};
    case BlockType::Divider:     return {L"Divider",       0.2, 0.0, 0.95};
    case BlockType::Star8:       return {L"AchtStern",     1.0, 0.2, 1.00};
    case BlockType::QuintMemory: return {L"QuintMemory",   0.8, 0.0, 1.02};
    case BlockType::Shield:      return {L"Shield",        0.4, 0.0, 0.90};
    case BlockType::Sensor:      return {L"Sensor",        0.1, 0.0, 0.99};
  }
  return {L"?", 0.0, 0.0, 1.0};
}

struct BuilderState {
  struct BlockInst {
    BlockType type{BlockType::Star8};
    double heatScale{1.0};
    double coolScale{1.0};
    double perfScale{1.0};
  };

  std::vector<BlockInst> pipeline;

  // Drag & drop (GUI-only simulation)
  bool dragging{false};
  bool dragFromPalette{false};
  BlockInst dragInst{};
  POINT dragPt{0,0};

  double ambient_c{28.0};
};

static BuilderState g_builder;

// --------------------------
// Win32 UI
// --------------------------

static const wchar_t* kWndClass = L"RAEL_GUI_WIN";
static const wchar_t* kCanvasClass = L"RAEL_CANVAS_WIN";
static const wchar_t* kFaceClass = L"RAEL_FACE_WIN";
static const UINT_PTR kTimerPoll = 1001;

// UiHandles already defined above (forward declaration)
static UiState g_state;
static ChildProc g_core;

static void ui_layout(HWND hwnd) {
  RECT rc{};
  GetClientRect(hwnd, &rc);
  int W = rc.right - rc.left;
  int H = rc.bottom - rc.top;

  const int pad = 10;
  const int chatW = (int)(W * 0.28); // right pane
  const int leftW = W - chatW - pad * 3;
  const int tabH = 28;
  const int inputH = 26;

  int xLeft = pad;
  int yTop = pad;

  // Tabs
  MoveWindow(g_ui.hTab, xLeft, yTop, leftW, tabH, TRUE);

  // main log area under tabs
  int yMain = yTop + tabH + pad;
  int hMain = H - yMain - pad - inputH - pad;
  MoveWindow(g_ui.hMainLog, xLeft, yMain, leftW, hMain, TRUE);

  // IMPROVEMENTS tab area (hidden unless active)
  const int imprListW = (int)(leftW * 0.40);
  const int imprBtnH = 26;
  const int imprBtnW = 92;
  const int imprBtnsY = yMain + hMain - imprBtnH;
  MoveWindow(g_ui.hImprList, xLeft, yMain, imprListW, hMain - pad - imprBtnH, TRUE);
  MoveWindow(g_ui.hImprDetail, xLeft + imprListW + pad, yMain, leftW - imprListW - pad, hMain - pad - imprBtnH, TRUE);
  // buttons row
  MoveWindow(g_ui.hImprApprove, xLeft, imprBtnsY + pad/2, imprBtnW, imprBtnH, TRUE);
  MoveWindow(g_ui.hImprReject, xLeft + imprBtnW + pad/2, imprBtnsY + pad/2, imprBtnW, imprBtnH, TRUE);
  MoveWindow(g_ui.hImprCopy, xLeft + (imprBtnW + pad/2) * 2, imprBtnsY + pad/2, imprBtnW, imprBtnH, TRUE);

  // BUILDER area (hidden unless BUILDER tab)
  const int paletteW = 220;
  const int statsH = 24;
  MoveWindow(g_ui.hPalette, xLeft, yMain, paletteW, hMain - pad - statsH, TRUE);
  MoveWindow(g_ui.hCanvas, xLeft + paletteW + pad, yMain, leftW - paletteW - pad, hMain - pad - statsH, TRUE);
  MoveWindow(g_ui.hBuilderStats, xLeft, yMain + (hMain - statsH), leftW, statsH, TRUE);

  // shell input at bottom of left pane
  int yShell = yMain + hMain + pad;
  MoveWindow(g_ui.hShellInput, xLeft, yShell, leftW - 90 - pad, inputH, TRUE);
  MoveWindow(g_ui.hBtnSendShell, xLeft + (leftW - 90), yShell, 90, inputH, TRUE);

  // right chat pane
  int xChat = xLeft + leftW + pad;
  int chatHeaderH = 28;
  int faceH = 150;
  int chatInputH = 26;
  int hChatLog = H - pad*4 - chatHeaderH - faceH - chatInputH;

  // arm button in header
  MoveWindow(g_ui.hBtnArm, xChat, yTop, 120, chatHeaderH, TRUE);

  // face (always visible)
  MoveWindow(g_ui.hFace, xChat, yTop + chatHeaderH + pad, chatW - pad, faceH, TRUE);

  // chat log
  MoveWindow(g_ui.hChatLog, xChat, yTop + chatHeaderH + pad + faceH + pad, chatW - pad, hChatLog, TRUE);

  // chat input + send
  int yChatInput = yTop + chatHeaderH + pad + faceH + pad + hChatLog + pad;
  MoveWindow(g_ui.hChatInput, xChat, yChatInput, chatW - 90 - pad*2, chatInputH, TRUE);
  MoveWindow(g_ui.hBtnSendChat, xChat + (chatW - 90 - pad), yChatInput, 90, chatInputH, TRUE);
}

static void ui_append_main(const std::wstring& wline) {
  edit_append(g_ui.hMainLog, wline + L"\r\n");
}

static void ui_append_chat(const std::wstring& wline) {
  edit_append(g_ui.hChatLog, wline + L"\r\n");
}


static Viseme viseme_from_char(char c) {
  switch (c) {
    case 'a': case 'A': return Viseme::A;
    case 'e': case 'E': return Viseme::E;
    case 'i': case 'I': return Viseme::I;
    case 'o': case 'O': return Viseme::O;
    case 'u': case 'U': return Viseme::U;
    case 'm': case 'M': return Viseme::M;
    case 'f': case 'F': return Viseme::F;
    case 's': case 'S': return Viseme::S;
    case 't': case 'T': return Viseme::T;
    case 'r': case 'R': return Viseme::R;
    case 'l': case 'L': return Viseme::L;
    default: return Viseme::Neutral;
  }
}

static void face_set(bool speaking, Viseme v, float mouthOpen) {
  {
    std::lock_guard<std::mutex> lk(g_face.mu);
    g_face.speaking = speaking;
    g_face.viseme = v;
    if (mouthOpen < 0.0f) mouthOpen = 0.0f;
    if (mouthOpen > 1.0f) mouthOpen = 1.0f;
    g_face.mouthOpen = mouthOpen;
    g_face.lastUpdateMs = GetTickCount();
  }
  if (g_ui.hFace) InvalidateRect(g_ui.hFace, nullptr, TRUE);
}


static void face_start_speaking_heuristic(const std::string& text, HWND hwndParent) {
  // Simple local animation as a fallback when the core doesn't emit VIS events.
  if (text.empty()) return;
  DWORD now = GetTickCount();
  {
    std::lock_guard<std::mutex> lk(g_face.mu);
    g_face.speaking = true;
    g_face.sayText = text;
    g_face.sayIdx = 0;
    // Rough duration: 55ms per char, clamped.
    DWORD dur = (DWORD)(text.size() * 55);
    if (dur < 700) dur = 700;
    if (dur > 6000) dur = 6000;
    g_face.sayEndMs = now + dur;
    g_face.viseme = viseme_from_char(text[0]);
    g_face.mouthOpen = 0.35f;
    g_face.lastUpdateMs = now;
  }
  if (g_ui.hFace) InvalidateRect(g_ui.hFace, nullptr, TRUE);
  SetTimer(hwndParent, IDT_FACE, 33, nullptr);
}


static void ui_apply_tab_visibility() {
  // BUILDER tab index (set in ui_init_tabs)
  const bool isBuilder = (g_state.activeTab == 3);
  const bool isImpr = (g_state.activeTab == 7);

  ShowWindow(g_ui.hMainLog, (isBuilder || isImpr) ? SW_HIDE : SW_SHOW);
  ShowWindow(g_ui.hShellInput, (isBuilder || isImpr) ? SW_HIDE : SW_SHOW);
  ShowWindow(g_ui.hBtnSendShell, (isBuilder || isImpr) ? SW_HIDE : SW_SHOW);

  ShowWindow(g_ui.hPalette, isBuilder ? SW_SHOW : SW_HIDE);
  ShowWindow(g_ui.hCanvas, isBuilder ? SW_SHOW : SW_HIDE);
  ShowWindow(g_ui.hBuilderStats, isBuilder ? SW_SHOW : SW_HIDE);

  ShowWindow(g_ui.hImprList, isImpr ? SW_SHOW : SW_HIDE);
  ShowWindow(g_ui.hImprDetail, isImpr ? SW_SHOW : SW_HIDE);
  ShowWindow(g_ui.hImprApprove, isImpr ? SW_SHOW : SW_HIDE);
  ShowWindow(g_ui.hImprReject, isImpr ? SW_SHOW : SW_HIDE);
  ShowWindow(g_ui.hImprCopy, isImpr ? SW_SHOW : SW_HIDE);
}

struct BuilderTotals {
  double perf{1.0};
  double heat{0.0};
  double cool{0.0};
  double temp_c{28.0};
};

static BuilderTotals builder_totals() {
  BuilderTotals t;
  t.temp_c = g_builder.ambient_c;
  for (auto& bi : g_builder.pipeline) {
    auto s = spec(bi.type);
    t.perf *= (s.perf_mul * bi.perfScale);
    t.heat += (s.heat_add * bi.heatScale);
    t.cool += (s.cool_add * bi.coolScale);
  }
  // very simple GUI-only thermal model (trend indicator):
  // - heat raises temperature
  // - cooling reduces temperature
  // - clamp to a reasonable range
  t.temp_c = g_builder.ambient_c + t.heat * 18.0 - t.cool * 14.0;
  if (t.temp_c < g_builder.ambient_c) t.temp_c = g_builder.ambient_c;
  if (t.temp_c > 120.0) t.temp_c = 120.0;
  return t;
}

static void builder_update_stats() {
  auto t = builder_totals();
  wchar_t buf[256];
  const wchar_t* zone = (t.temp_c < 55.0) ? L"GRUEN" : (t.temp_c < 75.0 ? L"ORANGE" : L"ROT");
  swprintf(buf, 256, L"Builder: Blocks=%d | Perf x%.2f | Temp %.1f C (%s)",
           (int)g_builder.pipeline.size(), t.perf, t.temp_c, zone);
  SetWindowTextW(g_ui.hBuilderStats, buf);
}

static void ui_poll_core() {
  if (!g_core.running.load()) return;

  // Poll reads (safe) depending on active tab
  // Keep it simple: always poll a minimal set.
  child_send(g_core, "node");
  child_send(g_core, "lane");
  child_send(g_core, "events 8");


auto lines = child_drain_output(g_core);
for (auto& l : lines) {
  // Optional: voice/viseme events for the face pane.
  // Protocol example (core may implement later):
  //   EVT|VOICE|state=talking|viseme=A|mouth=0.45
  if (l.rfind("EVT|VOICE|", 0) == 0) {
    bool speaking = true;
    Viseme v = Viseme::Neutral;
    float mouth = 0.35f;
    // parse k=v tokens
    size_t p = 0;
    while (p < l.size()) {
      size_t next = l.find('|', p);
      std::string tok = (next == std::string::npos) ? l.substr(p) : l.substr(p, next - p);
      size_t eq = tok.find('=');
      if (eq != std::string::npos) {
        std::string k = tok.substr(0, eq);
        std::string val = tok.substr(eq + 1);
        if (k == "state") {
          speaking = (val != "idle" && val != "0" && val != "off");
        } else if (k == "viseme" && !val.empty()) {
          v = viseme_from_char(val[0]);
        } else if (k == "mouth") {
          try { mouth = std::stof(val); } catch (...) {}
        }
      }
      if (next == std::string::npos) break;
      p = next + 1;
    }
    face_set(speaking, v, mouth);
    ui_append_chat(widen(l));
    continue;
  }

  // Improvements event (human-in-the-loop)
  // Example:
  //   EVT|IMPROVE|src=REFLECT|importance=8|risk=3|title=...|problem=...|rationale=...|code=...
  if (l.rfind("EVT|IMPROVE|", 0) == 0) {
    ImprovementItem it;
    it.source = ImproveSource::FromReflections;
    it.importance = 5;
    it.risk = 2;
    size_t p = 0;
    while (p < l.size()) {
      size_t next = l.find('|', p);
      std::string tok = (next == std::string::npos) ? l.substr(p) : l.substr(p, next - p);
      size_t eq = tok.find('=');
      if (eq != std::string::npos) {
        std::string k = tok.substr(0, eq);
        std::string val = tok.substr(eq + 1);
        if (k == "src") {
          it.source = str_to_src(val);
        } else if (k == "importance") {
          it.importance = to_int_safe(val, 5);
        } else if (k == "risk") {
          it.risk = to_int_safe(val, 2);
        } else if (k == "title") {
          it.title = val;
        } else if (k == "problem") {
          it.problem = val;
        } else if (k == "rationale") {
          it.rationale = val;
        } else if (k == "code") {
          it.code = val;
        }
      }
      if (next == std::string::npos) break;
      p = next + 1;
    }
    // Minimal guard: require title
    if (!it.title.empty()) {
      impr_add(it);
      impr_refresh_listbox();
      ui_append_chat(L"Rael: [IMPROVEMENT] " + widen(it.title));
    }
    continue;
  }
  ui_append_main(widen(l));
}
}

static void ui_send_shell() {
  wchar_t buf[2048];
  GetWindowTextW(g_ui.hShellInput, buf, 2048);
  std::wstring ws(buf);
  if (ws.empty()) return;
  SetWindowTextW(g_ui.hShellInput, L"");

  std::string cmd = narrow(ws);
  ui_append_main(L"> " + ws);
  if (!g_core.running.load()) {
    ui_append_main(L"[gui] core not running.");
    return;
  }
  // Shell is a power interface; in READ-ONLY we warn but still allow *read* commands.
  if (g_state.mode == Mode::ReadOnly) {
    // allow only a small safe set
    if (!(cmd.rfind("node", 0) == 0 || cmd.rfind("lane", 0) == 0 || cmd.rfind("events", 0) == 0 ||
          cmd.rfind("metrics", 0) == 0 || cmd.rfind("status", 0) == 0 || cmd.rfind("help", 0) == 0)) {
      ui_append_main(L"[gui] READ-ONLY: command blocked. Use ARM to enable control.");
      return;
    }
  }
  child_send(g_core, cmd);
}

static void ui_send_chat() {
  wchar_t buf[2048];
  GetWindowTextW(g_ui.hChatInput, buf, 2048);
  std::wstring ws(buf);
  if (ws.empty()) return;
  SetWindowTextW(g_ui.hChatInput, L"");

  ui_append_chat(L"You: " + ws);

  if (!g_core.running.load()) {
    ui_append_chat(L"Rael: Core is not running yet.");
    return;
  }

  // Chat is intentionally non-privileged. We send it as a best-effort "say" command.
  // If the core doesn't implement it, it will answer "Unknown command" (still visible).
  std::string msg = narrow(ws);
  std::string cmd = "say " + msg;
  face_start_speaking_heuristic(msg, g_ui.hwnd);
  child_send(g_core, cmd);
}

static void ui_set_arm_button() {
  if (!g_ui.hBtnArm) return;
  if (g_state.mode == Mode::Armed) {
    SetWindowTextW(g_ui.hBtnArm, L"ARMED (click to lock)");
  } else {
    SetWindowTextW(g_ui.hBtnArm, L"ARM (locked)");
  }
}

static void ui_toggle_arm() {
  if (g_state.mode == Mode::ReadOnly) {
    // For now, ARM is a local toggle. Recommended: replace with core challenge/response.
    g_state.mode = Mode::Armed;
    ui_append_chat(L"Rael: CONTROL enabled (ARMED). Be careful.");
  } else {
    g_state.mode = Mode::ReadOnly;
    ui_append_chat(L"Rael: CONTROL disabled (READ-ONLY).");
  }
  ui_set_arm_button();
}

static void ui_init_tabs(HWND hTab) {
  TCITEMW tie{};
  tie.mask = TCIF_TEXT;

  // Tab order is part of the UI contract.
  // Indices used elsewhere:
  //  - BUILDER: 3
  //  - IMPROVEMENTS: 7
  const wchar_t* names[] = {L"LIVE", L"FREQUENCY", L"SECURITY", L"BUILDER", L"DECISIONS", L"PERFORMANCE", L"MODULES", L"IMPROVEMENTS", L"LOGBOOK"};
  for (int i = 0; i < (int)(sizeof(names)/sizeof(names[0])); ++i) {
    tie.pszText = (LPWSTR)names[i];
    TabCtrl_InsertItem(hTab, i, &tie);
  }
  TabCtrl_SetCurSel(hTab, 0);
  g_state.activeTab = 0;
}

// --------------------------
// Canvas (BUILDER visual)
// --------------------------

static int builder_hit_test_block(int x, int y, const RECT& rcClient) {
  // layout constants should match paint
  const int pad = 12;
  const int blockW = 155;
  const int blockH = 74;
  const int gap = 10;
  int startX = pad;
  int startY = pad;
  if (y < startY || y > startY + blockH) return -1;
  int i = (x - startX) / (blockW + gap);
  if (i < 0 || i >= (int)g_builder.pipeline.size()) return -1;
  int bx = startX + i * (blockW + gap);
  if (x < bx || x > bx + blockW) return -1;
  return i;
}

static int builder_insert_index_from_x(int x, const RECT& rcClient) {
  const int pad = 12;
  const int blockW = 155;
  const int gap = 10;
  int startX = pad;
  int i = (x - startX) / (blockW + gap);
  if (i < 0) i = 0;
  if (i > (int)g_builder.pipeline.size()) i = (int)g_builder.pipeline.size();
  // If user drops in the right half of a block, insert after it.
  int bx = startX + i * (blockW + gap);
  if (i < (int)g_builder.pipeline.size() && x > bx + blockW / 2) i++;
  if (i < 0) i = 0;
  if (i > (int)g_builder.pipeline.size()) i = (int)g_builder.pipeline.size();
  return i;
}

static void builder_add(BlockType t, int insertIdx = -1) {
  BuilderState::BlockInst bi;
  bi.type = t;
  if (insertIdx < 0 || insertIdx > (int)g_builder.pipeline.size()) {
    g_builder.pipeline.push_back(bi);
  } else {
    g_builder.pipeline.insert(g_builder.pipeline.begin() + insertIdx, bi);
  }
}

static void builder_adjust(int idx, double heatMul, double coolMul, double perfMul) {
  if (idx < 0 || idx >= (int)g_builder.pipeline.size()) return;
  auto& b = g_builder.pipeline[idx];
  b.heatScale *= heatMul;
  b.coolScale *= coolMul;
  b.perfScale *= perfMul;
  // clamp to keep it sensible
  auto clamp = [](double v, double lo, double hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
  };
  b.heatScale = clamp(b.heatScale, 0.25, 4.0);
  b.coolScale = clamp(b.coolScale, 0.0, 4.0);
  b.perfScale = clamp(b.perfScale, 0.5, 2.0);
}

static COLORREF temp_color(double tempC) {
  if (tempC < 55.0) return RGB(0, 180, 0);
  if (tempC < 75.0) return RGB(255, 150, 0);
  return RGB(200, 0, 0);
}

static LRESULT CALLBACK CanvasProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_LBUTTONDBLCLK: {
      // double-click empty canvas -> clear pipeline
      g_builder.pipeline.clear();
      builder_update_stats();
      InvalidateRect(hwnd, nullptr, TRUE);
      return 0;
    }
    case WM_MOUSEMOVE: {
      if (g_builder.dragging) {
        g_builder.dragPt.x = GET_X_LPARAM(lParam);
        g_builder.dragPt.y = GET_Y_LPARAM(lParam);
        InvalidateRect(hwnd, nullptr, FALSE);
      }
      return 0;
    }
    case WM_RBUTTONUP: {
      RECT rc{};
      GetClientRect(hwnd, &rc);
      int x = GET_X_LPARAM(lParam);
      int y = GET_Y_LPARAM(lParam);
      int idx = builder_hit_test_block(x, y, rc);
      if (idx < 0 || idx >= (int)g_builder.pipeline.size()) return 0;

      enum {
        IDM_REMOVE = 2001,
        IDM_INFO = 2002,
        IDM_HEAT_UP = 2010,
        IDM_HEAT_DN = 2011,
        IDM_COOL_UP = 2020,
        IDM_COOL_DN = 2021,
        IDM_PERF_UP = 2030,
        IDM_PERF_DN = 2031,
      };

      HMENU menu = CreatePopupMenu();
      AppendMenuW(menu, MF_STRING, IDM_REMOVE, L"Entfernen");
      AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
      AppendMenuW(menu, MF_STRING, IDM_INFO, L"Info / Eigenschaften");
      AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
      AppendMenuW(menu, MF_STRING, IDM_HEAT_UP, L"Hitze +10% (mehr Leistung / mehr W\xE4rme)");
      AppendMenuW(menu, MF_STRING, IDM_HEAT_DN, L"Hitze -10% (weniger W\xE4rme)");
      AppendMenuW(menu, MF_STRING, IDM_COOL_UP, L"K\xFChlung +10% (k\xFChlt st\xE4rker)");
      AppendMenuW(menu, MF_STRING, IDM_COOL_DN, L"K\xFChlung -10% (k\xFChlt schw\xE4cher)");
      AppendMenuW(menu, MF_STRING, IDM_PERF_UP, L"Performance +5%");
      AppendMenuW(menu, MF_STRING, IDM_PERF_DN, L"Performance -5%");

      POINT pt{ x, y };
      ClientToScreen(hwnd, &pt);
      UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
      DestroyMenu(menu);

      if (cmd == IDM_REMOVE) {
        g_builder.pipeline.erase(g_builder.pipeline.begin() + idx);
      } else if (cmd == IDM_HEAT_UP) {
        builder_adjust(idx, 1.10, 1.0, 1.0);
      } else if (cmd == IDM_HEAT_DN) {
        builder_adjust(idx, 0.90, 1.0, 1.0);
      } else if (cmd == IDM_COOL_UP) {
        builder_adjust(idx, 1.0, 1.10, 1.0);
      } else if (cmd == IDM_COOL_DN) {
        builder_adjust(idx, 1.0, 0.90, 1.0);
      } else if (cmd == IDM_PERF_UP) {
        builder_adjust(idx, 1.0, 1.0, 1.05);
      } else if (cmd == IDM_PERF_DN) {
        builder_adjust(idx, 1.0, 1.0, 0.95);
      } else if (cmd == IDM_INFO) {
        auto& bi = g_builder.pipeline[idx];
        auto s = spec(bi.type);
        wchar_t msg[512];
        swprintf(msg, 512,
          L"%s\n\nBedeutung (einfach):\n- Dieser Block ver\xE4ndert Leistung und W\xE4rme in der Rechenstra\xDFe.\n\nParameter (SIM):\nHitze-Faktor: %.2f\nK\xFChl-Faktor: %.2f\nPerf-Faktor: %.2f\n\nTipp: Rechtsklick \x2192 Hitze/K\xFChlung/Performance feinjustieren.",
          s.name, bi.heatScale, bi.coolScale, bi.perfScale);
        MessageBoxW(hwnd, msg, L"Block-Info", MB_OK | MB_ICONINFORMATION);
      }

      builder_update_stats();
      InvalidateRect(hwnd, nullptr, TRUE);
      return 0;
    }
    case WM_PAINT: {
      PAINTSTRUCT ps{};
      HDC hdc = BeginPaint(hwnd, &ps);
      RECT rc{};
      GetClientRect(hwnd, &rc);

      // background
      HBRUSH bg = CreateSolidBrush(RGB(20, 20, 24));
      FillRect(hdc, &rc, bg);
      DeleteObject(bg);

      SetBkMode(hdc, TRANSPARENT);

      // title
      SetTextColor(hdc, RGB(230, 230, 240));
      TextOutW(hdc, 12, 8, L"RECHENSTRASSE (SIM)  —  Ziehen aus Palette | Doppelklick: reset | Rechtsklick: Menu", 88);

      const int pad = 12;
      const int blockW = 155;
      const int blockH = 74;
      const int gap = 10;
      int x = pad;
      int y = 32;

      double cumHeat = 0.0;
      double cumCool = 0.0;
      for (size_t i = 0; i < g_builder.pipeline.size(); ++i) {
        auto& bi = g_builder.pipeline[i];
        auto s = spec(bi.type);
        cumHeat += (s.heat_add * bi.heatScale);
        cumCool += (s.cool_add * bi.coolScale);
        double tC = g_builder.ambient_c + cumHeat * 18.0 - cumCool * 14.0;
        if (tC < g_builder.ambient_c) tC = g_builder.ambient_c;
        if (tC > 120.0) tC = 120.0;

        RECT r{ x, y, x + blockW, y + blockH };
        // block body
        HBRUSH b = CreateSolidBrush(RGB(34, 34, 40));
        FillRect(hdc, &r, b);
        DeleteObject(b);

        // temp bar
        RECT bar{ x, y + blockH - 12, x + blockW, y + blockH };
        HBRUSH tb = CreateSolidBrush(temp_color(tC));
        FillRect(hdc, &bar, tb);
        DeleteObject(tb);

        // border
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(80, 80, 90));
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        HGDIOBJ oldBr = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
        Rectangle(hdc, r.left, r.top, r.right, r.bottom);
        SelectObject(hdc, oldBr);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);

        // text
        wchar_t line1[128];
        wchar_t line2[128];
        swprintf(line1, 128, L"%d. %s", (int)i + 1, s.name);
        swprintf(line2, 128, L"Temp: %.1f C | Perf x%.2f", tC, s.perf_mul * bi.perfScale);
        SetTextColor(hdc, RGB(230, 230, 240));
        TextOutW(hdc, x + 8, y + 10, line1, (int)wcslen(line1));
        SetTextColor(hdc, RGB(190, 190, 205));
        TextOutW(hdc, x + 8, y + 32, line2, (int)wcslen(line2));

        x += blockW + gap;
        // wrap if needed
        if (x + blockW > rc.right - pad) {
          x = pad;
          y += blockH + gap;
        }
      }

      // Drag ghost (from palette) - gives the "pull into the Rechenstrasse" feel.
      if (g_builder.dragging && g_builder.dragFromPalette) {
        auto s = spec(g_builder.dragInst.type);
        int gx = g_builder.dragPt.x - 60;
        int gy = g_builder.dragPt.y - 28;
        RECT gr{ gx, gy, gx + 155, gy + 74 };

        // dotted outline
        HPEN pen = CreatePen(PS_DOT, 1, RGB(200, 200, 220));
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        HGDIOBJ oldBr = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
        Rectangle(hdc, gr.left, gr.top, gr.right, gr.bottom);
        SelectObject(hdc, oldBr);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);

        SetTextColor(hdc, RGB(200, 200, 220));
        TextOutW(hdc, gr.left + 8, gr.top + 10, s.name, (int)wcslen(s.name));
      }

      EndPaint(hwnd, &ps);
      return 0;
    }
  }
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}


static LRESULT CALLBACK FaceProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_PAINT: {
      PAINTSTRUCT ps{};
      HDC hdc = BeginPaint(hwnd, &ps);
      RECT rc{}; GetClientRect(hwnd, &rc);

      // background
      HBRUSH bg = CreateSolidBrush(RGB(16, 16, 20));
      FillRect(hdc, &rc, bg);
      DeleteObject(bg);

      // border
      HPEN pen = CreatePen(PS_SOLID, 1, RGB(60, 60, 70));
      HGDIOBJ oldPen = SelectObject(hdc, pen);
      HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
      Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
      SelectObject(hdc, oldBrush);
      SelectObject(hdc, oldPen);
      DeleteObject(pen);

      // face geometry
      int w = rc.right - rc.left;
      int h = rc.bottom - rc.top;
      int cx = rc.left + w/2;
      int cy = rc.top + h/2 - 6;
      int headR = (w < h ? w : h) / 3;
      if (headR < 22) headR = 22;

      // read face state
      bool speaking=false;
      float mouth=0.0f;
      Viseme vis = Viseme::Neutral;
      {
        std::lock_guard<std::mutex> lk(g_face.mu);
        speaking = g_face.speaking;
        mouth = g_face.mouthOpen;
        vis = g_face.viseme;
      }

      // head
      HBRUSH head = CreateSolidBrush(RGB(28, 28, 36));
      HBRUSH oldB = (HBRUSH)SelectObject(hdc, head);
      HPEN headPen = CreatePen(PS_SOLID, 2, RGB(90, 90, 110));
      oldPen = SelectObject(hdc, headPen);
      Ellipse(hdc, cx-headR, cy-headR, cx+headR, cy+headR);
      SelectObject(hdc, oldPen);
      DeleteObject(headPen);
      SelectObject(hdc, oldB);
      DeleteObject(head);

      // eyes
      HBRUSH eye = CreateSolidBrush(RGB(220, 220, 235));
      oldB = (HBRUSH)SelectObject(hdc, eye);
      int eyeY = cy - headR/5;
      int eyeDX = headR/2;
      int eyeR = headR/10;
      Ellipse(hdc, cx-eyeDX-eyeR, eyeY-eyeR, cx-eyeDX+eyeR, eyeY+eyeR);
      Ellipse(hdc, cx+eyeDX-eyeR, eyeY-eyeR, cx+eyeDX+eyeR, eyeY+eyeR);
      SelectObject(hdc, oldB);
      DeleteObject(eye);

      // mouth shape based on viseme + openness
      int mouthW = headR;
      int mouthH = (int)(headR * (0.15 + 0.55*mouth));
      if (mouthH < 4) mouthH = 4;
      int mouthY = cy + headR/3;

      COLORREF mouthCol = speaking ? RGB(230, 120, 120) : RGB(160, 160, 180);
      HPEN mouthPen = CreatePen(PS_SOLID, 2, mouthCol);
      oldPen = SelectObject(hdc, mouthPen);

      // Simple viseme mapping:
      // - vowels: oval mouth
      // - M/F: flat line
      // - S/T/R/L: wider, less open
      if (vis == Viseme::M || vis == Viseme::F) {
        MoveToEx(hdc, cx - mouthW/2, mouthY, nullptr);
        LineTo(hdc, cx + mouthW/2, mouthY);
      } else if (vis == Viseme::S || vis == Viseme::T || vis == Viseme::R || vis == Viseme::L) {
        int ww = (int)(mouthW * 0.70);
        int hh = (int)(mouthH * 0.55);
        Ellipse(hdc, cx-ww/2, mouthY-hh/2, cx+ww/2, mouthY+hh/2);
      } else {
        Ellipse(hdc, cx-mouthW/2, mouthY-mouthH/2, cx+mouthW/2, mouthY+mouthH/2);
      }

      SelectObject(hdc, oldPen);
      DeleteObject(mouthPen);

      // label
      SetBkMode(hdc, TRANSPARENT);
      SetTextColor(hdc, RGB(210, 210, 225));
      const wchar_t* st = speaking ? L"RAEL: spricht" : L"RAEL: bereit";
      TextOutW(hdc, rc.left + 10, rc.top + 8, st, (int)wcslen(st));

      EndPaint(hwnd, &ps);
      return 0;
    }
  }
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_LBUTTONDOWN: {
      // Start drag from palette (click-and-drag into the builder canvas).
      POINT spt{};
      GetCursorPos(&spt);
      HWND hAt = WindowFromPoint(spt);
      if (hAt == g_ui.hPalette) {
        int sel = (int)SendMessageW(g_ui.hPalette, LB_GETCURSEL, 0, 0);
        BlockType t = BlockType::Star8;
        switch (sel) {
          case 0: t = BlockType::Spiral; break;
          case 1: t = BlockType::Nozzle; break;
          case 2: t = BlockType::Divider; break;
          case 3: t = BlockType::Star8; break;
          case 4: t = BlockType::QuintMemory; break;
          case 5: t = BlockType::Shield; break;
          case 6: t = BlockType::Sensor; break;
        }

        g_builder.dragging = true;
        g_builder.dragFromPalette = true;
        g_builder.dragInst = BuilderState::BlockInst{};
        g_builder.dragInst.type = t;

        // Store initial drag position relative to canvas
        POINT cpt = spt;
        ScreenToClient(g_ui.hCanvas, &cpt);
        g_builder.dragPt = cpt;

        SetCapture(hwnd);
        InvalidateRect(g_ui.hCanvas, nullptr, FALSE);
        return 0;
      }
      return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    case WM_MOUSEMOVE: {
      if (g_builder.dragging && g_builder.dragFromPalette) {
        POINT spt{};
        GetCursorPos(&spt);
        POINT cpt = spt;
        ScreenToClient(g_ui.hCanvas, &cpt);
        g_builder.dragPt = cpt;
        InvalidateRect(g_ui.hCanvas, nullptr, FALSE);
        return 0;
      }
      break;
    }

    case WM_LBUTTONUP: {
      if (g_builder.dragging && g_builder.dragFromPalette) {
        POINT spt{};
        GetCursorPos(&spt);
        HWND hAt = WindowFromPoint(spt);
        // Drop only when cursor is over the canvas window
        if (hAt == g_ui.hCanvas) {
          POINT cpt = spt;
          ScreenToClient(g_ui.hCanvas, &cpt);
          RECT rc{};
          GetClientRect(g_ui.hCanvas, &rc);
          int ins = builder_insert_index_from_x(cpt.x, rc);
          builder_add(g_builder.dragInst.type, ins);
        }

        g_builder.dragging = false;
        g_builder.dragFromPalette = false;
        ReleaseCapture();
        builder_update_stats();
        InvalidateRect(g_ui.hCanvas, nullptr, TRUE);
        return 0;
      }
      break;
    }
    case WM_CREATE: {
      INITCOMMONCONTROLSEX icc{};
      icc.dwSize = sizeof(icc);
      icc.dwICC = ICC_TAB_CLASSES;
      InitCommonControlsEx(&icc);

      g_ui.hwnd = hwnd;
      g_ui.hTab = CreateWindowExW(0, WC_TABCONTROLW, L"",
                                 WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                 0, 0, 100, 100, hwnd, (HMENU)100, GetModuleHandleW(nullptr), nullptr);

      g_ui.hMainLog = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                     WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL,
                                     0, 0, 100, 100, hwnd, (HMENU)101, GetModuleHandleW(nullptr), nullptr);

      // BUILDER controls (initially hidden)
      g_ui.hPalette = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
                                      WS_CHILD | LBS_NOTIFY | WS_VSCROLL,
                                      0, 0, 100, 100, hwnd, (HMENU)120, GetModuleHandleW(nullptr), nullptr);

      g_ui.hCanvas = CreateWindowExW(WS_EX_CLIENTEDGE, kCanvasClass, L"",
                                     WS_CHILD | WS_VISIBLE,
                                     0, 0, 100, 100, hwnd, (HMENU)121, GetModuleHandleW(nullptr), nullptr);

      g_ui.hBuilderStats = CreateWindowExW(0, L"STATIC", L"Builder: ",
                                           WS_CHILD | WS_VISIBLE,
                                           0, 0, 100, 24, hwnd, (HMENU)122, GetModuleHandleW(nullptr), nullptr);

      // IMPROVEMENTS controls (hidden unless tab active)
      g_ui.hImprList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
                                       WS_CHILD | LBS_NOTIFY | WS_VSCROLL,
                                       0, 0, 100, 100, hwnd, (HMENU)130, GetModuleHandleW(nullptr), nullptr);

      g_ui.hImprDetail = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                         WS_CHILD | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL,
                                         0, 0, 100, 100, hwnd, (HMENU)131, GetModuleHandleW(nullptr), nullptr);

      g_ui.hImprApprove = CreateWindowExW(0, L"BUTTON", L"Approve",
                                          WS_CHILD,
                                          0, 0, 90, 24, hwnd, (HMENU)132, GetModuleHandleW(nullptr), nullptr);
      g_ui.hImprReject = CreateWindowExW(0, L"BUTTON", L"Reject",
                                         WS_CHILD,
                                         0, 0, 90, 24, hwnd, (HMENU)133, GetModuleHandleW(nullptr), nullptr);
      g_ui.hImprCopy = CreateWindowExW(0, L"BUTTON", L"Copy Code",
                                       WS_CHILD,
                                       0, 0, 90, 24, hwnd, (HMENU)134, GetModuleHandleW(nullptr), nullptr);

      // palette items
      const wchar_t* items[] = {L"Spirale (K\xFChlung)", L"Performance-D\xFCse", L"Divider", L"AchtStern (Star8)", L"QuintMemory", L"Shield", L"Sensor"};
      for (int i = 0; i < (int)(sizeof(items)/sizeof(items[0])); ++i) {
        SendMessageW(g_ui.hPalette, LB_ADDSTRING, 0, (LPARAM)items[i]);
      }
      SendMessageW(g_ui.hPalette, LB_SETCURSEL, 0, 0);

      g_ui.hShellInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                         WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                         0, 0, 100, 24, hwnd, (HMENU)102, GetModuleHandleW(nullptr), nullptr);

      g_ui.hBtnSendShell = CreateWindowExW(0, L"BUTTON", L"Send",
                                           WS_CHILD | WS_VISIBLE,
                                           0, 0, 90, 24, hwnd, (HMENU)103, GetModuleHandleW(nullptr), nullptr);

      g_ui.hBtnArm = CreateWindowExW(0, L"BUTTON", L"ARM (locked)",
                                     WS_CHILD | WS_VISIBLE,
                                     0, 0, 120, 24, hwnd, (HMENU)110, GetModuleHandleW(nullptr), nullptr);

      g_ui.hFace = CreateWindowExW(WS_EX_CLIENTEDGE, kFaceClass, L"",
                                   WS_CHILD | WS_VISIBLE,
                                   0, 0, 100, 120, hwnd, (HMENU)114, GetModuleHandleW(nullptr), nullptr);


      g_ui.hChatLog = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                      WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL,
                                      0, 0, 100, 100, hwnd, (HMENU)111, GetModuleHandleW(nullptr), nullptr);

      g_ui.hChatInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                        0, 0, 100, 24, hwnd, (HMENU)112, GetModuleHandleW(nullptr), nullptr);

      g_ui.hBtnSendChat = CreateWindowExW(0, L"BUTTON", L"Send",
                                          WS_CHILD | WS_VISIBLE,
                                          0, 0, 90, 24, hwnd, (HMENU)113, GetModuleHandleW(nullptr), nullptr);

      ui_init_tabs(g_ui.hTab);
      ui_set_arm_button();
      ui_layout(hwnd);

      // Load persisted improvements (human-in-the-loop backlog)
      impr_load();
      impr_refresh_listbox();
      impr_show_selected();

      builder_update_stats();
      ui_apply_tab_visibility();

      // Start RAEL core from ..\bin\rael.exe relative to this GUI exe.
      // Workdir is bin\ so modules and example_pack paths behave as expected.
      wchar_t exeDir[MAX_PATH];
      GetModuleFileNameW(nullptr, exeDir, MAX_PATH);
      std::wstring path(exeDir);
      size_t slash = path.find_last_of(L"\\/");
      std::wstring myDir = (slash == std::wstring::npos) ? L"." : path.substr(0, slash);
      std::wstring coreExe = myDir + L"\\rael.exe";

      if (child_spawn(g_core, coreExe, myDir)) {
        ui_append_main(L"[gui] core started: " + coreExe);
        // warm start
        child_send(g_core, "help");
        child_send(g_core, "status");
      } else {
        ui_append_main(L"[gui] failed to start core (expected bin\\rael.exe next to rael_gui.exe)");
      }

      // Poll timer (350ms)
      SetTimer(hwnd, kTimerPoll, 350, nullptr);
      ui_append_chat(L"Rael: Cockpit ready. READ-ONLY by default.");
      return 0;
    }

    case WM_SIZE:
      ui_layout(hwnd);
      return 0;


case WM_TIMER:
  if (wParam == kTimerPoll) {
    ui_poll_core();
    if (g_state.activeTab == 3) {
      builder_update_stats();
      InvalidateRect(g_ui.hCanvas, nullptr, FALSE);
    }
    return 0;
  }
  if (wParam == IDT_FACE) {
    // local face animation tick (fallback when core doesn't emit voice events)
    DWORD now = GetTickCount();
    bool stop = false;
    {
      std::lock_guard<std::mutex> lk(g_face.mu);
      if (!g_face.speaking) {
        stop = true;
      } else if (now >= g_face.sayEndMs) {
        g_face.speaking = false;
        g_face.mouthOpen = 0.0f;
        g_face.viseme = Viseme::Neutral;
        stop = true;
      } else {
        // advance pseudo-visemes through text
        if (!g_face.sayText.empty()) {
          size_t step = (size_t)((now - g_face.lastUpdateMs) / 90);
          if (step > 0) {
            g_face.sayIdx = (g_face.sayIdx + step) % g_face.sayText.size();
            g_face.viseme = viseme_from_char(g_face.sayText[g_face.sayIdx]);
            g_face.lastUpdateMs = now;
          }
        }
        // mouth open/close oscillation
        float t = (float)((now / 120) % 10) / 10.0f;
        g_face.mouthOpen = 0.18f + 0.42f * (t < 0.5f ? t*2.0f : (1.0f - (t-0.5f)*2.0f));
      }
    }
    if (g_ui.hFace) InvalidateRect(g_ui.hFace, nullptr, TRUE);
    if (stop) KillTimer(hwnd, IDT_FACE);
    return 0;
  }
  return 0;
    case WM_COMMAND: {
      int id = LOWORD(wParam);
      int notif = HIWORD(wParam);
      // Improvements tab interactions
      if (id == 130 && notif == LBN_SELCHANGE) { // list select
        int sel = (int)SendMessageW(g_ui.hImprList, LB_GETCURSEL, 0, 0);
        if (sel >= 0) {
          LRESULT data = SendMessageW(g_ui.hImprList, LB_GETITEMDATA, sel, 0);
          {
            std::lock_guard<std::mutex> lk(g_impr.mu);
            g_impr.selected = (int)data;
          }
          impr_show_selected();
        }
        return 0;
      }
      if (id == 132) { // approve
        {
          std::lock_guard<std::mutex> lk(g_impr.mu);
          if (g_impr.selected >= 0 && g_impr.selected < (int)g_impr.items.size()) {
            g_impr.items[(size_t)g_impr.selected].decision = ImproveDecision::Approved;
          }
        }
        impr_save();
        impr_refresh_listbox();
        impr_show_selected();
        ui_append_chat(L"Rael: Improvement approved (pending integration)." );
        return 0;
      }
      if (id == 133) { // reject
        {
          std::lock_guard<std::mutex> lk(g_impr.mu);
          if (g_impr.selected >= 0 && g_impr.selected < (int)g_impr.items.size()) {
            g_impr.items[(size_t)g_impr.selected].decision = ImproveDecision::Rejected;
          }
        }
        impr_save();
        impr_refresh_listbox();
        impr_show_selected();
        ui_append_chat(L"Rael: Improvement rejected." );
        return 0;
      }
      if (id == 134) { // copy code
        std::wstring code;
        {
          std::lock_guard<std::mutex> lk(g_impr.mu);
          if (g_impr.selected >= 0 && g_impr.selected < (int)g_impr.items.size()) {
            code = widen(g_impr.items[(size_t)g_impr.selected].code);
          }
        }
        if (!code.empty() && clipboard_set_text(code)) {
          ui_append_chat(L"Rael: Code copied to clipboard." );
        } else {
          ui_append_chat(L"Rael: No code to copy." );
        }
        return 0;
      }
      if (id == 120 && notif == LBN_DBLCLK) { // palette double click -> add block
        int sel = (int)SendMessageW(g_ui.hPalette, LB_GETCURSEL, 0, 0);
        BlockType t = BlockType::Star8;
        switch (sel) {
          case 0: t = BlockType::Spiral; break;
          case 1: t = BlockType::Nozzle; break;
          case 2: t = BlockType::Divider; break;
          case 3: t = BlockType::Star8; break;
          case 4: t = BlockType::QuintMemory; break;
          case 5: t = BlockType::Shield; break;
          case 6: t = BlockType::Sensor; break;
        }
        builder_add(t);
        builder_update_stats();
        InvalidateRect(g_ui.hCanvas, nullptr, TRUE);
        return 0;
      }
      if (id == 103) { // send shell
        ui_send_shell();
        return 0;
      }
      if (id == 113) { // send chat
        ui_send_chat();
        return 0;
      }
      if (id == 110) { // arm
        ui_toggle_arm();
        return 0;
      }
      return 0;
    }

    case WM_NOTIFY: {
      LPNMHDR hdr = (LPNMHDR)lParam;
      if (hdr && hdr->hwndFrom == g_ui.hTab && hdr->code == TCN_SELCHANGE) {
        g_state.activeTab = TabCtrl_GetCurSel(g_ui.hTab);
        ui_apply_tab_visibility();
        builder_update_stats();
        InvalidateRect(g_ui.hCanvas, nullptr, TRUE);
      }
      return 0;
    }

    case WM_DESTROY:
      KillTimer(hwnd, kTimerPoll);
      child_close(g_core);
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow) {
  // ═══════════════════════════════════════════════════════════════════════════
  // MACHINE BINDING CHECK - Hardware-Lizenzprüfung beim Start
  // ═══════════════════════════════════════════════════════════════════════════
  {
    using namespace rael::machine;
    BindingStatus status = check_binding();

    switch (status) {
      case BindingStatus::NOT_BOUND: {
        // Erster Start - Binding-Bestätigung anfordern
        MachineFingerprint fp;
        fp.collect();

        std::wstring msg = L"═══════════════════════════════════════════════════\n";
        msg += L"  RAEL V49 — ERSTER START\n";
        msg += L"═══════════════════════════════════════════════════\n\n";
        msg += L"Diese Software wird beim ersten Start an Ihre\n";
        msg += L"Hardware gebunden (Machine-Binding).\n\n";
        msg += L"Computer: " + widen(fp.computer_name) + L"\n";
        msg += L"Benutzer: " + widen(fp.username) + L"\n";
        msg += L"Fingerprint: " + widen(fp.combined_hash.substr(0, 24)) + L"...\n\n";
        msg += L"Nach der Bindung kann die Software nur auf\n";
        msg += L"diesem Computer ausgeführt werden.\n\n";
        msg += L"Möchten Sie fortfahren und die Bindung durchführen?";

        int result = MessageBoxW(nullptr, msg.c_str(),
                                  L"RAEL V49 — Machine Binding",
                                  MB_YESNO | MB_ICONQUESTION);

        if (result != IDYES) {
          MessageBoxW(nullptr,
                       L"Bindung abgebrochen.\nDie Software wird beendet.",
                       L"RAEL V49", MB_OK | MB_ICONINFORMATION);
          return 0;
        }

        // Binding durchführen
        if (perform_first_binding()) {
          MessageBoxW(nullptr,
                       L"✓ Machine-Binding erfolgreich!\n\n"
                       L"Die Software ist jetzt an diese Hardware gebunden.\n"
                       L"RAEL Cockpit wird gestartet...",
                       L"RAEL V49 — Bindung Erfolgreich",
                       MB_OK | MB_ICONINFORMATION);
        } else {
          MessageBoxW(nullptr,
                       L"✗ Fehler bei der Bindung.\n\n"
                       L"Die Lizenzdatei konnte nicht erstellt werden.\n"
                       L"Bitte prüfen Sie die Schreibrechte.",
                       L"RAEL V49 — Fehler",
                       MB_OK | MB_ICONERROR);
          return 1;
        }
        break;
      }

      case BindingStatus::BOUND_VALID:
        // Alles OK - weiter zum Hauptfenster
        break;

      case BindingStatus::BOUND_INVALID: {
        MachineFingerprint fp;
        fp.collect();

        std::wstring msg = L"═══════════════════════════════════════════════════\n";
        msg += L"  RAEL V49 — HARDWARE MISMATCH\n";
        msg += L"═══════════════════════════════════════════════════\n\n";
        msg += L"Diese Software ist an eine andere Hardware gebunden.\n\n";
        msg += L"Aktuelle Hardware:\n";
        msg += L"  Computer: " + widen(fp.computer_name) + L"\n";
        msg += L"  Hash: " + widen(fp.combined_hash.substr(0, 16)) + L"...\n\n";
        msg += L"Die Ausführung ist auf diesem Computer nicht erlaubt.";

        MessageBoxW(nullptr, msg.c_str(),
                     L"RAEL V49 — Zugriff Verweigert",
                     MB_OK | MB_ICONERROR);
        return 1;
      }

      case BindingStatus::LICENSE_CORRUPTED: {
        MessageBoxW(nullptr,
                     L"Die Lizenzdatei ist beschädigt.\n\n"
                     L"Bitte löschen Sie die Datei und starten Sie neu:\n"
                     L"%LOCALAPPDATA%\\RAEL\\rael_license.dat",
                     L"RAEL V49 — Lizenz Beschädigt",
                     MB_OK | MB_ICONWARNING);
        return 1;
      }
    }
  }

  // ═══════════════════════════════════════════════════════════════════════════
  // GUI INITIALIZATION
  // ═══════════════════════════════════════════════════════════════════════════

  // Canvas class (BUILDER)
  {
    WNDCLASSEXW cc{};
    cc.cbSize = sizeof(cc);
    cc.style = CS_DBLCLKS;

// Face class (right pane)
{
  WNDCLASSEXW fc{};
  fc.cbSize = sizeof(fc);
  fc.style = CS_DBLCLKS;
  fc.lpfnWndProc = FaceProc;
  fc.hInstance = hInst;
  fc.lpszClassName = kFaceClass;
  fc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  fc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  RegisterClassExW(&fc);
}

    cc.lpfnWndProc = CanvasProc;
    cc.hInstance = hInst;
    cc.lpszClassName = kCanvasClass;
    cc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    cc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassExW(&cc);
  }

  WNDCLASSEXW wc{};
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInst;
  wc.lpszClassName = kWndClass;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  RegisterClassExW(&wc);

  HWND hwnd = CreateWindowExW(
      0,
      kWndClass,
      L"RAEL Cockpit (Windows)",
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 1200, 760,
      nullptr, nullptr, hInst, nullptr);

  if (!hwnd) return 0;
  ShowWindow(hwnd, nCmdShow);
  UpdateWindow(hwnd);

  MSG msg;
  while (GetMessageW(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return 0;
}