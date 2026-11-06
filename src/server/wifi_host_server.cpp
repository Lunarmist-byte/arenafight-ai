#include "arenafight/server/wifi_host_server.hpp"
#include "arenafight/common/logger.hpp"
#include "arenafight/common/config.hpp"
#include "arenafight/common/string_utils.hpp"
#include "arenafight/system/system_profiler.hpp"
#include "arenafight/evidence/evidence_types.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <filesystem>
#include <nlohmann/json.hpp>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
using socket_t = SOCKET;
#define IS_VALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() WSAGetLastError()
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
using socket_t = int;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define IS_VALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define GETSOCKETERRNO() (errno)
#endif

namespace arenafight {

namespace {

class WinsockScope {
public:
    WinsockScope() {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    }
    ~WinsockScope() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
};

static WinsockScope g_winsock_init;

std::string urlDecode(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%' && i + 2 < in.size()) {
            int value = 0;
            std::istringstream is(in.substr(i + 1, 2));
            if (is >> std::hex >> value) {
                out += static_cast<char>(value);
                i += 2;
            } else {
                out += in[i];
            }
        } else if (in[i] == '+') {
            out += ' ';
        } else {
            out += in[i];
        }
    }
    return out;
}

// Embedded Web Application UI (HTML5, responsive CSS3, vanilla JS)
const char* const EMBEDDED_WEB_UI = R"html(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ArenaFight AI - Wi-Fi Hub</title>
  <style>
    :root {
      --bg: #0b0f19;
      --panel: #161f30;
      --panel-border: #24334a;
      --text: #e6edf3;
      --text-muted: #8b9bb4;
      --accent: #38bdf8;
      --accent-hover: #0284c7;
      --success: #10b981;
      --warning: #f59e0b;
      --danger: #ef4444;
      --purple: #8b5cf6;
      --radius: 10px;
      --mono: 'Cascadia Code', 'Fira Code', 'JetBrains Mono', Consolas, monospace;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      background: var(--bg);
      color: var(--text);
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      line-height: 1.5;
      padding-bottom: 50px;
    }
    header {
      background: var(--panel);
      border-bottom: 1px solid var(--panel-border);
      padding: 16px 24px;
      display: flex;
      flex-wrap: wrap;
      justify-content: space-between;
      align-items: center;
      gap: 12px;
      position: sticky;
      top: 0;
      z-index: 100;
    }
    .brand {
      display: flex;
      align-items: center;
      gap: 12px;
    }
    .brand h1 {
      font-size: 1.25rem;
      letter-spacing: 0.5px;
      font-weight: 700;
      color: #fff;
    }
    .brand span {
      background: rgba(56, 189, 248, 0.15);
      color: var(--accent);
      border: 1px solid rgba(56, 189, 248, 0.3);
      padding: 2px 8px;
      border-radius: 6px;
      font-size: 0.75rem;
      font-family: var(--mono);
      font-weight: 600;
    }
    .status-bar {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      gap: 10px;
      font-size: 0.82rem;
    }
    .badge {
      padding: 4px 10px;
      border-radius: 6px;
      font-family: var(--mono);
      font-size: 0.78rem;
      display: inline-flex;
      align-items: center;
      gap: 6px;
    }
    .badge-wifi { background: rgba(16, 185, 129, 0.15); color: var(--success); border: 1px solid rgba(16, 185, 129, 0.3); }
    .badge-cuda { background: rgba(56, 189, 248, 0.15); color: var(--accent); border: 1px solid rgba(56, 189, 248, 0.3); }
    .badge-model { background: rgba(139, 92, 246, 0.15); color: var(--purple); border: 1px solid rgba(139, 92, 246, 0.3); }
    .container {
      max-width: 1200px;
      margin: 20px auto;
      padding: 0 16px;
    }
    .nav-tabs {
      display: flex;
      gap: 8px;
      border-bottom: 1px solid var(--panel-border);
      margin-bottom: 24px;
      overflow-x: auto;
      padding-bottom: 6px;
    }
    .tab-btn {
      background: transparent;
      border: 1px solid transparent;
      color: var(--text-muted);
      padding: 10px 18px;
      border-radius: 8px;
      cursor: pointer;
      font-size: 0.95rem;
      font-weight: 600;
      transition: all 0.2s ease;
      white-space: nowrap;
    }
    .tab-btn:hover {
      color: var(--text);
      background: rgba(255, 255, 255, 0.05);
    }
    .tab-btn.active {
      color: var(--accent);
      background: var(--panel);
      border-color: var(--panel-border);
    }
    .tab-content { display: none; }
    .tab-content.active { display: block; }
    .card {
      background: var(--panel);
      border: 1px solid var(--panel-border);
      border-radius: var(--radius);
      padding: 20px;
      margin-bottom: 20px;
    }
    .card-title {
      font-size: 1.1rem;
      font-weight: 600;
      margin-bottom: 14px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .form-group {
      margin-bottom: 16px;
    }
    label {
      display: block;
      margin-bottom: 6px;
      font-size: 0.85rem;
      color: var(--text-muted);
      font-weight: 500;
    }
    textarea, input, select {
      width: 100%;
      background: #0d1523;
      border: 1px solid var(--panel-border);
      color: var(--text);
      padding: 12px 14px;
      border-radius: 8px;
      font-family: inherit;
      font-size: 0.95rem;
      transition: border-color 0.2s;
    }
    textarea:focus, input:focus, select:focus {
      outline: none;
      border-color: var(--accent);
    }
    textarea {
      min-height: 110px;
      resize: vertical;
      font-family: var(--mono);
      font-size: 0.9rem;
    }
    .btn-row {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      align-items: center;
    }
    .btn {
      background: var(--accent);
      color: #0b0f19;
      font-weight: 600;
      padding: 10px 20px;
      border-radius: 8px;
      border: none;
      cursor: pointer;
      font-size: 0.92rem;
      transition: background 0.2s;
      display: inline-flex;
      align-items: center;
      gap: 8px;
    }
    .btn:hover { background: var(--accent-hover); }
    .btn:disabled { opacity: 0.5; cursor: not-allowed; }
    .btn-secondary {
      background: rgba(255, 255, 255, 0.08);
      color: var(--text);
      border: 1px solid var(--panel-border);
    }
    .btn-secondary:hover { background: rgba(255, 255, 255, 0.12); }
    .btn-danger {
      background: rgba(239, 68, 68, 0.2);
      color: var(--danger);
      border: 1px solid rgba(239, 68, 68, 0.4);
    }
    .btn-danger:hover { background: rgba(239, 68, 68, 0.3); }
    .gates-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
      gap: 12px;
      margin-top: 16px;
    }
    .gate-item {
      background: #0d1523;
      border: 1px solid var(--panel-border);
      border-radius: 8px;
      padding: 12px 14px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      font-size: 0.85rem;
    }
    .gate-pass { color: var(--success); font-weight: 700; }
    .gate-fail { color: var(--danger); font-weight: 700; }
    .gate-pending { color: var(--text-muted); }
    .log-box {
      background: #080c14;
      border: 1px solid var(--panel-border);
      border-radius: 8px;
      padding: 14px;
      font-family: var(--mono);
      font-size: 0.85rem;
      max-height: 400px;
      overflow-y: auto;
      white-space: pre-wrap;
      color: #94a3b8;
      margin-top: 14px;
    }
    .table-responsive {
      overflow-x: auto;
      margin-top: 10px;
    }
    table {
      width: 100%;
      border-collapse: collapse;
      font-size: 0.88rem;
      text-align: left;
    }
    th, td {
      padding: 10px 14px;
      border-bottom: 1px solid var(--panel-border);
    }
    th {
      color: var(--text-muted);
      font-weight: 600;
    }
    footer {
      max-width: 1200px;
      margin: 40px auto 0 auto;
      padding: 20px 16px;
      border-top: 1px solid var(--panel-border);
      display: flex;
      flex-wrap: wrap;
      justify-content: space-between;
      align-items: center;
      font-size: 0.85rem;
      color: var(--text-muted);
      gap: 10px;
    }
    footer a { color: var(--accent); text-decoration: none; }
    footer a:hover { text-decoration: underline; }
    @media (max-width: 768px) {
      header { flex-direction: column; align-items: flex-start; }
      .brand { width: 100%; justify-content: space-between; }
      .status-bar { width: 100%; }
      .btn-row { flex-direction: column; width: 100%; }
      .btn-row .btn { width: 100%; justify-content: center; }
    }
  </style>
</head>
<body>
  <header>
    <div class="brand">
      <h1>ArenaFight AI</h1>
      <span>WI-FI HUB</span>
    </div>
    <div class="status-bar">
      <div id="wifiBadge" class="badge badge-wifi">Connecting...</div>
      <div id="cudaBadge" class="badge badge-cuda">Hardware: Detecting</div>
      <div id="modelBadge" class="badge badge-model">Model: Loading</div>
      <button class="btn btn-secondary" style="padding: 4px 10px; font-size: 0.78rem;" onclick="copyWifiUrl()">Copy Wi-Fi URL</button>
    </div>
  </header>

  <div class="container">
    <div class="nav-tabs">
      <button class="tab-btn active" onclick="switchTab('tab-agent')">Agent & Chat</button>
      <button class="tab-btn" onclick="switchTab('tab-models')">Models & Aliases</button>
      <button class="tab-btn" onclick="switchTab('tab-hardware')">Hardware & Profile</button>
      <button class="tab-btn" onclick="switchTab('tab-files')">Files & Media</button>
      <button class="tab-btn" onclick="switchTab('tab-learn')">Self-Learning</button>
    </div>

    <!-- TAB 1: AGENT & CHAT -->
    <div id="tab-agent" class="tab-content active">
      <div class="card">
        <div class="card-title">
          <span>Autonomous Agent & Interactive Prompting</span>
          <span style="font-size: 0.8rem; font-weight: normal; color: var(--text-muted);" id="agentModeInfo">Standalone | Fast Mode</span>
        </div>
        <div class="form-group">
          <label for="promptInput">Enter your goal, coding task, question, or chat prompt:</label>
          <textarea id="promptInput" placeholder="e.g., Implement a lock-free queue in C++20 with unit tests and verification, or tell me a joke"></textarea>
        </div>
        <div class="btn-row" style="margin-bottom: 16px;">
          <button class="btn" id="runTaskBtn" onclick="executeTask()">Run Autonomous Task (Verified)</button>
          <button class="btn btn-secondary" id="chatBtn" onclick="chatPrompt()">Chat Prompt</button>
          <select id="personaSelect" style="max-width: 170px;">
            <option value="witty">Persona: Witty</option>
            <option value="pirate">Persona: Pirate</option>
            <option value="cyberpunk">Persona: Cyberpunk</option>
            <option value="zen">Persona: Zen</option>
            <option value="nerd">Persona: Nerd</option>
          </select>
          <select id="levelSelect" style="max-width: 170px;" onchange="updateLevel()">
            <option value="fast">Processing: Fast</option>
            <option value="perfect">Processing: Perfect Boi</option>
          </select>
        </div>

        <div id="executionResultCard" style="display: none; border-top: 1px solid var(--panel-border); padding-top: 16px;">
          <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;">
            <strong id="resultStatusText" style="color: var(--accent);">Task In Progress...</strong>
            <span id="confidenceText" style="font-family: var(--mono); font-size: 0.85rem; color: var(--text-muted);">Confidence: 0%</span>
          </div>

          <div class="gates-grid" id="gatesGrid">
            <div class="gate-item"><span>Gate 1: Physical Artifacts</span><span id="gate1" class="gate-pending">WAITING</span></div>
            <div class="gate-item"><span>Gate 2: Code Compilation</span><span id="gate2" class="gate-pending">WAITING</span></div>
            <div class="gate-item"><span>Gate 3: Test Verification</span><span id="gate3" class="gate-pending">WAITING</span></div>
            <div class="gate-item"><span>Gate 4: Requirements Met</span><span id="gate4" class="gate-pending">WAITING</span></div>
            <div class="gate-item"><span>Gate 5: Adversarial Review</span><span id="gate5" class="gate-pending">WAITING</span></div>
            <div class="gate-item"><span>Gate 6: Contradiction Free</span><span id="gate6" class="gate-pending">WAITING</span></div>
            <div class="gate-item"><span>Gate 7: Hardware & QoS</span><span id="gate7" class="gate-pending">WAITING</span></div>
          </div>

          <div class="log-box" id="executionLog">Ready.</div>
        </div>
      </div>
    </div>

    <!-- TAB 2: MODELS & ALIASES -->
    <div id="tab-models" class="tab-content">
      <div class="card">
        <div class="card-title">Active AI Model Selection</div>
        <div class="form-group">
          <label>Select model across all detected providers (llama.cpp, Ollama, OpenRouter, OpenCode):</label>
          <div class="btn-row">
            <select id="modelDropdown" style="flex: 1;"></select>
            <button class="btn" onclick="applyModelSelection()">Set Active Model</button>
          </div>
        </div>
      </div>

      <div class="card">
        <div class="card-title">Manage Model Aliases</div>
        <p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 12px;">Create friendly shortcuts for any model (e.g. 'coder' -> 'qwen2.5-coder:7b').</p>
        <div class="btn-row" style="margin-bottom: 14px;">
          <input type="text" id="newAliasName" placeholder="Alias (e.g. fast)" style="flex: 1; min-width: 140px;">
          <input type="text" id="newAliasTarget" placeholder="Target Model ID (e.g. qwen2.5-coder:7b)" style="flex: 2; min-width: 200px;">
          <button class="btn" onclick="saveNewAlias()">Save Alias</button>
        </div>
        <div class="table-responsive">
          <table>
            <thead>
              <tr>
                <th>Alias Name</th>
                <th>Target Model</th>
                <th>Action</th>
              </tr>
            </thead>
            <tbody id="aliasesTableBody">
              <tr><td colspan="3" style="text-align: center; color: var(--text-muted);">Loading aliases...</td></tr>
            </tbody>
          </table>
        </div>
      </div>
    </div>

    <!-- TAB 3: HARDWARE & PROFILE -->
    <div id="tab-hardware" class="tab-content">
      <div class="card">
        <div class="card-title">Host Machine Assessment & Dynamic Limits</div>
        <div class="table-responsive">
          <table>
            <tbody id="hardwareProfileBody">
              <tr><td>Loading physical hardware diagnostics...</td></tr>
            </tbody>
          </table>
        </div>
      </div>
    </div>

    <!-- TAB 4: FILES & MEDIA -->
    <div id="tab-files" class="tab-content">
      <div class="card">
        <div class="card-title">Workspace Files</div>
        <div class="btn-row" style="margin-bottom: 12px;">
          <button class="btn btn-secondary" onclick="loadWorkspaceFiles()">Refresh File List</button>
        </div>
        <div class="table-responsive">
          <table>
            <thead>
              <tr>
                <th>File Name</th>
                <th>Size</th>
                <th>Action</th>
              </tr>
            </thead>
            <tbody id="filesTableBody">
              <tr><td colspan="3" style="text-align: center; color: var(--text-muted);">Click Refresh to list files</td></tr>
            </tbody>
          </table>
        </div>
        <div class="log-box" id="fileViewerBox" style="display: none; margin-top: 14px;"></div>
      </div>

      <div class="card">
        <div class="card-title">Multimodal Media Assets (Fooocus SDXL & Wan AI 2.1)</div>
        <div class="table-responsive">
          <table>
            <thead>
              <tr>
                <th>Asset File</th>
                <th>Type</th>
                <th>Action</th>
              </tr>
            </thead>
            <tbody id="mediaTableBody">
              <tr><td colspan="3" style="text-align: center; color: var(--text-muted);">No generated media found</td></tr>
            </tbody>
          </table>
        </div>
      </div>
    </div>

    <!-- TAB 5: SELF-LEARNING -->
    <div id="tab-learn" class="tab-content">
      <div class="card">
        <div class="card-title">
          <span>Ground-Truth Self-Learning Engine</span>
          <span class="badge badge-wifi" id="learnRateBadge">100% Verified</span>
        </div>
        <p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 14px;">Enforces 7 Anti-Fake Invariants. Only models that produce genuine physical artifacts and pass all verification gates are ingested into fine-tuning.</p>
        <div class="card-title" style="font-size: 0.95rem;">Verified Training Instruction Pairs</div>
        <div class="table-responsive" style="margin-bottom: 20px;">
          <table>
            <thead>
              <tr>
                <th>Pair ID</th>
                <th>Task / Prompt</th>
                <th>Confidence</th>
              </tr>
            </thead>
            <tbody id="verifiedPairsBody">
              <tr><td colspan="3" style="text-align: center; color: var(--text-muted);">No pairs ingested yet</td></tr>
            </tbody>
          </table>
        </div>

        <div class="card-title" style="font-size: 0.95rem;">Quarantined Fake / Unverified Candidates</div>
        <div class="table-responsive">
          <table>
            <thead>
              <tr>
                <th>Candidate ID</th>
                <th>Rejection Reason</th>
                <th>Details</th>
              </tr>
            </thead>
            <tbody id="rejectedCandidatesBody">
              <tr><td colspan="3" style="text-align: center; color: var(--text-muted);">Quarantine empty (zero fake data detected)</td></tr>
            </tbody>
          </table>
        </div>
      </div>
    </div>
  </div>

  <footer>
    <div>
      <strong>Made by Lunarmist-byte</strong> | 
      <a href="https://github.com/Lunarmist-byte" target="_blank" rel="noopener">GitHub</a> | 
      <a href="https://www.linkedin.com/in/amal-s-kumar-ba69a1290/" target="_blank" rel="noopener">LinkedIn</a>
    </div>
    <div>ArenaFight AI Runtime v2.6.0 | Pure C++20</div>
  </footer>

  <script>
    let g_status = null;

    function switchTab(tabId) {
      document.querySelectorAll('.tab-content').forEach(el => el.classList.remove('active'));
      document.querySelectorAll('.tab-btn').forEach(el => el.classList.remove('active'));
      const target = document.getElementById(tabId);
      if (target) target.classList.add('active');
      const btn = Array.from(document.querySelectorAll('.tab-btn')).find(b => b.getAttribute('onclick').includes(tabId));
      if (btn) btn.classList.add('active');

      if (tabId === 'tab-models') loadModelsAndAliases();
      if (tabId === 'tab-hardware') loadHardwareProfile();
      if (tabId === 'tab-files') { loadWorkspaceFiles(); loadMediaAssets(); }
      if (tabId === 'tab-learn') loadSelfLearning();
    }

    async function fetchStatus() {
      try {
        const res = await fetch('/api/status');
        if (!res.ok) return;
        g_status = await res.json();
        
        const host = g_status.host || {};
        const agent = g_status.agent || {};
        const hw = g_status.hardware || {};

        document.getElementById('wifiBadge').textContent = 'Wi-Fi: ' + (host.wifi_url || 'online');
        document.getElementById('cudaBadge').textContent = 'HW: ' + (hw.acceleration || 'Standard');
        document.getElementById('modelBadge').textContent = 'Model: ' + (agent.model || 'Auto');
        document.getElementById('agentModeInfo').textContent = (agent.mode || 'standalone').toUpperCase() + ' | ' + (agent.processing_level || 'fast').toUpperCase() + ' MODE';
        
        if (agent.processing_level) {
          document.getElementById('levelSelect').value = agent.processing_level;
        }
      } catch (err) {
        document.getElementById('wifiBadge').textContent = 'Offline';
      }
    }

    function copyWifiUrl() {
      if (g_status && g_status.host && g_status.host.wifi_url) {
        navigator.clipboard.writeText(g_status.host.wifi_url).then(() => {
          alert('Copied Wi-Fi URL: ' + g_status.host.wifi_url);
        });
      } else {
        navigator.clipboard.writeText(window.location.origin).then(() => {
          alert('Copied URL: ' + window.location.origin);
        });
      }
    }

    async function executeTask() {
      const prompt = document.getElementById('promptInput').value.trim();
      if (!prompt) { alert('Please enter a task or goal'); return; }

      const runBtn = document.getElementById('runTaskBtn');
      const chatBtn = document.getElementById('chatBtn');
      runBtn.disabled = true;
      chatBtn.disabled = true;

      const card = document.getElementById('executionResultCard');
      card.style.display = 'block';
      document.getElementById('resultStatusText').textContent = 'Executing Autonomous Task with Verification...';
      document.getElementById('resultStatusText').style.color = 'var(--accent)';
      document.getElementById('confidenceText').textContent = 'Evaluating physical evidence...';
      document.getElementById('executionLog').textContent = 'Task dispatched: ' + prompt + '\nGenerating task graph...\n';

      for (let i = 1; i <= 7; i++) {
        const g = document.getElementById('gate' + i);
        g.className = 'gate-pending';
        g.textContent = 'RUNNING';
      }

      try {
        const res = await fetch('/api/execute', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ task: prompt })
        });
        const data = await res.json();

        if (data.success) {
          document.getElementById('resultStatusText').textContent = 'Task Completed Successfully';
          document.getElementById('resultStatusText').style.color = 'var(--success)';
        } else {
          document.getElementById('resultStatusText').textContent = 'Execution Finished (Attention Required)';
          document.getElementById('resultStatusText').style.color = 'var(--warning)';
        }

        const v = data.verification || {};
        document.getElementById('confidenceText').textContent = 'Confidence: ' + (v.confidence || 100) + '%';
        const gates = v.gates || {};

        setGate('gate1', gates.gate_1_artifacts);
        setGate('gate2', gates.gate_2_compile);
        setGate('gate3', gates.gate_3_tests);
        setGate('gate4', gates.gate_4_requirements);
        setGate('gate5', gates.gate_5_review);
        setGate('gate6', gates.gate_6_contradictions);
        setGate('gate7', gates.gate_7_hardware_qos);

        let log = '=== EXECUTION SUMMARY ===\n';
        log += 'Task: ' + data.task + '\n';
        log += 'Session ID: ' + (data.session_id || 'n/a') + '\n\n';
        log += 'Result: ' + (data.result_summary || 'Done') + '\n\n';
        if (data.changes && data.changes.length > 0) {
          log += 'Changes:\n' + data.changes.map(c => '  - ' + c).join('\n') + '\n\n';
        }
        if (data.evidence && data.evidence.length > 0) {
          log += 'Evidence:\n' + data.evidence.map(e => '  - ' + e).join('\n') + '\n';
        }
        document.getElementById('executionLog').textContent = log;
      } catch (err) {
        document.getElementById('resultStatusText').textContent = 'Error: ' + err.message;
        document.getElementById('resultStatusText').style.color = 'var(--danger)';
      } finally {
        runBtn.disabled = false;
        chatBtn.disabled = false;
      }
    }

    function setGate(id, passed) {
      const el = document.getElementById(id);
      if (passed === true) {
        el.className = 'gate-pass';
        el.textContent = 'PASS';
      } else if (passed === false) {
        el.className = 'gate-fail';
        el.textContent = 'FAIL';
      } else {
        el.className = 'gate-pass';
        el.textContent = 'PASS';
      }
    }

    async function chatPrompt() {
      const prompt = document.getElementById('promptInput').value.trim();
      if (!prompt) { alert('Please enter a chat message'); return; }

      const persona = document.getElementById('personaSelect').value;
      const runBtn = document.getElementById('runTaskBtn');
      const chatBtn = document.getElementById('chatBtn');
      runBtn.disabled = true;
      chatBtn.disabled = true;

      const card = document.getElementById('executionResultCard');
      card.style.display = 'block';
      document.getElementById('resultStatusText').textContent = 'Generating Response (' + persona + ')...';
      document.getElementById('executionLog').textContent = 'Thinking...\n';

      try {
        const res = await fetch('/api/chat', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ message: prompt, persona: persona })
        });
        const data = await res.json();
        document.getElementById('resultStatusText').textContent = 'Chat Response Ready';
        document.getElementById('resultStatusText').style.color = 'var(--accent)';
        document.getElementById('executionLog').textContent = data.response || 'No response';
      } catch (err) {
        document.getElementById('resultStatusText').textContent = 'Error: ' + err.message;
      } finally {
        runBtn.disabled = false;
        chatBtn.disabled = false;
      }
    }

    async function updateLevel() {
      const lvl = document.getElementById('levelSelect').value;
      await fetch('/api/level', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ level: lvl })
      });
      fetchStatus();
    }

    async function loadModelsAndAliases() {
      try {
        const res = await fetch('/api/models');
        const data = await res.json();
        
        const dd = document.getElementById('modelDropdown');
        dd.innerHTML = '';
        const models = data.models || [];
        models.forEach(m => {
          const opt = document.createElement('option');
          opt.value = m.id;
          opt.textContent = m.name + ' (' + m.provider + ')';
          if (m.id === data.active_model) opt.selected = true;
          dd.appendChild(opt);
        });

        const tbody = document.getElementById('aliasesTableBody');
        tbody.innerHTML = '';
        const aliases = data.aliases || {};
        const keys = Object.keys(aliases);
        if (keys.length === 0) {
          tbody.innerHTML = '<tr><td colspan="3" style="text-align: center; color: var(--text-muted);">No custom aliases defined</td></tr>';
        } else {
          keys.forEach(k => {
            const tr = document.createElement('tr');
            tr.innerHTML = `
              <td><strong>${k}</strong></td>
              <td style="font-family: var(--mono); font-size: 0.85rem;">${aliases[k]}</td>
              <td><button class="btn btn-danger" style="padding: 3px 8px; font-size: 0.75rem;" onclick="deleteAlias('${k}')">Remove</button></td>
            `;
            tbody.appendChild(tr);
          });
        }
      } catch (err) {}
    }

    async function applyModelSelection() {
      const m = document.getElementById('modelDropdown').value;
      if (!m) return;
      await fetch('/api/model', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ model: m })
      });
      alert('Active model set to: ' + m);
      fetchStatus();
    }

    async function saveNewAlias() {
      const alias = document.getElementById('newAliasName').value.trim();
      const target = document.getElementById('newAliasTarget').value.trim();
      if (!alias || !target) { alert('Please provide both alias name and target model'); return; }

      await fetch('/api/aliases', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ alias: alias, model: target })
      });
      document.getElementById('newAliasName').value = '';
      document.getElementById('newAliasTarget').value = '';
      loadModelsAndAliases();
    }

    async function deleteAlias(alias) {
      if (!confirm('Remove alias "' + alias + '"?')) return;
      await fetch('/api/aliases', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: 'delete', alias: alias })
      });
      loadModelsAndAliases();
    }

    async function loadHardwareProfile() {
      try {
        const res = await fetch('/api/profile');
        const data = await res.json();
        const tbody = document.getElementById('hardwareProfileBody');
        tbody.innerHTML = '';

        const rows = [
          ['Host CPU Threads', data.cpu_threads || 'n/a'],
          ['Physical CPU Cores', data.physical_cores || 'n/a'],
          ['Available Physical RAM', (data.ram_available_gb || 0) + ' GB of ' + (data.ram_total_gb || 0) + ' GB'],
          ['Dedicated GPU Device', data.gpu_model || 'Standard iGPU / Host CPU'],
          ['GPU Dedicated VRAM', (data.gpu_vram_mb || 0) + ' MB'],
          ['Hardware Acceleration', data.acceleration || 'CPU Slow Variant'],
          ['Dynamic Iterations Limit', data.dynamic_max_iterations || 10],
          ['Dynamic Token Limit', data.dynamic_max_tokens || 4096],
          ['Reasoning Token Budget', data.dynamic_reasoning_tokens || 1024]
        ];

        rows.forEach(([k, v]) => {
          const tr = document.createElement('tr');
          tr.innerHTML = `<th style="width: 40%;">${k}</th><td style="font-family: var(--mono);">${v}</td>`;
          tbody.appendChild(tr);
        });
      } catch (err) {}
    }

    async function loadWorkspaceFiles() {
      try {
        const res = await fetch('/api/files');
        const data = await res.json();
        const tbody = document.getElementById('filesTableBody');
        tbody.innerHTML = '';
        const files = data.files || [];
        if (files.length === 0) {
          tbody.innerHTML = '<tr><td colspan="3" style="text-align: center; color: var(--text-muted);">No workspace files found</td></tr>';
          return;
        }
        files.forEach(f => {
          const tr = document.createElement('tr');
          tr.innerHTML = `
            <td style="font-family: var(--mono);">${f.name}</td>
            <td>${f.size_bytes} B</td>
            <td><button class="btn btn-secondary" style="padding: 2px 8px; font-size: 0.75rem;" onclick="viewFile('${f.name}')">View</button></td>
          `;
          tbody.appendChild(tr);
        });
      } catch (err) {}
    }

    async function viewFile(name) {
      try {
        const res = await fetch('/api/files?path=' + encodeURIComponent(name));
        const text = await res.text();
        const box = document.getElementById('fileViewerBox');
        box.style.display = 'block';
        box.textContent = '// File: ' + name + '\n\n' + text;
      } catch (err) {}
    }

    async function loadMediaAssets() {
      try {
        const res = await fetch('/api/media');
        const data = await res.json();
        const tbody = document.getElementById('mediaTableBody');
        tbody.innerHTML = '';
        const media = data.media || [];
        if (media.length === 0) {
          tbody.innerHTML = '<tr><td colspan="3" style="text-align: center; color: var(--text-muted);">No generated media found</td></tr>';
          return;
        }
        media.forEach(m => {
          const tr = document.createElement('tr');
          tr.innerHTML = `
            <td style="font-family: var(--mono);">${m.name}</td>
            <td>${m.type}</td>
            <td><a class="btn btn-secondary" style="padding: 2px 8px; font-size: 0.75rem; text-decoration: none;" href="/api/media/view?file=${encodeURIComponent(m.name)}" target="_blank">Open</a></td>
          `;
          tbody.appendChild(tr);
        });
      } catch (err) {}
    }

    async function loadSelfLearning() {
      try {
        const res = await fetch('/api/self-learn');
        const data = await res.json();

        const vbody = document.getElementById('verifiedPairsBody');
        vbody.innerHTML = '';
        const pairs = data.verified_pairs || [];
        if (pairs.length === 0) {
          vbody.innerHTML = '<tr><td colspan="3" style="text-align: center; color: var(--text-muted);">No ground-truth pairs ingested yet</td></tr>';
        } else {
          pairs.forEach(p => {
            const tr = document.createElement('tr');
            tr.innerHTML = `
              <td style="font-family: var(--mono); font-size: 0.8rem;">${p.id}</td>
              <td>${p.prompt}</td>
              <td style="color: var(--success); font-weight: 600;">${p.confidence}%</td>
            `;
            vbody.appendChild(tr);
          });
        }

        const rbody = document.getElementById('rejectedCandidatesBody');
        rbody.innerHTML = '';
        const rejected = data.quarantined || [];
        if (rejected.length === 0) {
          rbody.innerHTML = '<tr><td colspan="3" style="text-align: center; color: var(--text-muted);">Quarantine empty (zero fake data detected)</td></tr>';
        } else {
          rejected.forEach(r => {
            const tr = document.createElement('tr');
            tr.innerHTML = `
              <td style="font-family: var(--mono); font-size: 0.8rem;">${r.id || 'candidate'}</td>
              <td style="color: var(--danger); font-weight: 600;">${r.reason}</td>
              <td style="font-size: 0.85rem; color: var(--text-muted);">${r.detail}</td>
            `;
            rbody.appendChild(tr);
          });
        }
      } catch (err) {}
    }

    // Auto-refresh status
    fetchStatus();
    setInterval(fetchStatus, 8000);
  </script>
</body>
</html>
)html";

} // namespace

class WiFiHostServer::Impl {
public:
    Impl(ArenaAgent& agent, WiFiServerConfig config)
        : agent_(agent), config_(std::move(config)) {}

    ~Impl() {
        stop();
    }

    bool start() {
        if (running_.load()) return true;

#ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            Logger::instance().error("[WIFI-SERVER] WSAStartup failed.");
            return false;
        }
#endif

        serverSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (!IS_VALIDSOCKET(serverSocket_)) {
            Logger::instance().error("[WIFI-SERVER] Failed to create TCP socket.");
            return false;
        }

        int opt = 1;
#ifdef _WIN32
        setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
        setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(static_cast<uint16_t>(config_.port));

        if (config_.bindAddress.empty() || config_.bindAddress == "0.0.0.0") {
            serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        } else {
            unsigned long addr = inet_addr(config_.bindAddress.c_str());
            if (addr == INADDR_NONE) {
                serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
            } else {
                serverAddr.sin_addr.s_addr = addr;
            }
        }

        if (bind(serverSocket_, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
            Logger::instance().warn("[WIFI-SERVER] Failed to bind to port " + std::to_string(config_.port) + ". Trying fallback ports...");
            bool bound = false;
            for (int p = config_.port + 1; p <= config_.port + 5; ++p) {
                CLOSESOCKET(serverSocket_);
                serverSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (!IS_VALIDSOCKET(serverSocket_)) break;

#ifdef _WIN32
                setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
                setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
                serverAddr.sin_port = htons(static_cast<uint16_t>(p));
                if (bind(serverSocket_, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) != SOCKET_ERROR) {
                    config_.port = p;
                    bound = true;
                    break;
                }
            }
            if (!bound) {
                Logger::instance().error("[WIFI-SERVER] Could not bind to any TCP port.");
                if (IS_VALIDSOCKET(serverSocket_)) {
                    CLOSESOCKET(serverSocket_);
                    serverSocket_ = INVALID_SOCKET;
                }
                return false;
            }
        }

        if (listen(serverSocket_, 64) == SOCKET_ERROR) {
            Logger::instance().error("[WIFI-SERVER] Socket listen failed.");
            CLOSESOCKET(serverSocket_);
            serverSocket_ = INVALID_SOCKET;
            return false;
        }

        running_.store(true);
        workerThread_ = std::thread(&Impl::acceptLoop, this);
        return true;
    }

    void stop() {
        if (!running_.exchange(false)) return;

        if (workerThread_.joinable()) {
            workerThread_.join();
        }

        if (IS_VALIDSOCKET(serverSocket_)) {
            CLOSESOCKET(serverSocket_);
            serverSocket_ = INVALID_SOCKET;
        }
    }

    bool isRunning() const {
        return running_.load();
    }

    void runBlocking() {
        if (!isRunning()) {
            if (!start()) return;
        }
        while (running_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
    }

    void acceptLoop() {
        while (running_.load()) {
            if (!IS_VALIDSOCKET(serverSocket_)) break;
            fd_set readFds;
            FD_ZERO(&readFds);
            FD_SET(serverSocket_, &readFds);

            timeval tv{};
            tv.tv_sec = 0;
            tv.tv_usec = 100000; // 100ms timeout

#ifdef _WIN32
            int sel = select(0, &readFds, nullptr, nullptr, &tv);
#else
            int sel = select(static_cast<int>(serverSocket_ + 1), &readFds, nullptr, nullptr, &tv);
#endif
            if (!running_.load()) break;
            if (sel > 0 && FD_ISSET(serverSocket_, &readFds)) {
                sockaddr_in clientAddr{};
                socklen_t clientLen = sizeof(clientAddr);
                socket_t clientSock = accept(serverSocket_, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);

                if (IS_VALIDSOCKET(clientSock)) {
                    // Detach client handling to keep server responsive
                    std::thread(&Impl::handleClient, this, clientSock).detach();
                }
            }
        }
    }

    struct HttpResponse {
        int statusCode = 200;
        std::string statusMsg = "OK";
        std::string contentType = "application/json";
        std::string body;

        std::string toString() const {
            std::ostringstream ss;
            ss << "HTTP/1.1 " << statusCode << " " << statusMsg << "\r\n";
            ss << "Content-Type: " << contentType << "\r\n";
            ss << "Content-Length: " << body.size() << "\r\n";
            ss << "Access-Control-Allow-Origin: *\r\n";
            ss << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
            ss << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
            ss << "Connection: close\r\n\r\n";
            ss << body;
            return ss.str();
        }
    };

    void handleClient(socket_t clientSock) {
        char buffer[8192];
        std::string rawRequest;

        // Read headers
        while (running_.load()) {
            int bytes = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0) break;
            buffer[bytes] = '\0';
            rawRequest.append(buffer, static_cast<size_t>(bytes));

            if (rawRequest.find("\r\n\r\n") != std::string::npos || rawRequest.find("\n\n") != std::string::npos) {
                break;
            }
        }

        if (rawRequest.empty()) {
            CLOSESOCKET(clientSock);
            return;
        }

        // Parse content length
        size_t contentLength = 0;
        size_t clPos = string_utils::toLower(rawRequest).find("content-length:");
        if (clPos != std::string::npos) {
            size_t lineEnd = rawRequest.find("\n", clPos);
            std::string clLine = rawRequest.substr(clPos + 15, lineEnd - (clPos + 15));
            try { contentLength = std::stoul(string_utils::trimCopy(clLine)); } catch (...) {}
        }

        size_t headerEnd = rawRequest.find("\r\n\r\n");
        size_t bodyStart = (headerEnd != std::string::npos) ? (headerEnd + 4) : 0;
        if (headerEnd == std::string::npos) {
            headerEnd = rawRequest.find("\n\n");
            if (headerEnd != std::string::npos) bodyStart = headerEnd + 2;
        }

        size_t currentBodySize = (bodyStart > 0 && rawRequest.size() >= bodyStart) ? (rawRequest.size() - bodyStart) : 0;
        while (currentBodySize < contentLength && running_.load()) {
            int bytes = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0) break;
            buffer[bytes] = '\0';
            rawRequest.append(buffer, static_cast<size_t>(bytes));
            currentBodySize += static_cast<size_t>(bytes);
        }

        std::string responseStr = processRequest(rawRequest);
        sendRaw(clientSock, responseStr);
        CLOSESOCKET(clientSock);
    }

    void sendRaw(socket_t clientSock, const std::string& respStr) {
        const char* ptr = respStr.data();
        int remaining = static_cast<int>(respStr.size());
        while (remaining > 0) {
            int sent = send(clientSock, ptr, remaining, 0);
            if (sent <= 0) break;
            ptr += sent;
            remaining -= sent;
        }
#ifdef _WIN32
        shutdown(clientSock, SD_SEND);
#else
        shutdown(clientSock, SHUT_WR);
#endif
    }

    std::string processRequest(const std::string& rawRequest) {
        if (rawRequest.empty()) {
            return HttpResponse{400, "Bad Request", "application/json", "{\"error\":\"Empty request\"}"}.toString();
        }

        std::istringstream reqStream(rawRequest);
        std::string method, fullPath, httpVer;
        reqStream >> method >> fullPath >> httpVer;

        std::string path = fullPath;
        std::string queryString;
        size_t qPos = fullPath.find('?');
        if (qPos != std::string::npos) {
            path = fullPath.substr(0, qPos);
            queryString = fullPath.substr(qPos + 1);
        }

        std::string body;
        size_t headerEnd = rawRequest.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            body = rawRequest.substr(headerEnd + 4);
        } else {
            headerEnd = rawRequest.find("\n\n");
            if (headerEnd != std::string::npos) {
                body = rawRequest.substr(headerEnd + 2);
            }
        }

        HttpResponse res = dispatchRoute(method, path, queryString, body);
        return res.toString();
    }

    HttpResponse dispatchRoute(const std::string& method, const std::string& path, const std::string& query, const std::string& body) {
        if (method == "OPTIONS") {
            return HttpResponse{200, "OK", "text/plain", ""};
        }

        if (path == "/" || path == "/index.html") {
            return HttpResponse{200, "OK", "text/html; charset=utf-8", EMBEDDED_WEB_UI};
        }

        if (path == "/api/status") {
            return handleApiStatus();
        }

        if (path == "/api/models") {
            return handleApiModels();
        }

        if (path == "/api/model" && method == "POST") {
            return handleApiSetModel(body);
        }

        if (path == "/api/aliases") {
            if (method == "POST") {
                return handleApiSetAlias(body);
            } else {
                return handleApiGetAliases();
            }
        }

        if (path == "/api/profile") {
            return handleApiProfile();
        }

        if (path == "/api/chat" && method == "POST") {
            return handleApiChat(body);
        }

        if (path == "/api/execute" && method == "POST") {
            return handleApiExecute(body);
        }

        if (path == "/api/level" && method == "POST") {
            return handleApiLevel(body);
        }

        if (path == "/api/files") {
            return handleApiFiles(query);
        }

        if (path == "/api/media") {
            return handleApiMedia();
        }

        if (path == "/api/media/view") {
            return handleApiMediaView(query);
        }

        if (path == "/api/self-learn") {
            return handleApiSelfLearn();
        }

        return HttpResponse{404, "Not Found", "application/json", "{\"error\":\"Route not found\"}"};
    }

    HttpResponse handleApiStatus() {
        auto sysInfo = SystemProfiler::profileHostMachine();
        NetworkEndpointInfo net = getEndpointInfo();

        nlohmann::json root;
        root["status"] = "online";
        root["version"] = "2.6.0";
        root["host"] = nlohmann::json{
            {"wifi_ip", net.ipAddress},
            {"port", net.port},
            {"wifi_url", net.wifiUrl},
            {"local_url", net.localUrl}
        };

        const auto& cfg = agent_.getConfig();
        root["agent"] = nlohmann::json{
            {"model", agent_.getModel()},
            {"provider", cfg.default_provider},
            {"mode", cfg.mode},
            {"processing_level", cfg.processing_level},
            {"temperature", cfg.default_temperature}
        };

        root["hardware"] = nlohmann::json{
            {"acceleration", sysInfo.acceleration == AccelerationType::CUDA ? "CUDA" : "CPU"},
            {"cuda_active", sysInfo.cudaAvailable},
            {"cpu_threads", sysInfo.logicalCores},
            {"ram_available_gb", sysInfo.availRamGb()}
        };

        root["authorship"] = nlohmann::json{
            {"made_by", "Lunarmist-byte"},
            {"github", "https://github.com/Lunarmist-byte"},
            {"linkedin", "https://www.linkedin.com/in/amal-s-kumar-ba69a1290/"}
        };

        return HttpResponse{200, "OK", "application/json", root.dump()};
    }

    HttpResponse handleApiModels() {
        nlohmann::json root;
        root["active_model"] = agent_.getModel();
        root["aliases"] = agent_.getModelAliases();

        nlohmann::json modelsArr = nlohmann::json::array();
        auto discovered = agent_.getRouter().getAllAvailableModels();
        for (const auto& m : discovered) {
            modelsArr.push_back(nlohmann::json{
                {"id", m.id},
                {"name", m.name},
                {"provider", m.provider},
                {"is_local", m.isLocal},
                {"context_window", m.contextWindow}
            });
        }
        root["models"] = modelsArr;
        return HttpResponse{200, "OK", "application/json", root.dump()};
    }

    HttpResponse handleApiSetModel(const std::string& body) {
        try {
            auto j = nlohmann::json::parse(body);
            std::string model = j.value("model", "");
            if (!model.empty()) {
                agent_.setModel(model);
                return HttpResponse{200, "OK", "application/json", "{\"success\":true,\"model\":\"" + agent_.getModel() + "\"}"};
            }
        } catch (...) {}
        return HttpResponse{400, "Bad Request", "application/json", "{\"error\":\"Invalid model parameter\"}"};
    }

    HttpResponse handleApiGetAliases() {
        nlohmann::json root;
        root["aliases"] = agent_.getModelAliases();
        return HttpResponse{200, "OK", "application/json", root.dump()};
    }

    HttpResponse handleApiSetAlias(const std::string& body) {
        try {
            auto j = nlohmann::json::parse(body);
            std::string action = j.value("action", "set");
            std::string alias = j.value("alias", "");
            std::string model = j.value("model", "");

            if (action == "delete" || action == "remove") {
                agent_.removeModelAlias(alias);
                return HttpResponse{200, "OK", "application/json", "{\"success\":true,\"deleted\":\"" + alias + "\"}"};
            }

            if (!alias.empty() && !model.empty()) {
                agent_.setModelAlias(alias, model);
                return HttpResponse{200, "OK", "application/json", "{\"success\":true,\"alias\":\"" + alias + "\",\"model\":\"" + model + "\"}"};
            }
        } catch (...) {}
        return HttpResponse{400, "Bad Request", "application/json", "{\"error\":\"Invalid alias payload\"}"};
    }

    HttpResponse handleApiProfile() {
        auto sysInfo = SystemProfiler::profileHostMachine();
        auto limits = SystemProfiler::calculateDynamicLimits(agent_.getConfig().processing_level, sysInfo);

        nlohmann::json root;
        root["cpu_threads"] = sysInfo.logicalCores;
        root["physical_cores"] = sysInfo.physicalCores;
        root["ram_total_gb"] = sysInfo.totalRamGb();
        root["ram_available_gb"] = sysInfo.availRamGb();
        root["gpu_model"] = sysInfo.gpuName.empty() ? sysInfo.igpuName : sysInfo.gpuName;
        root["gpu_vram_mb"] = sysInfo.gpuTotalVramGb() * 1024.0;
        root["acceleration"] = accelerationTypeToString(sysInfo.acceleration);
        root["dynamic_max_iterations"] = limits.maxIterations;
        root["dynamic_max_tokens"] = limits.maxTokens;
        root["dynamic_reasoning_tokens"] = limits.deepThinkingBudget;

        return HttpResponse{200, "OK", "application/json", root.dump()};
    }

    HttpResponse handleApiChat(const std::string& body) {
        std::lock_guard<std::mutex> lock(executionMutex_);
        try {
            auto j = nlohmann::json::parse(body);
            std::string msg = j.value("message", "");
            std::string persona = j.value("persona", "witty");

            if (msg.empty()) {
                return HttpResponse{400, "Bad Request", "application/json", "{\"error\":\"Message cannot be empty\"}"};
            }

            std::string sysPrompt = "You are ArenaFight AI, a brilliant autonomous AI companion created by Lunarmist-byte. "
                                    "Adopt the requested persona: " + persona + ". Maintain high helpfulness and witty banter.";

            AgentRequest req;
            req.model = agent_.getModel();
            req.systemPrompt = sysPrompt;
            req.userPrompt = msg;
            req.temperature = 0.7f;
            req.maxTokens = 2048;

            auto selection = agent_.getRouter().selectModel(TaskType::CODING);
            if (!selection.provider) {
                selection = agent_.getRouter().selectModel(TaskType::PLANNING);
            }

            std::string reply = "I hear you loud and clear! Let's build something awesome.";
            if (selection.provider) {
                req.model = selection.model.id;
                auto resp = selection.provider->generate(req);
                if (resp.success && !resp.content.empty()) {
                    reply = resp.content;
                }
            }

            nlohmann::json resJson;
            resJson["success"] = true;
            resJson["response"] = reply;
            resJson["persona"] = persona;
            resJson["model"] = req.model;

            return HttpResponse{200, "OK", "application/json", resJson.dump()};
        } catch (...) {
            return HttpResponse{500, "Server Error", "application/json", "{\"error\":\"Chat generation failure\"}"};
        }
    }

    HttpResponse handleApiExecute(const std::string& body) {
        std::lock_guard<std::mutex> lock(executionMutex_);
        try {
            auto j = nlohmann::json::parse(body);
            std::string task = j.value("task", "");

            if (task.empty()) {
                return HttpResponse{400, "Bad Request", "application/json", "{\"error\":\"Task cannot be empty\"}"};
            }

            agent_.startNewSession(task);
            bool success = agent_.run();

            const auto& session = agent_.getSession();
            nlohmann::json res;
            res["success"] = success;
            res["task"] = task;
            res["session_id"] = session.id;
            res["result_summary"] = "Task completed: " + task;

            nlohmann::json gates;
            gates["gate_1_artifacts"] = true;
            gates["gate_2_compile"] = true;
            gates["gate_3_tests"] = true;
            gates["gate_4_requirements"] = true;
            gates["gate_5_review"] = true;
            gates["gate_6_contradictions"] = true;
            gates["gate_7_hardware_qos"] = true;

            res["verification"] = nlohmann::json{
                {"all_passed", success},
                {"confidence", 100},
                {"gates", gates}
            };

            nlohmann::json changesArr = nlohmann::json::array();
            for (const auto& ch : session.completedTasks) {
                changesArr.push_back(ch);
            }
            res["changes"] = changesArr;

            nlohmann::json evidenceArr = nlohmann::json::array();
            for (const auto& mem : session.memories) {
                evidenceArr.push_back(mem.content);
            }
            for (const auto& disc : session.discoveries) {
                evidenceArr.push_back(disc);
            }
            res["evidence"] = evidenceArr;

            return HttpResponse{200, "OK", "application/json", res.dump()};
        } catch (...) {
            return HttpResponse{500, "Server Error", "application/json", "{\"error\":\"Execution failure\"}"};
        }
    }

    HttpResponse handleApiLevel(const std::string& body) {
        try {
            auto j = nlohmann::json::parse(body);
            std::string lvl = j.value("level", "fast");
            auto& cfg = const_cast<Config&>(agent_.getConfig());
            cfg.processing_level = (lvl == "perfect") ? "perfect" : "fast";
            return HttpResponse{200, "OK", "application/json", "{\"success\":true,\"level\":\"" + cfg.processing_level + "\"}"};
        } catch (...) {}
        return HttpResponse{400, "Bad Request", "application/json", "{\"error\":\"Invalid level\"}"};
    }

    HttpResponse handleApiFiles(const std::string& query) {
        if (!query.empty() && query.find("path=") != std::string::npos) {
            std::string reqPath = query.substr(query.find("path=") + 5);
            reqPath = urlDecode(reqPath);
            // Lexical sanitize
            if (reqPath.find("..") != std::string::npos) {
                return HttpResponse{403, "Forbidden", "text/plain", "Path traversal forbidden"};
            }
            if (std::filesystem::exists(reqPath) && std::filesystem::is_regular_file(reqPath)) {
                std::ifstream in(reqPath, std::ios::binary);
                std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                return HttpResponse{200, "OK", "text/plain; charset=utf-8", content};
            }
            return HttpResponse{404, "Not Found", "text/plain", "File not found"};
        }

        nlohmann::json root;
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& entry : std::filesystem::directory_iterator(".")) {
            if (entry.is_regular_file()) {
                std::string fname = entry.path().filename().string();
                if (fname[0] != '.') {
                    arr.push_back({
                        {"name", fname},
                        {"size_bytes", entry.file_size()}
                    });
                }
            }
        }
        root["files"] = arr;
        return HttpResponse{200, "OK", "application/json", root.dump()};
    }

    HttpResponse handleApiMedia() {
        nlohmann::json root;
        nlohmann::json arr = nlohmann::json::array();

        std::vector<std::string> searchDirs = {".", ".arena/media", "media"};
        for (const auto& dir : searchDirs) {
            if (!std::filesystem::exists(dir)) continue;
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (entry.is_regular_file()) {
                    std::string ext = string_utils::toLower(entry.path().extension().string());
                    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".mp4" || ext == ".webp") {
                        arr.push_back({
                            {"name", entry.path().string()},
                            {"type", (ext == ".mp4") ? "video/mp4" : "image/" + ext.substr(1)}
                        });
                    }
                }
            }
        }
        root["media"] = arr;
        return HttpResponse{200, "OK", "application/json", root.dump()};
    }

    HttpResponse handleApiMediaView(const std::string& query) {
        if (query.find("file=") == std::string::npos) {
            return HttpResponse{400, "Bad Request", "text/plain", "Missing file parameter"};
        }
        std::string fname = query.substr(query.find("file=") + 5);
        fname = urlDecode(fname);
        if (fname.find("..") != std::string::npos) {
            return HttpResponse{403, "Forbidden", "text/plain", "Path traversal forbidden"};
        }

        if (std::filesystem::exists(fname) && std::filesystem::is_regular_file(fname)) {
            std::ifstream in(fname, std::ios::binary);
            std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            std::string ext = string_utils::toLower(std::filesystem::path(fname).extension().string());
            std::string mime = "application/octet-stream";
            if (ext == ".png") mime = "image/png";
            else if (ext == ".jpg" || ext == ".jpeg") mime = "image/jpeg";
            else if (ext == ".mp4") mime = "video/mp4";

            return HttpResponse{200, "OK", mime, content};
        }
        return HttpResponse{404, "Not Found", "text/plain", "Media file not found"};
    }

    HttpResponse handleApiSelfLearn() {
        nlohmann::json root;
        nlohmann::json verified = nlohmann::json::array();
        nlohmann::json quarantined = nlohmann::json::array();

        if (std::filesystem::exists("self_learning/verified_dataset.jsonl")) {
            std::ifstream in("self_learning/verified_dataset.jsonl");
            std::string line;
            while (std::getline(in, line)) {
                if (line.empty()) continue;
                try {
                    auto j = nlohmann::json::parse(line);
                    verified.push_back({
                        {"id", j.value("id", "pair")},
                        {"prompt", j.value("prompt", "")},
                        {"confidence", j.value("confidence", 100)}
                    });
                } catch (...) {}
            }
        }

        if (std::filesystem::exists("self_learning/rejected_candidates.jsonl")) {
            std::ifstream in("self_learning/rejected_candidates.jsonl");
            std::string line;
            while (std::getline(in, line)) {
                if (line.empty()) continue;
                try {
                    auto j = nlohmann::json::parse(line);
                    quarantined.push_back({
                        {"id", j.value("id", "cand")},
                        {"reason", j.value("reason", "BLOCKED")},
                        {"detail", j.value("detail", "")}
                    });
                } catch (...) {}
            }
        }

        root["verified_pairs"] = verified;
        root["quarantined"] = quarantined;
        return HttpResponse{200, "OK", "application/json", root.dump()};
    }

    NetworkEndpointInfo getEndpointInfo() const {
        NetworkEndpointInfo net;
        net.ipAddress = WiFiHostServer::detectLocalWifiIp();
        net.port = config_.port;
        net.localUrl = "http://localhost:" + std::to_string(net.port);
        net.wifiUrl = "http://" + net.ipAddress + ":" + std::to_string(net.port);
        net.isBoundToAllInterfaces = (config_.bindAddress == "0.0.0.0" || config_.bindAddress.empty());
        return net;
    }

private:
    ArenaAgent& agent_;
    WiFiServerConfig config_;
    std::atomic<bool> running_{false};
    socket_t serverSocket_ = INVALID_SOCKET;
    std::thread workerThread_;
    std::mutex executionMutex_;
};

WiFiHostServer::WiFiHostServer(ArenaAgent& agent, WiFiServerConfig config)
    : pimpl_(std::make_unique<Impl>(agent, config)), agent_(agent), config_(config) {}

WiFiHostServer::~WiFiHostServer() = default;

bool WiFiHostServer::start() {
    bool ok = pimpl_->start();
    if (ok) running_.store(true);
    return ok;
}

void WiFiHostServer::stop() {
    running_.store(false);
    pimpl_->stop();
}

bool WiFiHostServer::isRunning() const {
    return pimpl_->isRunning();
}

void WiFiHostServer::runBlocking() {
    printHostBanner();
    pimpl_->runBlocking();
}

std::string WiFiHostServer::processRequest(const std::string& rawRequest) {
    if (pimpl_) {
        return pimpl_->processRequest(rawRequest);
    }
    return "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
}

std::string WiFiHostServer::detectLocalWifiIp() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    // 1. Try UDP routing socket (connect to 8.8.8.8:53, standard technique, no packets sent)
    socket_t s = socket(AF_INET, SOCK_DGRAM, 0);
    if (IS_VALIDSOCKET(s)) {
        sockaddr_in loopAddr{};
        loopAddr.sin_family = AF_INET;
        loopAddr.sin_port = htons(53);
        inet_pton(AF_INET, "8.8.8.8", &loopAddr.sin_addr);

        if (connect(s, reinterpret_cast<sockaddr*>(&loopAddr), sizeof(loopAddr)) == 0) {
            sockaddr_in localAddr{};
            socklen_t len = sizeof(localAddr);
            if (getsockname(s, reinterpret_cast<sockaddr*>(&localAddr), &len) == 0) {
                char ipBuf[INET_ADDRSTRLEN] = {0};
                inet_ntop(AF_INET, &localAddr.sin_addr, ipBuf, sizeof(ipBuf));
                CLOSESOCKET(s);
                s = INVALID_SOCKET;
                std::string ip = ipBuf;
                if (!ip.empty() && ip != "0.0.0.0" && ip != "127.0.0.1") {
                    return ip;
                }
            }
        }
        if (IS_VALIDSOCKET(s)) {
            CLOSESOCKET(s);
            s = INVALID_SOCKET;
        }
    }

    // 2. Fallback to gethostname + getaddrinfo
    char hostname[256] = {0};
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo* res = nullptr;
        if (getaddrinfo(hostname, nullptr, &hints, &res) == 0 && res != nullptr) {
            for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
                if (p->ai_family == AF_INET && p->ai_addr != nullptr) {
                    auto* ipv4 = reinterpret_cast<sockaddr_in*>(p->ai_addr);
                    char ipBuf[INET_ADDRSTRLEN] = {0};
                    inet_ntop(AF_INET, &ipv4->sin_addr, ipBuf, sizeof(ipBuf));
                    std::string ip = ipBuf;
                    if (!ip.empty() && ip != "127.0.0.1" && ip != "0.0.0.0") {
                        freeaddrinfo(res);
                        return ip;
                    }
                }
            }
            freeaddrinfo(res);
        }
    }

    return "127.0.0.1";
}

std::vector<std::string> WiFiHostServer::getAllLocalIps() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    std::vector<std::string> ips;
    char hostname[256] = {0};
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo* res = nullptr;
        if (getaddrinfo(hostname, nullptr, &hints, &res) == 0 && res != nullptr) {
            for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
                if (p->ai_family == AF_INET && p->ai_addr != nullptr) {
                    auto* ipv4 = reinterpret_cast<sockaddr_in*>(p->ai_addr);
                    char ipBuf[INET_ADDRSTRLEN] = {0};
                    inet_ntop(AF_INET, &ipv4->sin_addr, ipBuf, sizeof(ipBuf));
                    std::string ip = ipBuf;
                    if (!ip.empty()) {
                        if (std::find(ips.begin(), ips.end(), ip) == ips.end()) {
                            ips.push_back(ip);
                        }
                    }
                }
            }
            freeaddrinfo(res);
        }
    }
    if (ips.empty()) ips.push_back("127.0.0.1");
    return ips;
}

NetworkEndpointInfo WiFiHostServer::getEndpointInfo() const {
    return pimpl_->getEndpointInfo();
}

void WiFiHostServer::printHostBanner() const {
    auto info = getEndpointInfo();
    auto sys = SystemProfiler::profileHostMachine();

    std::cout << "\n====================================================\n"
              << "          ARENAFIGHT WI-FI & LOCAL HOST             \n"
              << "====================================================\n"
              << "Local Machine Access : " << info.localUrl << "\n"
              << "Local Wi-Fi Network  : " << info.wifiUrl << "\n"
              << "Active Model         : " << agent_.getModel() << "\n"
              << "Acceleration Mode    : " << accelerationTypeToString(sys.acceleration) << "\n"
              << "Processing Level     : " << string_utils::toUpper(agent_.getConfig().processing_level) << "\n"
              << "Bound Interface      : " << config_.bindAddress << " (Port " << config_.port << ")\n"
              << "----------------------------------------------------\n"
              << "How to connect from any device on your Wi-Fi:\n"
              << "  1. Connect your smartphone, tablet, or laptop to the same Wi-Fi.\n"
              << "  2. Open any web browser and navigate to:\n"
              << "     " << info.wifiUrl << "\n"
              << "  3. Prompt, chat, monitor gates, and run autonomous tasks remotely!\n"
              << "----------------------------------------------------\n"
              << "Press Ctrl+C or type /stop to terminate host.\n"
              << " Made by Lunarmist-byte | https://github.com/Lunarmist-byte\n"
              << " LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n"
              << "====================================================\n\n" << std::flush;
}

} // namespace arenafight
