#!/usr/bin/env python3
"""Judge each recovered ghost layout against recorded sessions.

    vetghost.py CAPTURE [CAPTURE ...]

A walk that comes back acyclic can still be wrong.  Sky's cloud layers are
read by a loop of exactly three, and the walk read its bound off the wire
instead: the layout then took a thirty-two bit field as a repeat count, lost
the rest of the packet, and with it four guaranteed events.  Losing a
guaranteed event stalls the whole ordered queue, so the client sat in phase two
holding a MissionStartPhase3 it would never process.

Nothing about the graph's shape says which of those two readings is right --
TerrainBlock really does read its material count as a thirty-two bit word --
so the judge has to be a session.  This replays the captures, blames the
classes that appear only in packets that failed, writes them to
ghost_veto.json for genghost.py to leave out, and repeats until no packet
blames anything.  A class left out keeps whatever t2GhostTables.h says, which
is where the walk's own failures already land.
"""
import json, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
VETO = os.path.join(HERE, "ghost_veto.json")


def run(capture):
    """Replay one capture; return (failed, total, {class: [ok, bad]})."""
    env = dict(os.environ, T2_KEEP="1", T2_VERBOSE="1")
    p = subprocess.run([sys.executable, os.path.join(HERE, "replay.py"), capture],
                       capture_output=True, text=True, env=env, timeout=1800)
    out = p.stdout
    failed = total = 0
    m = re.search(r"packets failed : (\d+) of (\d+)", out)
    if m:
        failed, total = int(m.group(1)), int(m.group(2))
    bad = set(int(x) for x in re.findall(r"^   packet (\d+) ", out, re.M))

    # which classes each packet's bodies came from
    per, cur = {}, None
    for line in out.splitlines():
        m = re.match(r"packet (\d+) \(", line)
        if m:
            cur = int(m.group(1))
            continue
        m = re.match(r"\s+(?:ga \d+|ghost \d+ new) (\w+)", line)
        if m and cur is not None:
            per.setdefault(cur, set()).add(m.group(1))
    tally = {}
    for pkt, classes in per.items():
        for c in classes:
            e = tally.setdefault(c, [0, 0])
            e[1 if pkt in bad else 0] += 1
    # a class named in the failure text itself is blamed outright
    named = set(re.findall(r"(?:loop count \d+|graph loop) in (\w+)", out))
    named |= set(re.findall(r"no layout for (?:ghost(?: always)?) (\w+)", out))
    return failed, total, tally, named


def blame(captures):
    """Classes that never appear in a packet that read cleanly."""
    ok, bad, named = {}, {}, set()
    totals = []
    for c in captures:
        f, t, tally, nm = run(c)
        totals.append((os.path.basename(c), f, t))
        named |= nm
        for k, (o, b) in tally.items():
            ok[k] = ok.get(k, 0) + o
            bad[k] = bad.get(k, 0) + b
    suspects = {k: bad[k] for k in bad if bad[k] and not ok.get(k)}
    for k in named:
        suspects.setdefault(k, 1)
    return totals, suspects


def emit(veto):
    json.dump(veto, open(VETO, "w"), indent=1, sort_keys=True)
    subprocess.run([sys.executable, os.path.join(HERE, "genghost.py")],
                   capture_output=True, text=True, check=True)


def score(captures):
    totals, suspects = blame(captures)
    return sum(f for _, f, _ in totals), totals, suspects


def main(captures):
    veto = json.load(open(VETO)) if os.path.exists(VETO) else {}
    for round_no in range(1, 6):
        emit(veto)
        base, totals, suspects = score(captures)
        print("round %d  %d failed  %s" % (round_no, base, "  ".join(
            "%s %d/%d" % (n, f, t) for n, f, t in totals)), flush=True)
        fresh = [k for k in sorted(suspects) if k not in veto]
        if not fresh:
            print("nothing left to try")
            break
        # Appearing only in failing packets is suspicion, not proof: a class
        # that shares a packet with the guilty one looks exactly the same.  So
        # each suspect is vetoed on its own and kept out only if the recorded
        # sessions actually read better without it.
        gained = False
        for k in fresh:
            trial = dict(veto)
            trial[k] = "trial"
            emit(trial)
            got, _, _ = score(captures)
            verdict = "%d -> %d" % (base, got)
            if got < base:
                veto[k] = ("out: the captures read %s packets without it" % verdict)
                base = got
                gained = True
                print("   veto %-22s %s" % (k, verdict), flush=True)
            else:
                print("   keep %-22s %s" % (k, verdict), flush=True)
        emit(veto)
        if not gained:
            print("no single class left to veto improves the captures")
            break
    print("vetoed: %s" % (" ".join(sorted(veto)) or "(none)"))


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    main(sys.argv[1:])
