"""Minimal local web portal for LLVM workshop exercises (POC)."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

try:
    from .exercise_bank import get_exercise, list_exercises
except ImportError:
    from exercise_bank import get_exercise, list_exercises


THIS_DIR = Path(__file__).resolve().parent
EVALUATOR = THIS_DIR / "evaluator.py"


INDEX_HTML = """<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>LLVM Workshop Portal POC</title>
  <style>
    :root {
      --bg: #f5efe2;
      --panel: #fffaf1;
      --ink: #2a2a2a;
      --muted: #6e6a61;
      --accent: #2f6f47;
      --danger: #9a2f2f;
      --border: #d9c9ab;
      --mono: "Iosevka", "Fira Code", Consolas, monospace;
      --sans: "IBM Plex Sans", "Segoe UI", sans-serif;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      font-family: var(--sans);
      color: var(--ink);
      background: linear-gradient(160deg, #efe5d2 0%, #f9f4ea 50%, #eee6d7 100%);
    }
    header {
      padding: 16px 20px;
      border-bottom: 1px solid var(--border);
      background: rgba(255, 250, 241, 0.85);
      backdrop-filter: blur(3px);
    }
    header h1 { margin: 0; font-size: 20px; }
    header p { margin: 6px 0 0; color: var(--muted); font-size: 13px; }
    .layout {
      display: grid;
      grid-template-columns: 340px 1fr;
      gap: 12px;
      padding: 12px;
      height: calc(100vh - 86px);
      min-height: 0;
    }
    .card {
      background: var(--panel);
      border: 1px solid var(--border);
      border-radius: 10px;
      padding: 12px;
      min-height: 0;
    }
    .left { display: grid; grid-template-rows: auto 1fr auto; gap: 12px; min-height: 0; }
    .label { font-size: 12px; font-weight: 700; margin-bottom: 6px; text-transform: uppercase; color: #5b5246; }
    select, button, textarea {
      width: 100%;
      border: 1px solid var(--border);
      border-radius: 8px;
      padding: 8px;
      font-family: var(--sans);
      font-size: 14px;
      background: #fffdf8;
      color: var(--ink);
    }
    button {
      cursor: pointer;
      background: #f2e8d3;
      transition: background 0.15s ease;
    }
    button:hover { background: #e8dcc3; }
    .btn-primary { background: var(--accent); color: white; border-color: #2c5e40; }
    .btn-primary:hover { background: #285d3c; }
    .meta { font-size: 13px; color: var(--muted); line-height: 1.45; white-space: pre-wrap; }
    .main {
      display: grid;
      grid-template-rows: minmax(290px, 1.35fr) minmax(230px, 1fr);
      gap: 12px;
      min-height: 0;
    }
    .bottom-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
      min-height: 0;
    }
    .panel-title { margin: 0 0 8px; font-size: 14px; }
    .meta-divider { margin: 10px 0 8px; border: 0; border-top: 1px solid var(--border); }
    .panel-card { display: flex; flex-direction: column; min-height: 0; }
    .mono {
      font-family: var(--mono);
      font-size: 13px;
      line-height: 1.45;
      white-space: pre;
      overflow: auto;
      border: 1px solid var(--border);
      border-radius: 8px;
      background: #fff;
      padding: 10px;
      height: 100%;
      min-height: 0;
    }
    textarea.code {
      font-family: var(--mono);
      font-size: 13px;
      line-height: 1.45;
      resize: none;
      min-height: 0;
      height: 100%;
      background: #fff;
    }
    .result.pass { color: var(--accent); font-weight: 700; }
    .result.fail { color: var(--danger); font-weight: 700; }
    .actions { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
    #feedback { flex: 1; margin-top: 8px; }
    @media (max-width: 960px) {
      .layout { height: auto; }
      .layout { grid-template-columns: 1fr; }
      .main { grid-template-rows: minmax(260px, auto) minmax(260px, auto); }
      .bottom-grid { grid-template-columns: 1fr; }
      textarea.code { min-height: 260px; }
    }
  </style>
</head>
<body>
  <header>
    <h1>LLVM Python Workshop Portal (POC)</h1>
    <p>10 exercises: F01-F08, B01-B02. Local-only prototype.</p>
  </header>
  <main class="layout">
    <section class="left">
      <div class="card">
        <div class="label">Exercise</div>
        <select id="exercise-select"></select>
      </div>
      <div class="card">
        <div class="label">Prompt</div>
        <div id="prompt" class="meta"></div>
        <hr class="meta-divider" />
        <div class="label">Exercise Metadata</div>
        <div id="meta" class="meta"></div>
      </div>
      <div class="card">
        <div class="label">Actions</div>
        <div class="actions">
          <button id="reset-btn">Reset Starter</button>
          <button id="solution-btn">Show Solution</button>
          <button id="run-btn" class="btn-primary" style="grid-column: 1 / span 2;">Run Validation</button>
        </div>
      </div>
    </section>
    <section class="main">
      <div class="card panel-card">
        <h3 class="panel-title">Your Submission</h3>
        <textarea id="code" class="code" spellcheck="false"></textarea>
      </div>
      <div class="bottom-grid">
        <div class="card panel-card">
          <h3 class="panel-title">Input IR</h3>
          <pre id="input-ir" class="mono"></pre>
        </div>
        <div class="card panel-card">
          <h3 class="panel-title">Result</h3>
          <div id="status" class="result">No run yet.</div>
          <pre id="feedback" class="mono"></pre>
        </div>
      </div>
    </section>
  </main>
  <script>
    const selectEl = document.getElementById("exercise-select");
    const promptEl = document.getElementById("prompt");
    const metaEl = document.getElementById("meta");
    const inputIrEl = document.getElementById("input-ir");
    const codeEl = document.getElementById("code");
    const statusEl = document.getElementById("status");
    const feedbackEl = document.getElementById("feedback");
    const runBtn = document.getElementById("run-btn");
    const resetBtn = document.getElementById("reset-btn");
    const solutionBtn = document.getElementById("solution-btn");

    let cache = {};
    let currentId = null;

    function renderMeta(ex) {
      metaEl.textContent =
        `ID: ${ex.id}\\n` +
        `Title: ${ex.title}\\n` +
        `Track: ${ex.track}\\n` +
        `Level: ${ex.level}\\n` +
        `Estimate: ${ex.estimated_minutes} min\\n` +
        `Submission mode: ${ex.submission_mode || "function"}\\n` +
        `Hints:\\n- ${ex.hints.join("\\n- ")}`;
    }

    async function loadExercise(id) {
      const resp = await fetch(`/api/exercises/${id}`);
      const ex = await resp.json();
      cache[id] = ex;
      currentId = id;
      promptEl.textContent = ex.prompt;
      inputIrEl.textContent = ex.input_ir || "(No input IR for this exercise)";
      codeEl.value = ex.starter_code;
      renderMeta(ex);
      statusEl.textContent = "Ready.";
      statusEl.className = "result";
      feedbackEl.textContent = "";
    }

    async function init() {
      const resp = await fetch("/api/exercises");
      const data = await resp.json();
      for (const ex of data.exercises) {
        const option = document.createElement("option");
        option.value = ex.id;
        option.textContent = `${ex.id} - ${ex.title}`;
        selectEl.appendChild(option);
      }
      if (data.exercises.length > 0) {
        await loadExercise(data.exercises[0].id);
      }
    }

    selectEl.addEventListener("change", async (event) => {
      await loadExercise(event.target.value);
    });

    resetBtn.addEventListener("click", () => {
      if (!currentId) return;
      codeEl.value = cache[currentId].starter_code;
    });

    solutionBtn.addEventListener("click", async () => {
      if (!currentId) return;
      const resp = await fetch(`/api/exercises/${currentId}?include_solution=1`);
      const ex = await resp.json();
      codeEl.value = ex.solution_code || codeEl.value;
    });

    runBtn.addEventListener("click", async () => {
      if (!currentId) return;
      runBtn.disabled = true;
      statusEl.textContent = "Running...";
      statusEl.className = "result";
      feedbackEl.textContent = "";
      try {
        const resp = await fetch(`/api/submit/${currentId}`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ code: codeEl.value }),
        });
        const result = await resp.json();
        statusEl.textContent = result.passed ? "PASS" : "FAIL";
        statusEl.className = result.passed ? "result pass" : "result fail";
        let text = `${result.feedback}\\n\\n`;
        text += `score: ${result.score}\\n`;
        if (result.entrypoint_used) {
          text += `entrypoint used: ${result.entrypoint_used}\\n`;
        }
        if (result.result_preview) {
          text += `\\nresult preview:\\n${result.result_preview}\\n`;
        }
        if (result.stdout) {
          text += `\\nstdout:\\n${result.stdout}\\n`;
        }
        if (result.stderr) {
          text += `\\nstderr:\\n${result.stderr}\\n`;
        }
        feedbackEl.textContent = text.trim();
      } catch (err) {
        statusEl.textContent = "FAIL";
        statusEl.className = "result fail";
        feedbackEl.textContent = `Portal error: ${err}`;
      } finally {
        runBtn.disabled = false;
      }
    });

    init();
  </script>
</body>
</html>
"""


class PortalHandler(BaseHTTPRequestHandler):
    server_version = "LLVMWorkshopPortal/0.1"

    def _send_json(self, data: object, status: HTTPStatus = HTTPStatus.OK) -> None:
        payload = json.dumps(data).encode("utf-8")
        self.send_response(status.value)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def _send_html(self, html: str, status: HTTPStatus = HTTPStatus.OK) -> None:
        payload = html.encode("utf-8")
        self.send_response(status.value)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def do_GET(self) -> None:  # noqa: N802
        parsed = urlparse(self.path)
        path = parsed.path
        query = parse_qs(parsed.query)

        if path == "/":
            self._send_html(INDEX_HTML)
            return

        if path == "/api/exercises":
            self._send_json({"exercises": list_exercises()})
            return

        if path.startswith("/api/exercises/"):
            ex_id = path.rsplit("/", 1)[-1]
            include_solution = query.get("include_solution", ["0"])[0] in ("1", "true", "yes")
            try:
                ex = get_exercise(ex_id, include_solution=include_solution)
            except KeyError:
                self._send_json({"error": f"Unknown exercise id: {ex_id}"}, status=HTTPStatus.NOT_FOUND)
                return
            self._send_json(_public_exercise_payload(ex))
            return

        self._send_json({"error": "Not found"}, status=HTTPStatus.NOT_FOUND)

    def do_POST(self) -> None:  # noqa: N802
        parsed = urlparse(self.path)
        path = parsed.path
        if not path.startswith("/api/submit/"):
            self._send_json({"error": "Not found"}, status=HTTPStatus.NOT_FOUND)
            return

        ex_id = path.rsplit("/", 1)[-1]
        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(length)
        try:
            payload = json.loads(body.decode("utf-8"))
        except json.JSONDecodeError:
            self._send_json({"error": "Invalid JSON body"}, status=HTTPStatus.BAD_REQUEST)
            return

        code = payload.get("code")
        if not isinstance(code, str):
            self._send_json({"error": "`code` must be a string"}, status=HTTPStatus.BAD_REQUEST)
            return

        result = run_evaluation_subprocess(ex_id, code)
        self._send_json(result)

    def log_message(self, format: str, *args: object) -> None:
        # Keep local portal logs concise.
        sys.stderr.write("[portal] " + (format % args) + "\n")


def run_evaluation_subprocess(exercise_id: str, code: str) -> dict[str, object]:
    with tempfile.NamedTemporaryFile(
        mode="w",
        suffix=".py",
        encoding="utf-8",
        delete=False,
    ) as temp:
        temp.write(code)
        temp_path = Path(temp.name)

    cmd = [
        sys.executable,
        str(EVALUATOR),
        "--exercise-id",
        exercise_id,
        "--submission-file",
        str(temp_path),
    ]
    try:
        proc = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            encoding="utf-8",
            timeout=10,
            check=False,
            cwd=str(THIS_DIR.parent.parent),
        )
    except subprocess.TimeoutExpired:
        temp_path.unlink(missing_ok=True)
        return {
            "passed": False,
            "feedback": "Execution timed out after 10 seconds.",
            "score": 0.0,
            "stdout": "",
            "stderr": "",
        }
    finally:
        temp_path.unlink(missing_ok=True)

    stdout = proc.stdout.strip()
    if not stdout:
        return {
            "passed": False,
            "feedback": "Evaluator produced no JSON output.",
            "score": 0.0,
            "stdout": proc.stdout,
            "stderr": proc.stderr,
        }

    try:
        result = json.loads(stdout.splitlines()[-1])
    except json.JSONDecodeError:
        return {
            "passed": False,
            "feedback": "Evaluator returned invalid JSON.",
            "score": 0.0,
            "stdout": proc.stdout,
            "stderr": proc.stderr,
        }

    # Preserve evaluator stderr for debugging.
    if proc.stderr and isinstance(result, dict):
        existing = result.get("stderr", "")
        result["stderr"] = (str(existing) + "\n" + proc.stderr).strip()
    return result


def _public_exercise_payload(exercise: dict[str, object]) -> dict[str, object]:
    return {k: v for k, v in exercise.items() if not str(k).startswith("_")}


def main() -> int:
    parser = argparse.ArgumentParser(description="Run local LLVM workshop portal POC.")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", default=8123, type=int)
    args = parser.parse_args()

    server = ThreadingHTTPServer((args.host, args.port), PortalHandler)
    print(f"Portal running at http://{args.host}:{args.port}")
    print("Press Ctrl+C to stop.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
