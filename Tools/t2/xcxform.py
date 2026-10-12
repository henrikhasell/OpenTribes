"""Grade the transform rows in t2GhostFields.h against t2-demo-parser.

A row there names the node a transform starts at, and a node index is worth
nothing on its own: regenerate a table and every index after an inserted node
shifts, with no symptom but an object placed somewhere plausible and wrong.
The engine checks the shape of what it finds at the node it was given; this
checks the value, against the only other reader of the same bytes.

    T2_DEMO_PARSER=... python3 Tools/t2/xcxform.py <demo.rec>...

The same recovered graph is walked here, the same rows are read off it, and the
position, the quaternion, the sign of its w and the scale are compared body for
body with their parse.  A class with no body in any start block cannot be
graded at all -- a transient one never appears in one -- so the per-class
counts are the coverage as much as the result.
"""
import os, re, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rec
import xcfields

GB = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                  '..', '..', 'Engine', 'source', 'T3D', 'gameBase')

ROW = re.compile(r'\{\s*"(\w+)",\s*(-?\d+),\s*(Op\w+),\s*(\d+),\s*(T2Field\w+)\s*\}')


def rows():
    """class -> [(node, field)], straight out of the header the engine reads."""
    src = open(os.path.join(GB, 't2GhostFields.h')).read()
    out = {}
    for m in ROW.finditer(src):
        out.setdefault(m.group(1), []).append((int(m.group(2)), m.group(5)))
    return out


def as_f32(raw):
    return struct.unpack('<f', struct.pack('<I', raw & 0xffffffff))[0]


def run(layout, at, count):
    """The nodes a run of floats covers, in the order they are read."""
    nodes, out = layout[0], []
    for _ in range(count):
        out.append(at)
        at = nodes[at][2]
    return out


def assemble(cls, values, rowset):
    """What the engine would hand to onT2BodyRead for this body."""
    layout = rec.GH_LAYOUTS[cls]
    body = {}
    for node, field in rowset:
        if node not in values:
            continue
        if field == 'T2FieldTransform':
            ns = run(layout, node, 16)
            if all(k in values for k in ns):
                m = [as_f32(values[k]) for k in ns]
                body['position'] = (m[3], m[7], m[11])
        elif field == 'T2FieldAffine':
            ns = run(layout, node, 6)
            if all(k in values for k in ns):
                v = [as_f32(values[k]) for k in ns]
                body['position'] = tuple(v[0:3])
                body['quat'] = tuple(v[3:6])
        elif field == 'T2FieldQuatSign':
            body['wNegative'] = bool(values[node])
        elif field == 'T2FieldScale':
            ns = run(layout, node, 3)
            if all(k in values for k in ns):
                body['scale'] = tuple(as_f32(values[k]) for k in ns)
    return body


def off(got, want, tol=1e-4):
    return abs(got - want) > tol * max(1.0, abs(want))


def grade(path, R):
    t = xcfields.theirs(path)
    c = rec.read_container(path)
    per, msgs, bare = {}, [], 0

    for g in t['ghosts']:
        cls = (rec.OBJECTS[g['classId']] if g['classId'] < len(rec.OBJECTS)
               else None)
        if cls not in R or cls not in rec.GH_LAYOUTS:
            continue

        st = rec.Bits(c['block'])
        st.p = g['start']
        values = {}
        try:
            rec.Walk(st, values=values).run(cls, rec.GH_LAYOUTS[cls], True)
        except Exception as e:
            # The last entry of their list is an end marker, not a body.
            msgs.append('%-22s ghost %-5d walk failed: %s'
                        % (cls, g['index'], type(e).__name__))
            continue

        e = per.setdefault(cls, [0, 0, 0])
        e[0] += 1
        if st.p - g['start'] != g['width']:
            msgs.append('%-22s ghost %-5d WIDTH ours %d theirs %d'
                        % (cls, g['index'], st.p - g['start'], g['width']))
            e[2] += 1
            continue

        mine = assemble(cls, values, R[cls])
        want = g['data'].get('transform')
        if want is None:
            if 'position' in mine:
                msgs.append('%-22s ghost %-5d we place it, they report no '
                            'transform' % (cls, g['index']))
                e[2] += 1
            else:
                bare += 1
            continue

        if 'position' not in mine:
            msgs.append('%-22s ghost %-5d they have a transform, we read none'
                        % (cls, g['index']))
            e[2] += 1
            continue

        wp = want['position'] if isinstance(want.get('position'), dict) else want
        wq = want.get('rotation') or want.get('quat')
        ws = g['data'].get('scale')
        bad = []
        for i, k in enumerate('xyz'):
            if off(mine['position'][i], wp[k]):
                bad.append('%s %.6f vs %.6f' % (k, mine['position'][i], wp[k]))
        if wq and 'quat' in mine:
            for i, k in enumerate('xyz'):
                if off(mine['quat'][i], wq[k]):
                    bad.append('q%s %.6f vs %.6f' % (k, mine['quat'][i], wq[k]))
            if 'w' in wq and 'wNegative' in mine \
                    and mine['wNegative'] != (wq['w'] < 0):
                bad.append('qw sign %s vs %.6f' % (mine['wNegative'], wq['w']))
        if ws:
            if 'scale' not in mine:
                bad.append('they have a scale, we read none')
            else:
                for i, k in enumerate('xyz'):
                    if off(mine['scale'][i], ws[k]):
                        bad.append('s%s %.6f vs %.6f'
                                   % (k, mine['scale'][i], ws[k]))
        if bad:
            msgs.append('%-22s ghost %-5d %s'
                        % (cls, g['index'], '; '.join(bad[:4])))
            e[2] += 1
        else:
            e[1] += 1

    print(os.path.basename(path))
    for m in msgs[:28]:
        print('  ' + m)
    if len(msgs) > 28:
        print('  ... and %d more rows' % (len(msgs) - 28))
    print('  %-22s %7s %6s %6s' % ('class', 'bodies', 'agree', 'differ'))
    for cls in sorted(per):
        print('  %-22s %7d %6d %6d' % (cls, per[cls][0], per[cls][1],
                                       per[cls][2]))
    agree = sum(e[1] for e in per.values())
    differ = sum(e[2] for e in per.values())
    print('  %d agree, %d differ, %d bodies carried no transform'
          % (agree, differ, bare))
    return differ


def main(argv):
    if len(argv) < 2:
        raise SystemExit(__doc__)
    R = rows()
    bad = 0
    for path in argv[1:]:
        bad += grade(path, R)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
