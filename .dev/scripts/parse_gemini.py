#!/usr/bin/env python3
import sys
import json

def extract_and_replace(data, target_key, results):
    """
    Recursively searches for 'target_key' in 'data' (dict or list).
    If found and the value is non-empty:
      1. Appends the value to 'results'.
      2. Replaces the value in 'data' with "<extracted>".
    """
    if isinstance(data, dict):
        for k, v in list(data.items()): # iterate over copy to allow modification
            if k == target_key:
                if v: # Only extract/replace non-empty values
                    results.append(v)
                    data[k] = "<extracted>"
            
            # Recurse into children (v might be a dict or list)
            # Note: if we just replaced data[k], v is the OLD string value, so isinstance check fails safely.
            # But we need to handle the case where v IS a container.
            # If k == target_key, v is likely a string (for content/output).
            # If k != target_key, v is a container we should recurse into.
            
            # Re-fetch the value because we might have modified data[k] (though unlikely for a container key)
            current_val = data[k] 
            if isinstance(current_val, (dict, list)):
                extract_and_replace(current_val, target_key, results)
                
    elif isinstance(data, list):
        for item in data:
            extract_and_replace(item, target_key, results)

def parse_stream():
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        
        try:
            data = json.loads(line)
            
            contents = []
            extract_and_replace(data, "content", contents)
            
            outputs = []
            extract_and_replace(data, "output", outputs)
            
            # 1. Print the modified JSON line
            print(json.dumps(data))
            print("") # Blank line after JSON
            
            # 2. Print extracted stuff
            for c in contents:
                print(c)
                print("") # Blank line after each content item
            
            for o in outputs:
                print(o)
                print("") # Blank line after each output item

        except json.JSONDecodeError:
            # Fallback for non-JSON lines
            print(line)
            print("")

if __name__ == "__main__":
    parse_stream()
