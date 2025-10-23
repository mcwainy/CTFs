#!/usr/bin/env python3
import sys, struct, socket, subprocess, os

BIN = sys.argv[1] if len(sys.argv) > 1 else "./baseodyssey"
REMOTE = (len(sys.argv) > 2 and sys.argv[2].upper() == "REMOTE")
HOST = sys.argv[3] if len(sys.argv) > 3 else "www.cweaccessionsctf.com"
PORT = int(sys.argv[4]) if len(sys.argv) > 4 else 1425

def find_table(blob: bytes):
    # Scan for a 256-byte region with exactly 64 distinct values < 64
    for i in range(len(blob)-256):
        win = blob[i:i+256]
        s = {b for b in win if b < 64}
        if len(s) == 64:
            return i, win
    raise RuntimeError("scrambled table not found in ELF")

def build_encoder(tbl256: bytes):
    inv = {}
    for c, v in enumerate(tbl256):
        if v < 64 and v not in inv:
            inv[v] = c
    if len(inv) != 64:
        raise RuntimeError(f"table maps {len(inv)} sextets, expected 64")
    def enc(raw: bytes) -> bytes:
        assert len(raw) % 3 == 0
        out = bytearray()
        for i in range(0, len(raw), 3):
            a,b,c = raw[i], raw[i+1], raw[i+2]
            v = (a<<16)|(b<<8)|c
            out += bytes([inv[(v>>18)&63], inv[(v>>12)&63], inv[(v>>6)&63], inv[v&63]])
        return bytes(out)
    return enc

def read_elf(path: str) -> bytes:
    with open(path, "rb") as f:
        return f.read()

def make_encoded(encoder):
    # Required 10 bytes + 2 filler → 12 bytes (encodes to 16)
    payload10 = struct.pack("<i h f", 2001, 9000, 1.618)
    # Choose filler so last encoded byte is NOT space/newline/'='
    BAD_LAST = {0x20, 0x0a, 0x3d}
    for f0 in range(256):
        for f1 in range(256):
            raw = payload10 + bytes([f0, f1])
            enc = encoder(raw)
            if enc[-1] not in BAD_LAST:
                return enc, raw
    raise RuntimeError("could not pick safe filler")

def run_local(cmd_bytes: bytes):
    p = subprocess.run([BIN], input=cmd_bytes + b"\n",
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    print(p.stdout.decode("latin1", errors="ignore"))

def run_remote(cmd_bytes: bytes):
    s = socket.create_connection((HOST, PORT), timeout=5.0)
    s.sendall(cmd_bytes + b"\r\n")
    # Read whatever the service prints, then close
    s.shutdown(socket.SHUT_WR)
    out = bytearray()
    try:
        while True:
            chunk = s.recv(4096)
            if not chunk: break
            out += chunk
    finally:
        s.close()
    print(out.decode("latin1", errors="ignore"))

def main():
    blob = read_elf(BIN)
    off, tbl = find_table(blob)
    enc = build_encoder(tbl)
    encoded_bytes, raw = make_encoded(enc)

    # For debugging: show hex so you can reproduce with printf/xxd if needed
    print(f"[+] table @ file offset 0x{off:x}")
    print(f"[+] decoded bytes (payload): {raw.hex()}")
    print(f"[+] encoded bytes (to send): {encoded_bytes.hex()}")
    # DO NOT print the “string” — many bytes >0x7F / nonprintable

    if REMOTE:
        run_remote(encoded_bytes)
    else:
        run_local(encoded_bytes)

if __name__ == "__main__":
    main()
