#!/usr/bin/env python3
import os
import time
import subprocess
import re
import sys
from datetime import datetime

TODO_FILE = ".dev/TODO.md"
# The command provided by the user
JUNIOR_CMD = [
    "gemini",
    "-y",
    "-m", "gemini-3-pro-preview"
    "You are the Junior. Please complete the next task.",
    "--output-format", "stream-json"
]

def log(msg):
    print(f"[{datetime.now().strftime('%H:%M:%S')}] [WATCHER] {msg}")

def get_next_task(filepath):
    """
    Scans the TODO file for the first unchecked item.
    Returns the task line if found, otherwise None.
    """
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            for line in f:
                if re.search(r'^\s*-\s*\[ \]', line):
                    return line.strip()
    except FileNotFoundError:
        log(f"Error: {filepath} not found.")
        return None
    return None

def main():
    if not os.path.exists(TODO_FILE):
        log(f"Critical Error: {TODO_FILE} does not exist. Exiting.")
        sys.exit(1)

    log(f"Starting Junior Watcher on {TODO_FILE}...")
    log("Waiting for tasks...")

    last_mtime = 0
    
    # Force an initial check
    last_mtime = 0

    while True:
        try:
            try:
                current_stats = os.stat(TODO_FILE)
                current_mtime = current_stats.st_mtime
            except FileNotFoundError:
                log("TODO file disappeared!")
                time.sleep(2)
                continue

            if current_mtime != last_mtime:
                # File has changed (or it's the first loop)
                # Wait a brief moment to ensure file write is complete (debounce)
                time.sleep(0.5)
                
                task = get_next_task(TODO_FILE)
                if task:
                    log(f"New task detected: {task}")
                    log("Activating Junior Agent...")
                    print("-" * 40)
                    
                    # Run the Junior Agent
                    # We pass stdout/stderr to sys.stdout/stderr to stream output directly
                    start_time = time.time()
                    try:
                        result = subprocess.run(JUNIOR_CMD)
                        duration = time.time() - start_time
                        
                        print("-" * 40)
                        if result.returncode == 0:
                            log(f"Junior finished successfully in {duration:.1f}s.")
                        else:
                            log(f"Junior exited with error code {result.returncode}.")
                    except FileNotFoundError:
                         log("Error: 'gemini' command not found. Is it in your PATH?")
                         time.sleep(10)
                    except Exception as e:
                        log(f"Error running Junior: {e}")

                    # Update mtime to the *current* file state so we don't re-trigger 
                    # immediately on the changes the Junior just made, UNLESS 
                    # the Junior left more tasks or modified the file again.
                    # Actually, simply updating last_mtime to the current state 
                    # is risky if the Junior was fast. 
                    # Better: We just let the loop continue. 
                    # If Junior checked the box, the file changed.
                    # The next loop will see the new mtime.
                    # It will read the file.
                    # If there is *another* task, it will trigger again (Good! Chain tasks).
                    # If there are no tasks, it will print "No tasks found".
                    
                    # However, we need to update current_mtime to what it is NOW 
                    # to avoid comparing against the old mtime from *before* the Junior ran.
                    try:
                        last_mtime = os.stat(TODO_FILE).st_mtime
                    except:
                        last_mtime = time.time() # Fallback

                else:
                    log("No pending tasks found. Standing by.")
                    last_mtime = current_mtime

            time.sleep(2)

        except KeyboardInterrupt:
            log("Watcher stopped by user.")
            sys.exit(0)
        except Exception as e:
            log(f"Unexpected error: {e}")
            time.sleep(5)

if __name__ == "__main__":
    main()
