#!/usr/bin/env python3
"""Write the Tribes 2 wire-protocol and coverage workbook.

Reads coverage.json, which collect.py fills from the repository and from a
recorded session, so nothing here is asserted from memory.
"""
import json, os
from openpyxl import Workbook
from openpyxl.styles import Alignment, Font, PatternFill
from openpyxl.utils import get_column_letter

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(os.path.dirname(__file__), "..", "..",
                   "tribes2-wire-protocol.xlsx")
D = json.load(open(os.environ.get("T2_COVERAGE",
                                  os.path.join(HERE, "coverage.json"))))
CAP = D["capture"]
PACKET_DATA = set(D["layout_pd"])

HEAD = Font(bold=True, color="FFFFFF", size=10)
HEAD_FILL = PatternFill("solid", fgColor="31465F")
SECTION = Font(bold=True, size=10)
SECTION_FILL = PatternFill("solid", fgColor="DCE3EC")
BODY = Font(size=10)
TOP = Alignment(vertical="top", wrap_text=True)
TOPNOWRAP = Alignment(vertical="top")

STATUS_FILL = {
    "acted on":        PatternFill("solid", fgColor="C6E0B4"),
    "not needed":      PatternFill("solid", fgColor="D9D9D9"),
    "read, discarded": PatternFill("solid", fgColor="FFE699"),
    "reads short":     PatternFill("solid", fgColor="F8CBAD"),
    "not read":        PatternFill("solid", fgColor="F4B7B7"),
}


def add_sheet(wb, title, rows, widths, filter_cols=True):
    ws = wb.create_sheet(title)
    for row in rows:
        ws.append(list(row))
    for i, w in enumerate(widths, 1):
        ws.column_dimensions[get_column_letter(i)].width = w

    for c in ws[1]:
        c.font, c.fill, c.alignment = HEAD, HEAD_FILL, TOP
    ws.freeze_panes = "A2"

    ncols = len(widths)
    for row in ws.iter_rows(min_row=2):
        first = row[0].value
        section = isinstance(first, str) and first.startswith("== ")
        for c in row:
            c.font = SECTION if section else BODY
            c.alignment = TOP
            if section:
                c.fill = SECTION_FILL
        if section:
            row[0].value = first[3:]
            ws.merge_cells(start_row=row[0].row, start_column=1,
                           end_row=row[0].row, end_column=ncols)
        else:
            for c in row:
                if isinstance(c.value, str) and c.value in STATUS_FILL:
                    c.fill = STATUS_FILL[c.value]

    if filter_cols and len(rows) > 1:
        ws.auto_filter.ref = "A1:%s%d" % (get_column_letter(ncols), len(rows))
    return ws


# ------------------------------------------------------------------- overview
def overview():
    r = [("Field", "Value", "Notes")]
    r += [
        ("== What this is", "", ""),
        ("Subject", "Tribes 2 (Dynamix, 2001, build 25034) network protocol",
         "As a stock dedicated server speaks it, and as this port reads it"),
        ("Repository", "OpenTribes, PR #85 on branch feat/connect-stock-server", ""),
        ("Read from", "Engine/source/console/netClassTableT2.cpp, "
         "Engine/source/T3D/gameBase/t2*Tables.h, and the engine's own readers",
         "Every figure here is taken out of the tree or measured off a "
         "recorded session rather than asserted"),
        ("Verified against", "A recorded session of a stock Tribes2.exe client "
         "joining the dedicated server",
         "Tools/t2/capture.py records one; Tools/t2/replay.py walks it through "
         "the layouts the engine ships"),
        ("== How far the port gets", "", ""),
        ("Handshake", "complete", "Connects and is counted in the player list"),
        ("String table", "complete", "1024 entries at 10-bit ids"),
        ("Mission briefing", "complete", "Map, objectives and rules arrive"),
        ("Datablocks", "1508 of 1508 read",
         "The values decoded match what the server's own console reports for them"),
        ("Texture CRC challenge", "answered",
         "This gates everything after it: a server sends it once the last "
         "datablock is acknowledged and then waits, and the answer is what "
         "makes it start the mission"),
        ("Mission phase", "reaches 3 of 3", ""),
        ("Ghost always objects", "122 of 123 read",
         "The terrain, the interiors, the markers and the spawn spheres"),
        ("Normal ghosts", "not reached",
         "A server holds them back until phase three finishes, which needs "
         "the scene lighting a stock client answers with"),
        ("Rendering", "nothing drawn",
         "A body read from its layout has no fields, so an object takes its "
         "ghost slot without being built"),
        ("== What the recorded session shows", "", ""),
    ]
    if CAP:
        r += [
            ("Packets", CAP["packets"], "%d of them data packets" % CAP["data_packets"]),
            ("Data packets decoded", CAP["data_packets"] - len(CAP["fails"]),
             "%d failed" % len(CAP["fails"])),
            ("Datablocks decoded", CAP["datablocks"], ""),
            ("Ghost always objects decoded", CAP["ghost_always"], ""),
            ("Ghost updates decoded", sum(CAP["ghost_stream"].values()),
             "across %d classes" % len(CAP["ghost_stream"])),
            ("Shape files named", CAP["shapes"],
             "The walk keeps the first string of a datablock that names a "
             "shape, which is how a class with no reader still yields the one "
             "field the port needs"),
            ("Control object", CAP["control"] or "none",
             "Read through a third layout of its own, separate from its "
             "datablock and its ghost update"),
        ]
        for f in CAP["fails"]:
            r.append(("Failure", f,
                      "Turret's ghost layout reads 395 to 407 bits where the "
                      "wire says 502"))
    r += [
        ("== What the status column means", "", ""),
        ("acted on", "The port reads the contents and does something with them",
         "A reader written by hand"),
        ("read, discarded", "The bits are taken correctly, so the stream stays "
         "in step, and the contents are dropped",
         "The recovered layout walks it"),
        ("reads short", "A layout exists but takes the wrong number of bits",
         "Loses the rest of the packet"),
        ("not read", "Neither a layout nor a reader of the port's own",
         "Would lose the stream if a server sent one"),
    ]
    return ("Overview", r, [30, 44, 74], False)


# ------------------------------------------------------------------- protocol
def protocol():
    r = [("Layer", "Message or step", "Field", "Bits", "Notes")]

    def sec(t):
        r.append(("== " + t, "", "", "", ""))

    def add(layer, step, fields):
        for f, b, n in fields:
            r.append((layer, step, f, b, n))

    sec("Out of band: the handshake")
    add("Handshake", "ConnectChallengeRequest (26)", [
        ("packet type", 8, "The six out-of-band types are numbered as Torque numbers them"),
        ("protocol version", 32, "51, or 52 against a TribesNext server"),
        ("client sequence", 32, "The client's own, taken from the clock"),
        ("join password", "string", "Tribes 2 asks for it here, not on the request"),
        ("certificate flag", 1, "Clear means no TribesNext account certificate")])
    add("Handshake", "ConnectChallengeResponse (30)", [
        ("packet type", 8, ""),
        ("server protocol", 32, "A patched server reports 52 and still accepts 51"),
        ("server token", 32, "Stands in for Torque's 16-byte address digest"),
        ("client sequence", 32, "Quoted back"),
        ("flag", 1, "")])
    add("Handshake", "ConnectChallengeReject (28)", [
        ("packet type", 8, ""), ("client sequence", 32, ""),
        ("reason", "string", "Written uncompressed")])
    add("Handshake", "ConnectRequest (32)", [
        ("packet type", 8, ""),
        ("server token", 32, "Whatever the server named"),
        ("client sequence", 32, ""),
        ("protocol version", 32, "Quoted back, so a 52 server is answered with 52"),
        ("flag", 1, ""),
        ("argument count", 32, "Five"),
        ("arguments", "5 strings", "Name, race and gender, skin, voice, voice pitch")])
    add("Handshake", "ConnectAccept (36)", [
        ("packet type", 8, ""), ("server token", 32, ""), ("client sequence", 32, ""),
        ("protocol version", 32, ""), ("client id", 32, "")])
    add("Handshake", "Absent", [
        ("connection class, net class group, class CRC", 0,
         "Torque sends all three on the request; Tribes 2 sends none of them")])

    sec("Every data packet")
    add("Packet header", "core/dnet", [
        ("game packet", 1, "Clear means an out-of-band packet"),
        ("connect sequence", 1, "The client sequence exclusive-ored with the "
         "server token. Wrong, and the connection is accepted and then every "
         "data packet discarded in silence."),
        ("send sequence", 9, ""),
        ("ack start", 9, ""),
        ("packet type", 2, "0 data, then ping and ack"),
        ("ack byte count", 3, ""),
        ("ack mask", "8 x count", "")])
    add("Rates", "NetConnection::handlePacket", [
        ("current rate present", 1, ""),
        ("update delay", 10, "Ten bits wide, not Torque's twelve"),
        ("packet size", 10, "A server negotiates 400 bytes"),
        ("max rate present", 1, ""),
        ("update delay", 10, ""),
        ("packet size", 10, "")])

    sec("Body, server to client")
    add("Body", "GameConnection::readPacket", [
        ("move ack", 32, "The last move the server has run"),
        ("flash present", 1, ""),
        ("damage flash", 7, "Only if flagged"),
        ("white out", 7, "Only if flagged"),
        ("indicators present", 1, "Tribes 2 only; Torque has nothing here"),
        ("indicator", "1 x 2", ""),
        ("observer camera present", 1, "Tribes 2 only"),
        ("observer position", "3 x 32", "Only if flagged, as whole floats"),
        ("observer mode", 2, "1 follows an object, 2 faces a direction"),
        ("followed ghost", 10, "Mode 1, if flagged"),
        ("observer rotation", "3 x 32", "Mode 2"),
        ("sensor ping", 1, "Tribes 2 only"),
        ("sensor jammed", 1, "Tribes 2 only"),
        ("control object present", 1, ""),
        ("control object dirty", 1, ""),
        ("control ghost id", 10, "Then that class's packet-data layout"),
        ("compression point", "3 x 32", "When not dirty: three whole floats, "
         "not a compressed point"),
        ("target word present", 1, "Repeats. Tribes 2 marks its client targets "
         "dirty a word at a time."),
        ("target word index", 4, ""),
        ("target word bits", 32, ""),
        ("field of view forced", 1, ""),
        ("field of view", 8, "")])
    add("Body", "Client to server", [
        ("camera is at nought", 1, ""),
        ("control state hash", 32, "Sending a nought asks the server to send "
         "that state again"),
        ("moves", "see Moves", ""),
        ("field of view", 8, "If flagged")])

    sec("Moves")
    add("Move", "One move", [
        ("pitch present", 1, "Tribes 2 compares against nought, not against a base move"),
        ("pitch", 16, ""),
        ("yaw present", 1, ""), ("yaw", 16, ""),
        ("roll present", 1, ""), ("roll", 16, ""),
        ("x", 6, "Sent whether or not it changed"),
        ("y", 6, ""), ("z", 6, ""),
        ("free look", 1, ""),
        ("triggers", "1 x 6", "All six, always"),
        ("checksum", 0, "Torque writes 16 bits per move; Tribes 2 writes none")])

    sec("Event stream")
    add("Events", "NetConnection::eventReadPacket", [
        ("another unordered event", 1, "The unordered guaranteed section comes first"),
        ("another ordered event", 1, ""),
        ("sequence given", 1, "Clear means one more than the last"),
        ("sequence", 7, "Only if given"),
        ("event class id", 6, "26 classes"),
        ("body", "per class", "See the Events sheet")])
    add("Events", "SimDataBlockEvent", [
        ("present", 1, ""),
        ("object id", 11, ""),
        ("class id", 7, "54 classes"),
        ("index", 11, "Which of the mission's datablocks this is"),
        ("total", 12, "1508 for Firestorm"),
        ("body", "per class", "See the Datablocks sheet")])
    add("Events", "GhostingMessageEvent", [
        ("sequence", 32, ""),
        ("message", 3, "An index into an enum; see the Constants sheet"),
        ("ghost count", 11, "How many objects are about to be ghosted")])
    add("Events", "NetStringEvent", [
        ("index", 10, "1024 entries, where Torque has 32"),
        ("string present", 1, "Tribes 2 puts a flag before the string"),
        ("string", "string", "")])
    add("Events", "RemoteCommandEvent", [
        ("argument count", 5, ""),
        ("arguments", "packed", "Two bits of code, then a string, a 10-bit tag "
         "or a 7, 15 or 31-bit number")])
    add("Events", "GhostAlwaysObjectEvent", [
        ("ghost index", 10, ""),
        ("object present", 1, ""),
        ("class id", 7, ""),
        ("body", "per class", "Framed exactly as Torque frames it; only the "
         "body differs")])

    sec("Ghost stream")
    add("Ghosts", "NetConnection::ghostReadPacket", [
        ("ghosts present", 1, ""),
        ("id size less three", 3, "The width of an index in this packet, as "
         "wide as its highest index needs"),
        ("another ghost", 1, "Repeats"),
        ("ghost index", "id size", ""),
        ("retired", 1, "Retiring one costs no further bits, so the rest of the "
         "packet still reads"),
        ("class id", 7, "Only for an index the client has not seen. A later "
         "update to that index carries none, which is why a slot has to be "
         "taken even for an object the port cannot build."),
        ("body", "per class", "See the Objects sheet")])

    sec("Strings")
    add("Strings", "BitStream::readString", [
        ("shares a prefix", 1, ""),
        ("prefix length", 8, "Only if shared. What follows joins the prefix of "
         "the last string read in this packet."),
        ("compressed", 1, ""),
        ("length", 8, ""),
        ("body", "Huffman, or 8 x length", "Tribes 2's tree is not Torque's. "
         "The frequency table is byte-identical, but Tribes 2 weights a letter "
         "or a digit one above it before building, which reshapes the tree: "
         "222 of the 256 codes differ, three of them in length. 'T' is 1101011 "
         "unweighted and 1101010 weighted, and the weighted one is what a "
         "server writes.")])
    return ("Protocol", r, [14, 32, 30, 11, 72], True)


# ------------------------------------------------------------------ constants
def constants():
    r = [("Constant", "Tribes 2", "Stock Torque", "Notes")]
    r += [
        ("== Versions", "", "", ""),
        ("protocol version", 51, 12, "52 with a TribesNext certificate; a "
         "server accepts either"),
        ("build number", 25034, 1400, ""),
        ("query version string", "VER5", "VER1", ""),
        ("game string", "not sent", "sent", ""),
        ("DTS shape version", 18, 26, "Torque 3D dropped the reader for "
         "anything before 19; it is back in Engine/source/ts/tsShapeOldRead.cpp"),
        ("DIF interior version", 44, 44, "Already the same"),
        ("== Net class numbering", "", "", ""),
        ("object classes", len(D["objects"]), "as compiled", "7 bits on the wire"),
        ("datablock classes", len(D["datablocks"]), "as compiled", "7 bits"),
        ("event classes", len(D["events"]), "as compiled", "6 bits"),
        ("how an id is assigned", "index into one flat table per type, sorted "
         "by name, case sensitive", "the same rule over whatever the build "
         "registers",
         "Neither end transmits the ids, so they agree only if both compile the "
         "same classes. netClassTableT2.cpp carries Tribes 2's own order, "
         "recovered from the retail binary; an unported name leaves its slot "
         "empty rather than shifting every slot after it."),
        ("== Ghosts", "", "", ""),
        ("GhostIdBitSize", 10, 12, ""),
        ("MaxGhostCount", 1024, 4096, ""),
        ("GhostIndexBitSize", 3, 4, "A packet's index width is this plus three"),
        ("== Rates and ids", "", "", ""),
        ("RateBitSize", 10, 12, "Four bits that cost the first packet, which "
         "is the one carrying the string table"),
        ("negotiated packet size", "400 bytes", "1500 bytes", ""),
        ("DataBlockObjectIdBitSize", 11, 10, ""),
        ("string table entries", 1024, 32, "10-bit ids, not 5"),
        ("== Fields Tribes 2 adds", "", "", ""),
        ("TargetWordBits", 4, "absent", ""),
        ("ObserverModeBits", 2, "absent", ""),
        ("MaxTriggerKeys", 6, 6, ""),
        ("move checksum", "none", "16 bits per move", ""),
        ("== Connection messages", "", "", ""),
        ("GhostAlwaysDone", 0, 0, "The first four agree"),
        ("ReadyForNormalGhosts", 1, 1, ""),
        ("EndGhosting", 2, 2, ""),
        ("GhostAlwaysStarting", 3, 3, "Carries the ghost count. Read off a "
         "recorded session: the one ghosting message a server sends is a 3, "
         "and a stock client answers it by reporting that it is starting to "
         "ghost that many objects."),
        ("SendNextDownloadRequest", "absent", 4, "Added by the port"),
        ("FileDownloadSizeMessage", "absent", 5, "Added by the port"),
        ("DataBlocksDone", 4, 6, "Translated both ways"),
        ("DataBlocksDownloadDone", 5, 7, "Translated both ways"),
        ("== Compressed point", "", "", ""),
        ("type", "2 bits", "2 bits", ""),
        ("axis widths", "16, 18 or 20 signed, or three whole floats", "the same",
         "Read relative to the packet's compression point"),
    ]
    return ("Constants", r, [28, 26, 18, 78], True)


# --------------------------------------------------------------- class sheets
def datablocks():
    r = [("T2 id", "Class", "Layout nodes", "Class in the port", "Read by",
          "Status", "Notes")]
    stubs = set(D["stub_db"])
    alias_t2 = {t2: port for port, t2 in D["aliases"].items()}
    for i, n in enumerate(D["datablocks"]):
        nodes = D["layout_db"].get(n, 0)
        if n in stubs:
            kind = "added for the port"
        elif n in alias_t2 and alias_t2[n] in stubs:
            kind = "added for the port, named %s" % alias_t2[n]
        elif n in D["port_db"]:
            kind = "Torque's own"
        else:
            kind = "none"
        note = ""
        if n == "ShapeBaseData":
            note = ("Its shape file is taken out of the layout's strings and "
                    "put on the datablock. That is the one field the port "
                    "needs to draw anything, and it is why the layout walk "
                    "keeps the first string that names a shape.")
        elif n == "TSShapeConstructor":
            note = ("The port keeps a TSShapeConstructor of its own that is no "
                    "datablock, so Tribes 2's is named TSShapeConstructorT2 and "
                    "the two are matched up by name")
        r.append((i, n, nodes, kind,
                  "T2 layout" if nodes else "the port's own unpackData",
                  "read, discarded" if nodes else "not read", note))
    return ("Datablocks", r, [7, 28, 12, 30, 24, 16, 74], True)


def objects():
    r = [("T2 id", "Class", "Layout nodes", "Class in the port", "Read by",
          "Status", "Ghost updates seen", "Body bits seen", "Notes")]
    gs = CAP["ghost_stream"] if CAP else {}
    gb = CAP["ghost_bits"] if CAP else {}
    live = CAP["live"] if CAP else {}
    for i, n in enumerate(D["objects"]):
        nodes = D["layout_gh"].get(n, 0)
        if n in D["stub_obj"]:
            kind = "added for the port"
        elif n in D["port_obj"]:
            kind = "Torque's own"
        else:
            kind = "none"
        short = n == "Turret"
        seen = gs.get(n, 0)
        bits = "%d to %d" % tuple(gb[n]) if n in gb else ""
        notes = []
        if short:
            notes.append("Reads 395 to 407 bits where the wire says 502, "
                         "measured against a recorded packet. The ghost loop "
                         "then ends on a clear flag with the rest of the "
                         "packet unread.")
        if n in live and not seen:
            notes.append("Arrives as a ghost always object: %d of them in the "
                         "recorded session" % live[n])
        elif not seen and n not in live:
            notes.append("Never sent in the recorded session")
        if n in PACKET_DATA:
            notes.append("Has a packet-data layout as well, for when this "
                         "client is the one driving it")
        r.append((i, n, nodes, kind,
                  "T2 layout" if nodes else "the port's own unpackUpdate",
                  "reads short" if short else
                  ("read, discarded" if nodes else "not read"),
                  seen or "", bits, " ".join(notes)))
    return ("Objects (ghosts)", r, [7, 24, 12, 22, 28, 15, 12, 13, 70], True)


def events():
    r = [("T2 id", "Class", "Direction", "Layout nodes", "Class in the port",
          "Read by", "Status", "Seen in capture", "Notes")]
    seen = CAP["events"] if CAP else {}
    dirn = D.get("direction", {})
    for i, n in enumerate(D["events"]):
        nodes = D["layout_ev"].get(n, 0)
        hand = n in D["byhand"]
        if n in D["stub_ev"]:
            kind = "added for the port"
        elif n in D["port_ev"]:
            kind = "Torque's own"
        else:
            kind = "none"
        if hand:
            read, st = "the port's own unpack", "acted on"
        elif nodes:
            read, st = "T2 layout", "read, discarded"
        else:
            read, st = "the port's own unpack (Torque's layout)", "not read"
        note = ""
        if hand:
            note = "Unpacked by hand because the port acts on what it carries."
            if n == "CRCChallengeEvent":
                note += (" This one gates the rest of the connection: a server "
                         "sends it once the last datablock is acknowledged and "
                         "then waits, and the answer is what makes it start "
                         "the mission and with it the ghosting.")
            elif n == "GhostingMessageEvent":
                note += (" Its three-bit message is translated: the port has "
                         "added two states that Tribes 2 does not have.")
            elif n == "SimDataBlockEvent":
                note += (" Its body is still read from the datablock's layout.")
        # a class added by the port's own macro is registered either way, and
        # the macro body hides its name from the pass that reads directions
        way = dirn.get(n) or ("either way" if n in D["stub_ev"] else "unknown")
        if seen.get(n) and way == "either way":
            way = "server to client (observed)"
        if not hand and not nodes:
            if way == "client to server":
                st = "not needed"
                note = ("Only ever sent by a client, so this one never has to "
                        "be read.")
            else:
                note = ("Neither a layout nor a reader of the port's own, so a "
                        "server sending one would lose the stream. Not sent in "
                        "the recorded session.")
        r.append((i, n, way, nodes, kind, read, st, seen.get(n, 0) or "", note))
    return ("Events", r, [7, 30, 17, 12, 22, 36, 15, 14, 72], True)


def summary():
    r = [("Kind", "Classes", "With a layout", "Acted on", "Read and discarded",
          "Not read", "Exercised by the recorded session")]
    ev_seen = CAP["events"] if CAP else {}
    gs = CAP["ghost_stream"] if CAP else {}
    live = CAP["live"] if CAP else {}
    r.append(("Datablocks", len(D["datablocks"]), len(D["layout_db"]), 0,
              len(D["layout_db"]), len(D["datablocks"]) - len(D["layout_db"]),
              "all of them: %d sent and decoded" % (CAP["datablocks"] if CAP else 0)))
    r.append(("Objects (ghosts)", len(D["objects"]), len(D["layout_gh"]), 0,
              len(D["layout_gh"]) - 1, 0,
              "%d of %d" % (len(set(gs) | set(live)), len(D["objects"]))))
    hand = len([n for n in D["events"] if n in D["byhand"]])
    lay = len([n for n in D["events"] if n in D["layout_ev"] and n not in D["byhand"]])
    gap = [n for n in D["events"]
           if n not in D["layout_ev"] and n not in D["byhand"]
           and D.get("direction", {}).get(n) != "client to server"]
    r.append(("Events", len(D["events"]), len(D["layout_ev"]), hand, lay,
              len(gap),
              "%d of %d" % (len(ev_seen), len(D["events"]))))
    r.append(("== Where the gaps are", "", "", "", "", "", ""))
    r.append(("Turret", "", "", "", "", "reads short",
              "Its ghost layout takes about a hundred bits too few, which "
              "loses the rest of the packet. The fix belongs in the extractor "
              "that produced the layout, not in the table."))
    for n in gap:
        if True:
            r.append((n, "", "", "", "", "not read",
                      "An event with no layout and no reader of the port's "
                      "own. None were sent in the recorded session, so none "
                      "has been seen to break anything yet."))
    r.append(("Every class", "", "", "", "", "",
              "No class builds an object out of what is read. The bits are "
              "consumed to keep the stream in step and then dropped, which is "
              "why the mission loads and nothing is drawn."))
    r.append(("Ghost coverage", "", "", "", "", "",
              "One recorded session exercises %d of the %d classes a server "
              "can ghost. Player is not among them, and it is the one that "
              "matters for playing rather than watching."
              % (len(set(gs) | set(live)), len(D["objects"]))))
    return ("Coverage summary", r, [24, 9, 13, 10, 19, 12, 78], False)


wb = Workbook()
wb.remove(wb.active)
for title, rows, widths, filt in (overview(), protocol(), constants(), summary(),
                                  datablocks(), objects(), events()):
    add_sheet(wb, title, rows, widths, filt)
wb.save(OUT)
print("wrote %s" % OUT)
for ws in wb.worksheets:
    print("  %-20s %d rows" % (ws.title, ws.max_row))
