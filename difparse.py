# difparse.py [--ours] files...: walk .dif files the way retail Tribes2.exe reads them
# (InteriorResource::read 0x526990, Interior::read 0x513140, readVehicleCollision 0x515330)
# or, with --ours, the way our unpatched reader does; print structure counts and whether
# the walk ends exactly at EOF.
import struct, sys

class R:
    def __init__(s, b): s.b = b; s.p = 0
    def u8(s): v = s.b[s.p]; s.p += 1; return v
    def u16(s): v = struct.unpack_from('<H', s.b, s.p)[0]; s.p += 2; return v
    def u32(s): v = struct.unpack_from('<I', s.b, s.p)[0]; s.p += 4; return v
    def f32(s): v = struct.unpack_from('<f', s.b, s.p)[0]; s.p += 4; return v
    def skip(s, n):
        if s.p + n > len(s.b): raise EOFError('skip past end at %d (+%d)' % (s.p, n))
        s.p += n
    def string(s): n = s.u8(); v = s.b[s.p:s.p + n]; s.p += n; return v
    def png(s):
        assert s.b[s.p:s.p + 8] == b'\x89PNG\r\n\x1a\n', 'bad png at %d' % s.p
        s.p += 8
        while True:
            n = struct.unpack_from('>I', s.b, s.p)[0]; t = s.b[s.p + 4:s.p + 8]; s.p += 12 + n
            if t == b'IEND': return
    def vec(s, size, cnt=None):
        n = s.u32() if cnt is None else cnt
        if n > 10000000: raise ValueError('absurd count %d at %d' % (n, s.p - 4))
        s.skip(n * size); return n

def read_interior(r, ours, c):
    ver = r.u32()
    if ver != 0: raise ValueError('interior version %d' % ver)       # 0x513163
    r.skip(4 + 4 + 24 + 16)                                           # detail, minPixels, box, sphere
    r.u8(); r.u32()                                                   # alarm, numLightStateEntries
    r.vec(12); c['planes'] = r.u32(); r.skip(c['planes'] * 6)         # 0x516190
    c['points'] = r.vec(12); r.vec(1); r.vec(32)                      # points, visibility, texgens
    c['bsp'] = r.vec(6); r.vec(6)                                     # BSP nodes (U16 x3), solid leaves
    assert r.u8() == 1, 'material list version'                       # 0x44cdd0
    c['materials'] = r.u32()
    for _ in range(c['materials']): r.string()
    c['windings'] = r.vec(4); r.vec(8)
    c['zones'] = r.vec(12); r.vec(2); r.vec(2); r.vec(12)             # zones, zone surfaces, portal list, portals
    n = r.u32(); c['surfaces'] = n; start = r.p
    def surfaces(tgea):
        for _ in range(n):
            ws = r.u32(); wc = r.u8(); pl = r.u16(); tx = r.u16(); tg = r.u32()
            if tgea and (ws >= c['windings'] or ws + wc > c['windings'] or (pl & 0x7fff) >= c['planes']
                         or tx >= c['materials']): return False
            r.u8(); r.u32()
            fw = r.u16(); r.skip(8)
            if tgea and (fw >> 13) & 7 > 5: return False
            r.skip(2 + 4 + 4)
            if tgea: r.u32()                                          # TGEA's "unused" word
        return True
    tge = True
    if ours:
        # Interior::read's guess: TGEA's layout first, TGE's if any surface is invalid
        try: tge = not surfaces(True)
        except Exception: tge = True
        if tge: r.p = start; surfaces(False)
    else:
        surfaces(False)                                               # 0x513dc0
    c['tgeGuess'] = tge
    r.vec(1); r.vec(1)                                                # normal / alarm lmap indices (U8)
    c['nullSurfaces'] = r.vec(8)
    c['lightmaps'] = r.u32()
    for _ in range(c['lightmaps']):
        r.png()
        if ours and not tge: r.png()                                  # TGEA's light direction map
        r.u8()                                                        # 0x51431f
    r.vec(4); r.vec(16); r.vec(13); r.vec(10)                         # solid leaf surfs, anim lights, light states, state data
    n = r.u32(); r.u32(); r.skip(n)                                   # state data buffer
    r.vec(1)                                                          # name buffer
    c['subObjects'] = r.u32()
    for _ in range(c['subObjects']):
        assert r.u32() == 1, 'subobject key'; r.skip(32)              # 0x527710 / 0x5287c0
    c['hulls'] = r.vec(52)                                            # 4+2+24+4+2+4*4 bytes
    r.vec(1); r.vec(4); r.vec(2); r.vec(4); r.vec(4); r.vec(2); r.vec(4); r.vec(1)
    r.skip(256 * 8); r.vec(2); r.u32()                                # coord bins, indices, mode
    r.skip(8)                                                         # ambient colours
    for _ in range(4):
        if r.u32() != 0: raise ValueError('expansion word not 0')     # 0x515228..0x51528e

def read_vehicle(r, ours, c):
    if r.u32() != 0: raise ValueError('vehicle collision version')    # 0x515353
    c["vHulls"] = r.vec(52)
    r.vec(1); r.vec(4); r.vec(2); r.vec(4); r.vec(4); r.vec(2); r.vec(4); r.vec(1)
    c['vNullSurfaces'] = r.vec(4 + 2 + 1 + (4 if ours else 1))        # 0x515c58: U8 winding count
    c['vPoints'] = r.vec(12); c['vPlanes'] = r.vec(16); r.vec(4); r.vec(8)

def read_resource(b, ours):
    r = R(b); c = {}
    assert r.u32() == 44
    if r.u8(): r.png()
    c['details'] = r.u32(); det = []
    for _ in range(c['details']): d = {}; read_interior(r, ours, d); det.append(d)
    c['detail0'] = det[0] if det else {}
    c['subs'] = r.u32()
    for _ in range(c['subs']): read_interior(r, ours, {})
    c['triggers'] = r.u32()
    for _ in range(c['triggers']):
        r.string()
        if ours: r.string(); [r.string() for _ in range(2 * r.u32())]
        r.vec(12); r.vec(16); r.vec(16); r.skip(12)
    if ours:
        c['children'] = r.u32()
        for _ in range(c['children']):
            r.string(); r.string(); r.u32(); r.skip(12)
            [r.string() for _ in range(2 * r.u32())]
            r.vec(4); r.vec(36); r.u32()
        c['fields'] = r.u32()
        if c['fields']: raise ValueError('force fields %d (not modelled)' % c['fields'])
        c['specNodes'] = r.u32()
        for _ in range(c['specNodes']): r.string(); r.skip(12)
        flag = r.u32()
    else:
        c['paths'] = r.u32()
        for _ in range(c['paths']): r.string(); r.vec(32); r.u32(); r.u8()   # 0x527330
        c['children'] = r.u32()
        for _ in range(c['children']):                                        # 0x527510
            r.string(); r.skip(20); [r.string() for _ in range(r.u32())]
        c['skipped'] = r.u32()                                                # 0x526e1b
        c['specNodes'] = r.u32()
        for _ in range(c['specNodes']): r.string(); r.skip(12)                # 0x527640
        flag = r.u32()                                                        # 0x526ef9
    c['vflag'] = flag
    if flag == 1 and c['details']: read_vehicle(r, ours, c)
    last = r.u32()
    if ours and last == 2:
        n = r.u32()
        for _ in range(n): r.string(); r.string(); r.skip(12); [r.string() for _ in range(2 * r.u32())]
        r.u32()
    c['end'] = r.p; c['size'] = len(b)
    return c

if __name__ == '__main__':
    ours = '--ours' in sys.argv
    for f in [a for a in sys.argv[1:] if a != '--ours']:
        try:
            c = read_resource(open(f, 'rb').read(), ours)
            d = c.pop('detail0')
            ok = 'OK ' if c['end'] == c['size'] else 'END%+d' % (c['size'] - c['end'])
            print(ok, f, ' '.join('%s=%s' % kv for kv in c.items() if kv[0] not in ('end', 'size')),
                  'd0:', ' '.join('%s=%s' % kv for kv in d.items()))
        except Exception as e:
            print('ERR', f, repr(e))
