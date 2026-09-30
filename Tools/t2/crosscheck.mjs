/**
 * Read a capture with t2-demo-parser and report what each ghost body took.
 *
 *     node Tools/t2/crosscheck.mjs <capture> [--json out.json]
 *
 * t2-demo-parser (MIT, Brian Beck, github.com/exogen/t2-demo-parser) decodes
 * the same three tables this port carries, from the same retail binary, by
 * hand rather than by emulating unpackUpdate.  Two readings of one stream is
 * worth far more than one: where they agree the layout is settled, and where
 * they differ exactly one of them is wrong about a field that a recorded
 * session can then be asked about.
 *
 * It wants T2_DEMO_PARSER pointing at a checkout with `npm install` run in it.
 */
import { readFileSync, writeFileSync } from "node:fs";
import { resolve } from "node:path";

const root = process.env.T2_DEMO_PARSER;
if (!root) {
  console.error("set T2_DEMO_PARSER to a t2-demo-parser checkout");
  process.exit(2);
}
const { createLiveParser, passiveObserverProtocolState } = await import(
  resolve(root, "src/LiveParser.ts")
);

const args = process.argv.slice(2);
const capture = args[0];
const jsonAt = args.indexOf("--json");
if (!capture) {
  console.error("usage: crosscheck.mjs <capture> [--json out.json]");
  process.exit(2);
}

const buf = readFileSync(capture);

// A proxy capture holds only what the server sent, so nothing here owns the
// client->server side and the ack window would reject every packet.  The
// parser has a seed for exactly that case; the connect-sequence bit comes out
// of the first packet's header byte.
// The first datagram of a session is the out-of-band handshake, so the
// connect-sequence bit has to come from the first in-band one.
let firstByte = 0;
for (let o = 0; o + 4 <= buf.length; ) {
  const n = buf.readUInt32LE(o);
  o += 4;
  if (n === 0 || o + n > buf.length) break;
  if (buf[o] & 1) { firstByte = buf[o]; break; }
  o += n;
}
const { packetParser, registry, ghostTracker } = createLiveParser({
  connectionProtocolState: passiveObserverProtocolState(firstByte),
});

// Each ghost body's length, per class, plus whatever the parser could not read.
const perClass = new Map();
const faults = [];
let pkt = 0;
let data = 0;
let off = 0;

function note(name, bits) {
  let e = perClass.get(name);
  if (!e) perClass.set(name, (e = { count: 0, lengths: new Map() }));
  e.count++;
  e.lengths.set(bits, (e.lengths.get(bits) ?? 0) + 1);
}

while (off + 4 <= buf.length) {
  const n = buf.readUInt32LE(off);
  off += 4;
  if (n === 0 || off + n > buf.length) break;
  const p = buf.subarray(off, off + n);
  off += n;
  pkt++;
  if ((p[0] & 1) === 0) continue; // out of band
  let r;
  try {
    r = packetParser.parsePacket(new Uint8Array(p));
  } catch (e) {
    faults.push({ pkt, stage: "throw", message: `${e && e.message}` });
    continue;
  }
  data++;
  if (r.parseFault) {
    faults.push({ pkt, ...r.parseFault });
    continue;
  }
  // What the control object sent this tick.  It goes through readPacketData,
  // not unpackUpdate, so it is a separate layout and worth its own comparison.
  const gs = r.gameState;
  if (gs && gs.controlObjectDataStart !== undefined) {
    if (gs.controlObjectError) {
      faults.push({ pkt, stage: "gameState", message: gs.controlObjectError });
    } else if (gs.controlObjectDataEnd !== undefined) {
      const gi = gs.controlObjectGhostIndex;
      const cg = gi === undefined ? null : ghostTracker.getGhost(gi);
      const nm = cg
        ? registry.getGhostParser(cg.classId)?.name ?? `class${cg.classId}`
        : "?";
      note(
        `control:${nm}`,
        gs.controlObjectDataEnd - gs.controlObjectDataStart,
      );
    }
  }
  for (const g of r.ghosts ?? []) {
    if (g.type === "delete") continue;
    const name =
      registry.getGhostParser(g.classId)?.name ??
      (g.classId === undefined ? "?" : `class${g.classId}`);
    note(name, g.updateBitsEnd - g.updateBitsStart);
    if (g.failed) faults.push({ pkt, stage: "ghost", message: `${name}: ${g.error}` });
  }
  // A scope-always object's body rides inside GhostAlwaysObjectEvent rather
  // than the ghost section, and that is the whole of a mission's opening
  // traffic -- so its length has to be counted here or nearly every class
  // comes out unseen.  The event's own header is the ten-bit index, the flag
  // and the seven-bit class id.
  for (const e of r.events ?? []) {
    const d = e.parsedData;
    if (e.failed) {
      faults.push({ pkt, stage: "event", message: `${e.classId}: ${e.error}` });
      continue;
    }
    if (d && d.type === "SimDataBlockEvent" && d.classId !== undefined) {
      // The event's own header is the object id, the class id, the index and
      // the total: 1 + 11 + 7 + 11 + 12 bits before the datablock itself.
      const dbName =
        registry.getDataBlockParser?.(d.classId)?.name ?? `db${d.classId}`;
      note(`datablock:${dbName}`, e.dataBitsEnd - e.dataBitsStart - 42);
      continue;
    }
    if (!d || d.type !== "GhostAlwaysObjectEvent" || !d.hasObjectData) continue;
    const name = registry.getGhostParser(d.classId)?.name ?? `class${d.classId}`;
    note(name, e.dataBitsEnd - e.dataBitsStart - 18);
  }
}

const out = {
  capture,
  packets: pkt,
  dataPackets: data,
  faults,
  classes: Object.fromEntries(
    [...perClass.entries()].sort().map(([k, v]) => [
      k,
      { count: v.count, lengths: Object.fromEntries([...v.lengths].sort((a, b) => a[0] - b[0])) },
    ]),
  ),
};

if (jsonAt >= 0 && args[jsonAt + 1]) {
  writeFileSync(args[jsonAt + 1], JSON.stringify(out, null, 1));
}

console.log(
  `packets ${pkt}, of which ${data} data; ${faults.length} fault(s)` +
    `; protocol rejected ${packetParser.protocolRejected}` +
    `, no dispatch ${packetParser.protocolNoDispatch}`,
);
for (const f of faults.slice(0, 8)) {
  console.log(`   packet ${f.pkt} ${f.stage}: ${f.message}`);
}
for (const [name, v] of Object.entries(out.classes)) {
  const lens = Object.entries(v.lengths)
    .map(([b, c]) => (c > 1 ? `${b}x${c}` : b))
    .join(" ");
  console.log(`   ${name.padEnd(22)} ${String(v.count).padStart(4)}  ${lens}`);
}
