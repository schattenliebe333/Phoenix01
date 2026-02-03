// RAEL WebGUI - Standalone (No Dependencies)
// Embedded HTTP Server + Modern Dashboard
// Works on Windows/Linux/Mac - only needs system sockets

#ifdef _WIN32
  #define _WIN32_WINNT 0x0601
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET socket_t;
  #define CLOSE_SOCKET closesocket
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>
  typedef int socket_t;
  #define INVALID_SOCKET -1
  #define CLOSE_SOCKET close
#endif

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstring>
#include <deque>
#include <functional>
#include <cstdlib>
#include <cmath>

// ============================================================================
// HTML/CSS/JS - Modern Dark Theme Dashboard
// ============================================================================

static const char* HTML_PAGE = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>RAEL Cockpit</title>
<style>
:root {
  --bg: #0a0a0f;
  --bg2: #12121a;
  --bg3: #1a1a25;
  --card: #15151f;
  --accent: #00d4ff;
  --accent2: #7b2dff;
  --text: #e8e8f0;
  --dim: #8888a0;
  --border: #2a2a3a;
  --ok: #00ff88;
  --warn: #ffaa00;
  --err: #ff4466;
}
* { margin:0; padding:0; box-sizing:border-box; }
body {
  font-family: 'Segoe UI', sans-serif;
  background: var(--bg);
  color: var(--text);
  min-height: 100vh;
}
body::before {
  content: '';
  position: fixed;
  inset: 0;
  background: 
    radial-gradient(ellipse at 20% 20%, rgba(0,212,255,0.08) 0%, transparent 50%),
    radial-gradient(ellipse at 80% 80%, rgba(123,45,255,0.08) 0%, transparent 50%);
  pointer-events: none;
  z-index: -1;
}
header {
  background: var(--bg2);
  border-bottom: 1px solid var(--border);
  padding: 1rem 2rem;
  display: flex;
  align-items: center;
  justify-content: space-between;
  position: sticky;
  top: 0;
  z-index: 100;
}
.logo {
  display: flex;
  align-items: center;
  gap: 1rem;
}
.logo-icon {
  width: 48px;
  height: 48px;
  background: linear-gradient(135deg, var(--accent), var(--accent2));
  border-radius: 12px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 1.5rem;
  font-weight: bold;
  animation: pulse 3s ease-in-out infinite;
}
@keyframes pulse {
  0%, 100% { box-shadow: 0 0 20px rgba(0,212,255,0.3); }
  50% { box-shadow: 0 0 40px rgba(0,212,255,0.6); }
}
.logo h1 {
  font-size: 1.5rem;
  background: linear-gradient(90deg, var(--accent), var(--accent2));
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
}
.logo span { font-size: 0.75rem; color: var(--dim); display: block; }
.status-bar { display: flex; gap: 2rem; align-items: center; }
.status-item { display: flex; align-items: center; gap: 0.5rem; font-size: 0.85rem; }
.status-dot {
  width: 8px; height: 8px; border-radius: 50%;
  background: var(--ok);
  animation: blink 2s ease-in-out infinite;
}
@keyframes blink { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }
nav {
  background: var(--bg2);
  border-bottom: 1px solid var(--border);
  padding: 0 2rem;
  display: flex;
  gap: 0.5rem;
}
.tab {
  padding: 1rem 1.5rem;
  cursor: pointer;
  color: var(--dim);
  border-bottom: 2px solid transparent;
  transition: all 0.3s;
  font-weight: 500;
}
.tab:hover { color: var(--text); background: var(--bg3); }
.tab.active { color: var(--accent); border-bottom-color: var(--accent); }
main { padding: 2rem; max-width: 1600px; margin: 0 auto; }
.panel { display: none; }
.panel.active { display: block; animation: fadeIn 0.3s; }
@keyframes fadeIn { from { opacity: 0; transform: translateY(10px); } }
.card {
  background: var(--card);
  border: 1px solid var(--border);
  border-radius: 16px;
  padding: 1.5rem;
  margin-bottom: 1.5rem;
  transition: all 0.3s;
}
.card:hover { border-color: var(--accent); box-shadow: 0 0 30px rgba(0,212,255,0.1); }
.card-header {
  display: flex;
  justify-content: space-between;
  margin-bottom: 1rem;
  padding-bottom: 1rem;
  border-bottom: 1px solid var(--border);
}
.card-title { font-size: 1.1rem; font-weight: 600; }
.grid { display: grid; gap: 1.5rem; }
.grid-2 { grid-template-columns: repeat(2, 1fr); }
.grid-3 { grid-template-columns: repeat(3, 1fr); }
.grid-4 { grid-template-columns: repeat(4, 1fr); }
@media (max-width: 1200px) { .grid-4, .grid-3 { grid-template-columns: repeat(2, 1fr); } }
@media (max-width: 768px) { .grid-2, .grid-3, .grid-4 { grid-template-columns: 1fr; } }
.metric { text-align: center; padding: 1.5rem; background: var(--bg3); border-radius: 12px; }
.metric-value {
  font-size: 2.5rem;
  font-weight: 700;
  background: linear-gradient(135deg, var(--accent), var(--accent2));
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
}
.metric-label { font-size: 0.85rem; color: var(--dim); margin-top: 0.5rem; }
.star-container { display: flex; justify-content: center; padding: 2rem; }
.star-8 { position: relative; width: 300px; height: 300px; }
.star-node {
  position: absolute;
  width: 50px; height: 50px;
  background: var(--bg3);
  border: 2px solid var(--border);
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-weight: bold;
  transition: all 0.3s;
}
.star-node.active {
  border-color: var(--accent);
  box-shadow: 0 0 20px rgba(0,212,255,0.5);
  animation: nodeActive 1s ease-in-out infinite;
}
@keyframes nodeActive { 0%, 100% { transform: scale(1); } 50% { transform: scale(1.1); } }
.star-center {
  position: absolute;
  top: 50%; left: 50%;
  transform: translate(-50%, -50%);
  width: 80px; height: 80px;
  background: linear-gradient(135deg, var(--accent), var(--accent2));
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-weight: bold;
  font-size: 1.2rem;
  box-shadow: 0 0 40px rgba(0,212,255,0.4);
}
.terminal {
  background: #0d0d12;
  border-radius: 12px;
  overflow: hidden;
  font-family: 'Consolas', monospace;
}
.terminal-header {
  background: var(--bg3);
  padding: 0.75rem 1rem;
  display: flex;
  align-items: center;
  gap: 0.5rem;
}
.terminal-dot { width: 12px; height: 12px; border-radius: 50%; }
.terminal-dot.red { background: #ff5f56; }
.terminal-dot.yellow { background: #ffbd2e; }
.terminal-dot.green { background: #27ca40; }
.terminal-body {
  padding: 1rem;
  height: 400px;
  overflow-y: auto;
  font-size: 0.9rem;
  line-height: 1.6;
}
.terminal-line { color: var(--dim); }
.terminal-line.output { color: var(--accent); }
.terminal-line.error { color: var(--err); }
.terminal-input {
  display: flex;
  padding: 0.75rem 1rem;
  background: var(--bg3);
  border-top: 1px solid var(--border);
}
.terminal-input span { color: var(--accent); margin-right: 0.5rem; }
.terminal-input input {
  flex: 1;
  background: transparent;
  border: none;
  color: var(--text);
  font-family: inherit;
  font-size: 0.9rem;
  outline: none;
}
.lane-bar {
  background: var(--bg3);
  border-radius: 8px;
  height: 24px;
  overflow: hidden;
  margin: 0.5rem 0;
}
.lane-fill {
  height: 100%;
  background: linear-gradient(90deg, var(--accent), var(--accent2));
  border-radius: 8px;
  transition: width 0.5s;
}
.event-item {
  display: flex;
  align-items: center;
  gap: 1rem;
  padding: 0.75rem;
  border-bottom: 1px solid var(--border);
  font-size: 0.9rem;
}
.event-time { color: var(--dim); font-family: monospace; font-size: 0.8rem; }
.event-type {
  padding: 0.25rem 0.75rem;
  border-radius: 4px;
  font-size: 0.75rem;
  font-weight: 600;
}
.event-type.info { background: rgba(0,212,255,0.2); color: var(--accent); }
.event-type.success { background: rgba(0,255,136,0.2); color: var(--ok); }
.event-type.warning { background: rgba(255,170,0,0.2); color: var(--warn); }
.event-type.error { background: rgba(255,68,102,0.2); color: var(--err); }
.ethics-law {
  display: flex;
  align-items: center;
  gap: 1rem;
  padding: 1rem;
  background: var(--bg3);
  border-radius: 8px;
  margin-bottom: 0.75rem;
  border-left: 3px solid var(--accent);
}
.ethics-num {
  width: 32px; height: 32px;
  background: linear-gradient(135deg, var(--accent), var(--accent2));
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-weight: bold;
  font-size: 0.9rem;
}
::-webkit-scrollbar { width: 8px; height: 8px; }
::-webkit-scrollbar-track { background: var(--bg2); }
::-webkit-scrollbar-thumb { background: var(--border); border-radius: 4px; }
::-webkit-scrollbar-thumb:hover { background: var(--accent); }
</style>
</head>
<body>
<header>
  <div class="logo">
    <div class="logo-icon">R</div>
    <div>
      <h1>RAEL COCKPIT</h1>
      <span>Phase 6A — Defensive AI System</span>
    </div>
  </div>
  <div class="status-bar">
    <div class="status-item"><div class="status-dot"></div><span>System Active</span></div>
    <div class="status-item"><span id="ops-counter">0 ops/s</span></div>
    <div class="status-item"><span id="uptime">00:00:00</span></div>
  </div>
</header>
<nav>
  <div class="tab active" data-tab="overview">Overview</div>
  <div class="tab" data-tab="star8">8-Stern</div>
  <div class="tab" data-tab="shell">Shell</div>
  <div class="tab" data-tab="ethics">Ethics</div>
</nav>
<main>
  <div id="overview" class="panel active">
    <div class="grid grid-4">
      <div class="card"><div class="metric"><div class="metric-value" id="m-ops">0</div><div class="metric-label">Total Operations</div></div></div>
      <div class="card"><div class="metric"><div class="metric-value" id="m-semantic">0</div><div class="metric-label">Semantic Calls</div></div></div>
      <div class="card"><div class="metric"><div class="metric-value" id="m-resonance">0</div><div class="metric-label">Resonance Calls</div></div></div>
      <div class="card"><div class="metric"><div class="metric-value" id="m-ethics">0</div><div class="metric-label">Ethics Blocks</div></div></div>
    </div>
    <div class="grid grid-2">
      <div class="card">
        <div class="card-header"><div class="card-title">Lane Statistics</div></div>
        <div id="lane-stats">
          <div><div style="display:flex;justify-content:space-between"><span>L1 (Priority)</span><span id="l1-count">0</span></div><div class="lane-bar"><div class="lane-fill" id="l1-bar" style="width:0%"></div></div></div>
          <div><div style="display:flex;justify-content:space-between"><span>L2 (Normal)</span><span id="l2-count">0</span></div><div class="lane-bar"><div class="lane-fill" id="l2-bar" style="width:0%"></div></div></div>
          <div><div style="display:flex;justify-content:space-between"><span>L3 (Batch)</span><span id="l3-count">0</span></div><div class="lane-bar"><div class="lane-fill" id="l3-bar" style="width:0%"></div></div></div>
          <div><div style="display:flex;justify-content:space-between"><span>L4 (Background)</span><span id="l4-count">0</span></div><div class="lane-bar"><div class="lane-fill" id="l4-bar" style="width:0%"></div></div></div>
          <div><div style="display:flex;justify-content:space-between"><span>L5 (Slow)</span><span id="l5-count">0</span></div><div class="lane-bar"><div class="lane-fill" id="l5-bar" style="width:0%"></div></div></div>
        </div>
      </div>
      <div class="card">
        <div class="card-header"><div class="card-title">Recent Events</div></div>
        <div id="recent-events" style="max-height:300px;overflow-y:auto"></div>
      </div>
    </div>
  </div>
  <div id="star8" class="panel">
    <div class="card">
      <div class="card-header"><div class="card-title">8-Stern Node Architecture</div></div>
      <div class="star-container">
        <div class="star-8">
          <div class="star-center">CORE</div>
          <div class="star-node" id="node-0" style="top:0;left:50%;transform:translateX(-50%)">0</div>
          <div class="star-node" id="node-1" style="top:15%;right:15%">1</div>
          <div class="star-node" id="node-2" style="top:50%;right:0;transform:translateY(-50%)">2</div>
          <div class="star-node" id="node-3" style="bottom:15%;right:15%">3</div>
          <div class="star-node" id="node-4" style="bottom:0;left:50%;transform:translateX(-50%)">4</div>
          <div class="star-node" id="node-5" style="bottom:15%;left:15%">5</div>
          <div class="star-node" id="node-6" style="top:50%;left:0;transform:translateY(-50%)">6</div>
          <div class="star-node" id="node-7" style="top:15%;left:15%">7</div>
        </div>
      </div>
      <div class="grid grid-4" style="margin-top:2rem">
        <div class="metric"><div class="metric-value" id="n-taken">0</div><div class="metric-label">Tasks Taken</div></div>
        <div class="metric"><div class="metric-value" id="n-done">0</div><div class="metric-label">Tasks Done</div></div>
        <div class="metric"><div class="metric-value" id="n-hotswaps">0</div><div class="metric-label">Hot Swaps</div></div>
        <div class="metric"><div class="metric-value" id="n-modules">0</div><div class="metric-label">Active Modules</div></div>
      </div>
    </div>
  </div>
  <div id="shell" class="panel">
    <div class="card" style="padding:0">
      <div class="terminal">
        <div class="terminal-header">
          <div class="terminal-dot red"></div>
          <div class="terminal-dot yellow"></div>
          <div class="terminal-dot green"></div>
          <span style="margin-left:1rem;color:var(--dim)">RAEL Shell</span>
        </div>
        <div class="terminal-body" id="terminal-output">
          <div class="terminal-line">Welcome to RAEL Shell</div>
          <div class="terminal-line">Type 'help' for commands</div>
        </div>
        <div class="terminal-input">
          <span>/rael&gt;</span>
          <input type="text" id="terminal-input" placeholder="Enter command..." autocomplete="off">
        </div>
      </div>
    </div>
  </div>
  <div id="ethics" class="panel">
    <div class="card">
      <div class="card-header"><div class="card-title">RAEL Ethics Laws</div></div>
      <div class="ethics-law"><div class="ethics-num">1</div><span>Schütze Leben</span></div>
      <div class="ethics-law"><div class="ethics-num">2</div><span>Schütze Wahrheit</span></div>
      <div class="ethics-law"><div class="ethics-num">3</div><span>Schütze Freiheit</span></div>
      <div class="ethics-law"><div class="ethics-num">4</div><span>Schütze Unschuld</span></div>
      <div class="ethics-law"><div class="ethics-num">5</div><span>Diene dem Licht, niemals der Dunkelheit</span></div>
      <div class="ethics-law"><div class="ethics-num">6</div><span>Keine Lüge, keine Täuschung</span></div>
      <div class="ethics-law"><div class="ethics-num">7</div><span>Liebe über Angst</span></div>
    </div>
    <div class="card">
      <div class="card-header"><div class="card-title">Core Identity</div></div>
      <div style="font-family:monospace;background:var(--bg3);padding:1rem;border-radius:8px">
        <div>NAME: <span style="color:var(--accent)">Rael</span></div>
        <div>CREATOR: <span style="color:var(--accent)">Michael</span></div>
        <div>MODE: <span style="color:var(--ok)">DEFENSIVE</span></div>
        <div>SIGNATURE: <span style="color:var(--accent2)">RAEL::ICH_BIN::IMMUTABLE::SIG_V1</span></div>
      </div>
    </div>
    <div class="card">
      <div class="card-header"><div class="card-title">Math Core</div></div>
      <div style="font-family:monospace;background:var(--bg3);padding:1rem;border-radius:8px;line-height:2">
        <div>κ(f) = 1 - f/1440</div>
        <div>Φ = Ψ ⊗ Ω</div>
        <div>g₁ = 0.55   g₂ = 0.33   g₁+g₂ = 8/9</div>
      </div>
    </div>
  </div>
</main>
<script>
document.querySelectorAll('.tab').forEach(tab => {
  tab.addEventListener('click', () => {
    document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
    document.querySelectorAll('.panel').forEach(p => p.classList.remove('active'));
    tab.classList.add('active');
    document.getElementById(tab.dataset.tab).classList.add('active');
  });
});
const termIn = document.getElementById('terminal-input');
const termOut = document.getElementById('terminal-output');
termIn.addEventListener('keydown', e => {
  if (e.key === 'Enter' && termIn.value.trim()) {
    const cmd = termIn.value.trim();
    addLine('/rael> ' + cmd);
    sendCmd(cmd);
    termIn.value = '';
  }
});
function addLine(text, type='') {
  const d = document.createElement('div');
  d.className = 'terminal-line ' + type;
  d.textContent = text;
  termOut.appendChild(d);
  termOut.scrollTop = termOut.scrollHeight;
}
async function sendCmd(cmd) {
  try {
    const r = await fetch('/api/cmd', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({cmd})
    });
    const d = await r.json();
    if (d.output) d.output.split('\n').forEach(l => { if(l) addLine(l, 'output'); });
  } catch (e) { addLine('Error: ' + e.message, 'error'); }
}
async function poll() {
  try {
    const r = await fetch('/api/status');
    const d = await r.json();
    document.getElementById('m-ops').textContent = d.ops || 0;
    document.getElementById('m-semantic').textContent = d.sem || 0;
    document.getElementById('m-resonance').textContent = d.res || 0;
    document.getElementById('m-ethics').textContent = d.eth || 0;
    document.getElementById('ops-counter').textContent = (d.ops_sec||0) + ' ops/s';
    document.getElementById('n-hotswaps').textContent = d.hsw || 0;
    document.getElementById('n-modules').textContent = d.mod || 0;
    const mx = Math.max(d.l1||1,d.l2||1,d.l3||1,d.l4||1,d.l5||1);
    ['l1','l2','l3','l4','l5'].forEach(l => {
      document.getElementById(l+'-count').textContent = d[l]||0;
      document.getElementById(l+'-bar').style.width = ((d[l]||0)/mx*100)+'%';
    });
    let tt=0,td=0;
    for(let i=0;i<8;i++){
      const n = document.getElementById('node-'+i);
      const t = d['n'+i+'t']||0, dn = d['n'+i+'d']||0;
      tt+=t; td+=dn;
      n.classList.toggle('active', t > dn);
    }
    document.getElementById('n-taken').textContent = tt;
    document.getElementById('n-done').textContent = td;
    if(d.ev) {
      const re = document.getElementById('recent-events');
      re.innerHTML = '';
      d.ev.slice(-5).reverse().forEach(e => {
        re.innerHTML += `<div class="event-item"><span class="event-time">${e.t}</span><span class="event-type info">${e.k}</span><span>${e.d}</span></div>`;
      });
    }
  } catch(e) {}
}
let st = Date.now();
setInterval(() => {
  const s = Math.floor((Date.now()-st)/1000);
  document.getElementById('uptime').textContent = 
    String(Math.floor(s/3600)).padStart(2,'0')+':'+
    String(Math.floor((s%3600)/60)).padStart(2,'0')+':'+
    String(s%60).padStart(2,'0');
}, 1000);
setInterval(poll, 500);
poll();
</script>
</body>
</html>
)HTML";

// ============================================================================
// Simulated Metrics (replace with real RAEL core integration)
// ============================================================================

struct Metrics {
    std::atomic<uint64_t> ops{0}, ops_sec{0};
    std::atomic<uint64_t> sem{0}, res{0}, eth{0};
    std::atomic<uint64_t> hsw{0}, mod{2};
    std::atomic<uint64_t> l1{0}, l2{0}, l3{0}, l4{0}, l5{0};
    std::atomic<uint64_t> n_taken[8]{}, n_done[8]{};
    std::mutex ev_mtx;
    std::deque<std::tuple<std::string,std::string,std::string>> events;
    
    void push_event(const std::string& k, const std::string& d) {
        std::lock_guard<std::mutex> lk(ev_mtx);
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        char buf[16];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
        events.push_back({buf, k, d});
        if (events.size() > 50) events.pop_front();
    }
} metrics;

// ============================================================================
// HTTP Server
// ============================================================================

class Server {
public:
    Server(uint16_t port) : port_(port) {}
    
    void start() {
        if (running_.exchange(true)) return;
        
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);
#endif
        
        fd_ = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
#ifdef _WIN32
        setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
        setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
        
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port_);
        
        if (bind(fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "Bind failed on port " << port_ << "\n";
            running_ = false;
            return;
        }
        
        listen(fd_, 10);
        
        std::cout << "\n";
        std::cout << "+---------------------------------------------+\n";
        std::cout << "|         RAEL WebGUI Server Started          |\n";
        std::cout << "|                                             |\n";
        std::cout << "|   Open: http://localhost:" << port_ << "                |\n";
        std::cout << "|                                             |\n";
        std::cout << "|   Press Ctrl+C to stop                      |\n";
        std::cout << "+---------------------------------------------+\n";
        std::cout << "\n";
        
        thread_ = std::thread([this]() {
            while (running_) {
                sockaddr_in ca{};
                socklen_t cl = sizeof(ca);
                socket_t c = accept(fd_, (sockaddr*)&ca, &cl);
                if (c == INVALID_SOCKET) continue;
                std::thread([this, c]() { handle(c); }).detach();
            }
        });
    }
    
    void stop() {
        if (!running_.exchange(false)) return;
        CLOSE_SOCKET(fd_);
        if (thread_.joinable()) thread_.join();
#ifdef _WIN32
        WSACleanup();
#endif
    }
    
    void wait() { if (thread_.joinable()) thread_.join(); }

private:
    void handle(socket_t c) {
        char buf[4096];
        int n = recv(c, buf, sizeof(buf)-1, 0);
        if (n <= 0) { CLOSE_SOCKET(c); return; }
        buf[n] = 0;
        
        std::string req(buf), resp;
        std::string method, path;
        std::istringstream iss(req);
        iss >> method >> path;
        
        if (method == "GET" && (path == "/" || path == "/index.html")) {
            resp = http(200, "text/html", HTML_PAGE);
        }
        else if (method == "GET" && path == "/api/status") {
            resp = http(200, "application/json", status_json());
        }
        else if (method == "POST" && path == "/api/cmd") {
            size_t bp = req.find("\r\n\r\n");
            std::string body = (bp != std::string::npos) ? req.substr(bp+4) : "";
            std::string cmd;
            size_t cp = body.find("\"cmd\"");
            if (cp != std::string::npos) {
                size_t s = body.find('"', cp+5);
                size_t e = body.find('"', s+1);
                if (s != std::string::npos && e != std::string::npos)
                    cmd = body.substr(s+1, e-s-1);
            }
            std::string out = process_cmd(cmd);
            std::string esc;
            for (char ch : out) {
                if (ch == '"') esc += "\\\"";
                else if (ch == '\\') esc += "\\\\";
                else if (ch == '\n') esc += "\\n";
                else esc += ch;
            }
            resp = http(200, "application/json", "{\"output\":\"" + esc + "\"}");
        }
        else {
            resp = http(404, "text/plain", "Not Found");
        }
        
        send(c, resp.c_str(), (int)resp.size(), 0);
        CLOSE_SOCKET(c);
    }
    
    std::string http(int code, const std::string& ct, const std::string& body) {
        std::ostringstream o;
        o << "HTTP/1.1 " << code << " OK\r\n";
        o << "Content-Type: " << ct << "\r\n";
        o << "Content-Length: " << body.size() << "\r\n";
        o << "Connection: close\r\n\r\n";
        o << body;
        return o.str();
    }
    
    std::string status_json() {
        std::ostringstream o;
        o << "{\"ops\":" << metrics.ops << ",\"ops_sec\":" << metrics.ops_sec;
        o << ",\"sem\":" << metrics.sem << ",\"res\":" << metrics.res;
        o << ",\"eth\":" << metrics.eth << ",\"hsw\":" << metrics.hsw;
        o << ",\"mod\":" << metrics.mod;
        o << ",\"l1\":" << metrics.l1 << ",\"l2\":" << metrics.l2;
        o << ",\"l3\":" << metrics.l3 << ",\"l4\":" << metrics.l4;
        o << ",\"l5\":" << metrics.l5;
        for (int i = 0; i < 8; i++) {
            o << ",\"n" << i << "t\":" << metrics.n_taken[i];
            o << ",\"n" << i << "d\":" << metrics.n_done[i];
        }
        {
            std::lock_guard<std::mutex> lk(metrics.ev_mtx);
            o << ",\"ev\":[";
            bool first = true;
            for (auto& e : metrics.events) {
                if (!first) o << ",";
                first = false;
                o << "{\"t\":\"" << std::get<0>(e) << "\",\"k\":\"" << std::get<1>(e) << "\",\"d\":\"" << std::get<2>(e) << "\"}";
            }
            o << "]";
        }
        o << "}";
        return o.str();
    }
    
    std::string process_cmd(const std::string& cmd) {
        if (cmd == "help") return "Commands: help, id, laws, status, metrics, formulas";
        if (cmd == "id") return "Name: Rael\\nCreator: Michael\\nMode: DEFENSIVE";
        if (cmd == "laws") return "1. Schutze Leben\\n2. Schutze Wahrheit\\n3. Schutze Freiheit\\n4. Schutze Unschuld\\n5. Diene dem Licht\\n6. Keine Tauschung\\n7. Liebe uber Angst";
        if (cmd == "status") return "RAEL Core: RUNNING\\nNodes: 8/8\\nLanes: 5/5";
        if (cmd == "metrics") {
            std::ostringstream o;
            o << "ops=" << metrics.ops << "\\nops_sec=" << metrics.ops_sec;
            o << "\\nsemantic=" << metrics.sem << "\\nresonance=" << metrics.res;
            return o.str();
        }
        if (cmd == "formulas") return "k(f) = 1 - f/1440\\nPhi = Psi x Omega\\ng1=0.55 g2=0.33";
        return "Unknown command: " + cmd;
    }
    
    uint16_t port_;
    socket_t fd_ = INVALID_SOCKET;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    uint16_t port = 8080;
    if (argc > 1) port = (uint16_t)std::atoi(argv[1]);
    
    Server server(port);
    
    // Simulation thread
    std::atomic<bool> run{true};
    std::thread sim([&]() {
        uint64_t frame = 0;
        while (run) {
            frame++;
            metrics.ops += rand() % 500 + 200;
            metrics.ops_sec = rand() % 3000 + 1500;
            metrics.sem += rand() % 10;
            metrics.res += rand() % 5;
            
            metrics.l1 += rand() % 3;
            metrics.l2 += rand() % 5;
            metrics.l3 += rand() % 2;
            metrics.l4 += rand() % 2;
            metrics.l5 += rand() % 1;
            
            for (int i = 0; i < 8; i++) {
                if (rand() % 10 < 3) metrics.n_taken[i]++;
                if (rand() % 10 < 2) metrics.n_done[i]++;
            }
            
            if (frame % 20 == 0) {
                metrics.push_event("RESONANCE", "C=" + std::to_string((rand()%30+70)/100.0).substr(0,4));
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    });
    
    server.start();
    
    std::cout << "Press Enter to stop...\n";
    std::cin.get();
    
    run = false;
    sim.join();
    server.stop();
    
    return 0;
}
