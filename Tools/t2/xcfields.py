"""Diff the *values* our layouts decode against t2-demo-parser's, per field.

The check widths cannot give.  Two readers agreeing on how wide a body is says
nothing about what is in it -- and this oracle's names have been wrong here
before while its widths held -- so a described class is graded field by field
against the same bytes: every body of that class in a recording, every named
node, decoded and compared.

    T2_DEMO_PARSER=... python3 Tools/t2/xcfields.py <demo.rec>

The comparison walks *their* structure and looks up our leaves, rather than the
other way round: their `sounds` is a list of slots and their `transform` is a
nested point and quaternion, so matching shapes is what decides whether a field
was found at all.  A leaf of ours that nothing consumed is reported too -- a
name we invented is as much a fault as a value we got wrong.

Only classes described in `describe.py` can be graded: the walk's own graphs
name nothing, so there is no field to compare.  That is the coverage limit, and
it is the reason to describe a class rather than leave it walked.

One input is taken on trust: the compression point a compressed point is
relative to comes from their parse.  It is a connection-level value, not a
field of any class, but until our reader tracks it a position comparison tests
the offsets and not the base.
"""
import json, math, os, struct, subprocess, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rec
import describe

HOOK = """
import { DemoParser } from './dist/index.js';
import { readFileSync } from 'node:fs';
const parser = new DemoParser(readFileSync(process.argv[2]));
await parser.load();
const ib = parser.initialBlock;
console.log(JSON.stringify({
  compressionPoint: ib.initialCompressionPoint ?? null,
  ghosts: ib.initialGhosts.map(g => ({
    index: g.index, classId: g.classId, start: g.updateBitsStart,
    width: g.updateBitsEnd - g.updateBitsStart, data: g.parsedData ?? {},
  })),
}));
"""


def theirs(path):
    root = os.environ.get('T2_DEMO_PARSER')
    if not root:
        raise SystemExit('set T2_DEMO_PARSER to a built t2-demo-parser checkout')
    hook = os.path.join(root, '.xcfields.mjs')
    open(hook, 'w').write(HOOK)
    out = subprocess.run(['node', hook, path], cwd=root, capture_output=True,
                         text=True)
    if out.returncode:
        raise SystemExit('their parser failed:\n%s' % out.stderr[-1200:])
    return json.loads(out.stdout)


# ------------------------------------------------------------------- decoding
def signed_float(raw, bits):
    """`readSignedFloat`: a unit float widened to -1..1, not a signed int."""
    return (raw / float((1 << bits) - 1)) * 2.0 - 1.0


def normal_vector(phi_raw, theta_raw, bits):
    """`readNormalVector(bits)`: two signed floats read as angles."""
    phi = math.pi * signed_float(phi_raw, bits + 1)
    theta = (math.pi / 2.0) * signed_float(theta_raw, bits)
    return {'x': math.sin(phi) * math.cos(theta),
            'y': math.cos(phi) * math.cos(theta),
            'z': math.sin(theta)}


def as_f32(raw):
    return struct.unpack('<f', struct.pack('<I', raw & 0xffffffff))[0]


def leaves(block, body, cls):
    """Every named node this body read, as name -> (op, bits, value)."""
    st = rec.Bits(block)
    st.p = body
    values = {}
    rec.Walk(st, values=values).run(cls, rec.GH_LAYOUTS[cls], True)
    nodes = rec.GH_LAYOUTS[cls][0]
    names = describe.names(cls)
    out = {}
    for at, v in values.items():
        name = names.get(at)
        if name:
            out[name] = (nodes[at][0], nodes[at][1], v)
    return out, st.p - body


def scalar(leaf, want, base):
    """One of our leaves, decoded as whatever wants it is shaped."""
    op, bits, raw = leaf
    if op == 'OpFlag':
        # A flag they report as a sign rather than a boolean.
        if isinstance(want, (int, float)) and not isinstance(want, bool) \
                and want in (-1, 1):
            return -1 if raw else 1
        return bool(raw)
    if op == 'OpString':
        return raw
    if op == 'OpFloat':
        return raw / float((1 << bits) - 1)
    if op == 'OpRaw' and bits == 32 and isinstance(want, (int, float)) \
            and not isinstance(want, bool):
        # Every raw 32-bit node a description names is a float; their JSON
        # drops the point on a whole number, so the want's type cannot decide.
        return as_f32(raw)
    if op == 'OpPoint':
        if all(isinstance(c, float) for c in raw):
            return {'x': raw[0], 'y': raw[1], 'z': raw[2]}
        return {'x': base[0] + raw[0] * 0.01, 'y': base[1] + raw[1] * 0.01,
                'z': base[2] + raw[2] * 0.01}
    return raw


def close(a, b, tol=1e-3):
    if isinstance(a, bool) or isinstance(b, bool):
        return bool(a) == bool(b)
    if isinstance(a, (int, float)) and isinstance(b, (int, float)):
        return abs(a - b) <= tol * max(1.0, abs(b))
    return a == b


def compare(prefix, want, ours, base, used, bad):
    """Walk their value, find ours under `prefix`, record what disagrees."""
    if isinstance(want, dict):
        if set(want) == {'x', 'y', 'z', 'w'} and prefix + '.wNegative' in ours:
            # They recover w from the other three; the stream carries only its
            # sign, so the sign is the whole of what there is to compare.
            used.add(prefix + '.wNegative')
            if bool(ours[prefix + '.wNegative'][2]) != (want['w'] < 0):
                bad.append((prefix + '.w sign', ours[prefix + '.wNegative'][2],
                            want['w']))
            for k in 'xyz':
                compare('%s.%s' % (prefix, k), want[k], ours, base, used, bad)
            return True
        if set(want) == {'x', 'y', 'z'} and prefix + '.phi' in ours:
            used.add(prefix + '.phi')
            used.add(prefix + '.theta')
            got = normal_vector(ours[prefix + '.phi'][2],
                                ours[prefix + '.theta'][2],
                                ours[prefix + '.theta'][1])
            for k in 'xyz':
                if not close(got[k], want[k]):
                    bad.append((prefix + '.' + k, got[k], want[k]))
            return True
        if prefix in ours and ours[prefix][0] == 'OpPoint':
            used.add(prefix)
            got = scalar(ours[prefix], want, base)
            for k in 'xyz':
                if not close(got[k], want[k]):
                    bad.append((prefix + '.' + k, got[k], want[k]))
            return True
        found = False
        for k, v in want.items():
            if k == 'elements':         # a matrix they expand from the parts
                continue
            found = compare('%s.%s' % (prefix, k), v, ours, base, used,
                            bad) or found
        return found
    if isinstance(want, list):
        if not want:
            return True                 # nothing read is nothing to compare
        if all(isinstance(e, dict) and 'index' in e for e in want):
            found = False
            for e in want:
                for k, v in e.items():
                    if k == 'index':
                        continue
                    found = compare('%s[%d].%s' % (prefix, e['index'], k), v,
                                    ours, base, used, bad) or found
            return found
        got = ours.get(prefix)          # a counted run: one node, many values
        if got is None:
            return False
        used.add(prefix)
        mine = got[2] if isinstance(got[2], list) else [got[2]]
        if mine != want:
            bad.append((prefix, mine[:4], want[:4]))
        return True
    leaf = ours.get(prefix)
    if leaf is None:
        return False
    used.add(prefix)
    got = scalar(leaf, want, base)
    if not close(got, want):
        bad.append((prefix, got, want))
    return True


def main(argv):
    path = argv[1]
    t = theirs(path)
    cp = t['compressionPoint'] or {}
    base = (cp.get('x', 0.0), cp.get('y', 0.0), cp.get('z', 0.0))
    for name in describe.DESCRIPTIONS:
        rec.GH_LAYOUTS[name] = describe.layout(name)
    c = rec.read_container(path)

    agree = differ = missing = spare = 0
    rows, per = [], {}
    for g in t['ghosts']:
        cls = (rec.OBJECTS[g['classId']] if g['classId'] < len(rec.OBJECTS)
               else None)
        if cls not in describe.DESCRIPTIONS:
            continue
        ours, width = leaves(c['block'], g['start'], cls)
        if width != g['width']:
            rows.append((cls, g['index'], 'WIDTH', width, g['width']))
            differ += 1
            continue
        for k, v in describe.DEFAULTS.get(cls, {}).items():
            if k in g['data'] and k not in ours:
                ours[k] = ('OpInt', 0, v)       # its flag was clear: this is it
        for alias, real in describe.ALIASES.get(cls, {}).items():
            if real in ours:
                ours.setdefault(alias, ours[real])

        used, bad, lost = set(), [], 0
        for k, v in g['data'].items():
            if k.startswith('_'):
                continue
            if not compare(k, v, ours, base, used, bad):
                missing += 1
                lost += 1
                rows.append((cls, g['index'], k, '(we do not name it)', v))
        extra = [k for k in ours if k not in used
                 and k not in describe.ALIASES.get(cls, {})]
        spare += len(extra)
        for k in sorted(extra)[:4]:
            rows.append((cls, g['index'], k, ours[k][2],
                         '(they do not report it)'))
        differ += len(bad)
        agree += len(used) - len(bad)
        for k, got, want in bad[:6]:
            rows.append((cls, g['index'], k, got, want))
        e = per.setdefault(cls, [0, 0, 0, 0])
        e[0] += len(used) - len(bad)
        e[1] += len(bad)
        # Only a field `compare` could not find at all: a container of theirs
        # such as `sounds` never appears in `used` itself -- its leaves do --
        # so counting absent keys would report found fields as missing.
        e[2] += lost
        e[3] += len(extra)

    print('%s -- described classes only' % os.path.basename(path))
    for cls, index, k, got, want in rows[:28]:
        print('  %-17s ghost %-5d %-26s ours %-22s theirs %s'
              % (cls, index, k, str(got)[:22], str(want)[:32]))
    if len(rows) > 28:
        print('  ... and %d more rows' % (len(rows) - 28))
    print('  %-17s %6s %6s %8s %7s' % ('class', 'agree', 'differ', 'we lack',
                                       'spare'))
    for cls in sorted(per):
        print('  %-17s %6d %6d %8d %7d' % (cls, *per[cls]))
    print('  total: %d agree, %d differ, %d they name and we do not, '
          '%d we name and they do not' % (agree, differ, missing, spare))
    return 0 if differ == 0 and missing == 0 and spare == 0 else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
