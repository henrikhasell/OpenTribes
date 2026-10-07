"""Width-diff every ghost body in a demo's *packet log* against t2-demo-parser.

The start-block graders cannot see these.  A start block holds only initial
bodies of the classes a mission opens with; the packet log holds update bodies
and the transient classes -- projectiles, vehicles, debris -- that never appear
in one.  A single wrong width there desyncs the rest of the packet, which
surfaces much later as an out-of-range class id, so the fault needs finding by
width rather than by symptom.

    T2_DEMO_PARSER=... python3 Tools/t2/xcpackets.py <demo.rec>

Each body is read at the offset their parser reports, so one wrong class cannot
drag the others out of position -- the same discipline perclass.py uses for the
start block.
"""
import base64, collections, json, os, subprocess, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rec
import describe

HOOK = """
import { DemoParser } from './dist/index.js';
import { readFileSync } from 'node:fs';
const p = new DemoParser(readFileSync(process.argv[2]));
const demo = await p.parseFullDemo();
for (const b of demo.blocks) {
  const g = (b.parsed && b.parsed.ghosts) || [];
  if (!g.length) continue;
  process.stdout.write(JSON.stringify({
    index: b.index,
    data: Buffer.from(b.data).toString('base64'),
    ghosts: g.map(x => ({ i: x.index, t: x.type, c: x.classId,
      s: x.updateBitsStart, e: x.updateBitsEnd, f: !!x.failed })),
  }) + '\\n');
}
"""


def theirs(path):
    root = os.environ.get('T2_DEMO_PARSER')
    if not root:
        raise SystemExit('set T2_DEMO_PARSER to a built t2-demo-parser checkout')
    hook = os.path.join(root, '.xcpackets.mjs')
    open(hook, 'w').write(HOOK)
    out = subprocess.run(['node', hook, path], cwd=root, capture_output=True,
                         text=True)
    if out.returncode:
        raise SystemExit('their parser failed:\n%s' % out.stderr[-1200:])
    return [json.loads(l) for l in out.stdout.splitlines() if l.strip()]


def main(argv):
    for name in describe.DESCRIPTIONS:
        rec.GH_LAYOUTS[name] = describe.layout(name)

    per = collections.defaultdict(lambda: {'ok': 0, 'bad': 0, 'threw': 0,
                                           'deltas': collections.Counter()})
    rows = []
    for d in theirs(argv[1]):
        blob = base64.b64decode(d['data'])
        for g in d['ghosts']:
            # A delete costs no body, and a body their parser gave up on is
            # not a comparison -- it is their gap, not ours.
            if g['t'] == 'delete' or g['f']:
                continue
            cid = g['c']
            cls = (rec.OBJECTS[cid] if cid is not None and cid < len(rec.OBJECTS)
                   else '?%s' % cid)
            want = g['e'] - g['s']
            st = rec.Bits(blob)
            st.p = g['s']
            e = per['%s/%s' % (cls, g['t'])]
            try:
                rec.Walk(st).run(cls, rec.GH_LAYOUTS[cls], g['t'] == 'create')
            except Exception:
                e['threw'] += 1
                continue
            got = st.p - g['s']
            if got == want:
                e['ok'] += 1
                continue
            e['bad'] += 1
            e['deltas'][got - want] += 1
            if len(rows) < 8:
                rows.append((d['index'], g['i'], cls, g['t'], want, got))

    print('%s -- packet log' % os.path.basename(argv[1]))
    print('  %-26s %7s %7s %7s  %s'
          % ('class/op', 'match', 'differ', 'threw', 'deltas'))
    ok = bad = 0
    for k in sorted(per, key=lambda k: (-per[k]['bad'] - per[k]['threw'], k)):
        e = per[k]
        ok += e['ok']
        bad += e['bad'] + e['threw']
        d = ', '.join('%+d x%d' % (v, n) for v, n in e['deltas'].most_common(4))
        print('  %-26s %7d %7d %7d  %s'
              % (k, e['ok'], e['bad'], e['threw'], d))
    for pkt, idx, cls, t, want, got in rows:
        print('  first: packet %d ghost %d %s %s theirs %d ours %d'
              % (pkt, idx, cls, t, want, got))
    print('  %d bodies our layouts read correctly, %d they do not' % (ok, bad))
    return 0 if bad == 0 else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
