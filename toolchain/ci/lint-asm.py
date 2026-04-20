#!/usr/bin/env python3
"""Lint assembly files for style consistency."""
import sys
import re

RULES = [
    ("No tabs in source (use spaces)", lambda l, n: '\t' in l and n > 0),
    ("Line too long (>120 chars)", lambda l, n: len(l.rstrip()) > 120),
]

def lint_file(path):
    issues = []
    try:
        with open(path) as f:
            lines = f.readlines()
    except IOError as e:
        return [(0, f"Cannot read: {e}")]

    for i, line in enumerate(lines, 1):
        for desc, check in RULES:
            if check(line, i):
                issues.append((i, desc))
    return issues

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <file1.asm> [file2.asm ...]")
        sys.exit(0)

    total_issues = 0
    for path in sys.argv[1:]:
        issues = lint_file(path)
        if issues:
            for line_num, desc in issues:
                print(f"{path}:{line_num}: {desc}")
            total_issues += len(issues)

    if total_issues:
        print(f"\n{total_issues} style issues found")
        sys.exit(1)
    else:
        print(f"Lint passed: {len(sys.argv)-1} files checked")

if __name__ == '__main__':
    main()
