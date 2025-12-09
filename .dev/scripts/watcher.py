import os
import time
import subprocess
import re
import sys
from datetime import datetime

TODO_FILE = ".dev/TODO.md"

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

def watch_and_run(command_list):
    """
    Watches the TODO file and runs the provided command_list when a task is found.
    """
    if not os.path.exists(TODO_FILE):
        log(f"Critical Error: {TODO_FILE} does not exist. Exiting.")
        sys.exit(1)

    log(f"Starting Watcher on {TODO_FILE}...")
    log(f"Command: {' '.join(command_list)}")
    log("Waiting for tasks...")

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
                    log("Activating Agent...")
                    print("-" * 40)
                    
                    # Run the Agent
                    start_time = time.time()
                    try:
                        result = subprocess.run(command_list)
                        duration = time.time() - start_time
                        
                        print("-" * 40)
                        if result.returncode == 0:
                            log(f"Agent finished successfully in {duration:.1f}s.")
                        else:
                            log(f"Agent exited with error code {result.returncode}.")
                    except FileNotFoundError:
                         log(f"Error: Command '{command_list[0]}' not found. Is it in your PATH?")
                         time.sleep(10)
                    except Exception as e:
                        log(f"Error running Agent: {e}")

                    # Update mtime to the *current* file state
                    try:
                        last_mtime = os.stat(TODO_FILE).st_mtime
                    except:
                        last_mtime = time.time()

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
