"""Diff our ghost body widths against t2-demo-parser's, over a recording.

Phase 2 of `GHOST_DERIVATION_PLAN.md`, done by comparison rather than by
measurement.  `exogen/t2-demo-parser` (MIT, Brian Beck) decodes a `.rec` from
the same binary by hand off the disassembly, and its `GhostUpdate` carries
`updateBitsStart`/`updateBitsEnd` -- a width per ghost body, which is exactly
what this phase set out to measure a client launch at a time.

    T2_DEMO_PARSER=/path/to/t2-demo-parser python3 Tools/t2/xcwidths.py <demo.rec>

Clone and build it first (`npm install` runs `tsc`); it needs only `debug` and
`fflate` at runtime.

Where the two agree the width is settled.  Where they differ, exactly one is
wrong about that class, and the arbiter is retail: `craft.py` plus a client,
which is how `LinearProjectile` was settled at 144 against our 46.  Do not
take agreement as proof of the *fields* -- that oracle's names have been wrong
here before while its widths held.
"""
import json, os, subprocess, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rec

HOOK = """
import { DemoParser } from './dist/index.js';
import { readFileSync } from 'node:fs';
const parser = new DemoParser(readFileSync(process.argv[2]));
await parser.load();
const ib = parser.initialBlock;
console.log(JSON.stringify({
  mission: ib.missionName,
  warnings: ib.warnings ?? [],
  ghosts: ib.initialGhosts.map(g => ({
    index: g.index, classId: g.classId,
    start: g.updateBitsStart, width: g.updateBitsEnd - g.updateBitsStart,
    failed: !!g.failed, error: g.error ?? null,
  })),
}));
"""


def theirs(path):
    root = os.environ.get('T2_DEMO_PARSER')
    if not root:
        raise SystemExit('set T2_DEMO_PARSER to a built t2-demo-parser checkout')
    hook = os.path.join(root, '.xcwidths.mjs')
    open(hook, 'w').write(HOOK)
    out = subprocess.run(['node', hook, path], cwd=root, capture_output=True,
                          text=True)
    if out.returncode:
        raise SystemExit('their parser failed:\n%s' % out.stderr[-1500:])
    d = json.loads(out.stdout)
    for g in d['ghosts']:
        g['name'] = (rec.OBJECTS[g['classId']]
                     if g['classId'] < len(rec.OBJECTS)
                     else '?%d' % g['classId'])
    return d


def ours(path):
    c = rec.read_container(path)
    try:
        out, st = rec.read_start_block(c['block'])
        stopped = None
    except rec.GhostFault as e:
        out, st, stopped = e.out, e.st, str(e)
    return {body: (width, name) for _, body, width, index, name
            in out['ghostRecords']}, stopped


def main(argv):
    path = argv[1]
    t = theirs(path)
    mine, stopped = ours(path)

    rows = []
    for g in t['ghosts']:
        got = mine.get(g['start'])
        rows.append((g['name'], g['width'], got[0] if got else None))

    per = {}
    for name, theirw, ourw in rows:
        e = per.setdefault(name, {'widths': {}, 'agree': 0, 'differ': 0,
                                  'unread': 0})
        e['widths'][theirw] = e['widths'].get(theirw, 0) + 1
        if ourw is None:
            e['unread'] += 1
        elif ourw == theirw:
            e['agree'] += 1
        else:
            e['differ'] += 1
            e.setdefault('ourWidths', set()).add(ourw)

    print('%s -- mission %r, %d initial ghosts%s'
          % (os.path.basename(path), t['mission'], len(rows),
             ', %d their parser could not read' % sum(1 for g in t['ghosts']
                                                      if g['failed'])
             if any(g['failed'] for g in t['ghosts']) else ''))
    print('%-22s %-26s %6s %6s %6s' % ('class', 'widths (theirs)', 'agree',
                                       'differ', 'unread'))
    for name in sorted(per):
        e = per[name]
        w = ', '.join('%d x%d' % (k, v) for k, v in sorted(e['widths'].items()))
        note = ''
        if e['differ']:
            note = '   ours: %s' % sorted(e['ourWidths'])
        print('%-22s %-26s %6d %6d %6d%s'
              % (name, w, e['agree'], e['differ'], e['unread'], note))
    if '--emit' in argv:
        # Their widths, keyed by position in the run, which is what
        # `rec.read_start_block(widths=...)` pins.  Written so our reader can
        # walk a whole block before our own layouts can read every body.
        dest = argv[argv.index('--emit') + 1]
        json.dump({str(i): g['width'] for i, g in enumerate(t['ghosts'])},
                  open(dest, 'w'))
        print('wrote %d widths to %s' % (len(t['ghosts']), dest))
    if stopped:
        print('our walk stopped: %s' % stopped)
    for w in t['warnings']:
        print('their warning: %s' % w)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
