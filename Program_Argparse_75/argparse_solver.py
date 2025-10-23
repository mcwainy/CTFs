#!/usr/bin/env python3
import argparse
import socket
import sys
from pathlib import Path

# Token keys
KEY_IP    = 0x01
KEY_NUM   = 0x02
KEY_SFLAG = 0x03
KEY_LFLAG = 0x04
KEY_STR   = 0x05
NEWLINE   = 0x0A  # CLI delimiter

def decode_stream(payload: bytes, num_endianness: str = "big"):
    """
    Parse the entire byte stream left-to-right.
    Treat 0x0A as a CLI delimiter ONLY when expecting a key (i.e., outside token payload).
    """
    lines = []
    argv = []
    i, n = 0, len(payload)

    def need(k):
        return i + k <= n

    while i < n:
        key = payload[i]; i += 1

        # CLI boundary
        if key == NEWLINE:
            lines.append(argv)
            argv = []
            continue

        # Token decoding
        if key == KEY_IP:
            if not need(4): break
            o1, o2, o3, o4 = payload[i:i+4]; i += 4
            argv.append(f"{o1}.{o2}.{o3}.{o4}")

        elif key == KEY_NUM:
            if not need(1): break
            size = payload[i]; i += 1
            if size == 0 or size > 8 or not need(size): break
            raw = payload[i:i+size]; i += size
            argv.append(str(int.from_bytes(raw, num_endianness, signed=False)))

        elif key == KEY_SFLAG:
            if not need(1): break
            ch = payload[i]; i += 1
            argv.append("-" + chr(ch))

        elif key == KEY_LFLAG:
            if not need(1): break
            L = payload[i]; i += 1
            if not need(L): break
            s = payload[i:i+L]; i += L
            argv.append("--" + s.decode("utf-8", errors="replace"))

        elif key == KEY_STR:
            if not need(1): break
            L = payload[i]; i += 1
            if not need(L): break
            s = payload[i:i+L]; i += L
            argv.append(s.decode("utf-8", errors="replace"))

        else:
            # Unknown key -> stop (best-effort)
            break

    if argv:
        lines.append(argv)
    return lines

def read_byte(sock_file):
    b = sock_file.read(1)
    return "" if not b else b.decode("utf-8", errors="replace")

def read_line(sock_file):
    """
    Server seems to emit lines like: 'Subprocess Output: X'
    Grab the last meaningful character from the line.
    """
    line = sock_file.readline()
    if not line:
        return ""
    s = line.decode("utf-8", errors="replace").strip()
    if ":" in s:
        tail = s.split(":", 1)[1].strip()
        return tail[-1:] if tail else ""
    return s[-1:] if s else ""

def stream_flag(host, port, lines, resp_mode="auto", use_crlf=False):
    """
    Send one decoded CLI per line; read one character per CLI back.
    resp_mode: 'auto' (detect), 'line' (readline & parse), 'byte' (read 1 byte)
    """
    term = "\r\n" if use_crlf else "\n"
    if not lines:
        return ""

    flag_chars = []
    with socket.create_connection((host, port), timeout=5) as s:
        sf = s.makefile("rb", buffering=0)

        # Pick reader
        if resp_mode == "line":
            reader = lambda: read_line(sf)
        elif resp_mode == "byte":
            reader = lambda: read_byte(sf)
        else:
            # AUTO: send first line, inspect response, then choose
            first = lines[0]
            s.sendall((" ".join(first) + term).encode("utf-8"))
            probe = sf.readline()
            if not probe:
                return ""
            txt = probe.decode("utf-8", errors="replace").strip()
            if "Subprocess Output" in txt or ":" in txt or len(txt) > 1:
                c = (txt.split(":", 1)[1].strip()[-1:] if ":" in txt else txt[-1:])
                reader = lambda: read_line(sf)
            else:
                c = txt[-1:] if txt else ""
                reader = lambda: read_byte(sf)
            if c:
                flag_chars.append(c)
                sys.stdout.write(c); sys.stdout.flush()
            # send the rest
            for argv in lines[1:]:
                s.sendall((" ".join(argv) + term).encode("utf-8"))
                ch = reader()
                if not ch: break
                flag_chars.append(ch)
                sys.stdout.write(ch); sys.stdout.flush()
            print()
            return "".join(flag_chars)

        # Explicit mode loop
        for argv in lines:
            s.sendall((" ".join(argv) + term).encode("utf-8"))
            ch = reader()
            if not ch: break
            flag_chars.append(ch)
            sys.stdout.write(ch); sys.stdout.flush()
        print()
        return "".join(flag_chars)

def main():
    ap = argparse.ArgumentParser(description="Argparse challenge solver (streaming decoder).")
    ap.add_argument("binfile", help="Path to cmds.bin")
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=1445)
    ap.add_argument("--resp-mode", choices=["auto", "line", "byte"], default="auto")
    ap.add_argument("--endianness", choices=["big", "little"], default="big",
                    help="Endianness for numbers (default: big)")
    ap.add_argument("--crlf", action="store_true", help="Use CRLF line endings")
    ap.add_argument("--dry-run", action="store_true", help="Print decoded lines only")
    ap.add_argument("--save-flag", default="", help="Write final flag to this file")
    args = ap.parse_args()

    payload = Path(args.binfile).read_bytes()
    lines = decode_stream(payload, num_endianness=args.endianness)

    if args.dry_run:
        for i, argv in enumerate(lines, 1):
            print(f"{i:03d}: {' '.join(argv)}")
        print(f"\nDecoded CLI strings: {len(lines)}")
        return

    flag = stream_flag(
        host=args.host,
        port=args.port,
        lines=lines,
        resp_mode=args.resp_mode,
        use_crlf=args.crlf
    )
    if flag:
        print(flag)
    if args.save_flag:
        Path(args.save_flag).write_text(flag)

if __name__ == "__main__":
    main()
