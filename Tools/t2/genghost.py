"""Write the ghost layout header from the walk's output plus the descriptions.

Phase 4 of `GHOST_DERIVATION_PLAN.md`.  The shipped header is the emulator
walk's output; a described class replaces its entry, and this composes the two
so the result is still a file something generates.  The walk's own arrays are
copied through untouched -- its output is not reproducible from this tree, so
regenerating it from nothing would mean throwing away 52 classes to fix 2.

    python3 Tools/t2/genghost.py            what would change
    python3 Tools/t2/genghost.py --write     change it

Provenance stays in the comment above each array, so which classes came from
the walk and which from a description is readable in the header itself.

Verification is `Tools/t2/rec.py`: it reads these same tables, so regenerating
and re-reading a recording end to end checks the file the engine compiles
rather than a copy of the description.
"""
import os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import describe

HEADER = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                      '..', '..', 'Engine', 'source', 'T3D', 'gameBase',
                      't2GhostTablesRecovered.h')


def nodes_text(name, nodes):
    rows = '\n'.join('      { %s, %d, %d, %d },' % n for n in nodes)
    return ('   /// %s: %d nodes described in Tools/t2/describe.py, checked\n'
            '   /// against every body of the class in Classic/recordings\n'
            '   static const T2DataNode sT2RcNodes_%s[] =\n   {\n%s\n   };'
            % (name, len(nodes), name, rows))


def replace(src, name, nodes, root):
    """Swap one class's array, its select table and its row."""
    # The select table a walked layout needed is dropped with it: a
    # description branches on flags it names, so there is nothing to select
    # through, and leaving the array behind would be an unused static.
    src = re.sub(r'[ ]*static const S16 sT2RcSel_%s\[\][^\n]*\n' % name, '',
                 src)
    pat = (r'(?:[ ]*///[^\n]*\n)*'
           r'[ ]*static const T2DataNode sT2RcNodes_%s\[\]\s*=\s*\{.*?\n   \};'
           % name)
    new, n = re.subn(pat, lambda m: nodes_text(name, nodes), src, count=1,
                     flags=re.S)
    if not n:
        raise SystemExit('no array for %s in the header' % name)
    row = (r'\{ "%s", sT2RcNodes_%s, \d+, (?:sT2RcSel_%s|NULL), -?\d+ \}'
           % (name, name, name))
    new, n = re.subn(row, '{ "%s", sT2RcNodes_%s, %d, NULL, %d }'
                     % (name, name, len(nodes), root), new, count=1)
    if not n:
        raise SystemExit('no table row for %s in the header' % name)
    return new


def main(argv):
    src = open(HEADER, encoding='latin1').read()
    out = src
    for name in sorted(describe.DESCRIPTIONS):
        nodes, _, root = describe.layout(name)
        out = replace(out, name, nodes, root)
        print('%-22s %3d nodes, root %d' % (name, len(nodes), root))
    if '--write' in argv:
        open(HEADER, 'w', encoding='latin1').write(out)
        print('wrote %s' % os.path.relpath(HEADER))
    else:
        print('%d bytes -> %d; pass --write to apply' % (len(src), len(out)))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
