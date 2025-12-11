#!/usr/bin/env python3
import sys
from watcher import watch_and_run

if __name__ == "__main__":
    model = sys.argv[1] if len(sys.argv) > 1 else "claude-sonnet-4.5"
    command = [
        "copilot",
        "--allow-all-tools",
        "-p", "You are the Task Agent. Please complete the next task.",
        "--model", model
    ]
    watch_and_run(command)
