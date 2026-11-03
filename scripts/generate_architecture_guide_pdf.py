#!/usr/bin/env python3
"""
ArenaFight: Architectural Specification & Capabilities Guide
Generates a publication-grade technical PDF detailing what ArenaFight excels at.
Made by Lunarmist-byte
GitHub: https://github.com/Lunarmist-byte
LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/
"""

import os
import sys
from reportlab.lib.pagesizes import letter
from reportlab.lib import colors
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import inch
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, KeepTogether, HRFlowable
)
from reportlab.pdfgen import canvas

# Professional Palette: Slate 900, Blue 900, Sky 600, Off-White, Slate 800
PRIMARY_NAVY = colors.HexColor("#0f172a")       # Slate 900
ACCENT_BLUE  = colors.HexColor("#1e3a8a")       # Blue 900
CYAN_ACCENT  = colors.HexColor("#0284c7")       # Sky 600
TEXT_DARK    = colors.HexColor("#1e293b")       # Slate 800
TEXT_MUTED   = colors.HexColor("#475569")       # Slate 600
BG_LIGHT     = colors.HexColor("#f8fafc")       # Slate 50
BORDER_COLOR = colors.HexColor("#cbd5e1")       # Slate 300
CODE_BG      = colors.HexColor("#f1f5f9")       # Slate 100

class NumberedCanvas(canvas.Canvas):
    def __init__(self, *args, **kwargs):
        super(NumberedCanvas, self).__init__(*args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_page_decorations(num_pages)
            super(NumberedCanvas, self).showPage()
        super(NumberedCanvas, self).save()

    def draw_page_decorations(self, page_count):
        self.saveState()
        self.setFont("Helvetica-Bold", 8)
        self.setFillColor(TEXT_MUTED)

        # Header (pages 2+)
        if self._pageNumber > 1:
            self.drawString(54, 752, "ARENAFIGHT AI RUNTIME v2.5.0")
            self.setFont("Helvetica", 8)
            self.drawRightString(558, 752, "SYSTEM ARCHITECTURE & CAPABILITIES SPECIFICATION")
            self.setStrokeColor(BORDER_COLOR)
            self.setLineWidth(0.5)
            self.line(54, 746, 558, 746)

        # Footer (all pages)
        self.setStrokeColor(BORDER_COLOR)
        self.setLineWidth(0.5)
        self.line(54, 46, 558, 46)

        self.setFont("Helvetica-Bold", 7.5)
        self.setFillColor(PRIMARY_NAVY)
        self.drawString(54, 34, "Made by Lunarmist-byte")
        self.setFont("Helvetica", 7.5)
        self.setFillColor(TEXT_MUTED)
        self.drawString(160, 34, "|   GitHub: https://github.com/Lunarmist-byte   |   LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/")
        page_str = f"Page {self._pageNumber} of {page_count}"
        self.drawRightString(558, 34, page_str)

        self.restoreState()


def build_pdf(filename="docs/ArenaFight_Architecture_and_Capabilities.pdf"):
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    doc = SimpleDocTemplate(
        filename,
        pagesize=letter,
        leftMargin=54,
        rightMargin=54,
        topMargin=54,
        bottomMargin=54
    )

    styles = getSampleStyleSheet()

    # Typography styles
    title_style = ParagraphStyle(
        'DocTitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=21,
        leading=25,
        textColor=PRIMARY_NAVY,
        spaceAfter=3
    )

    subtitle_style = ParagraphStyle(
        'DocSubtitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=10,
        leading=14,
        textColor=CYAN_ACCENT,
        spaceAfter=10
    )

    meta_style = ParagraphStyle(
        'DocMeta',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8,
        leading=11.5,
        textColor=TEXT_MUTED,
        spaceAfter=10
    )

    h1_style = ParagraphStyle(
        'Heading1Custom',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=12,
        leading=16,
        textColor=PRIMARY_NAVY,
        spaceBefore=8,
        spaceAfter=4,
        keepWithNext=True
    )

    h2_style = ParagraphStyle(
        'Heading2Custom',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=9.5,
        leading=13,
        textColor=ACCENT_BLUE,
        spaceBefore=6,
        spaceAfter=3,
        keepWithNext=True
    )

    body_style = ParagraphStyle(
        'BodyCustom',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8.2,
        leading=11.5,
        textColor=TEXT_DARK,
        spaceAfter=4
    )

    bullet_style = ParagraphStyle(
        'BulletCustom',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8.2,
        leading=11.5,
        textColor=TEXT_DARK,
        leftIndent=12,
        firstLineIndent=-8,
        spaceAfter=3
    )

    table_header_style = ParagraphStyle(
        'TableHeader',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=8,
        leading=10.5,
        textColor=colors.white
    )

    table_cell_style = ParagraphStyle(
        'TableCell',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=7.5,
        leading=10,
        textColor=TEXT_DARK
    )

    table_cell_bold = ParagraphStyle(
        'TableCellBold',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=7.5,
        leading=10,
        textColor=PRIMARY_NAVY
    )

    code_style = ParagraphStyle(
        'CodeStyle',
        parent=styles['Normal'],
        fontName='Courier',
        fontSize=7.2,
        leading=9.5,
        textColor=PRIMARY_NAVY
    )

    story = []

    # =========================================================================
    # PAGE 1: EXECUTIVE OVERVIEW & LEVEL 1 PHYSICAL EVIDENCE ENGINE
    # =========================================================================
    story.append(Paragraph("ARENAFIGHT AI RUNTIME", title_style))
    story.append(Paragraph("System Architecture, Physical Evidence Verification &amp; Autonomous Multi-Model Engine", subtitle_style))
    story.append(Paragraph(
        "<b>Version:</b> 2.5.0 Enterprise CLI &nbsp;|&nbsp; <b>Release:</b> Production &nbsp;|&nbsp; <b>Target Platforms:</b> Windows (Win32/x64) &amp; Linux (POSIX)<br/>"
        "<b>Author:</b> Lunarmist-byte &nbsp;|&nbsp; <b>GitHub:</b> https://github.com/Lunarmist-byte &nbsp;|&nbsp; "
        "<b>LinkedIn:</b> https://www.linkedin.com/in/amal-s-kumar-ba69a1290/",
        meta_style
    ))
    story.append(HRFlowable(width="100%", thickness=1.5, color=CYAN_ACCENT, spaceBefore=0, spaceAfter=8))

    story.append(Paragraph("1. Executive Summary &amp; Design Philosophy", h1_style))
    story.append(Paragraph(
        "ArenaFight is a high-performance, deterministic C++20 multi-model autonomous agent runtime designed "
        "to completely eliminate hallucinated success in artificial intelligence workflows. While conventional agentic systems "
        "rely upon an LLM self-reporting whether its code or solution functions, ArenaFight enforces the <b>Level 1 "
        "Physical Dominance Rule</b>: an assertion is classified as ground truth only when corroborated by concrete "
        "operating system artifacts, process return codes, compiler diagnostics, and cryptographic hashes.",
        body_style
    ))
    story.append(Paragraph(
        "The runtime operates across dual paradigms: functioning either as a blistering fast, zero-overhead "
        "<b>Standalone Native CLI</b> with direct hardware and local provider hooks, or as a synchronized <b>OpenCode Connected "
        "Bridge</b>. It autonomously assesses host compute topologies, manages background AI service daemons, and enforces "
        "rigorous defense-in-depth isolation.",
        body_style
    ))

    story.append(Spacer(1, 4))
    story.append(Paragraph("2. What ArenaFight Does Well: Architectural Deep-Dive", h1_style))

    story.append(Paragraph("A. Level 1 Physical Dominance Rule &amp; 6 Physical Verification Gates", h2_style))
    story.append(Paragraph(
        "Every code mutation, task execution step, and generated artifact passes through an uncompromising 6-gate physical "
        "verification harness. Unverified assertions are strictly discarded by the ClaimRegistry.",
        body_style
    ))

    # Verification Gates Table
    gates_data = [
        [
            Paragraph("Gate #", table_header_style),
            Paragraph("Verification Type", table_header_style),
            Paragraph("Physical Evidence Sensor", table_header_style),
            Paragraph("Pass Criteria &amp; Deterministic Latch", table_header_style)
        ],
        [
            Paragraph("Gate 1", table_cell_bold),
            Paragraph("Compiler &amp; Build Diagnostics", table_cell_style),
            Paragraph("Direct OS process exit code + stderr pipe from g++, clang++, or cl.exe", table_cell_style),
            Paragraph("Zero compile errors, zero fatal diagnostics; clean binary artifact creation", table_cell_style)
        ],
        [
            Paragraph("Gate 2", table_cell_bold),
            Paragraph("Automated Test Suite Execution", table_cell_style),
            Paragraph("Test runner stdout stream, assertion counters, process exit status", table_cell_style),
            Paragraph("Process exit code 0; 100% assertions green; zero test failures or leaks", table_cell_style)
        ],
        [
            Paragraph("Gate 3", table_cell_bold),
            Paragraph("Process Execution Validation", table_cell_style),
            Paragraph("OS PID spawn, stdout/stderr capture, memory/cpu heartbeat", table_cell_style),
            Paragraph("Process terminates with return code 0 or matches expected output contract", table_cell_style)
        ],
        [
            Paragraph("Gate 4", table_cell_bold),
            Paragraph("Cryptographic Hash Integrity", table_cell_style),
            Paragraph("FIPS 180-4 SHA-256 digest computation over generated disk artifacts", table_cell_style),
            Paragraph("Hash matches manifest; audit log record appended with parent hash pointer", table_cell_style)
        ],
        [
            Paragraph("Gate 5", table_cell_bold),
            Paragraph("Filesystem Mutation Audit", table_cell_style),
            Paragraph("Pre/post execution directory diff and inode/timestamp tracking", table_cell_style),
            Paragraph("Target files created/modified within path boundaries; zero unauthorized leaks", table_cell_style)
        ],
        [
            Paragraph("Gate 6", table_cell_bold),
            Paragraph("Quality of Service (QoS)", table_cell_style),
            Paragraph("Static AST scanner inspecting heap allocation and blocking calls", table_cell_style),
            Paragraph("Zero-heap verification in ISR/tight-loops; deterministic latency bounded", table_cell_style)
        ]
    ]

    t_gates = Table(gates_data, colWidths=[46, 114, 172, 172])
    t_gates.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), PRIMARY_NAVY),
        ('ALIGN', (0,0), (-1,-1), 'LEFT'),
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('GRID', (0,0), (-1,-1), 0.5, BORDER_COLOR),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, BG_LIGHT]),
        ('TOPPADDING', (0,0), (-1,-1), 3),
        ('BOTTOMPADDING', (0,0), (-1,-1), 3),
    ]))
    story.append(t_gates)

    # =========================================================================
    # PAGE 2: PROCESSING LEVELS, HARDWARE PROFILING & HARDWARE ACCELERATION
    # =========================================================================
    story.append(PageBreak())

    story.append(Paragraph("B. Dual Processing Levels: Fast vs Perfect Boi", h2_style))
    story.append(Paragraph(
        "ArenaFight eliminates the one-size-fits-all tradeoff between turnaround speed and absolute correctness "
        "by introducing two specialized runtime execution levels (switchable via <code>/level [fast|perfect]</code>):",
        body_style
    ))
    story.append(Paragraph(
        "- <b>FAST Level:</b> Engineered for rapid iteration, daily coding tasks, and low-latency interaction. Employs "
        "standard reasoning token budgets, fast path verification, and rapid single-turn self-correction.",
        bullet_style
    ))
    story.append(Paragraph(
        "- <b>PERFECT BOI Level:</b> Deep, exhaustive perfectionist execution mode. ArenaFight dynamically scales its "
        "reasoning depth, adversarial cross-checks, and execution iterations up to the absolute physical capability of the "
        "host machine. No stone is left unturned: every gate must pass with cryptographic proof, and solutions are "
        "adversarially stress-tested against synthetic edge cases.",
        bullet_style
    ))

    story.append(Spacer(1, 4))
    story.append(Paragraph("C. Dynamic Host PC Hardware Profiling (Zero Hardcoded Limits)", h2_style))
    story.append(Paragraph(
        "Unlike brittle agents that hardcode token ceilings, timeouts, or thread limits, ArenaFight features an autonomous "
        "host machine profiler (<code>SystemProfiler</code>). It actively inspects the system topology on Windows and Linux:",
        body_style
    ))
    story.append(Paragraph(
        "- <b>RAM Inspection:</b> Detects installed physical RAM and dynamically computes free available memory.<br/>"
        "- <b>CPU Core Topology:</b> Measures physical cores and logical hyper-threads to set parallel execution limits.<br/>"
        "- <b>Dynamic Budget Derivation:</b> In Perfect Boi mode, limits scale dynamically:<br/>"
        "&nbsp;&nbsp;&nbsp;&nbsp;* High-end workstations (&gt;32GB RAM, &gt;12 cores): 60 iterations, 32,768 reasoning tokens, 4 adversarial passes.<br/>"
        "&nbsp;&nbsp;&nbsp;&nbsp;* Mid-range PCs (16-32GB RAM, 8-12 cores): 35 iterations, 16,384 tokens, 3 adversarial passes.<br/>"
        "&nbsp;&nbsp;&nbsp;&nbsp;* Constrained laptops (&lt;16GB RAM): safe, un-choked budgets scaled down to preserve host responsiveness.",
        body_style
    ))

    story.append(Spacer(1, 4))
    story.append(Paragraph("D. NVIDIA CUDA Hardware Acceleration &amp; Graceful Slow-Variant Fallback", h2_style))
    story.append(Paragraph(
        "ArenaFight natively probes for dedicated NVIDIA graphics processing units via driver APIs and <code>nvidia-smi</code>. "
        "When an NVIDIA GPU is present, tensor acceleration, VRAM allocation, and local model inferencing are routed through CUDA.",
        body_style
    ))
    story.append(Paragraph(
        "<b>Explicit Slow-Variant Classification:</b> If the host lacks a dedicated CUDA GPU (operating on an integrated GPU "
        "or pure CPU), ArenaFight refuses to silently degrade or freeze. Instead, it explicitly classifies the environment as a "
        "<b>Slow-Variant Runtime</b>, surfaces clear latency expectations in the terminal dashboard, and throttles parallel batch sizes "
        "to prevent memory starvation and thrashing.",
        body_style
    ))

    story.append(Spacer(1, 4))
    story.append(Paragraph("E. Autonomous Background Service Daemons (Ollama Zero-Friction)", h2_style))
    story.append(Paragraph(
        "ArenaFight eliminates the friction of requiring users to manually launch terminal servers or AI daemons. "
        "The <code>ServiceDaemon</code> subsystem provides autonomous daemon lifecycle management:",
        body_style
    ))
    story.append(Paragraph(
        "- <b>Autonomous Discovery:</b> Scans standard binary distributions on Windows (<code>%LOCALAPPDATA%\\Programs\\Ollama</code>) "
        "and Linux (<code>/usr/local/bin</code>, <code>/usr/bin</code>).<br/>"
        "- <b>Process-Level Duplication Prevention:</b> Queries OS process snapshots via Win32 <code>CreateToolhelp32Snapshot</code> "
        "and Linux <code>pgrep</code> before attempting to spawn, ensuring no duplicate instances are created.<br/>"
        "- <b>Thread-Safe Mutual Exclusion &amp; Cooldown Latch:</b> A thread-safe mutex and 45-second launch cooldown latch "
        "completely eliminate background process spawning storms.<br/>"
        "- <b>Fast-Path Loopback Polling:</b> Direct IPv4 loopback (<code>127.0.0.1</code>) avoids IPv6 <code>[::1]</code> name "
        "resolution delays on modern Windows builds, achieving sub-millisecond status responsiveness.",
        body_style
    ))

    story.append(Spacer(1, 4))
    story.append(Paragraph("F. Universal Model Benchmarking &amp; Thinking Phase Duels", h2_style))
    story.append(Paragraph(
        "The CLI includes a universal benchmarking engine capable of evaluating any local or remote model across four distinct vectors: "
        "<b>Reasoning</b>, <b>Code Generation</b>, <b>Physical Verification</b>, and <b>Tokens-per-Second Throughput</b>. "
        "Furthermore, the <code>/think</code> (alias <code>/duel</code>) tournament command disallows code generation entirely, pitting "
        "competing AI models against each other to evaluate pure chain-of-thought logic, architectural reasoning, and trade-off synthesis.",
        body_style
    ))

    # =========================================================================
    # PAGE 3: EMBEDDED QoS STUDIO, SECURITY SANDBOX & ARCHITECTURE SPEC
    # =========================================================================
    story.append(PageBreak())

    story.append(Paragraph("G. Hardware Studio &amp; Embedded QoS Engineering", h2_style))
    story.append(Paragraph(
        "ArenaFight incorporates a specialized embedded systems studio catering to mission-critical hardware firmware development:",
        body_style
    ))
    story.append(Paragraph(
        "- <b>Board &amp; Pinout Intelligence:</b> Complete hardware registry for ESP32, ESP32-S3, RP2040, STM32F4, and Arduino boards, "
        "tracking flash sizes, SRAM, strapping pins, ADC channels, and default bus configurations.<br/>"
        "- <b>Static QoS Compliance Engine:</b> Enforces zero-heap allocation inside interrupt service routines (ISRs) and "
        "disallows blocking delays (<code>delay()</code>) in real-time loops.<br/>"
        "- <b>Circuit Design Calculators:</b> Built-in analytical solvers for LED current-limiting resistors, analog voltage dividers, "
        "I2C pullup resistors, and battery discharge lifespans.<br/>"
        "- <b>Wokwi Simulation Scaffolding:</b> Generates instant <code>diagram.json</code> circuit simulation files alongside firmware.",
        body_style
    ))

    story.append(Spacer(1, 4))
    story.append(Paragraph("H. Defense-in-Depth Security Sandbox", h2_style))
    story.append(Paragraph(
        "To safeguard the developer host from rogue scripts or accidental credential leakage, ArenaFight enforces three security tiers "
        "(Strict, Balanced, Permissive):",
        body_style
    ))
    story.append(Paragraph(
        "- <b>Path Confinement (Jail):</b> Rejects any file access, read, or write operation that resolves outside the authorized workspace directory.<br/>"
        "- <b>Data Loss Prevention (DLP):</b> Scans workspace files and agent outputs for sensitive tokens, AWS keys, GitHub tokens, and private RSA keys.<br/>"
        "- <b>SHA-256 Tamper-Evident Audit Chain:</b> Every security event, tool execution, and configuration change is hashed and "
        "cryptographically linked to the previous log record, creating an unalterable blockchain-style verification trail.",
        body_style
    ))

    story.append(Spacer(1, 6))
    story.append(Paragraph("3. Technical Architecture Specification", h1_style))
    spec_data = [
        [Paragraph("Subsystem", table_header_style), Paragraph("Implementation Details", table_header_style), Paragraph("Key Capabilities", table_header_style)],
        [
            Paragraph("Core Language &amp; Standard", table_cell_bold),
            Paragraph("C++20 (GCC 14 / Clang 18 / MSVC 19.3x)", table_cell_style),
            Paragraph("Concepts, std::span, std::string_view, filesystem, zero-copy string manipulation", table_cell_style)
        ],
        [
            Paragraph("CLI REPL Engine", table_cell_bold),
            Paragraph("ReplEngine with unified command dispatcher", table_cell_style),
            Paragraph("Interactive slash commands, context mentions (@file, @git, @claims, @memory), live telemetry", table_cell_style)
        ],
        [
            Paragraph("Provider Bridge", table_cell_bold),
            Paragraph("Modular multi-provider routing layer", table_cell_style),
            Paragraph("Ollama (local), OpenCode CLI, OpenRouter, Claude Code, custom OpenAI-compatible endpoints", table_cell_style)
        ],
        [
            Paragraph("Hardware Profiler", table_cell_bold),
            Paragraph("SystemProfiler (Win32 API &amp; POSIX /proc)", table_cell_style),
            Paragraph("Probes RAM, CPU core topology, NVIDIA CUDA GPU state; computes non-hardcoded dynamic limits", table_cell_style)
        ],
        [
            Paragraph("Service Daemon", table_cell_bold),
            Paragraph("ServiceDaemon background process manager", table_cell_style),
            Paragraph("Auto-detects &amp; starts Ollama, process-level deduplication, cooldown latches, model puller", table_cell_style)
        ],
        [
            Paragraph("Telemetry Observatory", table_cell_bold),
            Paragraph("TerminalUI high-density ANSI terminal", table_cell_style),
            Paragraph("Real-time latch counters, memory pressure gauges, gate verification pass rates, latency metrics", table_cell_style)
        ],
        [
            Paragraph("Security Sandbox", table_cell_bold),
            Paragraph("SecuritySandbox &amp; Audit Trail", table_cell_style),
            Paragraph("Workspace path jail, DLP secret scanner, FIPS 180-4 SHA-256 cryptographic audit chain", table_cell_style)
        ]
    ]

    t_spec = Table(spec_data, colWidths=[104, 176, 224])
    t_spec.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), PRIMARY_NAVY),
        ('ALIGN', (0,0), (-1,-1), 'LEFT'),
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('GRID', (0,0), (-1,-1), 0.5, BORDER_COLOR),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, BG_LIGHT]),
        ('TOPPADDING', (0,0), (-1,-1), 3),
        ('BOTTOMPADDING', (0,0), (-1,-1), 3),
    ]))
    story.append(t_spec)

    # =========================================================================
    # PAGE 4: CLEANED COMMAND REFERENCE, CONTEXT MENTIONS & AUTHOR CREDIT
    # =========================================================================
    story.append(PageBreak())

    cmd_h1_style = ParagraphStyle(
        'CmdHeading1Custom',
        parent=h1_style,
        fontSize=11,
        leading=14,
        spaceBefore=4,
        spaceAfter=3,
        keepWithNext=True
    )

    story.append(Paragraph("4. Categorized Slash Command Quick-Reference", cmd_h1_style))
    story.append(Paragraph(
        "ArenaFight v2.5.0 features a completely restructured, de-duplicated command reference. Aliases are documented "
        "inline next to primary commands across 6 logical functional groups:",
        body_style
    ))

    cmd_data = [
        [Paragraph("Category &amp; Command", table_header_style), Paragraph("Inline Alias", table_header_style), Paragraph("Functional Description &amp; Usage", table_header_style)],
        [Paragraph("<b>1. Autonomous Execution</b><br/>/goal<br/>/plan<br/>/grill-me<br/>/think", table_cell_style),
         Paragraph("-<br/>-<br/>-<br/><b>/duel</b>", table_cell_style),
         Paragraph("Autonomous self-healing goal pursuit (/goal &lt;task&gt;)<br/>Milestone plan generator (/plan &lt;task&gt;)<br/>Interactive requirements interview (/grill-me &lt;topic&gt;)<br/>Thinking tournament: Compare reasoning without code gen", table_cell_style)],
        [Paragraph("<b>2. Hardware &amp; Processing</b><br/>/level<br/>/profile<br/>/services", table_cell_style),
         Paragraph("<b>/processing</b><br/>-<br/><b>/daemon</b>", table_cell_style),
         Paragraph("Set processing level: Fast (standard) vs Perfect Boi (adaptive)<br/>Assess physical host PC hardware &amp; dynamic limits (/profile [cpu|cuda])<br/>Autonomous service daemon manager: auto-detect and start Ollama", table_cell_style)],
        [Paragraph("<b>3. AI Models &amp; Providers</b><br/>/models<br/>/abilities<br/>/benchmark<br/>/mode<br/>/connect<br/>/opencode<br/>/unsloth", table_cell_style),
         Paragraph("<b>/model</b><br/>-<br/><b>/bench</b><br/>-<br/>-<br/>-<br/>-", table_cell_style),
         Paragraph("List discovered models or set active model (/models set &lt;name&gt;)<br/>Display multi-dimensional capability breakdown matrix<br/>Universal benchmark across reasoning, code, verification &amp; throughput<br/>Switch mode (standalone native vs OpenCode connected)<br/>Connect and synchronize with OpenCode bridge<br/>OpenCode model discovery, execution and status<br/>Unsloth AI on-the-fly fine-tuning engine", table_cell_style)],
        [Paragraph("<b>4. Security &amp; Telemetry</b><br/>/status<br/>/metrics<br/>/security<br/>/audit", table_cell_style),
         Paragraph("-<br/><b>/telemetry</b><br/>-<br/>-", table_cell_style),
         Paragraph("View runtime execution mode, session state &amp; verification stats<br/>Display terminal live telemetry observatory &amp; gate pass rates<br/>Inspect defense-in-depth posture, scan workspace, set level<br/>Inspect tamper-evident SHA-256 audit log and verify hash chain", table_cell_style)],
        [Paragraph("<b>5. Embedded Studio</b><br/>/boards<br/>/components<br/>/scaffold<br/>/qos<br/>/calculate", table_cell_style),
         Paragraph("<b>/board</b><br/>-<br/>-<br/>-<br/>-", table_cell_style),
         Paragraph("List or inspect microcontroller boards, pinouts, strapping pins<br/>List supported sensors, actuators, and communication ICs<br/>Scaffold embedded project with Wokwi diagram and QoS firmware<br/>Evaluate firmware for zero-heap, non-blocking QoS compliance<br/>Calculate circuit values: LED, divider, I2C pullup, battery life", table_cell_style)],
        [Paragraph("<b>6. Context &amp; Utilities</b><br/>/files<br/>/learn<br/>/schedule<br/>/artifacts<br/>/skills, /rules<br/>/temp, /tokens<br/>/guide, /clear, /exit", table_cell_style),
         Paragraph("<b>/file</b><br/>-<br/>-<br/>-<br/>-<br/><b>/temperature</b>, <b>/max-tokens</b><br/><b>/help-session</b>, <b>/quit</b>", table_cell_style),
         Paragraph("Inspect, view, read, and attach workspace files to prompt<br/>Persist Knowledge Items (KI) or rules into repo memory<br/>Schedule periodic or delayed autonomous task execution<br/>Inspect and preview generated project artifacts<br/>Manage agent skills and system/workspace operational rules<br/>Adjust temperature (0.0-2.0) and max generation token limits<br/>Interactive guided help, screen clear, and REPL exit", table_cell_style)]
    ]

    t_cmd = Table(cmd_data, colWidths=[126, 94, 284])
    t_cmd.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), PRIMARY_NAVY),
        ('ALIGN', (0,0), (-1,-1), 'LEFT'),
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('GRID', (0,0), (-1,-1), 0.5, BORDER_COLOR),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, BG_LIGHT]),
        ('TOPPADDING', (0,0), (-1,-1), 1.8),
        ('BOTTOMPADDING', (0,0), (-1,-1), 1.8),
    ]))
    story.append(t_cmd)
    story.append(Spacer(1, 4))

    # Context Mentions Callout Box
    story.append(Paragraph("5. Interactive Context Mentions", cmd_h1_style))
    mentions_box = [
        [
            Paragraph("<b>Context Mention</b>", table_cell_bold),
            Paragraph("<b>Runtime Syntax</b>", table_cell_bold),
            Paragraph("<b>Context Expansion &amp; Injection Mechanism</b>", table_cell_bold)
        ],
        [
            Paragraph("File Injection", table_cell_style),
            Paragraph("<code>@file &lt;path&gt;</code>", code_style),
            Paragraph("Reads the physical file from disk, attaches it to the session, and injects contents directly into prompt context.", table_cell_style)
        ],
        [
            Paragraph("Working Tree Diff", table_cell_style),
            Paragraph("<code>@git</code>", code_style),
            Paragraph("Executes git status and git diff; injects unstaged/staged working tree changes into prompt.", table_cell_style)
        ],
        [
            Paragraph("Verified Claims", table_cell_style),
            Paragraph("<code>@claims</code>", code_style),
            Paragraph("Injects ClaimRegistry evidence report and physically verified facts discovered during the session.", table_cell_style)
        ],
        [
            Paragraph("Session Memory", table_cell_style),
            Paragraph("<code>@memory</code>", code_style),
            Paragraph("Injects historical session discoveries, user preferences, and persistent repository Knowledge Items.", table_cell_style)
        ]
    ]
    t_mentions = Table(mentions_box, colWidths=[86, 110, 308])
    t_mentions.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), BG_LIGHT),
        ('ALIGN', (0,0), (-1,-1), 'LEFT'),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('GRID', (0,0), (-1,-1), 0.5, BORDER_COLOR),
        ('TOPPADDING', (0,0), (-1,-1), 1.8),
        ('BOTTOMPADDING', (0,0), (-1,-1), 1.8),
    ]))
    story.append(t_mentions)
    story.append(Spacer(1, 4))

    # Licensing & Author Credits
    story.append(Paragraph("6. Project Attributions &amp; Licensing", cmd_h1_style))
    story.append(Paragraph(
        "ArenaFight is authored and engineered by <b>Lunarmist-byte</b>. Licensed under custom All Rights Received terms: "
        "strictly free for personal non-commercial experimentation and research; commercial and enterprise deployment requires "
        "written licensing authorization.",
        body_style
    ))
    story.append(Paragraph(
        "<b>Repository &amp; Developer Links:</b><br/>"
        "- <b>Author:</b> Lunarmist-byte<br/>"
        "- <b>GitHub Repository:</b> https://github.com/Lunarmist-byte<br/>"
        "- <b>LinkedIn Profile:</b> https://www.linkedin.com/in/amal-s-kumar-ba69a1290/",
        body_style
    ))

    # Build the document
    doc.build(story, canvasmaker=NumberedCanvas)
    print(f"Publication-grade PDF successfully created at: {filename}")

if __name__ == "__main__":
    out_path = sys.argv[1] if len(sys.argv) > 1 else "docs/ArenaFight_Architecture_and_Capabilities.pdf"
    build_pdf(out_path)
