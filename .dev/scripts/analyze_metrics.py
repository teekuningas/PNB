#!/usr/bin/env python3
"""
Information-theoretic code quality metrics analyzer.

Computes two key metrics for tracking refactoring progress:
1. I_sig - Average signature weight (recursive type complexity)
2. LOC_avg - Average function size

Also reports test coverage if available.

Usage: ./analyze_metrics.py [source_dir]
"""
import re
import sys
import math
from pathlib import Path
from collections import defaultdict

def parse_struct_definition(content, struct_name):
    """Extract field types from a struct definition."""
    pattern = rf'typedef\s+struct\s+\w*\s*\{{([^}}]*)\}}\s*{struct_name}\s*;'
    match = re.search(pattern, content, re.DOTALL)
    if not match:
        return []
    
    body = match.group(1)
    fields = []
    for line in body.split(';'):
        line = line.strip()
        if not line or line.startswith('//') or line.startswith('/*'):
            continue
        parts = line.split()
        if len(parts) >= 2:
            field_type = ' '.join(parts[:-1]).replace('const', '').replace('*', '').strip()
            fields.append(field_type)
    return fields

def build_struct_graph(src_dir):
    """Build struct dependency graph."""
    structs = {}
    for file_path in src_dir.rglob('*.h'):
        content = file_path.read_text(errors='ignore')
        pattern = r'typedef\s+struct\s+\w*\s*\{[^}]*\}\s*(\w+)\s*;'
        for match in re.finditer(pattern, content):
            struct_name = match.group(1)
            fields = parse_struct_definition(content, struct_name)
            structs[struct_name] = fields
    return structs

def compute_recursive_weight(type_name, struct_graph, memo, primitives):
    """Recursively compute type weight."""
    if type_name in memo:
        return memo[type_name]
    if type_name in primitives:
        memo[type_name] = 1
        return 1
    if type_name not in struct_graph:
        memo[type_name] = 3
        return 3
    if type_name in memo and memo[type_name] == -1:
        return 5  # Cycle detection
    
    memo[type_name] = -1
    total = sum(compute_recursive_weight(f, struct_graph, memo, primitives) 
                for f in struct_graph[type_name])
    memo[type_name] = total
    return total

def compute_all_weights(struct_graph):
    """Compute all recursive weights."""
    primitives = {
        'int', 'float', 'double', 'char', 'void', 'size_t',
        'uint32_t', 'int32_t', 'uint8_t', 'uint16_t', 'int16_t',
        'uint64_t', 'int64_t', 'bool', '_Bool',
        'GLuint', 'GLint', 'GLenum', 'GLfloat', 'GLdouble',
        'ma_uint32', 'ma_int32', 'ma_uint64', 'ma_bool32'
    }
    memo = {}
    weights = {}
    for struct_name in struct_graph:
        weights[struct_name] = compute_recursive_weight(struct_name, struct_graph, memo, primitives)
    return weights, primitives

def extract_functions(content):
    """Extract function signatures."""
    pattern = r'^\s*(?:static\s+)?(\w+(?:\s*\*)?)\s+(\w+)\s*\(([^)]*)\)'
    functions = []
    for match in re.finditer(pattern, content, re.MULTILINE):
        return_type, name, params = match.groups()
        functions.append({
            'name': name,
            'return_type': return_type.strip(),
            'params': params.strip()
        })
    return functions

def count_loc(content):
    """Count lines of code."""
    lines = content.split('\n')
    loc = 0
    in_multiline_comment = False
    for line in lines:
        line = line.strip()
        if '/*' in line:
            in_multiline_comment = True
        if '*/' in line:
            in_multiline_comment = False
            continue
        if in_multiline_comment or not line or line.startswith('//'):
            continue
        loc += 1
    return loc

def analyze_signature(func, type_weights, primitives):
    """Analyze function signature."""
    params = func['params']
    if not params or params == 'void':
        return {'sig_weight': 0}
    
    param_list = [p.strip() for p in params.split(',')]
    sig_weight = 0
    
    for param in param_list:
        parts = param.split()
        if len(parts) >= 2:
            param_type = ' '.join(parts[:-1]).replace('const', '').replace('*', '').strip()
            if param_type in primitives:
                sig_weight += 1
            elif param_type in type_weights:
                sig_weight += type_weights[param_type]
            else:
                sig_weight += 3
    
    return {'sig_weight': sig_weight}

def analyze_codebase(src_dir):
    """Analyze entire codebase and return metrics."""
    # Build type system
    struct_graph = build_struct_graph(src_dir)
    type_weights, primitives = compute_all_weights(struct_graph)
    
    # Analyze functions
    all_funcs = []
    total_loc = 0
    
    for file_path in src_dir.rglob('*.c'):
        content = file_path.read_text(errors='ignore')
        functions = extract_functions(content)
        loc = count_loc(content)
        total_loc += loc
        
        for func in functions:
            metrics = analyze_signature(func, type_weights, primitives)
            all_funcs.append(metrics)
    
    # Compute metrics
    total_funcs = len(all_funcs)
    if total_funcs == 0:
        return None
    
    total_sig_weight = sum(f['sig_weight'] for f in all_funcs)
    
    i_sig = total_sig_weight / total_funcs
    loc_avg = total_loc / total_funcs
    
    return {
        'I_sig': i_sig,
        'LOC_avg': loc_avg,
        'total_funcs': total_funcs,
        'total_loc': total_loc
    }

def run_tests():
    """Run tests and extract coverage information."""
    import subprocess
    try:
        result = subprocess.run(
            ['devenv', 'shell', 'make', 'test'],
            capture_output=True,
            text=True,
            timeout=120
        )
        
        # Extract test results
        output = result.stdout + result.stderr
        tests_run = 0
        tests_failed = 0
        
        for line in output.split('\n'):
            if 'Tests run:' in line:
                tests_run = int(line.split(':')[1].strip())
            elif 'Tests failed:' in line:
                tests_failed = int(line.split(':')[1].strip())
        
        if tests_run > 0:
            return {
                'tests_run': tests_run,
                'tests_passed': tests_run - tests_failed,
                'tests_failed': tests_failed
            }
    except:
        pass
    
    return None

def main():
    if len(sys.argv) < 2:
        src_dir = Path('src')
    else:
        src_dir = Path(sys.argv[1])
    
    if not src_dir.exists():
        print(f"Error: Directory '{src_dir}' does not exist")
        sys.exit(1)
    
    print("Analyzing codebase...")
    metrics = analyze_codebase(src_dir)
    
    if not metrics:
        print("No functions found")
        return
    
    # Run tests if available
    test_results = run_tests()
    
    print("\n" + "="*70)
    print("INFORMATION-THEORETIC CODE QUALITY METRICS")
    print("="*70)
    print(f"\nTotal Functions: {metrics['total_funcs']}")
    print(f"Total LOC: {metrics['total_loc']}")
    
    if test_results:
        print(f"Tests: {test_results['tests_passed']}/{test_results['tests_run']} passed")
    
    print(f"\n{'Metric':<30} {'Value':>10} {'Goal':>10}")
    print("-" * 70)
    print(f"{'I_sig (signature weight)':<30} {metrics['I_sig']:>10.2f} {'↓ lower':>10}")
    print(f"{'LOC_avg (function size)':<30} {metrics['LOC_avg']:>10.1f} {'↓ lower':>10}")
    print("="*70)

if __name__ == '__main__':
    main()
