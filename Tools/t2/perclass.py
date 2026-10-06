"""Grade every ghost class in a recording, one body at a time.

Phase 3's worklist.  A body cannot be checked where it stands, and a greedy
walk drifts as soon as one layout is wrong -- so this pins the second reader's
widths for every ghost *except* one, lets our layout read that one, and
compares.  Each class then gets a verdict that does not depend on any other
class being right.

    T2_DEMO_PARSER=... python3 Tools/t2/perclass.py <demo.rec>

A class passes when our layout reads every occurrence at the width both
readers agree on.  Passing is not proof of its fields -- see
`xcwidths.py` -- but failing is proof of a fault.
"""
import os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rec
import xcwidths
import describe


def grade(path, described=True):
    if described:
        # A described class is graded as described, not as shipped: the point
        # of a description is to replace a table that reads a body wrong.
        for name in describe.DESCRIPTIONS:
            rec.GH_LAYOUTS[name] = describe.layout(name)
    t = xcwidths.theirs(path)
    theirs = {i: g['width'] for i, g in enumerate(t['ghosts'])}
    names = {i: g['name'] for i, g in enumerate(t['ghosts'])}
    c = rec.read_container(path)

    per = {}
    for k in range(len(theirs)):
        pinned = {i: w for i, w in theirs.items() if i != k}
        name = names[k]
        try:
            out, st = rec.read_start_block(c['block'], widths=pinned)
            got = None
            for ordinal, (_, _, width, _, n) in enumerate(out['ghostRecords']):
                if ordinal == k:
                    got = width
        except rec.GhostFault as e:
            got = e.ours if e.ordinal == k else None
        except Exception:
            got = None
        e = per.setdefault(name, {'pass': 0, 'fail': 0, 'threw': 0,
                                  'theirs': set(), 'ours': set()})
        e['theirs'].add(theirs[k])
        if got is None:
            e['threw'] += 1
        elif got == theirs[k]:
            e['pass'] += 1
        else:
            e['fail'] += 1
            e['ours'].add(got)
    return per, t


def main(argv):
    path = argv[1]
    per, t = grade(path)
    print('%s -- mission %r, %d initial ghosts'
          % (os.path.basename(path), t['mission'], len(t['ghosts'])))
    print('%-22s %5s %5s %5s  %s' % ('class', 'pass', 'fail', 'threw',
                                     'widths'))
    ok = bad = 0
    for name in sorted(per, key=lambda n: (-per[n]['fail'], n)):
        e = per[name]
        ok += e['pass']
        bad += e['fail'] + e['threw']
        w = 'theirs %s' % sorted(e['theirs'])
        if e['ours']:
            w += '   ours %s' % sorted(e['ours'])
        print('%-22s %5d %5d %5d  %s'
              % (name, e['pass'], e['fail'], e['threw'], w))
    print('%d bodies our layouts read correctly, %d they do not' % (ok, bad))
    return 0 if bad == 0 else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
