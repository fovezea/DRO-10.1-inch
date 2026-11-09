#!/usr/bin/env python3
"""
dro_link_test.py - bench tool for the ESP32-WROOM DRO backend link.

Speaks exactly the protocol documented in docs/PROTOCOL.md:
COBS-framed packets, CRC-16/CCITT-FALSE, little-endian payloads.

Typical use while bringing a board up:

    # live positions over the J3 UART link (needs a USB-TTL adapter)
    python dro_link_test.py --port COM7 monitor

    # zero every axis
    python dro_link_test.py --port COM7 zero

    # raw diagnostics (heap, frame counters, per-axis rates)
    python dro_link_test.py --port COM7 diag

    # prove the codec on the host without any hardware
    python dro_link_test.py selftest

If pyserial is missing, run it with the ESP-IDF Python instead:
    C:\\Espressif\\tools\\python\\v6.1\\venv\\Scripts\\python.exe tools\\dro_link_test.py ...
"""

from __future__ import annotations

import argparse
import struct
import sys
import time
from dataclasses import dataclass

try:
    import serial
    from serial.tools import list_ports
except ImportError:  # pragma: no cover - helpful message instead of a traceback
    serial = None
    list_ports = None


# ---------------------------------------------------------------------------
# Protocol constants (mirror components/dro_link/include/dro_link_protocol.h)
# ---------------------------------------------------------------------------
PROTOCOL_VERSION = 0x01

MSG_HELLO = 0x01
MSG_TELEMETRY = 0x02
MSG_CONFIG = 0x03
MSG_ACK = 0x04
MSG_ERROR = 0x05
MSG_DIAG = 0x06
MSG_PONG = 0x07
MSG_INDEX_EVENT = 0x08

MSG_HELLO_REQ = 0x10
MSG_SET_STREAM = 0x11
MSG_ZERO = 0x12
MSG_PRESET = 0x13
MSG_SET_AXIS = 0x14
MSG_GET_CONFIG = 0x15
MSG_PING = 0x16
MSG_SAVE_CONFIG = 0x17
MSG_RESET_CONFIG = 0x18
MSG_REBOOT = 0x19
MSG_SET_LINK = 0x1A
MSG_GET_DIAG = 0x1B

FLAG_ACK_REQ = 0x01
FLAG_IS_REPLY = 0x02

ST_UART_SESSION = 1 << 0
ST_ENOW_SESSION = 1 << 1
ST_WIFI_READY = 1 << 2
ST_WIFI_ERROR = 1 << 3
ST_CFG_DIRTY = 1 << 4
ST_SIMULATION = 1 << 5
ST_INDEX_SEEN = 1 << 6
ST_SELFTEST_OK = 1 << 7

AXIS_NAMES = ("X", "Y", "Z", "W", "V")
MAX_PAYLOAD = 192

TYPE_NAMES = {
    MSG_HELLO: "HELLO",
    MSG_TELEMETRY: "TELEMETRY",
    MSG_CONFIG: "CONFIG",
    MSG_ACK: "ACK",
    MSG_ERROR: "ERROR",
    MSG_DIAG: "DIAG",
    MSG_PONG: "PONG",
    MSG_INDEX_EVENT: "INDEX",
    MSG_HELLO_REQ: "HELLO_REQ",
    MSG_SET_STREAM: "SET_STREAM",
}

# Payload layouts.
FMT_TELEMETRY = struct.Struct("<IIHBB5i")          # 32 bytes
FMT_HELLO = struct.Struct("<HBBBB6s16sI")          # 32 bytes
FMT_HELLO_REQ = struct.Struct("<BBH16s")           # 20 bytes
FMT_CONFIG = struct.Struct("<HBBBBBBBBHH I6s6s16s".replace(" ", ""))  # 46 bytes
FMT_ACK = struct.Struct("<BBBB")
FMT_ERROR = struct.Struct("<BBH")
FMT_DIAG_FIXED = struct.Struct("<IIHBB")           # 12 bytes
FMT_INDEX_EVENT = struct.Struct("<B3xiI")          # 12 bytes
FMT_STREAM = struct.Struct("<HBB")


def crc16_ccitt(data: bytes) -> int:
    """CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflection."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def cobs_encode(data: bytes) -> bytes:
    out = bytearray()
    code_idx = 0
    out.append(0)          # reserve
    code = 1
    for byte in data:
        if byte == 0:
            out[code_idx] = code
            code_idx = len(out)
            out.append(0)
            code = 1
        else:
            out.append(byte)
            code += 1
            if code == 0xFF:
                out[code_idx] = code
                code_idx = len(out)
                out.append(0)
                code = 1
    out[code_idx] = code
    return bytes(out)


def cobs_decode(data: bytes) -> bytes:
    out = bytearray()
    read = 0
    length = len(data)
    while read < length:
        code = data[read]
        read += 1
        if code == 0:
            raise ValueError("zero byte inside COBS frame")
        for _ in range(1, code):
            if read >= length:
                raise ValueError("truncated COBS frame")
            out.append(data[read])
            read += 1
        if code < 0xFF and read < length:
            out.append(0)
    return bytes(out)


def encode_frame(msg_type: int, payload: bytes = b"", seq: int = 0, flags: int = 0) -> bytes:
    if len(payload) > MAX_PAYLOAD:
        raise ValueError("payload too large")
    header = struct.pack("<BBBBH", PROTOCOL_VERSION, msg_type, seq & 0xFF, flags, len(payload))
    body = header + payload
    body += struct.pack("<H", crc16_ccitt(body))
    return cobs_encode(body) + b"\x00"


@dataclass
class Frame:
    msg_type: int
    seq: int
    flags: int
    payload: bytes


def decode_body(body: bytes) -> Frame:
    if len(body) < 8:
        raise ValueError("short frame")
    version, msg_type, seq, flags, plen = struct.unpack_from("<BBBBH", body)
    if version != PROTOCOL_VERSION:
        raise ValueError(f"bad version {version}")
    if len(body) != plen + 8:
        raise ValueError("length mismatch")
    crc_rx = struct.unpack_from("<H", body, 6 + plen)[0]
    if crc16_ccitt(body[: 6 + plen]) != crc_rx:
        raise ValueError("crc mismatch")
    return Frame(msg_type, seq, flags, body[6 : 6 + plen])


# ---------------------------------------------------------------------------
# Serial link
# ---------------------------------------------------------------------------
class DroLink:
    def __init__(self, port: str, baud: int = 460800, timeout: float = 0.2):
        if serial is None:
            raise RuntimeError(
                "pyserial is not installed. Use the ESP-IDF Python:\n"
                r"  C:\Espressif\tools\python\v6.1\venv\Scripts\python.exe "
                r"tools\dro_link_test.py --port COMx monitor"
            )
        self.ser = serial.Serial(port, baud, timeout=timeout)
        self.ser.reset_input_buffer()
        self.buf = bytearray()
        self.seq = 0
        self.dropped = 0

    def close(self) -> None:
        try:
            self.ser.close()
        except Exception:
            pass

    def send(self, msg_type: int, payload: bytes = b"", flags: int = 0) -> int:
        seq = self.seq
        self.seq = (self.seq + 1) & 0xFF
        self.ser.write(encode_frame(msg_type, payload, seq, flags))
        return seq

    def read_frame(self) -> Frame | None:
        """Return the next valid frame, or None on timeout."""
        deadline = time.monotonic() + self.ser.timeout
        while True:
            idx = self.buf.find(b"\x00")
            if idx >= 0:
                raw = bytes(self.buf[:idx])
                del self.buf[: idx + 1]
                if not raw:
                    continue
                try:
                    return decode_body(cobs_decode(raw))
                except ValueError:
                    self.dropped += 1
                    continue

            if time.monotonic() > deadline:
                return None

            chunk = self.ser.read(256)
            if chunk:
                self.buf.extend(chunk)
                if len(self.buf) > 8192:      # noise without delimiters
                    del self.buf[:-256]

    def wait_for(self, msg_type: int, timeout: float = 1.0) -> Frame | None:
        deadline = time.monotonic() + timeout
        saved_timeout = self.ser.timeout
        self.ser.timeout = min(0.2, timeout)
        try:
            while time.monotonic() < deadline:
                frame = self.read_frame()
                if frame is not None and frame.msg_type == msg_type:
                    return frame
        finally:
            self.ser.timeout = saved_timeout
        return None

    def hello(self, hz: int = 50, axes_mask: int = 0x1F, name: str = "bench-tool") -> Frame | None:
        payload = FMT_HELLO_REQ.pack(1, axes_mask, hz, name.encode()[:16].ljust(16, b"\x00"))
        self.send(MSG_HELLO_REQ, payload)
        return self.wait_for(MSG_HELLO, 2.0)

    def request(self, msg_type: int, payload: bytes, reply: int, timeout: float = 1.5) -> Frame | None:
        self.send(msg_type, payload, FLAG_ACK_REQ)
        return self.wait_for(reply, timeout)


# ---------------------------------------------------------------------------
# Decoders / printers
# ---------------------------------------------------------------------------
def describe_status(status: int) -> str:
    bits = []
    if status & ST_UART_SESSION:
        bits.append("uart")
    if status & ST_ENOW_SESSION:
        bits.append("espnow")
    if status & ST_WIFI_READY:
        bits.append("wifi")
    if status & ST_WIFI_ERROR:
        bits.append("wifi-ERR")
    if status & ST_CFG_DIRTY:
        bits.append("cfg-dirty")
    if status & ST_SIMULATION:
        bits.append("SIM")
    if status & ST_INDEX_SEEN:
        bits.append("index")
    if status & ST_SELFTEST_OK:
        bits.append("selftest")
    return ",".join(bits) if bits else "-"


def print_hello(frame: Frame) -> None:
    fw, proto, axes, features, _res, mac, name, uptime = FMT_HELLO.unpack(frame.payload)
    mac_s = ":".join(f"{b:02x}" for b in mac)
    print(f"backend   : {name.decode(errors='replace')}  fw {fw >> 8}.{fw & 0xFF}  "
          f"protocol {proto}  axes {axes}")
    print(f"mac       : {mac_s}   uptime {uptime / 1000:.1f} s")
    feats = []
    for bit, label in ((1 << 0, "quad"), (1 << 1, "index"), (1 << 2, "espnow"),
                       (1 << 3, "uart"), (1 << 4, "sim")):
        if features & bit:
            feats.append(label)
    print(f"features  : {', '.join(feats) if feats else '-'}")


def print_config(frame: Frame) -> None:
    (fw, proto, axes, enabled, invert, index, mode, channel, locked, hz, _res,
     baud, peer, mac, name) = FMT_CONFIG.unpack(frame.payload)
    mode_s = ("auto", "uart", "espnow")[mode] if mode < 3 else str(mode)
    print(f"backend   : {name.decode(errors='replace')}  fw {fw >> 8}.{fw & 0xFF}  protocol {proto}")
    print(f"mask      : enabled 0x{enabled:02x}  invert 0x{invert:02x}  index 0x{index:02x}")
    print(f"link      : mode {mode_s}  channel {channel}  peer-locked {bool(locked)}")
    print(f"uart baud : {baud}   stream {hz} Hz")
    print(f"peer mac  : {':'.join(f'{b:02x}' for b in peer)}")
    print(f"own mac   : {':'.join(f'{b:02x}' for b in mac)}")


def print_diag(frame: Frame) -> None:
    free_heap, loop_hz, hz, axes, _res = FMT_DIAG_FIXED.unpack_from(frame.payload, 0)
    offset = FMT_DIAG_FIXED.size
    edges = struct.unpack_from("<5I", frame.payload, offset); offset += 20
    overflow = struct.unpack_from("<5I", frame.payload, offset); offset += 20
    index = struct.unpack_from("<5I", frame.payload, offset); offset += 20
    (uart_rx, uart_tx, enow_rx, enow_tx, crc_errors, dropped) = struct.unpack_from(
        "<6I", frame.payload, offset)

    print(f"heap      : {free_heap} bytes   sampler {loop_hz} Hz   stream {hz} Hz")
    print(f"frames    : uart rx {uart_rx}/tx {uart_tx}   espnow rx {enow_rx}/tx {enow_tx}")
    print(f"errors    : crc {crc_errors}   dropped {dropped}")
    for i in range(5):
        print(f" {AXIS_NAMES[i]}: counts/s {edges[i]:>8}   overflow {overflow[i]:>4}   index {index[i]:>4}")


def print_telemetry_line(frame: Frame, prev: dict | None) -> dict:
    sample_seq, uptime, status, axes, index_flags = FMT_TELEMETRY.unpack_from(frame.payload, 0)
    pos = struct.unpack_from("<5i", frame.payload, FMT_TELEMETRY.size - 20)

    rates = [0.0] * 5
    if prev is not None:
        dt = (uptime - prev["uptime"]) / 1000.0
        if dt > 0:
            for i in range(5):
                rates[i] = (pos[i] - prev["pos"][i]) / dt

    cells = "  ".join(f"{AXIS_NAMES[i]} {pos[i]:>10d}" for i in range(axes))
    print(f"{uptime / 1000.0:8.2f}s  {cells}   [{describe_status(status)}]")

    return {"uptime": uptime, "pos": pos, "rates": rates, "status": status,
            "sample_seq": sample_seq, "index_flags": index_flags}


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------
def cmd_monitor(link: DroLink, args) -> int:
    frame = link.hello(args.hz, 0x1F)
    if frame is None:
        print("no HELLO reply - is the frontend link connected and the baud rate right?",
              file=sys.stderr)
        return 2
    print_hello(frame)
    print()
    print("monitoring, Ctrl+C to stop")
    print("     time            X          Y          Z          W          V")
    print("-" * 78)

    prev = None
    last_hello = time.monotonic()
    started = time.monotonic()
    printed = 0
    try:
        while True:
            if time.monotonic() - last_hello > 1.0:
                link.send(MSG_HELLO_REQ, FMT_HELLO_REQ.pack(1, 0x1F, args.hz,
                                                            b"bench-tool".ljust(16, b"\x00")))
                last_hello = time.monotonic()

            frame = link.read_frame()
            if frame is None:
                continue
            if frame.msg_type == MSG_TELEMETRY:
                prev = print_telemetry_line(frame, prev)
                printed += 1
                if args.seconds and time.monotonic() - started >= args.seconds:
                    break
            elif frame.msg_type == MSG_INDEX_EVENT:
                axis, pos, t_ms = FMT_INDEX_EVENT.unpack(frame.payload)
                print(f"  INDEX {AXIS_NAMES[axis]} at {pos} ({t_ms} ms)")
            elif frame.msg_type == MSG_ERROR:
                code, detail, arg = FMT_ERROR.unpack(frame.payload)
                print(f"  ERROR code={code} detail={detail} arg={arg}")
    except KeyboardInterrupt:
        pass

    print(f"\n{printed} telemetry frames, {link.dropped} malformed bytes dropped")
    return 0


def cmd_info(link: DroLink, args) -> int:
    frame = link.hello(args.hz)
    if frame is None:
        print("no reply", file=sys.stderr)
        return 2
    print_hello(frame)
    cfg = link.request(MSG_GET_CONFIG, b"", MSG_CONFIG)
    print()
    if cfg:
        print_config(cfg)
    else:
        print("no CONFIG reply", file=sys.stderr)
        return 2
    return 0


def cmd_zero(link: DroLink, args) -> int:
    if link.hello() is None:
        print("no reply", file=sys.stderr)
        return 2
    if args.axis.lower() == "all":
        mask = 0x1F
    else:
        mask = 1 << AXIS_NAMES.index(args.axis.upper())
    ack = link.request(MSG_ZERO, struct.pack("<B3x", mask), MSG_ACK)
    if ack is None:
        print("no ACK", file=sys.stderr)
        return 2
    print(f"zeroed mask 0x{mask:02x}")
    return 0


def cmd_preset(link: DroLink, args) -> int:
    if link.hello() is None:
        print("no reply", file=sys.stderr)
        return 2
    axis = AXIS_NAMES.index(args.axis.upper())
    ack = link.request(MSG_PRESET, struct.pack("<B3xi", axis, args.value), MSG_ACK)
    if ack is None:
        print("no ACK", file=sys.stderr)
        return 2
    print(f"{AXIS_NAMES[axis]} preset to {args.value}")
    return 0


def cmd_diag(link: DroLink, args) -> int:
    if link.hello() is None:
        print("no reply", file=sys.stderr)
        return 2
    frame = link.request(MSG_GET_DIAG, b"", MSG_DIAG)
    if frame is None:
        print("no DIAG reply", file=sys.stderr)
        return 2
    print_diag(frame)
    return 0


def cmd_ping(link: DroLink, args) -> int:
    if link.hello() is None:
        print("no reply", file=sys.stderr)
        return 2
    t0 = time.monotonic()
    pong = link.request(MSG_PING, b"0123456", MSG_PONG)
    if pong is None:
        print("no PONG", file=sys.stderr)
        return 2
    ms = (time.monotonic() - t0) * 1000.0
    print(f"PONG in {ms:.1f} ms, echo {list(pong.payload)}")
    return 0


def cmd_selftest(_link, _args) -> int:
    ok = True

    crc = crc16_ccitt(b"123456789")
    print(f"crc16     : 0x{crc:04x} (expect 0x29b1)")
    ok &= crc == 0x29B1

    # Fixed vector shared with the firmware self-test (console command
    # `selftest`): TELEMETRY, seq 42, payload 00 01 .. 1f.
    expected = bytes([
        0x04, 0x01, 0x02, 0x2a, 0x02, 0x20, 0x01, 0x22, 0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c,
        0x1d, 0x1e, 0x1f, 0x50, 0x56, 0x00,
    ])
    produced = encode_frame(0x02, bytes(range(32)), seq=42, flags=0)
    print(f"vector    : {'OK' if produced == expected else 'FAIL'} ({len(produced)} bytes)")
    ok &= produced == expected

    payload = bytes(range(64)) + b"\x00" * 8 + b"\xff" * 8
    enc = cobs_encode(payload)
    dec = cobs_decode(enc)
    print(f"cobs      : {len(payload)} -> {len(enc)} -> {len(dec)}")
    ok &= dec == payload

    frame = encode_frame(MSG_TELEMETRY, b"\x00" * 32, seq=7)
    body = cobs_decode(frame[:-1])
    parsed = decode_body(body)
    print(f"frame     : {len(frame)} bytes, type 0x{parsed.msg_type:02x}, seq {parsed.seq}")
    ok &= parsed.msg_type == MSG_TELEMETRY and parsed.seq == 7

    corrupted = bytearray(frame)
    corrupted[len(corrupted) // 2] ^= 0x01
    try:
        decode_body(cobs_decode(bytes(corrupted[:-1])))
        print("corrupt    : ACCEPTED (FAIL)")
        ok = False
    except ValueError:
        print("corrupt    : rejected (OK)")

    print(f"selftest  : {'PASS' if ok else 'FAIL'}")
    return 0 if ok else 1


def cmd_ports(_link, _args) -> int:
    if list_ports is None:
        print("pyserial not available", file=sys.stderr)
        return 2
    for port in list_ports.comports():
        print(f"{port.device:12s} {port.description}")
    return 0


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="DRO backend link bench tool")
    parser.add_argument("--port", help="serial port, e.g. COM7")
    parser.add_argument("--baud", type=int, default=460800, help="link baud rate")
    parser.add_argument("--hz", type=int, default=50, help="requested telemetry rate")
    parser.add_argument("--seconds", type=float, default=0, help="monitor duration (0 = forever)")

    sub = parser.add_subparsers(dest="command", required=True)

    sub.add_parser("monitor", help="print live positions")
    sub.add_parser("info", help="identity and configuration")
    sub.add_parser("diag", help="heap/frame/per-axis diagnostics")
    sub.add_parser("ping", help="round-trip test")
    sub.add_parser("selftest", help="codec self-test (no hardware needed)")
    sub.add_parser("ports", help="list serial ports")

    p_zero = sub.add_parser("zero", help="zero one axis or all")
    p_zero.add_argument("axis", nargs="?", default="all", help="X/Y/Z/W/V or all")

    p_preset = sub.add_parser("preset", help="set an axis position")
    p_preset.add_argument("axis", help="X/Y/Z/W/V")
    p_preset.add_argument("value", type=int, help="count value")

    return parser


def main(argv: list[str]) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    if args.command == "selftest":
        return cmd_selftest(None, args)
    if args.command == "ports":
        return cmd_ports(None, args)

    if not args.port:
        parser.error("--port is required for this command")

    try:
        link = DroLink(args.port, args.baud)
    except Exception as exc:
        print(f"cannot open {args.port}: {exc}", file=sys.stderr)
        return 2

    handlers = {
        "monitor": cmd_monitor,
        "info": cmd_info,
        "zero": cmd_zero,
        "preset": cmd_preset,
        "diag": cmd_diag,
        "ping": cmd_ping,
    }

    try:
        return handlers[args.command](link, args)
    finally:
        link.close()


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
