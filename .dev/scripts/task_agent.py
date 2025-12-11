#!/usr/bin/env python3
import sys
import os
from watcher import watch_and_run

if __name__ == "__main__":
    model = sys.argv[1] if len(sys.argv) > 1 else "gemini-3-pro-preview"
    
    # Construct the gemini command string
    gemini_cmd = [
        "gemini",
        "-y",
        "-m", model,
        "You are the Task Agent. Please complete the next task.",
        "--output-format", "stream-json"
    ]
    gemini_cmd_str = " ".join(gemini_cmd)
    
    # Path to the parser script
    script_dir = os.path.dirname(os.path.abspath(__file__))
    parser_script = os.path.join(script_dir, "parse_gemini.py")
    
    # Create the pipeline command
    # We use sh -c to execute the pipe
    command = [
        "sh", 
        "-c", 
        f"{gemini_cmd_str} | python3 {parser_script}"
    ]
    
    watch_and_run(command)

