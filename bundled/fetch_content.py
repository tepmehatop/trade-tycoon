#!/usr/bin/env python3
"""Download the content listed in content.json from BaNaNaS.

Uses the same protocol as the in-game content downloader (src/network/network_content.cpp):
  1. TCP content server: ClientInfoExternalID -> ServerInfo (internal content id, version, size).
  2. HTTP mirror: POST "<id>\\n..." -> lines "id,type,size,url", then GET each url (.tar.gz).
Files end up as bundled/content/<subdir>/<name>.tar, which CMake copies next to the binary.
"""
import gzip
import hashlib
import json
import pathlib
import socket
import struct
import sys
import urllib.request

CONTENT_SERVER = ("content.openttd.org", 3978)
MIRROR_URI = "https://binaries.openttd.org/bananas"
# The mirror rejects unknown clients; identify the same way as the game does (http_curl.cpp).
USER_AGENT = "OpenTTD/trade-tycoon"

PACKET_CLIENT_INFO_EXTID = 2
PACKET_SERVER_INFO = 4

CONTENT_TYPES = {"base-graphics": 1, "newgrf": 2, "ai": 3, "ai-library": 4, "scenario": 5,
                 "heightmap": 6, "base-sounds": 7, "base-music": 8, "game-script": 9, "game-script-library": 10}
SUBDIRS = {1: "baseset", 2: "newgrf", 3: "ai", 4: "ai/library", 5: "scenario", 6: "scenario/heightmap",
           7: "baseset", 8: "baseset", 9: "game", 10: "game/library"}

HERE = pathlib.Path(__file__).resolve().parent


class Reader:
    def __init__(self, data):
        self.data, self.pos = data, 0

    def take(self, fmt):
        value = struct.unpack_from(fmt, self.data, self.pos)
        self.pos += struct.calcsize(fmt)
        return value[0] if len(value) == 1 else value

    def string(self):
        end = self.data.index(b"\0", self.pos)
        value = self.data[self.pos:end].decode("utf-8", "replace")
        self.pos = end + 1
        return value

    def raw(self, n):
        value = self.data[self.pos:self.pos + n]
        self.pos += n
        return value


def send_packet(sock, ptype, payload):
    sock.sendall(struct.pack("<HB", 3 + len(payload), ptype) + payload)


def recv_exact(sock, n):
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError("content server closed the connection")
        buf += chunk
    return buf


def query_info(ctype, uid):
    """Ask the content server about one (type, unique_id); returns a ServerInfo dict or None.

    The server does not answer at all for unknown content, hence the short timeout.
    """
    payload = struct.pack("<BB", 1, ctype) + uid
    with socket.create_connection(CONTENT_SERVER, timeout=15) as sock:
        send_packet(sock, PACKET_CLIENT_INFO_EXTID, payload)
        try:
            while True:
                size, ptype = struct.unpack("<HB", recv_exact(sock, 3))
                body = recv_exact(sock, size - 3)
                if ptype == PACKET_SERVER_INFO:
                    break
        except TimeoutError:
            return None
    r = Reader(body)
    info = {"type": r.take("<B"), "id": r.take("<I"), "filesize": r.take("<I"),
            "name": r.string(), "version": r.string(), "url": r.string(), "description": r.string(),
            "unique_id": r.raw(4).hex(), "md5": r.raw(16).hex()}
    info["dependencies"] = [r.take("<I") for _ in range(r.take("<B"))]
    info["tags"] = [r.string() for _ in range(r.take("<B"))]
    return info


def mirror_urls(ids):
    body = "".join(f"{i}\n" for i in ids).encode()
    req = urllib.request.Request(MIRROR_URI, data=body, headers={"Content-Type": "application/x-www-form-urlencoded", "User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=60) as resp:
        lines = resp.read().decode().splitlines()
    urls = {}
    for line in lines:
        cid, _ctype, _size, url = line.split(",", 3)
        if not url.startswith("ottd"):
            urls[int(cid)] = url
    return urls


def main():
    manifest = json.loads((HERE / "content.json").read_text())["content"]
    out_root = HERE / "content"
    infos, missing = [], []
    for c in manifest:
        ctype = CONTENT_TYPES[c["type"]]
        uid = bytes.fromhex(c["unique_id"])
        # BaNaNaS shows a GRF ID as a big-endian number, the protocol sends it as stored in the GRF.
        if ctype == CONTENT_TYPES["newgrf"]:
            uid = uid[::-1]
        info = query_info(ctype, uid)
        if info is None:
            missing.append(f"{c['name']} ({c['unique_id']})")
        else:
            infos.append(info)
    if missing:
        sys.exit(f"not found on content server: {', '.join(missing)}")
    if any(i["filesize"] == 0 for i in infos):
        sys.exit("content server reports unavailable content: " + ", ".join(i["name"] for i in infos if i["filesize"] == 0))

    urls = mirror_urls([i["id"] for i in infos])
    lock = []
    for info in infos:
        url = urls[info["id"]]
        filename = url.rsplit("/", 1)[1].removesuffix(".tar.gz") + ".tar"
        target = out_root / SUBDIRS[info["type"]] / filename
        target.parent.mkdir(parents=True, exist_ok=True)
        print(f"{info['name']} {info['version']} -> {target.relative_to(HERE)}")
        with urllib.request.urlopen(urllib.request.Request(url, headers={"User-Agent": USER_AGENT}), timeout=300) as resp:
            data = gzip.decompress(resp.read())
        target.write_bytes(data)
        lock.append({"name": info["name"], "version": info["version"], "unique_id": info["unique_id"],
                     "md5": info["md5"], "file": str(target.relative_to(HERE)),
                     "sha256": hashlib.sha256(data).hexdigest()})

    (HERE / "content.lock.json").write_text(json.dumps(lock, indent="\t", ensure_ascii=False) + "\n")


if __name__ == "__main__":
    main()
