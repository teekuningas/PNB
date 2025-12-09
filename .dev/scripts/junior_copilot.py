#!/usr/bin/env python3
from watcher import watch_and_run

JUNIOR_CMD = [
    "copilot",
    "--allow-all-tools",
    "-p", "You are the junior. Please complete the next task.",
    "--model", "claude-sonnet-4"
]

if __name__ == "__main__":
    watch_and_run(JUNIOR_CMD)
