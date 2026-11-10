#!/usr/bin/env python3
"""Independently inspect a CPack source tarball before uploading it."""
import sys
import tarfile
from pathlib import PurePosixPath

BLOCKED = (
    (".meristem",),
    ("ACTIVE_WORK.md",),
    (".deps",),
    ("build",),
    ("build-test",),
    ("build_test",),
    ("tests", "build"),
    ("install",),
    (".cache",),
    ("memory",),
    (".grepai",),
)


def verify(archive):
    with tarfile.open(archive, "r:xz") as bundle:
        members = [tuple(PurePosixPath(m.name).parts[1:]) for m in bundle.getmembers()]
    if not members:
        raise ValueError("Empty source archive")
    for parts in members:
        for banned in BLOCKED:
            if parts[:len(banned)] == banned:
                raise ValueError("Private/generated path in source archive: " + "/".join(parts))
        if parts and (parts[-1] in {"AGENTS.md", "mcp.db", "SESSION-STATE.md", "SOUL.md", "USER.md", "MEMORY.md", "HEARTBEAT.md"} or
                      parts[-1].startswith(".env") or parts[-1].endswith((".pem", ".key"))):
            raise ValueError("Private file in source archive: " + "/".join(parts))
    for expected in (("buildspec.json",), ("data", "emulens.effect"), ("README.md",), ("LICENSE",)):
        if expected not in members:
            raise ValueError("Missing required public file: " + "/".join(expected))
    print(f"PASS: verified {len(members)} source-archive members; no private/generated entries")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: check_source_archive.py <archive.tar.xz>")
    try:
        verify(sys.argv[1])
    except (ValueError, tarfile.TarError) as ex:
        raise SystemExit(f"FAIL: {ex}")
