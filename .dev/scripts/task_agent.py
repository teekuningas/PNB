#!/usr/bin/env python3
import sys
from watcher import watch_and_run

if __name__ == "__main__":
    model = sys.argv[1] if len(sys.argv) > 1 else "gemini-3-pro-preview"
    
    # Use gemini CLI with default text output (streams directly)
    command = [
        "gemini",
        "-y",
        "-m", model,
        "You are the Task Agent. Please complete the next task."
    ]
    
    watch_and_run(command)

