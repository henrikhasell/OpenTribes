#!/usr/bin/env python3
"""Compare what this port's layouts read against t2-demo-parser's, per class.

    T2_DEMO_PARSER=<checkout> python3 Tools/t2/xcdiff.py CAPTURE [CAPTURE ...]

Two independent readings of one recorded session is worth much more than one.
t2-demo-parser (MIT, Brian Beck, github.com/exogen/t2-demo-parser) decodes the
same ghost, event and datablock tables out of the same retail binary, by
reading the disassembly rather than by emulating it, so where the two agree on
how many bits an object's body took the layout is settled, and where they
differ exactly one of them is wrong about a field.

That is how TerrainBlock's material-name list was found: this port read 1219
bits where a session carries 1735, because the walk judged the guarded call
ending unpackUpdate to read nothing.

Disagreements are printed per class with the lengths each side saw.
"""
import collections
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def theirs(capture):
    root = os.environ.get("T2_DEMO_PARSER")
    if not root:
        raise SystemExit("set T2_DEMO_PARSER to a t2-demo-parser checkout")
    out = os.path.join(HERE, ".xc.json")
    r = subprocess.run(
        ["node", "--import", os.path.join(HERE, "tsresolve.mjs"),
         os.path.join(HERE, "crosscheck.mjs"), capture, "--json", out],
        capture_output=True, text=True, timeout=3600,
        env=dict(os.environ, T2_DEMO_PARSER=root))
    if r.returncode != 0:
        raise SystemExit("crosscheck failed:\n%s" % (r.stderr[-2000:]))
    d = json.load(open(out))
    os.unlink(out)
    return ({k: {int(b): c for b, c in v["lengths"].items()}
             for k, v in d["classes"].items()},
            d["faults"])


def mine(capture):
    r = subprocess.run(
        [sys.executable, os.path.join(HERE, "replay.py"), capture],
        capture_output=True, text=True, timeout=3600,
        env=dict(os.environ, T2_KEEP="1", T2_VERBOSE="1"))
    per = collections.defaultdict(collections.Counter)
    for line in r.stdout.splitlines():
        m = re.match(r"\s+(?:ga|ghost) \d+ (?:upd |new )?(\w+) (\d+) bits", line)
        if m:
            per[m.group(1)][int(m.group(2))] += 1
            continue
        m = re.match(r"\s+control (\w+) (\d+) bits", line)
        if m:
            per["control:" + m.group(1)][int(m.group(2))] += 1
            continue
        m = re.match(r"\s+datablock (\w+) (\d+) bits", line)
        if m:
            per["datablock:" + m.group(1)][int(m.group(2))] += 1
    m = re.search(r"packets failed : (\d+) of (\d+)", r.stdout)
    return {k: dict(v) for k, v in per.items()}, (m.groups() if m else ("?", "?"))


def fmt(d):
    return " ".join("%dx%d" % (k, v) if v > 1 else str(k)
                    for k, v in sorted(d.items())) or "-"


def main(captures):
    agree = set()
    differ = collections.defaultdict(list)
    unseen = collections.Counter()
    for cap in captures:
        t, faults = theirs(cap)
        m, failed = mine(cap)
        name = os.path.basename(cap)
        print("%-16s theirs %d fault(s), mine %s of %s packets failed"
              % (name, len(faults), failed[0], failed[1]))
        for cls in sorted(set(t) | set(m)):
            a, b = m.get(cls, {}), t.get(cls, {})
            if not a:
                unseen[cls] += sum(b.values())
                continue
            if a == b:
                agree.add(cls)
            else:
                differ[cls].append((name, a, b))
    print()
    if agree:
        print("agree exactly (%d): %s" % (len(agree), " ".join(sorted(agree))))
    if unseen:
        print("only they read (%d): %s"
              % (len(unseen), " ".join("%s(%d)" % (k, v)
                                       for k, v in sorted(unseen.items()))))
    for cls in sorted(differ):
        print()
        print("%s differs:" % cls)
        for name, a, b in differ[cls]:
            print("   %-12s mine   %s" % (name, fmt(a)))
            print("   %-12s theirs %s" % ("", fmt(b)))
    if not differ:
        print()
        print("no class the two both read disagrees")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    main(sys.argv[1:])
