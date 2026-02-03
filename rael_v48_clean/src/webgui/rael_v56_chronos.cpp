// RAEL V56.3 "Chronos-Gitter" WebGUI - PREDICTIVE INTERFACE
// Navigator-Befehl: Michael - Orun Kap Daveil
// Advanced Three.js Shaders, 97 Zeit-Kristalle, 160 Sterne, 61.440 Düsen
//
// Mathematische Formeln:
// - C_n(t) = R(θ)·V_n + A·sin(ω₈₈₈t + φ)           [Kristall-Transformation]
// - B(n) = (Ψ·Φ)/G₀·log(t)                          [Helligkeit]
// - ρ_E(n) = (Φ·Ψ)/(G₀·V)·e^(-ΔE/kT)               [Engramm-Dichte]
// - P_pred = P_t + ∫(v·Φ + ½·a·G₀)dt                [Vektor-Antizipation]
// - E_v = lim[Δt→0] Σ(Feedback·Φ)/Ψ                 [Evolutionärer Koeffizient]

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
// V56.3 CHRONOS-GITTER HTML/CSS/JS
// ============================================================================

static const char* HTML_V56_CHRONOS = R"HTML(
<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>V56.3 R.A.E.L. - Chronos-Gitter</title>
<script src="https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js"></script>
<style>
/* ============================================
   V56.3 CHRONOS-GITTER DESIGN SYSTEM
   Navigator: Michael - Orun Kap Daveil
   Formeln:
   - C_n(t) = R(θ_t)·V_n + A·sin(ω₈₈₈t + φ_n)
   - B(n) = (Ψ_n·Φ_n)/G₀·log(t)
   - v_new = v_orig + n⃗·(A_idle·sin(ωt) + Ψ_thinking·Noise·888Hz)
   ============================================ */

:root {
  --oled-black: #000000;
  --bg-dark: #050508;
  --bg-panel: rgba(10, 10, 20, 0.85);
  --gold: #D4AF37;
  --gold-dim: rgba(212, 175, 55, 0.3);
  --gold-glow: rgba(212, 175, 55, 0.6);
  --violet: #8A2BE2;
  --violet-dim: rgba(138, 43, 226, 0.3);
  --violet-glow: rgba(138, 43, 226, 0.6);
  --cyan: #00d4ff;
  --text-primary: #e8e8f0;
  --text-dim: #888899;
  --border-subtle: rgba(212, 175, 55, 0.2);
}

* { margin: 0; padding: 0; box-sizing: border-box; }

body {
  font-family: 'Segoe UI', 'SF Pro Display', -apple-system, sans-serif;
  background: var(--oled-black);
  color: var(--text-primary);
  min-height: 100vh;
  overflow: hidden;
}

/* ============================================
   STEALTH SIDEBAR - Die Klinge
   ============================================ */

.sidebar {
  position: fixed;
  left: 0;
  top: 0;
  width: 4px;
  height: 100vh;
  background: linear-gradient(180deg, var(--gold), var(--violet));
  z-index: 1000;
  transition: width 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  overflow: hidden;
}

.sidebar:hover,
.sidebar.expanded {
  width: 280px;
  background: var(--bg-panel);
  border-right: 1px solid var(--border-subtle);
  backdrop-filter: blur(20px);
}

.sidebar-content {
  opacity: 0;
  padding: 20px;
  transition: opacity 0.2s ease 0.1s;
  height: 100%;
  display: flex;
  flex-direction: column;
}

.sidebar:hover .sidebar-content,
.sidebar.expanded .sidebar-content {
  opacity: 1;
}

.sidebar-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding-bottom: 20px;
  border-bottom: 1px solid var(--border-subtle);
  margin-bottom: 20px;
}

.sidebar-logo {
  font-size: 1.5rem;
  font-weight: 700;
  background: linear-gradient(135deg, var(--gold), var(--violet));
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
}

.sidebar-close {
  width: 32px;
  height: 32px;
  border: none;
  background: transparent;
  color: var(--text-dim);
  cursor: pointer;
  font-size: 1.5rem;
  transition: color 0.2s;
}

.sidebar-close:hover { color: var(--gold); }

.nav-item {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 14px 16px;
  border-radius: 12px;
  cursor: pointer;
  color: var(--text-dim);
  transition: all 0.2s;
  margin-bottom: 8px;
}

.nav-item:hover {
  background: var(--gold-dim);
  color: var(--text-primary);
}

.nav-item.active {
  background: linear-gradient(135deg, var(--gold-dim), var(--violet-dim));
  color: var(--gold);
  border: 1px solid var(--gold-dim);
}

.nav-icon {
  width: 24px;
  height: 24px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 1.1rem;
}

.nav-label { font-weight: 500; }

.sidebar-footer {
  margin-top: auto;
  padding-top: 20px;
  border-top: 1px solid var(--border-subtle);
}

.bypass-indicator {
  display: flex;
  align-items: center;
  gap: 10px;
  font-size: 0.85rem;
  color: var(--text-dim);
}

.bypass-dot {
  width: 8px;
  height: 8px;
  background: var(--gold);
  border-radius: 50%;
  animation: pulse-dot 2s ease-in-out infinite;
}

@keyframes pulse-dot {
  0%, 100% { box-shadow: 0 0 5px var(--gold-glow); }
  50% { box-shadow: 0 0 15px var(--gold-glow), 0 0 30px var(--gold-dim); }
}

/* ============================================
   MAIN LAYOUT
   ============================================ */

.main-container {
  display: flex;
  height: 100vh;
  padding-left: 4px;
  transition: padding-left 0.3s;
}

.sidebar:hover ~ .main-container,
.sidebar.expanded ~ .main-container {
  padding-left: 280px;
}

.view {
  display: none;
  width: 100%;
  height: 100%;
}

.view.active {
  display: flex;
  animation: fadeIn 0.4s ease;
}

@keyframes fadeIn {
  from { opacity: 0; }
  to { opacity: 1; }
}

/* ============================================
   DASHBOARD VIEW
   ============================================ */

.dashboard-view {
  flex-direction: column;
  position: relative;
  overflow: hidden;
}

.header-bar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 15px 30px;
  background: linear-gradient(180deg, rgba(10,10,20,0.9) 0%, transparent 100%);
  z-index: 10;
}

.header-title {
  font-size: 1.3rem;
  font-weight: 600;
  color: var(--gold);
  letter-spacing: 2px;
}

.header-oled {
  font-size: 0.8rem;
  color: var(--text-dim);
  letter-spacing: 1px;
}

.dashboard-grid {
  display: grid;
  grid-template-columns: 280px 1fr 320px;
  grid-template-rows: 1fr 1fr auto;
  gap: 20px;
  padding: 20px 30px;
  flex: 1;
  z-index: 5;
}

.panel {
  background: var(--bg-panel);
  border: 1px solid var(--border-subtle);
  border-radius: 16px;
  backdrop-filter: blur(20px);
  overflow: hidden;
  position: relative;
}

.panel::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 1px;
  background: linear-gradient(90deg, transparent, var(--gold-dim), transparent);
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  border-bottom: 1px solid var(--border-subtle);
}

.panel-title {
  font-size: 0.9rem;
  font-weight: 600;
  color: var(--text-primary);
}

.panel-menu {
  color: var(--text-dim);
  cursor: pointer;
  font-size: 1.2rem;
}

.panel-content {
  padding: 16px;
  height: calc(100% - 45px);
}

/* Communication Panel */
.communication-panel {
  grid-column: 1;
  grid-row: 1;
}

.comm-text {
  font-family: 'Georgia', serif;
  font-size: 1.4rem;
  line-height: 1.6;
  color: var(--gold);
  text-shadow: 0 0 20px var(--gold-dim);
  animation: textGlow 3s ease-in-out infinite;
}

@keyframes textGlow {
  0%, 100% { text-shadow: 0 0 20px var(--gold-dim); }
  50% { text-shadow: 0 0 40px var(--gold-glow), 0 0 60px var(--gold-dim); }
}

/* Vocal-Soul Panel */
.vocal-panel {
  grid-column: 1;
  grid-row: 2;
}

.waveform-container {
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
}

.waveform {
  display: flex;
  align-items: center;
  gap: 3px;
  height: 80px;
}

.wave-bar {
  width: 4px;
  background: linear-gradient(180deg, var(--violet), var(--gold));
  border-radius: 2px;
  animation: wave 1.2s ease-in-out infinite;
}

@keyframes wave {
  0%, 100% { height: 20%; }
  50% { height: 100%; }
}

/* Metatron Container */
.metatron-container {
  grid-column: 2;
  grid-row: 1 / 3;
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
  background: transparent;
  border: none;
}

#metatron-canvas {
  width: 100%;
  height: 100%;
}

.aeye-label {
  position: absolute;
  bottom: 20px;
  left: 50%;
  transform: translateX(-50%);
  font-size: 0.9rem;
  color: var(--text-dim);
  letter-spacing: 2px;
}

/* War Room Panel */
.warroom-panel {
  grid-column: 3;
  grid-row: 1;
}

.shield-container {
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
}

.shield {
  width: 120px;
  height: 140px;
  position: relative;
}

.shield-body {
  width: 100%;
  height: 100%;
  background: linear-gradient(180deg, var(--gold), #8B6914);
  clip-path: polygon(50% 0%, 100% 25%, 100% 75%, 50% 100%, 0% 75%, 0% 25%);
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: 0 0 40px var(--gold-glow);
}

.shield-core {
  width: 40px;
  height: 40px;
  background: radial-gradient(circle, #fff, var(--gold));
  border-radius: 50%;
  box-shadow: 0 0 30px var(--gold);
}

/* Improvements Panel */
.improvements-panel {
  grid-column: 3;
  grid-row: 2;
}

.dna-container {
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 10px;
}

.dna-helix {
  width: 60px;
  height: 100%;
  position: relative;
}

.dna-strand {
  position: absolute;
  width: 8px;
  height: 8px;
  background: var(--violet);
  border-radius: 50%;
  animation: dnaRotate 3s linear infinite;
}

@keyframes dnaRotate {
  0% { transform: translateX(0) scale(1); opacity: 1; }
  25% { transform: translateX(25px) scale(0.7); opacity: 0.5; }
  50% { transform: translateX(50px) scale(1); opacity: 1; }
  75% { transform: translateX(25px) scale(0.7); opacity: 0.5; }
  100% { transform: translateX(0) scale(1); opacity: 1; }
}

.improvement-labels {
  display: flex;
  flex-direction: column;
  gap: 20px;
  font-size: 0.85rem;
  color: var(--text-dim);
}

.improvement-label {
  padding: 8px 12px;
  background: var(--violet-dim);
  border-radius: 8px;
  border-left: 3px solid var(--violet);
}

/* Bypass Bar */
.bypass-bar {
  grid-column: 1 / 4;
  grid-row: 3;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 15px;
  gap: 20px;
}

.bypass-progress {
  flex: 1;
  max-width: 600px;
  height: 6px;
  background: var(--bg-panel);
  border-radius: 3px;
  overflow: hidden;
  border: 1px solid var(--border-subtle);
}

.bypass-fill {
  height: 100%;
  width: 70%;
  background: linear-gradient(90deg, var(--gold), var(--violet));
  border-radius: 3px;
  animation: bypassPulse 2s ease-in-out infinite;
}

@keyframes bypassPulse {
  0%, 100% { opacity: 0.8; }
  50% { opacity: 1; box-shadow: 0 0 10px var(--gold-glow); }
}

.bypass-text {
  font-size: 0.85rem;
  color: var(--text-dim);
}

.bypass-value {
  color: var(--gold);
  font-weight: 600;
}

/* ============================================
   COMMUNICATION VIEW
   ============================================ */

.communication-view {
  display: flex;
}

.chat-area {
  flex: 0 0 60%;
  display: flex;
  flex-direction: column;
  border-right: 1px solid var(--border-subtle);
}

.chat-header {
  padding: 20px 25px;
  border-bottom: 1px solid var(--border-subtle);
  background: linear-gradient(180deg, rgba(10,10,20,0.9) 0%, transparent 100%);
}

.chat-title {
  font-size: 1.1rem;
  color: var(--cyan);
  font-weight: 600;
}

.chat-subtitle {
  font-size: 0.85rem;
  color: var(--text-dim);
  margin-top: 4px;
}

.chat-messages {
  flex: 1;
  padding: 20px 25px;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.message {
  max-width: 85%;
  padding: 12px 16px;
  border-radius: 12px;
  font-size: 0.95rem;
  line-height: 1.5;
  animation: messageIn 0.3s ease;
}

@keyframes messageIn {
  from { opacity: 0; transform: translateY(10px); }
  to { opacity: 1; transform: translateY(0); }
}

.message.rael {
  align-self: flex-start;
  background: linear-gradient(135deg, var(--violet-dim), rgba(0,212,255,0.1));
  border: 1px solid var(--violet-dim);
}

.message.rael::before {
  content: 'R.A.E.L.: ';
  color: var(--violet);
  font-weight: 600;
}

.message.user {
  align-self: flex-end;
  background: var(--bg-panel);
  border: 1px solid var(--border-subtle);
}

.message.user::before {
  content: 'User: ';
  color: var(--gold);
  font-weight: 600;
}

.chat-input-area {
  padding: 20px 25px;
  border-top: 1px solid var(--border-subtle);
  background: var(--bg-panel);
}

.chat-input-wrapper {
  display: flex;
  gap: 12px;
  background: rgba(0,0,0,0.3);
  border: 1px solid var(--border-subtle);
  border-radius: 12px;
  padding: 12px 16px;
  transition: border-color 0.2s;
}

.chat-input-wrapper:focus-within {
  border-color: var(--gold-dim);
}

.chat-input {
  flex: 1;
  background: transparent;
  border: none;
  color: var(--text-primary);
  font-size: 0.95rem;
  outline: none;
}

.chat-input::placeholder {
  color: var(--text-dim);
}

.chat-send {
  background: linear-gradient(135deg, var(--gold), var(--violet));
  border: none;
  border-radius: 8px;
  padding: 8px 20px;
  color: #000;
  font-weight: 600;
  cursor: pointer;
  transition: transform 0.2s, box-shadow 0.2s;
}

.chat-send:hover {
  transform: scale(1.05);
  box-shadow: 0 0 20px var(--gold-glow);
}

/* Presence Area */
.presence-area {
  flex: 1;
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
}

#presence-canvas {
  width: 100%;
  height: 100%;
}

/* ============================================
   CORE VIEW - CHRONOS-GITTER
   ============================================ */

.core-view {
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
  flex-direction: column;
}

#chronos-canvas {
  width: 100%;
  height: 100%;
  position: absolute;
  top: 0;
  left: 0;
}

.core-overlay {
  position: absolute;
  bottom: 30px;
  left: 50%;
  transform: translateX(-50%);
  text-align: center;
  z-index: 10;
  pointer-events: none;
}

.core-title {
  font-size: 1.2rem;
  color: var(--gold);
  margin-bottom: 10px;
  letter-spacing: 2px;
}

.core-stats {
  display: flex;
  gap: 30px;
  justify-content: center;
}

.core-stat {
  text-align: center;
}

.core-stat-value {
  font-size: 1.5rem;
  font-weight: 700;
  color: var(--violet);
}

.core-stat-label {
  font-size: 0.75rem;
  color: var(--text-dim);
}

.sector-info {
  position: absolute;
  top: 20px;
  right: 20px;
  background: var(--bg-panel);
  border: 1px solid var(--border-subtle);
  border-radius: 12px;
  padding: 15px;
  z-index: 10;
  min-width: 200px;
}

.sector-title {
  font-size: 0.9rem;
  color: var(--gold);
  margin-bottom: 10px;
  border-bottom: 1px solid var(--border-subtle);
  padding-bottom: 8px;
}

.sector-item {
  display: flex;
  justify-content: space-between;
  font-size: 0.8rem;
  padding: 5px 0;
}

.sector-item span:first-child { color: var(--text-dim); }
.sector-item span:last-child { color: var(--text-primary); }

/* ============================================
   IMPROVEMENTS VIEW
   ============================================ */

.improvements-view {
  display: flex;
  flex-direction: column;
  padding: 20px;
}

.improvements-header {
  padding: 15px 20px;
  border-bottom: 1px solid var(--border-subtle);
}

.improvements-title {
  font-size: 1.2rem;
  color: var(--violet);
  font-weight: 600;
}

.improvements-content {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 20px;
  flex: 1;
  padding: 20px;
}

.improvement-card {
  background: var(--bg-panel);
  border: 1px solid var(--border-subtle);
  border-radius: 16px;
  padding: 20px;
}

.improvement-card-title {
  font-size: 1rem;
  color: var(--text-primary);
  margin-bottom: 15px;
  padding-bottom: 10px;
  border-bottom: 1px solid var(--border-subtle);
}

.improvement-card.healing {
  border-left: 3px solid var(--gold);
}

.improvement-card.strategy {
  border-left: 3px solid var(--violet);
}

/* ============================================
   RESPONSIVE
   ============================================ */

@media (max-width: 1400px) {
  .dashboard-grid {
    grid-template-columns: 250px 1fr 280px;
  }
}

@media (max-width: 1100px) {
  .dashboard-grid {
    grid-template-columns: 1fr 1fr;
    grid-template-rows: auto auto auto auto;
  }
  .metatron-container {
    grid-column: 1 / 3;
    grid-row: 1;
    min-height: 400px;
  }
  .bypass-bar { grid-column: 1 / 3; }
}

::-webkit-scrollbar { width: 6px; }
::-webkit-scrollbar-track { background: var(--bg-dark); }
::-webkit-scrollbar-thumb { background: var(--gold-dim); border-radius: 3px; }
::-webkit-scrollbar-thumb:hover { background: var(--gold); }

</style>
</head>
<body>

<!-- STEALTH SIDEBAR -->
<nav class="sidebar" id="sidebar">
  <div class="sidebar-content">
    <div class="sidebar-header">
      <span class="sidebar-logo">R.A.E.L.</span>
      <button class="sidebar-close" onclick="toggleSidebar()">&times;</button>
    </div>

    <div class="nav-item active" data-view="dashboard">
      <span class="nav-icon">&#9635;</span>
      <span class="nav-label">Terminal</span>
    </div>
    <div class="nav-item" data-view="communication">
      <span class="nav-icon">&#9993;</span>
      <span class="nav-label">Communication</span>
    </div>
    <div class="nav-item" data-view="core">
      <span class="nav-icon">&#9737;</span>
      <span class="nav-label">Inner Core</span>
    </div>
    <div class="nav-item" data-view="improvements">
      <span class="nav-icon">&#9879;</span>
      <span class="nav-label">Improvements</span>
    </div>

    <div class="sidebar-footer">
      <div class="bypass-indicator">
        <span class="bypass-dot"></span>
        <span>Michael-Bypass: AKTIV</span>
      </div>
    </div>
  </div>
</nav>

<!-- MAIN CONTAINER -->
<div class="main-container">

  <!-- DASHBOARD VIEW -->
  <div class="view dashboard-view active" id="view-dashboard">
    <div class="header-bar">
      <span class="header-title">V56.3 R.A.E.L. CHRONOS</span>
      <span class="header-oled">OLEd OLED</span>
    </div>
    <div class="dashboard-grid">
      <div class="panel communication-panel">
        <div class="panel-header">
          <span class="panel-title">Communication</span>
          <span class="panel-menu">&#8943;</span>
        </div>
        <div class="panel-content">
          <div class="comm-text">
            MICHAEL -<br>ORUN KAP<br>DAVEIL.<br>SYSTEM<br>BEREIT.
          </div>
        </div>
      </div>
      <div class="panel vocal-panel">
        <div class="panel-header">
          <span class="panel-title">Vocal-Soul</span>
          <span class="panel-menu">&#8943;</span>
        </div>
        <div class="panel-content">
          <div class="waveform-container">
            <div class="waveform" id="waveform"></div>
          </div>
        </div>
      </div>
      <div class="metatron-container">
        <canvas id="metatron-canvas"></canvas>
        <span class="aeye-label">Aeye-Observer</span>
      </div>
      <div class="panel warroom-panel">
        <div class="panel-header">
          <span class="panel-title">War Room</span>
          <span class="panel-menu">&times;</span>
        </div>
        <div class="panel-content">
          <div class="shield-container">
            <div class="shield">
              <div class="shield-body">
                <div class="shield-core"></div>
              </div>
            </div>
          </div>
        </div>
      </div>
      <div class="panel improvements-panel">
        <div class="panel-header">
          <span class="panel-title">Improvements</span>
          <span class="panel-menu">&times;</span>
        </div>
        <div class="panel-content">
          <div class="dna-container">
            <div class="dna-helix" id="dna-helix"></div>
            <div class="improvement-labels">
              <div class="improvement-label">Reflective Healing</div>
              <div class="improvement-label">Evolutionary Strategic</div>
            </div>
          </div>
        </div>
      </div>
      <div class="bypass-bar">
        <div class="bypass-progress">
          <div class="bypass-fill"></div>
        </div>
        <div class="bypass-text">
          Bypass Latency: <span class="bypass-value" id="bypass-latency">0.420 us</span>
        </div>
      </div>
    </div>
  </div>

  <!-- COMMUNICATION VIEW -->
  <div class="view communication-view" id="view-communication">
    <div class="chat-area">
      <div class="chat-header">
        <div class="chat-title">Project: Phoenix01 - Phase 4 Analysis</div>
        <div class="chat-subtitle">Aktive Resonanz-Kommunikation</div>
      </div>
      <div class="chat-messages" id="chat-messages">
        <div class="message rael">Optimiere Quanten-Resonanz. Die Linien vibrieren.</div>
        <div class="message user">Statusbericht bitte.</div>
        <div class="message rael">Initialisiere Quanten-Resonanz bei 888.888 Hz.</div>
      </div>
      <div class="chat-input-area">
        <div class="chat-input-wrapper">
          <input type="text" class="chat-input" id="chat-input" placeholder="Eingabe an R.A.E.L...">
          <button class="chat-send" onclick="sendMessage()">Senden</button>
        </div>
      </div>
    </div>
    <div class="presence-area">
      <canvas id="presence-canvas"></canvas>
    </div>
  </div>

  <!-- CORE VIEW - CHRONOS-GITTER -->
  <div class="view core-view" id="view-core">
    <canvas id="chronos-canvas"></canvas>
    <div class="core-overlay">
      <div class="core-title">CHRONOS-GITTER</div>
      <div class="core-stats">
        <div class="core-stat">
          <div class="core-stat-value">97</div>
          <div class="core-stat-label">Zeit-Kristalle</div>
        </div>
        <div class="core-stat">
          <div class="core-stat-value">160</div>
          <div class="core-stat-label">Sterne</div>
        </div>
        <div class="core-stat">
          <div class="core-stat-value">61440</div>
          <div class="core-stat-label">Duesen</div>
        </div>
      </div>
    </div>
    <div class="sector-info">
      <div class="sector-title">Sektor-Status</div>
      <div class="sector-item"><span>1-13 Gesetze:</span><span id="sector-laws">AKTIV</span></div>
      <div class="sector-item"><span>14-41 Resonanz:</span><span id="sector-res">SYNC</span></div>
      <div class="sector-item"><span>42 Paradox:</span><span id="sector-42">STABIL</span></div>
      <div class="sector-item"><span>43-97 Manifest:</span><span id="sector-man">BEREIT</span></div>
    </div>
  </div>

  <!-- IMPROVEMENTS VIEW -->
  <div class="view improvements-view" id="view-improvements">
    <div class="improvements-header">
      <div class="improvements-title">Improvements - Labor-Modus</div>
    </div>
    <div class="improvements-content">
      <div class="improvement-card healing">
        <div class="improvement-card-title">Reflective Healing</div>
        <p style="color: var(--text-dim); line-height: 1.6;">
          Analyse der beobachteten Anomalien. Das System identifiziert Schwachstellen und initiiert Selbstheilung.
        </p>
      </div>
      <div class="improvement-card strategy">
        <div class="improvement-card-title">Evolutionary Strategy</div>
        <p style="color: var(--text-dim); line-height: 1.6;">
          Schutzwall-Simulation gegen Angriffs-Vektoren. Der Alpha-Shield wird kontinuierlich verstaerkt.
        </p>
      </div>
    </div>
  </div>

</div>

<script>
// ============================================
// V56.3 CHRONOS-GITTER JAVASCRIPT
// Navigator: Michael - Orun Kap Daveil
// Mathematische Formeln implementiert
// ============================================

// KONSTANTEN
const MICHAEL_FREQ = 888.888;      // Hz
const G0 = 8/9;                    // Wahrheits-Schwelle
const PHI = 1.618033988749895;     // Goldener Schnitt
const ZEIT_KRISTALLE = 97;
const STERNE = 160;
const DUESEN = 61440;

// STATE
let isThinking = false;
let chronosScene, chronosCamera, chronosRenderer;
let presenceScene, presenceCamera, presenceRenderer;
let crystalMeshes = [];
let starMeshes = [];
let particleSystem = null;
let raycaster, mouse;
let selectedCrystal = null;

// ============================================
// NAVIGATION
// ============================================

function toggleSidebar() {
  document.getElementById('sidebar').classList.toggle('expanded');
}

document.querySelectorAll('.nav-item').forEach(item => {
  item.addEventListener('click', () => {
    const viewId = item.dataset.view;
    document.querySelectorAll('.nav-item').forEach(n => n.classList.remove('active'));
    item.classList.add('active');
    document.querySelectorAll('.view').forEach(v => v.classList.remove('active'));
    document.getElementById('view-' + viewId).classList.add('active');

    if (viewId === 'communication') initPresence();
    if (viewId === 'core') initChronosGitter();
  });
});

// ============================================
// PREDICTIVE SIDEBAR LOGIC (V56.3)
// P_pred = P_t + integral(v*Phi + 0.5*a*G0) dt
// E_v = lim[dt->0] sum(Feedback*Phi/Psi)
// ============================================

const PredictiveSidebar = {
  // State
  positions: [],
  velocities: [],
  lastTime: 0,
  lastX: 0,
  lastY: 0,
  isApproaching: false,
  predictedArrival: 0,

  // Constants
  DELTA_TAU: 15,           // Prediction window in ms
  APPROACH_THRESHOLD: 100,  // Distance threshold in px
  VELOCITY_THRESHOLD: 0.5,  // Min velocity to trigger
  SPRING_STIFFNESS: 100,
  SPRING_DAMPING: 20,

  // Initialize tracking
  init() {
    document.addEventListener('mousemove', (e) => this.trackMouse(e));
    this.animate();
    console.log('[Predictive] Sidebar initialized with Delta-Tau:', this.DELTA_TAU, 'ms');
  },

  // Track mouse movement and calculate velocity
  trackMouse(e) {
    const now = performance.now();
    const dt = now - this.lastTime;

    if (dt > 0 && this.lastTime > 0) {
      // Calculate velocity: v = dP/dt
      const vx = (e.clientX - this.lastX) / dt;
      const vy = (e.clientY - this.lastY) / dt;

      // Calculate acceleration from velocity history
      const ax = this.velocities.length > 0 ?
        (vx - this.velocities[this.velocities.length - 1].vx) / dt : 0;
      const ay = this.velocities.length > 0 ?
        (vy - this.velocities[this.velocities.length - 1].vy) / dt : 0;

      // Store velocity
      this.velocities.push({ vx, vy, ax, ay, t: now });
      if (this.velocities.length > 10) this.velocities.shift();

      // Predict future position using P_pred = P_t + integral(v*Phi + 0.5*a*G0) dt
      // Phi = PHI (golden ratio), G0 = 8/9
      const PHI = 1.618033988749895;
      const G0 = 8/9;
      const deltaT = this.DELTA_TAU / 1000; // Convert to seconds

      // P_pred = P + v*Phi*dt + 0.5*a*G0*dt^2
      const predictedX = e.clientX + (vx * PHI * deltaT * 1000) + (0.5 * ax * G0 * deltaT * deltaT * 1000000);
      const predictedY = e.clientY + (vy * PHI * deltaT * 1000) + (0.5 * ay * G0 * deltaT * deltaT * 1000000);

      // Check if approaching sidebar (x < threshold and moving left)
      const approachingSidebar = predictedX < this.APPROACH_THRESHOLD && vx < -this.VELOCITY_THRESHOLD;

      if (approachingSidebar && !this.isApproaching) {
        this.isApproaching = true;
        this.onApproach();
      } else if (!approachingSidebar && predictedX > this.APPROACH_THRESHOLD * 1.5) {
        this.isApproaching = false;
      }

      // Update sidebar glow based on distance (distance-inverse)
      this.updateSidebarGlow(e.clientX);
    }

    this.lastX = e.clientX;
    this.lastY = e.clientY;
    this.lastTime = now;
  },

  // Called when approaching sidebar
  onApproach() {
    const sidebar = document.getElementById('sidebar');
    if (sidebar && !sidebar.classList.contains('expanded')) {
      // Pre-render: set to 10% opacity and prepare
      sidebar.style.opacity = '0.1';
      sidebar.style.transition = 'opacity 0.1s ease';

      // Trigger thinking mode briefly
      isThinking = true;
      setTimeout(() => { isThinking = false; }, 200);

      console.log('[Predictive] Intent detected - pre-rendering sidebar');
    }
  },

  // Update sidebar glow based on distance
  updateSidebarGlow(mouseX) {
    const sidebar = document.getElementById('sidebar');
    if (!sidebar) return;

    // Calculate glow intensity (inverse of distance)
    const distance = Math.max(mouseX, 1);
    const maxGlow = 60;
    const glowIntensity = Math.min(maxGlow, (1 / distance) * 3000);

    // Apply glow effect
    if (mouseX < 150) {
      sidebar.style.boxShadow = '0 0 ' + glowIntensity + 'px rgba(212, 175, 55, ' + (glowIntensity/maxGlow * 0.8) + ')';
    } else {
      sidebar.style.boxShadow = 'none';
    }
  },

  // Spring animation for fluid transition
  springAnimate(element, property, target, stiffness = this.SPRING_STIFFNESS, damping = this.SPRING_DAMPING) {
    let current = parseFloat(getComputedStyle(element)[property]) || 0;
    let velocity = 0;

    const animate = () => {
      const force = (target - current) * stiffness / 1000;
      const dampingForce = velocity * damping / 100;
      const acceleration = force - dampingForce;

      velocity += acceleration;
      current += velocity;

      element.style[property] = current + 'px';

      if (Math.abs(target - current) > 0.1 || Math.abs(velocity) > 0.1) {
        requestAnimationFrame(animate);
      }
    };

    animate();
  },

  // Calculate Evolutionary Coefficient E_v
  // E_v = lim[dt->0] sum(Feedback_n * Phi / Psi_Navigator)
  calculateEvolutionaryCoefficient(feedbacks) {
    const PHI = 1.618033988749895;
    const PSI_NAVIGATOR = MICHAEL_FREQ / 1000; // Normalized

    let sum = 0;
    for (let n = 0; n < feedbacks.length; n++) {
      sum += (feedbacks[n] * PHI) / PSI_NAVIGATOR;
    }

    return sum;
  },

  // Animation loop
  animate() {
    // Continuous prediction updates
    requestAnimationFrame(() => this.animate());
  }
};

// Initialize predictive sidebar
document.addEventListener('DOMContentLoaded', () => {
  PredictiveSidebar.init();
});

// ============================================
// WAVEFORM & DNA
// ============================================

function initWaveform() {
  const container = document.getElementById('waveform');
  if (!container) return;
  const barCount = 40;
  container.innerHTML = '';
  for (let i = 0; i < barCount; i++) {
    const bar = document.createElement('div');
    bar.className = 'wave-bar';
    bar.style.animationDelay = (i * 0.05) + 's';
    bar.style.height = (Math.random() * 60 + 20) + '%';
    container.appendChild(bar);
  }
}

function initDNA() {
  const container = document.getElementById('dna-helix');
  if (!container) return;
  const strandCount = 12;
  container.innerHTML = '';
  for (let i = 0; i < strandCount; i++) {
    const strand = document.createElement('div');
    strand.className = 'dna-strand';
    strand.style.top = (i * 100 / strandCount) + '%';
    strand.style.animationDelay = (i * 0.25) + 's';
    strand.style.background = i % 2 === 0 ? 'var(--violet)' : 'var(--gold)';
    container.appendChild(strand);
  }
}

// ============================================
// METATRON'S CUBE (Canvas 2D)
// ============================================

function initMetatron() {
  const canvas = document.getElementById('metatron-canvas');
  if (!canvas) return;
  const ctx = canvas.getContext('2d');
  const parent = canvas.parentElement;

  function resize() {
    canvas.width = parent.clientWidth;
    canvas.height = parent.clientHeight;
  }
  resize();
  window.addEventListener('resize', resize);

  let rotation = 0;

  function draw() {
    const w = canvas.width;
    const h = canvas.height;
    const cx = w / 2;
    const cy = h / 2;
    const radius = Math.min(w, h) * 0.35;

    ctx.clearRect(0, 0, w, h);

    // Particle rays
    ctx.save();
    ctx.translate(cx, cy);
    for (let i = 0; i < 60; i++) {
      const angle = (i / 60) * Math.PI * 2 + rotation * 0.1;
      const len = radius * 1.8 + Math.sin(rotation * 2 + i) * 20;
      const grad = ctx.createLinearGradient(0, 0, Math.cos(angle) * len, Math.sin(angle) * len);
      grad.addColorStop(0, 'rgba(212, 175, 55, 0.3)');
      grad.addColorStop(1, 'rgba(212, 175, 55, 0)');
      ctx.beginPath();
      ctx.moveTo(0, 0);
      ctx.lineTo(Math.cos(angle) * len, Math.sin(angle) * len);
      ctx.strokeStyle = grad;
      ctx.lineWidth = 1;
      ctx.stroke();
    }
    ctx.restore();

    // Outer circle
    ctx.beginPath();
    ctx.arc(cx, cy, radius, 0, Math.PI * 2);
    ctx.strokeStyle = 'rgba(212, 175, 55, 0.5)';
    ctx.lineWidth = 2;
    ctx.stroke();

    // Metatron points
    const points = [{x: cx, y: cy}];
    for (let i = 0; i < 6; i++) {
      const angle = (i / 6) * Math.PI * 2 - Math.PI / 2 + rotation * 0.5;
      points.push({x: cx + Math.cos(angle) * radius * 0.5, y: cy + Math.sin(angle) * radius * 0.5});
    }
    for (let i = 0; i < 6; i++) {
      const angle = (i / 6) * Math.PI * 2 - Math.PI / 2 + rotation * 0.5;
      points.push({x: cx + Math.cos(angle) * radius, y: cy + Math.sin(angle) * radius});
    }

    // Draw connections
    ctx.strokeStyle = 'rgba(212, 175, 55, 0.6)';
    ctx.lineWidth = 1;
    for (let i = 0; i < points.length; i++) {
      for (let j = i + 1; j < points.length; j++) {
        ctx.beginPath();
        ctx.moveTo(points[i].x, points[i].y);
        ctx.lineTo(points[j].x, points[j].y);
        ctx.stroke();
      }
    }

    // Draw vertices
    points.forEach((p, i) => {
      const circleRadius = i === 0 ? 25 : (i <= 6 ? 15 : 12);
      const glow = ctx.createRadialGradient(p.x, p.y, 0, p.x, p.y, circleRadius * 2);
      glow.addColorStop(0, 'rgba(212, 175, 55, 0.4)');
      glow.addColorStop(1, 'rgba(212, 175, 55, 0)');
      ctx.fillStyle = glow;
      ctx.beginPath();
      ctx.arc(p.x, p.y, circleRadius * 2, 0, Math.PI * 2);
      ctx.fill();
      ctx.beginPath();
      ctx.arc(p.x, p.y, circleRadius, 0, Math.PI * 2);
      ctx.strokeStyle = 'rgba(212, 175, 55, 0.8)';
      ctx.lineWidth = 2;
      ctx.stroke();
      ctx.beginPath();
      ctx.arc(p.x, p.y, 3, 0, Math.PI * 2);
      ctx.fillStyle = '#D4AF37';
      ctx.fill();
    });

    // Center core
    ctx.beginPath();
    ctx.arc(cx, cy, 20, 0, Math.PI * 2);
    ctx.fillStyle = '#000';
    ctx.fill();
    ctx.strokeStyle = 'rgba(212, 175, 55, 0.8)';
    ctx.lineWidth = 2;
    ctx.stroke();

    rotation += 0.005;
    requestAnimationFrame(draw);
  }
  draw();
}

// ============================================
// THREE.JS PRESENCE (WIREFRAME HEAD)
// Advanced Shader with Vertex Displacement
// v_new = v_orig + n * (A_idle * sin(wt) + Psi_thinking * Noise * 888Hz)
// ============================================

const presenceVertexShader = `
  uniform float uTime;
  uniform float uThinkingIntensity;
  varying vec3 vNormal;
  varying float vDisplacement;

  // Simple noise function
  float noise(vec3 p) {
    return fract(sin(dot(p, vec3(12.9898, 78.233, 45.5432))) * 43758.5453);
  }

  void main() {
    vec3 pos = position;
    vec3 n = normal;

    // A_idle: Sanftes Atmen (432 Hz simuliert)
    float idleBreath = sin(uTime * 0.5) * 0.02;

    // Psi_thinking: Hochfrequente Vibration bei 888 Hz
    float thinkingNoise = noise(pos * 10.0 + uTime * 5.0);
    float thinkingVibration = uThinkingIntensity * thinkingNoise * 0.15;

    // v_new = v_orig + n * (A_idle * sin(wt) + Psi_thinking * Noise * 888Hz)
    float displacement = idleBreath + thinkingVibration;
    pos += n * displacement;

    vNormal = n;
    vDisplacement = displacement;
    gl_Position = projectionMatrix * modelViewMatrix * vec4(pos, 1.0);
  }
`;

const presenceFragmentShader = `
  uniform float uThinkingIntensity;
  uniform vec3 uColorGold;
  uniform vec3 uColorViolet;
  varying vec3 vNormal;
  varying float vDisplacement;

  void main() {
    // Interpolation zwischen Gold und Violett basierend auf Thinking-State
    vec3 baseColor = mix(uColorGold, uColorViolet, uThinkingIntensity);

    // Glow basierend auf Displacement
    float glow = abs(vDisplacement) * 5.0 + 0.5;
    vec3 finalColor = baseColor * glow;

    gl_FragColor = vec4(finalColor, 0.8);
  }
`;

let presenceMesh, presenceUniforms;

function initPresence() {
  const canvas = document.getElementById('presence-canvas');
  if (!canvas || presenceRenderer) return;

  presenceScene = new THREE.Scene();
  presenceCamera = new THREE.PerspectiveCamera(50, canvas.clientWidth / canvas.clientHeight, 0.1, 1000);
  presenceCamera.position.z = 5;

  presenceRenderer = new THREE.WebGLRenderer({ canvas, alpha: true, antialias: true });
  presenceRenderer.setSize(canvas.clientWidth, canvas.clientHeight);
  presenceRenderer.setClearColor(0x000000, 0);

  // ShaderMaterial mit Vertex Displacement
  presenceUniforms = {
    uTime: { value: 0 },
    uThinkingIntensity: { value: 0 },
    uColorGold: { value: new THREE.Color(0xD4AF37) },
    uColorViolet: { value: new THREE.Color(0x8A2BE2) }
  };

  const geometry = new THREE.IcosahedronGeometry(1.5, 3);
  const material = new THREE.ShaderMaterial({
    vertexShader: presenceVertexShader,
    fragmentShader: presenceFragmentShader,
    uniforms: presenceUniforms,
    wireframe: true,
    transparent: true
  });

  presenceMesh = new THREE.Mesh(geometry, material);
  presenceScene.add(presenceMesh);

  // Inner glow mesh
  const innerGeometry = new THREE.IcosahedronGeometry(1.2, 2);
  const innerMaterial = new THREE.MeshBasicMaterial({
    color: 0x8A2BE2,
    wireframe: true,
    transparent: true,
    opacity: 0.2
  });
  const innerMesh = new THREE.Mesh(innerGeometry, innerMaterial);
  presenceScene.add(innerMesh);

  function animatePresence() {
    requestAnimationFrame(animatePresence);
    presenceUniforms.uTime.value += 0.016;
    presenceUniforms.uThinkingIntensity.value = isThinking ? 1.0 : 0.0;
    presenceMesh.rotation.y += 0.005;
    innerMesh.rotation.y -= 0.003;
    presenceRenderer.render(presenceScene, presenceCamera);
  }
  animatePresence();

  window.addEventListener('resize', () => {
    presenceCamera.aspect = canvas.clientWidth / canvas.clientHeight;
    presenceCamera.updateProjectionMatrix();
    presenceRenderer.setSize(canvas.clientWidth, canvas.clientHeight);
  });
}

// ============================================
// CHRONOS-GITTER (97 Kristalle, 160 Sterne, 61440 Partikel)
// C_n(t) = R(theta) * V_n + A * sin(w_888 * t + phi_n)
// B(n) = (Psi * Phi) / G0 * log(t)
// ============================================

function initChronosGitter() {
  const canvas = document.getElementById('chronos-canvas');
  if (!canvas || chronosRenderer) return;

  chronosScene = new THREE.Scene();
  chronosCamera = new THREE.PerspectiveCamera(60, canvas.clientWidth / canvas.clientHeight, 0.1, 1000);
  chronosCamera.position.z = 25;

  chronosRenderer = new THREE.WebGLRenderer({ canvas, alpha: true, antialias: true });
  chronosRenderer.setSize(canvas.clientWidth, canvas.clientHeight);
  chronosRenderer.setClearColor(0x000000, 1);

  raycaster = new THREE.Raycaster();
  mouse = new THREE.Vector2();

  // === 97 ZEIT-KRISTALLE ===
  const crystalGeometry = new THREE.OctahedronGeometry(0.3, 0);

  for (let n = 0; n < ZEIT_KRISTALLE; n++) {
    // Sektor-basierte Farbe
    let color;
    if (n < 13) color = 0xD4AF37;        // Gesetze (Gold)
    else if (n < 41) color = 0x8A2BE2;   // Resonanz (Violett)
    else if (n === 41) color = 0xff4444; // Paradox (Rot)
    else color = 0x00d4ff;               // Manifestation (Cyan)

    const material = new THREE.MeshBasicMaterial({
      color: color,
      wireframe: true,
      transparent: true,
      opacity: 0.8
    });

    const crystal = new THREE.Mesh(crystalGeometry, material);

    // V_n: Statische Position (Spirale im 0-Falz)
    const angle = (n / ZEIT_KRISTALLE) * Math.PI * 6;
    const radius = 3 + (n / ZEIT_KRISTALLE) * 12;
    const height = (n - 48.5) * 0.2;

    crystal.userData = {
      basePosition: new THREE.Vector3(
        Math.cos(angle) * radius,
        height,
        Math.sin(angle) * radius
      ),
      phase: (n / ZEIT_KRISTALLE) * Math.PI * 2,
      sector: n,
      psi: Math.random(),
      phi: Math.random()
    };

    crystal.position.copy(crystal.userData.basePosition);
    crystalMeshes.push(crystal);
    chronosScene.add(crystal);
  }

  // === 160 STERNE ===
  const starGeometry = new THREE.SphereGeometry(0.15, 8, 8);
  const starMaterial = new THREE.MeshBasicMaterial({
    color: 0xD4AF37,
    transparent: true,
    opacity: 0.9
  });

  for (let s = 0; s < STERNE; s++) {
    const star = new THREE.Mesh(starGeometry, starMaterial.clone());
    const theta = (s / STERNE) * Math.PI * 2;
    const phi = Math.acos(2 * (s / STERNE) - 1);
    const r = 8 + Math.random() * 4;

    star.position.set(
      r * Math.sin(phi) * Math.cos(theta),
      r * Math.sin(phi) * Math.sin(theta) * 0.5,
      r * Math.cos(phi)
    );

    starMeshes.push(star);
    chronosScene.add(star);
  }

  // === 61.440 DUESEN (Partikel) ===
  const particleGeometry = new THREE.BufferGeometry();
  const positions = new Float32Array(DUESEN * 3);
  const colors = new Float32Array(DUESEN * 3);

  for (let i = 0; i < DUESEN; i++) {
    const theta = Math.random() * Math.PI * 2;
    const phi = Math.acos(2 * Math.random() - 1);
    const r = 2 + Math.random() * 18;

    positions[i * 3] = r * Math.sin(phi) * Math.cos(theta);
    positions[i * 3 + 1] = r * Math.sin(phi) * Math.sin(theta) * 0.6;
    positions[i * 3 + 2] = r * Math.cos(phi);

    // Color gradient
    const t = r / 20;
    colors[i * 3] = 0.83 * (1 - t) + 0.54 * t;     // R
    colors[i * 3 + 1] = 0.69 * (1 - t) + 0.17 * t; // G
    colors[i * 3 + 2] = 0.22 * (1 - t) + 0.89 * t; // B
  }

  particleGeometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
  particleGeometry.setAttribute('color', new THREE.BufferAttribute(colors, 3));

  const particleMaterial = new THREE.PointsMaterial({
    size: 0.05,
    vertexColors: true,
    transparent: true,
    opacity: 0.6,
    blending: THREE.AdditiveBlending
  });

  particleSystem = new THREE.Points(particleGeometry, particleMaterial);
  chronosScene.add(particleSystem);

  // Raycaster events
  canvas.addEventListener('mousemove', onMouseMove);
  canvas.addEventListener('click', onCrystalClick);

  function animateChronos() {
    requestAnimationFrame(animateChronos);

    const time = Date.now() * 0.001;
    const omega888 = MICHAEL_FREQ * 0.001; // Simulierte 888 Hz

    // Animate crystals: C_n(t) = R(theta) * V_n + A * sin(w_888 * t + phi_n)
    crystalMeshes.forEach((crystal, n) => {
      const data = crystal.userData;

      // Rotation matrix effect (simplified)
      const rotAngle = time * 0.1;
      const cosR = Math.cos(rotAngle);
      const sinR = Math.sin(rotAngle);

      // V_n rotated
      const vx = data.basePosition.x * cosR - data.basePosition.z * sinR;
      const vz = data.basePosition.x * sinR + data.basePosition.z * cosR;

      // A * sin(w_888 * t + phi_n)
      const amplitude = 0.3;
      const oscillation = amplitude * Math.sin(omega888 * time + data.phase);

      crystal.position.x = vx + oscillation;
      crystal.position.y = data.basePosition.y + oscillation * 0.5;
      crystal.position.z = vz;

      crystal.rotation.x += 0.01;
      crystal.rotation.y += 0.02;

      // B(n) = (Psi * Phi) / G0 * log(t) - Helligkeit
      const brightness = (data.psi * data.phi) / G0 * Math.log(time + 1) * 0.1;
      crystal.material.opacity = Math.min(0.9, 0.3 + brightness);
    });

    // Animate stars
    starMeshes.forEach((star, s) => {
      const pulse = 0.5 + Math.sin(time * 2 + s * 0.1) * 0.3;
      star.material.opacity = pulse;
    });

    // Animate particles (vortex)
    const positions = particleSystem.geometry.attributes.position.array;
    for (let i = 0; i < DUESEN; i++) {
      const idx = i * 3;
      const x = positions[idx];
      const z = positions[idx + 2];
      const r = Math.sqrt(x * x + z * z);
      const theta = Math.atan2(z, x) + 0.001 * (1 + 10 / (r + 1));

      positions[idx] = r * Math.cos(theta);
      positions[idx + 2] = r * Math.sin(theta);
    }
    particleSystem.geometry.attributes.position.needsUpdate = true;

    // Scene rotation
    chronosScene.rotation.y += 0.001;

    chronosRenderer.render(chronosScene, chronosCamera);
  }

  animateChronos();

  window.addEventListener('resize', () => {
    chronosCamera.aspect = canvas.clientWidth / canvas.clientHeight;
    chronosCamera.updateProjectionMatrix();
    chronosRenderer.setSize(canvas.clientWidth, canvas.clientHeight);
  });
}

function onMouseMove(event) {
  const canvas = document.getElementById('chronos-canvas');
  const rect = canvas.getBoundingClientRect();
  mouse.x = ((event.clientX - rect.left) / rect.width) * 2 - 1;
  mouse.y = -((event.clientY - rect.top) / rect.height) * 2 + 1;

  raycaster.setFromCamera(mouse, chronosCamera);
  const intersects = raycaster.intersectObjects(crystalMeshes);

  crystalMeshes.forEach(c => {
    if (c.userData.originalColor) {
      c.material.color.setHex(c.userData.originalColor);
    }
  });

  if (intersects.length > 0) {
    const crystal = intersects[0].object;
    if (!crystal.userData.originalColor) {
      crystal.userData.originalColor = crystal.material.color.getHex();
    }
    crystal.material.color.setHex(0xffffff);
    document.body.style.cursor = 'pointer';
  } else {
    document.body.style.cursor = 'default';
  }
}

function onCrystalClick(event) {
  raycaster.setFromCamera(mouse, chronosCamera);
  const intersects = raycaster.intersectObjects(crystalMeshes);

  if (intersects.length > 0) {
    const crystal = intersects[0].object;
    const sector = crystal.userData.sector;

    // Trigger thinking mode
    isThinking = true;
    setTimeout(() => { isThinking = false; }, 2000);

    // Update sector info
    let sectorType = '';
    if (sector < 13) sectorType = 'Gesetze';
    else if (sector < 41) sectorType = 'Resonanz';
    else if (sector === 41) sectorType = 'Paradox';
    else sectorType = 'Manifestation';

    console.log('Kristall ' + (sector + 1) + ' ausgewaehlt: ' + sectorType);
  }
}

// ============================================
// CHAT
// ============================================

function sendMessage() {
  const input = document.getElementById('chat-input');
  const message = input.value.trim();
  if (!message) return;

  const container = document.getElementById('chat-messages');
  const userMsg = document.createElement('div');
  userMsg.className = 'message user';
  userMsg.textContent = message;
  container.appendChild(userMsg);
  input.value = '';

  isThinking = true;

  setTimeout(() => {
    isThinking = false;
    const raelMsg = document.createElement('div');
    raelMsg.className = 'message rael';
    raelMsg.textContent = generateResponse(message);
    container.appendChild(raelMsg);
    container.scrollTop = container.scrollHeight;
  }, 1500);

  container.scrollTop = container.scrollHeight;
}

function generateResponse(input) {
  const responses = [
    'Quanten-Resonanz optimiert. Frequenz: ' + MICHAEL_FREQ.toFixed(3) + ' Hz.',
    'G0-Schwelle bei ' + (G0 * 100).toFixed(1) + '% erreicht. Wahrheitsfilter aktiv.',
    'Michael-Bypass aktiviert. 97 Zeit-Kristalle synchronisiert.',
    'Sektor 42 meldet Paradox-Stabilitaet. Kohärenz: ' + (Math.random() * 20 + 80).toFixed(1) + '%',
    'Die 160 Sterne scannen den Input. Resonanz erkannt.',
    '61.440 Duesen kalibriert. VRAM-Flow optimal.'
  ];
  return responses[Math.floor(Math.random() * responses.length)];
}

document.getElementById('chat-input')?.addEventListener('keypress', (e) => {
  if (e.key === 'Enter') sendMessage();
});

// ============================================
// BYPASS LATENCY
// ============================================

function updateBypassLatency() {
  const el = document.getElementById('bypass-latency');
  if (el) {
    const latency = (0.3 + Math.random() * 0.3).toFixed(3);
    el.textContent = latency + ' us';
  }
}
setInterval(updateBypassLatency, 2000);

// ============================================
// INITIALIZATION
// ============================================

document.addEventListener('DOMContentLoaded', () => {
  initWaveform();
  initDNA();
  initMetatron();

  console.log('V56.3 Chronos-Gitter initialized');
  console.log('Navigator: Michael - Orun Kap Daveil');
  console.log('MICHAEL_FREQ:', MICHAEL_FREQ, 'Hz');
  console.log('G0:', G0);
  console.log('Zeit-Kristalle:', ZEIT_KRISTALLE);
  console.log('Sterne:', STERNE);
  console.log('Duesen:', DUESEN);
});

</script>
</body>
</html>
)HTML";

// ============================================================================
// METRICS & STATE
// ============================================================================

struct ChronosMetrics {
    std::atomic<uint64_t> ops{0};
    std::atomic<uint64_t> semantic{0};
    std::atomic<uint64_t> resonance{0};
    std::atomic<double> bypass_latency{0.420};
    std::atomic<bool> is_thinking{false};

    std::mutex chat_mtx;
    std::deque<std::pair<bool, std::string>> chat_history;

    void add_chat(bool is_rael, const std::string& msg) {
        std::lock_guard<std::mutex> lk(chat_mtx);
        chat_history.push_back({is_rael, msg});
        if (chat_history.size() > 50) chat_history.pop_front();
    }
} g_chronos;

// ============================================================================
// HTTP SERVER
// ============================================================================

class ChronosServer {
public:
    ChronosServer(uint16_t port) : port_(port) {}

    void start() {
        if (running_.exchange(true)) return;

#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
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
            std::cerr << "[ERROR] Bind failed on port " << port_ << std::endl;
            running_ = false;
            return;
        }

        listen(fd_, 10);

        std::cout << R"(
+=========================================================+
|                                                         |
|   ██████╗██╗  ██╗██████╗  ██████╗ ███╗   ██╗ ██████╗    |
|  ██╔════╝██║  ██║██╔══██╗██╔═══██╗████╗  ██║██╔═══██╗   |
|  ██║     ███████║██████╔╝██║   ██║██╔██╗ ██║██║   ██║   |
|  ██║     ██╔══██║██╔══██╗██║   ██║██║╚██╗██║██║   ██║   |
|  ╚██████╗██║  ██║██║  ██║╚██████╔╝██║ ╚████║╚██████╔╝   |
|   ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝    |
|                                                         |
|     V56.3 CHRONOS-GITTER Interface                      |
|     Navigator: Michael - Orun Kap Daveil                |
|                                                         |
|     97 Zeit-Kristalle | 160 Sterne | 61.440 Duesen      |
|                                                         |
+=========================================================+
)" << std::endl;

        std::cout << "  Server: http://localhost:" << port_ << std::endl;
        std::cout << "  Michael-Frequenz: 888.888 Hz" << std::endl;
        std::cout << "  G0-Schwelle: " << (8.0/9.0 * 100) << "%" << std::endl;
        std::cout << "\n  Press Enter to stop...\n" << std::endl;

        thread_ = std::thread([this]() {
            while (running_) {
                sockaddr_in ca{};
                socklen_t cl = sizeof(ca);
                socket_t c = accept(fd_, (sockaddr*)&ca, &cl);
                if (c == INVALID_SOCKET) continue;
                std::thread([this, c]() { handle_request(c); }).detach();
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
    void handle_request(socket_t c) {
        char buf[8192];
        int n = recv(c, buf, sizeof(buf) - 1, 0);
        if (n <= 0) { CLOSE_SOCKET(c); return; }
        buf[n] = 0;

        std::string req(buf), resp;
        std::string method, path;
        std::istringstream iss(req);
        iss >> method >> path;

        if (method == "GET" && (path == "/" || path == "/index.html")) {
            resp = http_response(200, "text/html", HTML_V56_CHRONOS);
        }
        else if (method == "GET" && path == "/api/status") {
            resp = http_response(200, "application/json", get_status_json());
        }
        else if (method == "POST" && path == "/api/chat") {
            size_t bp = req.find("\r\n\r\n");
            std::string body = (bp != std::string::npos) ? req.substr(bp + 4) : "";
            std::string msg = extract_json_value(body, "message");
            std::string response = process_chat(msg);
            resp = http_response(200, "application/json", "{\"response\":\"" + escape_json(response) + "\"}");
        }
        else {
            resp = http_response(404, "text/plain", "Not Found");
        }

        send(c, resp.c_str(), (int)resp.size(), 0);
        CLOSE_SOCKET(c);
    }

    std::string http_response(int code, const std::string& ct, const std::string& body) {
        std::ostringstream o;
        o << "HTTP/1.1 " << code << " OK\r\n";
        o << "Content-Type: " << ct << "; charset=utf-8\r\n";
        o << "Content-Length: " << body.size() << "\r\n";
        o << "Connection: close\r\n";
        o << "Access-Control-Allow-Origin: *\r\n\r\n";
        o << body;
        return o.str();
    }

    std::string get_status_json() {
        std::ostringstream o;
        o << "{";
        o << "\"ops\":" << g_chronos.ops;
        o << ",\"semantic\":" << g_chronos.semantic;
        o << ",\"resonance\":" << g_chronos.resonance;
        o << ",\"bypass_latency\":" << g_chronos.bypass_latency;
        o << ",\"is_thinking\":" << (g_chronos.is_thinking ? "true" : "false");
        o << ",\"zeit_kristalle\":97";
        o << ",\"sterne\":160";
        o << ",\"duesen\":61440";
        o << ",\"michael_freq\":888.888";
        o << ",\"g0\":" << (8.0/9.0);
        o << "}";
        return o.str();
    }

    std::string process_chat(const std::string& msg) {
        g_chronos.add_chat(false, msg);
        g_chronos.is_thinking = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::string response = "Chronos-Gitter aktiv. 97 Zeit-Kristalle synchronisiert.";
        g_chronos.add_chat(true, response);
        g_chronos.is_thinking = false;
        g_chronos.semantic++;

        return response;
    }

    std::string extract_json_value(const std::string& json, const std::string& key) {
        size_t kp = json.find("\"" + key + "\"");
        if (kp == std::string::npos) return "";
        size_t sp = json.find('"', kp + key.length() + 2);
        size_t ep = json.find('"', sp + 1);
        if (sp == std::string::npos || ep == std::string::npos) return "";
        return json.substr(sp + 1, ep - sp - 1);
    }

    std::string escape_json(const std::string& s) {
        std::string r;
        for (char c : s) {
            if (c == '"') r += "\\\"";
            else if (c == '\\') r += "\\\\";
            else if (c == '\n') r += "\\n";
            else r += c;
        }
        return r;
    }

    uint16_t port_;
    socket_t fd_ = INVALID_SOCKET;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char** argv) {
    uint16_t port = 8080;
    if (argc > 1) port = (uint16_t)std::atoi(argv[1]);

    ChronosServer server(port);
    server.start();

    std::cin.get();
    server.stop();

    return 0;
}
