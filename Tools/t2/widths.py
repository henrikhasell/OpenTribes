"""Measure every ghost body in a recording, one stall at a time.

Phase 2 of `GHOST_DERIVATION_PLAN.md`.  A body's width is not observable on
its own, so this does not search for it blindly: `rec.py` walks the run until a
body it cannot read, the bits after that body propose where it could have
ended (a header has to follow, and that header's body has to land somewhere a
header could be), and the walk carries on with the proposal pinned.  Where the
proposal is not unique it is retail's to arbitrate -- `craft.py` plus a client
per candidate.

    python3 Tools/t2/widths.py <demo.rec>          propose and walk
    python3 Tools/t2/widths.py <demo.rec> --json   the same, as data

What comes out is a width per ghost and, where the run completes, the block
consumed exactly.  That is phase 2's deliverable and it is what lets `rec.py`
finish a start block before any field has been attributed.
"""
import json, os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rec


def reach(block, widths):
    """How far the run gets with these widths pinned: (completed, ghosts)."""
    try:
        out, st = rec.read_start_block(block, widths=widths)
        return True, len(out['ghostRecords'])
    except rec.GhostFault as e:
        return False, len(e.out['ghostRecords'])
    except Exception:
        return False, -1


def choose(block, widths, ordinal, candidates, ours, seen_widths=()):
    """Which candidate width the rest of the block agrees with.

    A width is not checkable where it stands -- but a wrong one derails every
    ghost behind it, and the right one lets the run reach the end of the block.
    So each candidate is tried and scored by how far the walk gets, which is
    the whole-block constraint doing the arbitrating instead of a heuristic
    about what a header looks like.  Our own reading is tried first because it
    is right far more often than not; it wins only by the same score as any
    other.
    """
    tried = []
    # Our own reading first and unconditionally, then widths this class has
    # taken elsewhere in the same block, then what the bits after the body
    # suggest.  The proposer only has to supply a superset: it is the reach
    # that decides, and leaving `ours` out because a header test disliked it
    # is how the one right answer gets excluded from the search.
    order = [ours] + [c for c in seen_widths if c != ours] + \
            [c for c in candidates if c != ours and c not in seen_widths]
    for cand in order[:32]:
        pinned = dict(widths)
        pinned[ordinal] = cand
        done, got = reach(block, pinned)
        tried.append((done, got, cand))
        if done:
            return cand, tried                  # it reaches the end: decisive
    best = max(tried, key=lambda t: t[1]) if tried else None
    if not best or best[1] < 0:
        return None, tried
    top = [t for t in tried if t[1] == best[1]]
    return (top[0][2], tried) if len(top) == 1 else (None, tried)


def solve(block, widths=None, rounds=64):
    """Walk, propose, arbitrate, pin, repeat."""
    widths = dict(widths or {})
    log = []
    for _ in range(rounds):
        try:
            out, st = rec.read_start_block(block, widths=widths)
            return True, out, widths, log
        except rec.GhostFault as e:
            out, st = e.out, e.st
            ordinal = e.ordinal
            body = st.p - e.ours
            hits, how = rec.propose_widths(st, body, set(out['ghosts']),
                                           next_index=e.index + 1)
            row = {'ordinal': ordinal, 'index': e.index, 'class': e.name,
                   'ours': e.ours, 'candidates': hits[:8], 'how': how}
            log.append(row)
            if len(hits) == 1:
                width = hits[0]
            else:
                seen = sorted({w for _, _, w, _, n in out['ghostRecords']
                               if n == e.name})
                width, tried = choose(block, widths, ordinal, hits, e.ours,
                                      seen)
                row['how'] = '%s, %d tried' % (how, len(tried))
            if width is None:
                row['unresolved'] = True
                return False, out, widths, log
            row['width'] = width
            widths[ordinal] = width
    return False, out, widths, log


def main(argv):
    path = argv[1]
    c = rec.read_container(path)
    done, out, widths, log = solve(c['block'])

    print('%s' % os.path.basename(path))
    for row in log:
        print('  ghost %-3d index %-4d %-22s ours %5d  ->  %s   (%s)'
              % (row['ordinal'], row['index'], row['class'], row['ours'],
                 row.get('width', 'UNRESOLVED %s' % (row['candidates'] or
                                                     'nothing plausible')),
                 row['how']))

    seen = {}
    for _, _, width, _, name in out['ghostRecords']:
        seen.setdefault(name, {}).setdefault(width, 0)
        seen[name][width] += 1
    print('  %d ghosts read' % len(out['ghostRecords']))
    for name in sorted(seen):
        print('    %-22s %s' % (name, ', '.join(
            '%d bits x%d' % (w, n) for w, n in sorted(seen[name].items()))))
    if done:
        over = len(c['block']) * 8 - out['end']
        print('  mission %r, consumed %d of %d -- %d over'
              % (out['missionName'], out['end'], len(c['block']) * 8, over))
    else:
        print('  not solved: the last row above needs retail')

    if '--json' in argv:
        print(json.dumps({'widths': widths, 'log': log, 'solved': done,
                          'classes': {k: sorted(v) for k, v in seen.items()}},
                         indent=1))
    return 0 if done else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
