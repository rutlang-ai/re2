#!/usr/bin/env python3
"""Alternate old/new executables on one CPU and report median nanoseconds."""
import argparse
import json
import os
import statistics
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("baseline")
p.add_argument("candidate")
p.add_argument("--runs", type=int, default=11)
p.add_argument("--cpu", type=int)
p.add_argument("--output", default="runtime-benchmark.json")
a = p.parse_args()
if hasattr(os, "sched_setaffinity"):
    os.sched_setaffinity(0, {a.cpu if a.cpu is not None else min(os.sched_getaffinity(0))})
results = {"baseline": {}, "candidate": {}}
for run in range(a.runs):
    order = ["baseline", "candidate"] if run % 2 == 0 else ["candidate", "baseline"]
    for name in order:
        for line in subprocess.check_output([getattr(a, name)], text=True).splitlines():
            case, ns = line.split()
            results[name].setdefault(case, []).append(float(ns))
with open(a.output, "w") as f:
    json.dump(results, f, indent=2)
print("| Case | Baseline ns | Candidate ns | Change |")
print("| --- | ---: | ---: | ---: |")
for case in results["baseline"]:
    old = statistics.median(results["baseline"][case])
    new = statistics.median(results["candidate"][case])
    print(f"| {case} | {old:.2f} | {new:.2f} | {(new / old - 1) * 100:+.1f}% |")
