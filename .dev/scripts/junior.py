#!/usr/bin/env python3
from watcher import watch_and_run

JUNIOR_CMD = [
    "gemini",
    "-y",
    "-m", "gemini-3-pro-preview",
    "You are the Junior. Please complete the next task.",
    "--output-format", "stream-json"
]

if __name__ == "__main__":
    watch_and_run(JUNIOR_CMD)