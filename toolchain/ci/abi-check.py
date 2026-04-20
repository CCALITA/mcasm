#!/usr/bin/env python3
"""Verify exported ASM symbols match C header declarations."""
import sys
import subprocess
import re
import os

def get_header_functions(include_dir):
    """Extract function declarations from C headers."""
    functions = set()
    for fname in os.listdir(include_dir):
        if not fname.endswith('.h'):
            continue
        with open(os.path.join(include_dir, fname)) as f:
            for line in f:
                m = re.match(r'^\w[\w\s\*]*\s+(mc_\w+)\s*\(', line)
                if m:
                    functions.add(m.group(1))
    return functions

def get_lib_symbols(lib_paths):
    """Extract global symbols from static libraries."""
    symbols = set()
    for lib in lib_paths:
        if not os.path.exists(lib):
            continue
        result = subprocess.run(['nm', '-g', lib], capture_output=True, text=True)
        for line in result.stdout.splitlines():
            parts = line.split()
            if len(parts) >= 3 and parts[1] in ('T', 't'):
                sym = parts[2]
                if sym.startswith('_'):
                    sym = sym[1:]
                if sym.startswith('mc_'):
                    symbols.add(sym)
    return symbols

def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <include_dir> <lib1.a> [lib2.a ...]")
        sys.exit(1)

    include_dir = sys.argv[1]
    lib_paths = sys.argv[2:]

    declared = get_header_functions(include_dir)
    exported = get_lib_symbols(lib_paths)

    missing = declared - exported
    extra = exported - declared

    ok = True
    if missing:
        print(f"ERROR: {len(missing)} declared functions not found in libraries:")
        for f in sorted(missing):
            print(f"  - {f}")
        ok = False

    if extra:
        print(f"WARNING: {len(extra)} exported mc_* symbols not in headers:")
        for f in sorted(extra):
            print(f"  + {f}")

    if ok:
        print(f"ABI check passed: {len(declared)} functions declared, {len(exported)} exported")
    else:
        sys.exit(1)

if __name__ == '__main__':
    main()
