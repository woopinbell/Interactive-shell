#!/usr/bin/env python3
"""Render the README terminal demo from a real interactive shell session."""

from __future__ import annotations

import fcntl
import os
import pty
import select
import shutil
import struct
import subprocess
import termios
import tempfile
import time
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[2]
SHELL_BIN = ROOT_DIR / "build" / "bin" / "shell"
OUTPUT_GIF = ROOT_DIR / "docs" / "assets" / "readme" / "interactive-shell-demo.gif"

WIDTH = 960
HEIGHT = 540
FPS = 12
TERMINAL_COLS = 88
TERMINAL_ROWS = 17
FONT_SIZE = 19
LINE_SPACING = 5

STEPS = [
    ("export DEMO_WORD=readme-demo", "shell$ "),
    ("echo alpha beta gamma | tr a-z A-Z | grep BETA", "shell$ "),
    ("echo saved:$DEMO_WORD > shell-demo.txt", "shell$ "),
    ("cat < shell-demo.txt", "shell$ "),
    ("cat <<EOF", "heredoc> "),
    ("heredoc expands $DEMO_WORD", "heredoc> "),
    ("EOF", "shell$ "),
    ("cd /tmp", "shell$ "),
    ("pwd", "shell$ "),
    ("exit", None),
]

FONT_CANDIDATES = [
    "/System/Library/Fonts/SFNSMono.ttf",
    Path("/System/Library/Fonts/Menlo.ttc"),
    Path("/System/Library/Fonts/Monaco.ttf"),
    Path("/System/Library/Fonts/Supplemental/Andale Mono.ttf"),
]


class TerminalBuffer:
    def __init__(self, cols: int, rows: int) -> None:
        self.cols = cols
        self.rows = rows
        self.lines = [""]
        self.row = 0
        self.col = 0
        self.in_escape = False

    def feed(self, text: str) -> None:
        for char in text:
            if self.in_escape:
                if char.isalpha() or char in "@~":
                    self.in_escape = False
                continue
            if char == "\x1b":
                self.in_escape = True
            elif char == "\r":
                self.col = 0
            elif char == "\n":
                self._newline()
            elif char in ("\b", "\x7f"):
                self.col = max(0, self.col - 1)
                line = self.lines[self.row]
                if self.col < len(line):
                    self.lines[self.row] = line[: self.col] + line[self.col + 1 :]
            elif char == "\t":
                self.feed(" " * (4 - (self.col % 4)))
            elif char.isprintable():
                self._put_char(char)

    def _newline(self) -> None:
        self.lines.append("")
        self.row += 1
        self.col = 0
        if len(self.lines) > self.rows:
            self.lines.pop(0)
            self.row = self.rows - 1

    def _put_char(self, char: str) -> None:
        if self.col >= self.cols:
            self._newline()
        line = self.lines[self.row]
        if self.col > len(line):
            line = line + (" " * (self.col - len(line)))
        if self.col == len(line):
            line += char
        else:
            line = line[: self.col] + char + line[self.col + 1 :]
        self.lines[self.row] = line[: self.cols]
        self.col += 1

    def snapshot(self) -> str:
        visible = self.lines[-self.rows :]
        return "\n".join(line.rstrip() for line in visible).rstrip()


def find_font() -> Path:
    configured_font = os.environ.get("README_DEMO_FONT")
    if configured_font:
        candidate = Path(configured_font)
        if candidate.exists():
            return candidate
    for candidate in FONT_CANDIDATES:
        candidate = Path(candidate)
        if str(candidate) and candidate.exists():
            return candidate
    raise SystemExit("No monospace font found. Set README_DEMO_FONT to a font file.")


def require_tool(name: str) -> str:
    path = shutil.which(name)
    if path is None:
        raise SystemExit(f"{name} is required to render the README GIF.")
    return path


def set_pty_size(fd: int) -> None:
    size = struct.pack("HHHH", TERMINAL_ROWS, TERMINAL_COLS, 0, 0)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, size)


def capture_demo() -> list[tuple[float, str]]:
    if not SHELL_BIN.exists():
        raise SystemExit(f"{SHELL_BIN} does not exist. Run `make` first.")

    events: list[tuple[float, str]] = []
    output = ""
    work_dir = Path(tempfile.mkdtemp(prefix="interactive-shell-demo-"))
    master_fd, slave_fd = pty.openpty()
    set_pty_size(slave_fd)
    start = time.monotonic()

    env = os.environ.copy()
    env.update(
        {
            "TERM": "xterm-256color",
            "DEMO_WORD": "initial-value",
            "LC_ALL": "C",
        }
    )
    process = subprocess.Popen(
        [str(SHELL_BIN)],
        cwd=work_dir,
        env=env,
        stdin=slave_fd,
        stdout=slave_fd,
        stderr=slave_fd,
        close_fds=True,
    )
    os.close(slave_fd)
    os.set_blocking(master_fd, False)

    def drain(duration: float) -> None:
        nonlocal output
        end = time.monotonic() + duration
        while time.monotonic() < end:
            ready, _, _ = select.select([master_fd], [], [], 0.02)
            if not ready:
                continue
            try:
                data = os.read(master_fd, 4096)
            except OSError:
                return
            if not data:
                return
            chunk = data.decode("utf-8", "replace")
            output += chunk
            events.append((time.monotonic() - start, chunk))

    def wait_for(token: str, search_from: int, timeout: float = 3.0) -> None:
        nonlocal output
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if token in output[search_from:]:
                return
            drain(0.05)
        raise RuntimeError(f"Timed out waiting for terminal token: {token!r}")

    try:
        wait_for("shell$ ", 0)
        for line, expected_prompt in STEPS:
            search_from = len(output)
            for char in line:
                os.write(master_fd, char.encode("utf-8"))
                drain(0.010)
            os.write(master_fd, b"\n")
            drain(0.05)
            if expected_prompt is not None:
                wait_for(expected_prompt, search_from)
        drain(0.7)
        process.wait(timeout=5)
        drain(0.1)
        if process.returncode != 0:
            raise RuntimeError(f"Demo shell exited with status {process.returncode}.")
    finally:
        try:
            if process.poll() is None:
                process.terminate()
        finally:
            os.close(master_fd)
            shutil.rmtree(work_dir)

    return events


def snapshots_from_events(events: list[tuple[float, str]]) -> list[str]:
    if not events:
        return []
    duration = events[-1][0] + 1.2
    frame_count = int(duration * FPS) + 1
    terminal = TerminalBuffer(TERMINAL_COLS, TERMINAL_ROWS)
    event_index = 0
    snapshots: list[str] = []

    for frame_index in range(frame_count):
        frame_time = frame_index / FPS
        while event_index < len(events) and events[event_index][0] <= frame_time:
            terminal.feed(events[event_index][1])
            event_index += 1
        snapshots.append(terminal.snapshot())
    return snapshots


def ffmpeg_escape(value: Path | str) -> str:
    return str(value).replace("\\", "\\\\").replace(":", "\\:")


def render_frame(ffmpeg: str, font: Path, text_file: Path, output_file: Path) -> None:
    font_arg = ffmpeg_escape(font)
    text_arg = ffmpeg_escape(text_file)
    filters = ",".join(
        [
            "drawbox=x=24:y=24:w=912:h=492:color=0x111827:t=fill",
            "drawbox=x=24:y=24:w=912:h=44:color=0x1f2937:t=fill",
            "drawbox=x=46:y=41:w=10:h=10:color=0xef4444:t=fill",
            "drawbox=x=66:y=41:w=10:h=10:color=0xf59e0b:t=fill",
            "drawbox=x=86:y=41:w=10:h=10:color=0x22c55e:t=fill",
            (
                f"drawtext=fontfile={font_arg}:text=build/bin/shell:"
                "fontcolor=0xcbd5e1:fontsize=15:x=(w-text_w)/2:y=38"
            ),
            (
                f"drawtext=fontfile={font_arg}:textfile={text_arg}:"
                f"fontcolor=0xe5e7eb:fontsize={FONT_SIZE}:"
                f"x=44:y=86:line_spacing={LINE_SPACING}"
            ),
        ]
    )
    subprocess.run(
        [
            ffmpeg,
            "-y",
            "-v",
            "error",
            "-f",
            "lavfi",
            "-i",
            f"color=c=0x0b1020:s={WIDTH}x{HEIGHT}",
            "-frames:v",
            "1",
            "-vf",
            filters,
            str(output_file),
        ],
        check=True,
    )


def render_gif(snapshots: list[str]) -> None:
    ffmpeg = require_tool("ffmpeg")
    font = find_font()
    OUTPUT_GIF.parent.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="interactive-shell-frames-") as temp_name:
        temp_dir = Path(temp_name)
        frame_dir = temp_dir / "frames"
        text_dir = temp_dir / "text"
        frame_dir.mkdir()
        text_dir.mkdir()

        for index, snapshot in enumerate(snapshots):
            text_file = text_dir / f"frame-{index:04d}.txt"
            frame_file = frame_dir / f"frame-{index:04d}.png"
            text_file.write_text(snapshot, encoding="utf-8")
            render_frame(ffmpeg, font, text_file, frame_file)

        palette = temp_dir / "palette.png"
        subprocess.run(
            [
                ffmpeg,
                "-y",
                "-v",
                "error",
                "-framerate",
                str(FPS),
                "-i",
                str(frame_dir / "frame-%04d.png"),
                "-vf",
                "palettegen=stats_mode=diff",
                str(palette),
            ],
            check=True,
        )
        subprocess.run(
            [
                ffmpeg,
                "-y",
                "-v",
                "error",
                "-framerate",
                str(FPS),
                "-i",
                str(frame_dir / "frame-%04d.png"),
                "-i",
                str(palette),
                "-lavfi",
                "paletteuse=dither=bayer:bayer_scale=5:diff_mode=rectangle",
                "-loop",
                "0",
                str(OUTPUT_GIF),
            ],
            check=True,
        )


def main() -> None:
    events = capture_demo()
    snapshots = snapshots_from_events(events)
    render_gif(snapshots)
    print(f"wrote {OUTPUT_GIF}")


if __name__ == "__main__":
    main()
