# COL333 AI Assignments

<div align="center">

Coursework repository for **COL333 (Artificial Intelligence)** at IIT Delhi, containing three major programming assignments across SAT solving, probabilistic learning, and game AI.

</div>

---

## Repository Overview

This repository is organized assignment-wise:

| Folder | Theme | Primary Language | What it contains |
|---|---|---|---|
| `A3/` | Metro Map Planning via SAT | C++ (+ Python utilities) | CNF encoder/decoder pipeline, testcase generator, format checker |
| `A4/` | Bayesian Network Parameter Learning | C++ | EM-based learning over `.bif` networks and data records |
| `A5/` | Rivers and Stones Game AI | Python + C++ sample bridge | Game engine, bot clients, web server, student agent scaffolding |

---

## High-Level Architecture

### A3 — Metro Planner (SAT workflow)
- Converts `.city` problem files into SAT (`.satinput`) using `metro_planner`.
- Solves SAT externally (MiniSat expected).
- Decodes SAT assignments into metro routes (`.metromap`).

**Entry points**
- `A3/compile.sh` → build `metro_planner`
- `A3/run1.sh <basename>` → encode (`.city` → `.satinput`)
- `A3/run2.sh <basename>` → decode (`.satoutput` + `.city` → `.metromap`)
- `A3/testcase_gen.py` → synthetic test generation

### A4 — Bayesian Network Solver
- Parses Bayesian network definitions in BIF format.
- Learns/estimates CPT values from data records using EM-style iterations.
- Writes solved network output (e.g., `solved_hailfinder.bif`).

**Entry points**
- `A4/compile.sh` → build solver binary
- `A4/run.sh <bif_file> <data_file>` → run learning pipeline
- `A4/format_checker.cpp` (compiled separately) → structural and error validation

### A5 — Rivers and Stones (Game + Agents)
- Implements game rules, state transitions, and AI-vs-AI / Human-vs-AI modes.
- Provides local simulation (`gameEngine.py`) and multiplayer web orchestration (`web_server.py`).
- Includes sample C++ agent integration (`c++_sample_files/`).

**Entry points**
- `A5/client_server/gameEngine.py` → local game modes (`hvh`, `hvai`, `aivai`)
- `A5/client_server/start_server.sh [port]` → launch web server
- `A5/client_server/bot_client.py <circle|square> <port>` → connect bot clients
- `A5/client_server/student_agent.py` → student strategy implementation target

---

## Main Artifacts by Assignment

- **Problem statements/docs:** `A3/A3.pdf`, `A4/A4.pdf`, `A5/A5.pdf`, `A5/A5_assignment.pdf`
- **Rules reference (A5):** `A5/game.md`
- **Sample/game data:**
  - A3: `sample_test_000.*`
  - A4: `hailfinder*.bif`, `records*.dat`

---

## Quick Start

```bash
# A3
cd A3 && bash compile.sh

# A4
cd ../A4 && bash compile.sh && bash run.sh hailfinder.bif records.dat

# A5 (local engine)
cd ../A5/client_server && pip install -r requirements.txt
python gameEngine.py --mode aivai --circle random --square student
```

---

## Notes

- Each assignment folder is largely self-contained.
- Some folders include multiple solver variants (e.g., optimized or alternate implementations).
- Use the assignment-specific READMEs and PDFs for detailed I/O specifications and grading constraints.
