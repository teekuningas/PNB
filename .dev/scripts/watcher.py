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
    Polls every 2 seconds.
    """
    if not os.path.exists(TODO_FILE):
        log(f"Critical Error: {TODO_FILE} does not exist. Exiting.")
        sys.exit(1)

    # Set SSL_CERT_DIR for OpenSSL to find certificates
    os.environ['SSL_CERT_DIR'] = '/etc/ssl/certs'

    log(f"Starting Watcher on {TODO_FILE} (Polling Mode)...")
    log(f"Command: {' '.join(command_list)}")
    log("Waiting for tasks...")

    while True:
        try:
            # Check for task
            task = get_next_task(TODO_FILE)
            if task:
                log(f"Pending task detected: {task}")
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

                # After running, we just loop again. 
                # If the agent marked the task as done, get_next_task will return None (or the next task).
                # If the agent failed, we'll see the same task again in 2 seconds.
            else:
                # No task, just wait
                pass

            time.sleep(2)

        except KeyboardInterrupt:
            log("Watcher stopped by user.")
            sys.exit(0)
        except Exception as e:
            log(f"Unexpected error: {e}")
            time.sleep(5)