#!/usr/bin/env python3
import json
import sys
import os

def load_log(filepath):
    if not os.path.exists(filepath):
        print(f"Error: File {filepath} not found.")
        return None
    try:
        with open(filepath, 'r') as f:
            return json.load(f)
    except json.JSONDecodeError as e:
        print(f"Error decoding JSON: {e}")
        return None

def state_to_string(s):
    return s  # It's already a string in the JSON ("AT_BAT", etc.)

def base_to_string(b):
    if b == 0: return "HOME"
    if b == 1: return "1ST"
    if b == 2: return "2ND"
    if b == 3: return "3RD"
    if b == 4: return "SCORED"
    if b == -1: return "NONE"
    return str(b)

def print_diff(prev, curr, seq):
    print(f"--- Pitch Sequence {seq} ---")
    
    # Check Half Inning State
    prev_hi = prev.get('halfInningState', {})
    curr_hi = curr.get('halfInningState', {})
    
    changes = []
    if prev_hi.get('outs') != curr_hi.get('outs'):
        changes.append(f"Outs: {prev_hi.get('outs')} -> {curr_hi.get('outs')}")
    if prev_hi.get('runsInTheInning') != curr_hi.get('runsInTheInning'):
        changes.append(f"Runs (Inning): {prev_hi.get('runsInTheInning')} -> {curr_hi.get('runsInTheInning')}")
    if prev_hi.get('strikes') != curr_hi.get('strikes'):
        changes.append(f"Strikes: {prev_hi.get('strikes')} -> {curr_hi.get('strikes')}")
    if prev_hi.get('balls') != curr_hi.get('balls'):
        changes.append(f"Balls: {prev_hi.get('balls')} -> {curr_hi.get('balls')}")
        
    if changes:
        print("  Game State Changes:")
        for c in changes:
            print(f"    {c}")

    # Check Batter
    prev_batter = prev.get('pII', {}).get('batterIndex', -1)
    curr_batter = curr.get('pII', {}).get('batterIndex', -1)
    if prev_batter != curr_batter:
        print(f"  Batter Changed: {prev_batter} -> {curr_batter}")

    # Check Players on Base
    # We need to map players to verify movement
    # JSON has "players" array.
    prev_players = {p['id']: p for p in prev.get('players', []) if p['id'] != -1}
    curr_players = {p['id']: p for p in curr.get('players', []) if p['id'] != -1}
    
    moved = []
    for pid, p_curr in curr_players.items():
        p_prev = prev_players.get(pid)
        if not p_prev:
            continue # New player? Should not happen usually
            
        # Check Base Change
        if p_prev['baseId'] != p_curr['baseId']:
            moved.append(f"Player {pid}: {p_prev['baseStr']} -> {p_curr['baseStr']}")
        
        # Check State Change (e.g. Wounded)
        if p_prev['state'] != p_curr['state']:
             moved.append(f"Player {pid} State: {p_prev['state']} -> {p_curr['state']}")
             
    if moved:
        print("  Player Movement:")
        for m in moved:
            print(f"    {m}")

def analyze(log):
    print(f"Analysis of Debug Log")
    print(f"Reason: {log.get('failure_reason')}")
    print(f"Timestamp: {log.get('timestamp')}")
    
    current = log.get('currentState', {})
    history = log.get('history', [])
    
    print("\n=== HISTORY (Last 100 Pitches) ===")
    
    if not history:
        print("No history found.")
    else:
        # Sort by seq just in case, though usually ordered
        history.sort(key=lambda x: x['seq'])
        
        for i in range(len(history)):
            entry = history[i]
            seq = entry['seq']
            snapshot = entry['snapshot']
            
            if i == 0:
                print(f"--- Pitch Sequence {seq} (Start of Log) ---")
                # Just print state summary
                hi = snapshot.get('halfInningState', {})
                print(f"  Outs: {hi.get('outs')}, Strikes: {hi.get('strikes')}, Balls: {hi.get('balls')}")
                print(f"  Batter: {snapshot.get('pII', {}).get('batterIndex')}")
            else:
                prev_snapshot = history[i-1]['snapshot']
                print_diff(prev_snapshot, snapshot, seq)

    print("\n=== CURRENT STATE (Final) ===")
    # Compare last history to current
    if history:
        last_hist = history[-1]['snapshot']
        print_diff(last_hist, current, "FINAL (Manual Pause)")
    
    # Print detailed global info
    glob = current.get('global', {})
    print(f"\nFinal Scoreboard:")
    print(f"  Inning: {glob.get('inning')}")
    print(f"  Period: {glob.get('period')}")
    print(f"  Runs: Team 0: {glob.get('team0_runs')}, Team 1: {glob.get('team1_runs')}")
    print(f"  Batter Order Index: T0: {glob.get('team0_batterOrderIndex')}, T1: {glob.get('team1_batterOrderIndex')}")
    
    # Counters
    cnt = current.get('playerCounters', {})
    print(f"\nCounters:")
    print(f"  Non-Jokers Left: {cnt.get('nonJokerPlayersLeft')}")
    print(f"  Jokers Left: {cnt.get('jokersLeft')}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        path = "debug.log"
    else:
        path = sys.argv[1]
        
    log = load_log(path)
    if log:
        analyze(log)
