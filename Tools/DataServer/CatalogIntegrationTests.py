#!/usr/bin/env python3
"""Black-box catalog and legacy protocol tests for iGameVisDataServer.

The test deliberately uses only the Python standard library.  It launches the
real server executable against disposable roots and speaks protocol v1 over a
TCP socket, so the filesystem scan and the wire implementation are exercised
together.
"""

from __future__ import print_function

import argparse
import hashlib
import os
from pathlib import Path
import socket
import struct
import subprocess
import sys
import tempfile
import time
import unittest
import zlib


MAGIC = 0x4947504B
PROTOCOL_VERSION = 1
HEADER = struct.Struct("!IHHQQQ")

INFO_REQUEST = 1
INFO_RESPONSE = 2
GET_REQUEST = 3
DATA_CHUNK = 4
ERROR_RESPONSE = 5
GOODBYE = 6
CATALOG_REQUEST = 7
CATALOG_RESPONSE = 8

BAD_REQUEST = 1
PACKAGE_NOT_FOUND = 2
CATALOG_FORCE_REFRESH = 0x0001
CATALOG_END_CURSOR = 0xFFFFFFFF


def _receive_exact(connection, size):
    chunks = []
    remaining = size
    while remaining:
        chunk = connection.recv(remaining)
        if not chunk:
            raise AssertionError(
                "connection closed with {} byte(s) still expected".format(remaining)
            )
        chunks.append(chunk)
        remaining -= len(chunk)
    return b"".join(chunks)


def _decode_signed_u64(value):
    return value if value < (1 << 63) else value - (1 << 64)


def _take(payload, offset, size, field):
    end = offset + size
    if end > len(payload):
        raise AssertionError("{} is truncated".format(field))
    return payload[offset:end], end


def _take_utf8(payload, offset, size, field):
    raw, offset = _take(payload, offset, size, field)
    try:
        return raw.decode("utf-8"), offset
    except UnicodeDecodeError as error:
        raise AssertionError("{} is not UTF-8: {}".format(field, error))


def decode_error(payload):
    if len(payload) < 8:
        raise AssertionError("ERROR payload is shorter than 8 bytes")
    code, message_length, token_length = struct.unpack_from("!IHH", payload, 0)
    offset = 8
    message, offset = _take_utf8(payload, offset, message_length, "error message")
    token, offset = _take_utf8(payload, offset, token_length, "current version")
    if offset != len(payload):
        raise AssertionError("ERROR payload contains trailing bytes")
    return {"code": code, "message": message, "version_token": token}


def decode_info(payload):
    if len(payload) < 32:
        raise AssertionError("INFO_RESPONSE payload is shorter than 32 bytes")
    (file_size, mtime_bits, max_frame_size, max_chunk_size, package_id_length,
     file_name_length, token_length, sha_length) = struct.unpack_from(
        "!QQIIHHHH", payload, 0
    )
    offset = 32
    package_id, offset = _take_utf8(
        payload, offset, package_id_length, "INFO package id"
    )
    file_name, offset = _take_utf8(
        payload, offset, file_name_length, "INFO filename"
    )
    version_token, offset = _take_utf8(
        payload, offset, token_length, "INFO version token"
    )
    sha256, offset = _take(payload, offset, sha_length, "INFO SHA-256")
    if offset != len(payload):
        raise AssertionError("INFO_RESPONSE payload contains trailing bytes")
    return {
        "file_size": file_size,
        "mtime_ticks": _decode_signed_u64(mtime_bits),
        "max_frame_size": max_frame_size,
        "max_chunk_size": max_chunk_size,
        "package_id": package_id,
        "file_name": file_name,
        "version_token": version_token,
        "sha256": sha256,
    }


def decode_catalog(payload):
    if len(payload) < 16:
        raise AssertionError("CATALOG_RESPONSE payload is shorter than 16 bytes")
    revision, next_cursor, count, reserved = struct.unpack_from("!QIHH", payload, 0)
    if reserved != 0:
        raise AssertionError("CATALOG_RESPONSE reserved field is non-zero")
    if count > 100:
        raise AssertionError("CATALOG_RESPONSE has more than 100 entries")
    offset = 16
    entries = []
    for index in range(count):
        if offset + 56 > len(payload):
            raise AssertionError("catalog entry {} fixed data is truncated".format(index))
        (file_size, mtime_bits, package_id_length, display_name_length,
         file_name_length, token_length) = struct.unpack_from("!QQHHHH", payload, offset)
        offset += 24
        sha256, offset = _take(payload, offset, 32, "catalog SHA-256")
        package_id, offset = _take_utf8(
            payload, offset, package_id_length, "catalog package id"
        )
        display_name, offset = _take_utf8(
            payload, offset, display_name_length, "catalog display name"
        )
        file_name, offset = _take_utf8(
            payload, offset, file_name_length, "catalog filename"
        )
        version_token, offset = _take_utf8(
            payload, offset, token_length, "catalog version token"
        )
        entries.append({
            "file_size": file_size,
            "mtime_ticks": _decode_signed_u64(mtime_bits),
            "package_id": package_id,
            "display_name": display_name,
            "file_name": file_name,
            "version_token": version_token,
            "sha256": sha256,
        })
    if offset != len(payload):
        raise AssertionError("CATALOG_RESPONSE payload contains trailing bytes")
    return {
        "revision": revision,
        "next_cursor": next_cursor,
        "entries": entries,
    }


class ProtocolConnection(object):
    def __init__(self, host, port):
        self._socket = socket.create_connection((host, port), timeout=5.0)
        self._socket.settimeout(5.0)
        self._request_id = 0

    def __enter__(self):
        return self

    def __exit__(self, unused_type, unused_value, unused_traceback):
        self.close()

    def close(self):
        if self._socket is None:
            return
        try:
            self._request_id += 1
            self._send_frame(GOODBYE, self._request_id, b"")
        except (OSError, AssertionError):
            pass
        try:
            self._socket.close()
        finally:
            self._socket = None

    def _send_frame(self, message_type, request_id, payload):
        header = HEADER.pack(
            MAGIC, PROTOCOL_VERSION, message_type, request_id, len(payload), 0
        )
        self._socket.sendall(header + payload)

    def _receive_frame(self):
        raw_header = _receive_exact(self._socket, HEADER.size)
        magic, version, message_type, request_id, payload_size, reserved = HEADER.unpack(
            raw_header
        )
        if magic != MAGIC:
            raise AssertionError("response has invalid protocol magic")
        if version != PROTOCOL_VERSION:
            raise AssertionError("response has unexpected protocol version")
        if reserved != 0:
            raise AssertionError("response header reserved field is non-zero")
        if payload_size > 4 * 1024 * 1024 - HEADER.size:
            raise AssertionError("response exceeds the protocol frame limit")
        return message_type, request_id, _receive_exact(self._socket, payload_size)

    def exchange(self, message_type, payload):
        self._request_id += 1
        request_id = self._request_id
        self._send_frame(message_type, request_id, payload)
        response_type, response_id, response_payload = self._receive_frame()
        if response_id != request_id:
            raise AssertionError(
                "response id {} does not match request {}".format(response_id, request_id)
            )
        return response_type, response_payload

    def catalog(self, revision=0, cursor=0, page_size=100, force_refresh=False):
        flags = CATALOG_FORCE_REFRESH if force_refresh else 0
        payload = struct.pack("!QIHH", revision, cursor, page_size, flags)
        response_type, response_payload = self.exchange(CATALOG_REQUEST, payload)
        if response_type == CATALOG_RESPONSE:
            return response_type, decode_catalog(response_payload)
        if response_type == ERROR_RESPONSE:
            return response_type, decode_error(response_payload)
        raise AssertionError("unexpected CATALOG response type {}".format(response_type))

    def info(self, package_id):
        encoded_id = package_id.encode("utf-8")
        payload = struct.pack("!HH", len(encoded_id), 0) + encoded_id
        response_type, response_payload = self.exchange(INFO_REQUEST, payload)
        if response_type == INFO_RESPONSE:
            return response_type, decode_info(response_payload)
        if response_type == ERROR_RESPONSE:
            return response_type, decode_error(response_payload)
        raise AssertionError("unexpected INFO response type {}".format(response_type))

    def get(self, package_id, version_token, offset, requested_length):
        encoded_id = package_id.encode("utf-8")
        encoded_token = version_token.encode("utf-8")
        payload = struct.pack(
            "!HHIQ", len(encoded_id), len(encoded_token), requested_length, offset
        ) + encoded_id + encoded_token
        response_type, response_payload = self.exchange(GET_REQUEST, payload)
        if response_type == ERROR_RESPONSE:
            return response_type, decode_error(response_payload)
        if response_type != DATA_CHUNK:
            raise AssertionError("unexpected GET response type {}".format(response_type))
        if len(response_payload) < 16:
            raise AssertionError("DATA_CHUNK payload is shorter than 16 bytes")
        response_offset, data_length, checksum = struct.unpack_from(
            "!QII", response_payload, 0
        )
        data = response_payload[16:]
        if len(data) != data_length:
            raise AssertionError("DATA_CHUNK length does not match its payload")
        if zlib.crc32(data) & 0xFFFFFFFF != checksum:
            raise AssertionError("DATA_CHUNK CRC-32 mismatch")
        return response_type, {"offset": response_offset, "data": data}


def _unused_loopback_port():
    probe = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        probe.bind(("127.0.0.1", 0))
        return probe.getsockname()[1]
    finally:
        probe.close()


class ServerProcess(object):
    def __init__(self, executable, arguments, log_directory):
        self.executable = str(executable)
        self.arguments = [str(value) for value in arguments]
        self.log_directory = Path(log_directory)
        self.port = _unused_loopback_port()
        self.process = None
        self.log_path = self.log_directory / "server-{}.log".format(self.port)
        self._log_file = None

    def __enter__(self):
        self._log_file = self.log_path.open("wb")
        command = [self.executable] + self.arguments + [
            "--bind", "127.0.0.1", "--port", str(self.port)
        ]
        self.process = subprocess.Popen(
            command,
            cwd=str(self.log_directory),
            stdout=self._log_file,
            stderr=subprocess.STDOUT,
        )
        deadline = time.monotonic() + 20.0
        last_error = None
        while time.monotonic() < deadline:
            return_code = self.process.poll()
            if return_code is not None:
                message = "server exited with {} before listening:\n{}".format(
                    return_code, self.read_log()
                )
                self.stop()
                raise AssertionError(message)
            try:
                probe = socket.create_connection(("127.0.0.1", self.port), timeout=0.2)
                probe.close()
                return self
            except OSError as error:
                last_error = error
                time.sleep(0.05)
        message = "server did not listen within 20 seconds ({}):\n{}".format(
            last_error, self.read_log()
        )
        self.stop()
        raise AssertionError(message)

    def __exit__(self, unused_type, unused_value, unused_traceback):
        self.stop()

    def stop(self):
        if self.process is not None and self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=5.0)
        if self._log_file is not None:
            self._log_file.close()
            self._log_file = None

    def read_log(self):
        if self._log_file is not None:
            self._log_file.flush()
        try:
            return self.log_path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            return "<server log unavailable>"


def _fetch_all_catalog(host, port, page_size=100):
    entries = []
    revision = 0
    cursor = 0
    first = True
    with ProtocolConnection(host, port) as client:
        while True:
            response_type, response = client.catalog(
                revision=revision,
                cursor=cursor,
                page_size=page_size,
                force_refresh=first,
            )
            if response_type != CATALOG_RESPONSE:
                raise AssertionError("catalog fetch failed: {}".format(response))
            if first:
                revision = response["revision"]
                if revision == 0:
                    raise AssertionError("server returned reserved catalog revision zero")
            elif response["revision"] != revision:
                raise AssertionError("catalog revision changed between pages")
            entries.extend(response["entries"])
            cursor = response["next_cursor"]
            if cursor == CATALOG_END_CURSOR:
                break
            first = False
    return revision, entries


def _try_create_file_symlink(target, link):
    try:
        os.symlink(str(target), str(link))
        return True
    except (AttributeError, NotImplementedError, OSError):
        return False


def _atomic_replace(source, destination):
    """Replace an existing file with the platform's atomic replace primitive."""
    if os.name != "nt":
        os.replace(str(source), str(destination))
        return

    # ReplaceFileW is the Windows API specifically intended for atomically
    # publishing a staged file over an existing file.  Python's os.replace uses
    # MoveFileExW, whose behavior with an open destination is more restrictive.
    import ctypes
    from ctypes import wintypes

    replace_file = ctypes.WinDLL("kernel32", use_last_error=True).ReplaceFileW
    replace_file.argtypes = [
        wintypes.LPCWSTR,
        wintypes.LPCWSTR,
        wintypes.LPCWSTR,
        wintypes.DWORD,
        wintypes.LPVOID,
        wintypes.LPVOID,
    ]
    replace_file.restype = wintypes.BOOL
    replace_file_ignore_merge_errors = 0x00000002
    if not replace_file(
        str(destination), str(source), None,
        replace_file_ignore_merge_errors, None, None
    ):
        raise ctypes.WinError(ctypes.get_last_error())


class CatalogIntegrationTests(unittest.TestCase):
    server_executable = None

    def test_catalog_scans_two_packages_pages_refreshes_and_transfers_selected(self):
        with tempfile.TemporaryDirectory(prefix="igame-catalog-") as temporary:
            base = Path(temporary)
            root = base / "root"
            root.mkdir()
            alpha_data = b"alpha package bytes"
            beta_data = b"beta package has different bytes"
            (root / "beta.tar.zst").write_bytes(beta_data)
            (root / "alpha.tar.zst").write_bytes(alpha_data)

            # These are deliberately not direct, complete package archives.
            nested = root / "nested"
            nested.mkdir()
            (nested / "nested.tar.zst").write_bytes(b"nested")
            (root / "notes.txt").write_bytes(b"not a package")
            (root / "unfinished.tar.zst.part").write_bytes(b"partial")
            (root / " leading-space.tar.zst").write_bytes(
                b"client trimming must not change a published package id"
            )

            outside = base / "outside.tar.zst"
            outside.write_bytes(b"must never be published through the link")
            link_created = _try_create_file_symlink(outside, root / "linked.tar.zst")

            cache = base / "catalog-cache.bin"
            with ServerProcess(
                self.server_executable,
                ["--root", root, "--catalog-cache", cache],
                base,
            ) as server:
                with ProtocolConnection("127.0.0.1", server.port) as client:
                    response_type, first = client.catalog(
                        revision=0, cursor=0, page_size=1, force_refresh=True
                    )
                    self.assertEqual(CATALOG_RESPONSE, response_type)
                    self.assertNotEqual(0, first["revision"])
                    self.assertEqual(1, first["next_cursor"])
                    self.assertEqual(["alpha.tar.zst"], [
                        entry["package_id"] for entry in first["entries"]
                    ])
                    alpha_entry = first["entries"][0]
                    self.assertEqual("alpha", alpha_entry["display_name"])
                    self.assertEqual("alpha.tar.zst", alpha_entry["file_name"])
                    self.assertEqual(len(alpha_data), alpha_entry["file_size"])
                    self.assertEqual(hashlib.sha256(alpha_data).digest(), alpha_entry["sha256"])
                    self.assertEqual(hashlib.sha256(alpha_data).hexdigest(),
                                     alpha_entry["version_token"])

                    response_type, second = client.catalog(
                        revision=first["revision"],
                        cursor=first["next_cursor"],
                        page_size=1,
                    )
                    self.assertEqual(CATALOG_RESPONSE, response_type)
                    self.assertEqual(first["revision"], second["revision"])
                    self.assertEqual(CATALOG_END_CURSOR, second["next_cursor"])
                    self.assertEqual(["beta.tar.zst"], [
                        entry["package_id"] for entry in second["entries"]
                    ])

                # A catalog connection is closed before INFO/GET starts, just as
                # the GUI does when the user opens one selected row.
                with ProtocolConnection("127.0.0.1", server.port) as client:
                    response_type, beta_info = client.info("beta.tar.zst")
                    self.assertEqual(INFO_RESPONSE, response_type)
                    self.assertEqual("beta.tar.zst", beta_info["package_id"])
                    self.assertEqual("beta.tar.zst", beta_info["file_name"])
                    self.assertEqual(hashlib.sha256(beta_data).digest(), beta_info["sha256"])
                    split = 8
                    response_type, chunk = client.get(
                        "beta.tar.zst", beta_info["version_token"], 0, split
                    )
                    self.assertEqual(DATA_CHUNK, response_type)
                    self.assertEqual(0, chunk["offset"])
                    self.assertEqual(beta_data[:split], chunk["data"])

                    # A transfer session pins the already-open file. Replacing
                    # the directory entry and refreshing the global catalog must
                    # not splice new bytes into that in-flight version.
                    new_beta_data = b"replacement beta content from a new archive"
                    # Stage beside the destination: this is the deployment
                    # pattern that gives rename/replace atomicity on Windows
                    # and POSIX filesystems alike.
                    replacement = root / "beta.tar.zst.tmp"
                    replacement.write_bytes(new_beta_data)
                    try:
                        _atomic_replace(replacement, root / "beta.tar.zst")
                    except OSError as error:
                        self.fail(
                            "atomic package replacement failed: {}\nserver log:\n{}".format(
                                error, server.read_log()
                            )
                        )
                    response_type, replacement_page = client.catalog(
                        revision=0, cursor=0, page_size=100, force_refresh=True
                    )
                    self.assertEqual(CATALOG_RESPONSE, response_type)
                    replacement_beta = next(
                        entry for entry in replacement_page["entries"]
                        if entry["package_id"] == "beta.tar.zst"
                    )
                    self.assertEqual(hashlib.sha256(new_beta_data).digest(),
                                     replacement_beta["sha256"])
                    self.assertNotEqual(beta_info["version_token"],
                                        replacement_beta["version_token"])

                    response_type, old_tail = client.get(
                        "beta.tar.zst", beta_info["version_token"], split,
                        len(beta_data) - split
                    )
                    self.assertEqual(DATA_CHUNK, response_type)
                    self.assertEqual(split, old_tail["offset"])
                    self.assertEqual(beta_data[split:], old_tail["data"])

                # A new connection resolves the refreshed record and sees only
                # the replacement archive/version.
                with ProtocolConnection("127.0.0.1", server.port) as client:
                    response_type, new_beta_info = client.info("beta.tar.zst")
                    self.assertEqual(INFO_RESPONSE, response_type)
                    self.assertEqual(hashlib.sha256(new_beta_data).digest(),
                                     new_beta_info["sha256"])
                    response_type, new_chunk = client.get(
                        "beta.tar.zst", new_beta_info["version_token"], 0,
                        len(new_beta_data)
                    )
                    self.assertEqual(DATA_CHUNK, response_type)
                    self.assertEqual(new_beta_data, new_chunk["data"])

                with ProtocolConnection("127.0.0.1", server.port) as client:
                    response_type, missing = client.info("../beta.tar.zst")
                    self.assertEqual(ERROR_RESPONSE, response_type)
                    self.assertEqual(PACKAGE_NOT_FOUND, missing["code"])

                old_revision = first["revision"]
                (root / "alpha.tar.zst").unlink()
                gamma_data = b"new gamma package"
                (root / "gamma.tar.zst").write_bytes(gamma_data)
                new_revision, refreshed = _fetch_all_catalog(
                    "127.0.0.1", server.port, page_size=100
                )
                self.assertNotEqual(old_revision, new_revision)
                self.assertEqual(
                    ["beta.tar.zst", "gamma.tar.zst"],
                    [entry["package_id"] for entry in refreshed],
                )
                self.assertNotIn("linked.tar.zst", [
                    entry["package_id"] for entry in refreshed
                ])
                self.assertNotIn(" leading-space.tar.zst", [
                    entry["package_id"] for entry in refreshed
                ])

                with ProtocolConnection("127.0.0.1", server.port) as client:
                    response_type, stale = client.catalog(
                        revision=old_revision, cursor=1, page_size=1
                    )
                    self.assertEqual(ERROR_RESPONSE, response_type)
                    self.assertIn(stale["code"], (BAD_REQUEST, 3))
                    self.assertIn("revision", stale["message"].lower())

                with ProtocolConnection("127.0.0.1", server.port) as client:
                    response_type, invalid = client.catalog(
                        revision=0, cursor=0, page_size=101, force_refresh=True
                    )
                    self.assertEqual(ERROR_RESPONSE, response_type)
                    self.assertEqual(BAD_REQUEST, invalid["code"])

            if not link_created:
                print(
                    "NOTE: file symlink/reparse creation was unavailable; "
                    "the direct-entry and temporary-file filters were still tested.",
                    file=sys.stderr,
                )

    def test_empty_catalog(self):
        with tempfile.TemporaryDirectory(prefix="igame-empty-catalog-") as temporary:
            base = Path(temporary)
            root = base / "root"
            root.mkdir()
            with ServerProcess(self.server_executable, ["--root", root], base) as server:
                revision, entries = _fetch_all_catalog("127.0.0.1", server.port)
                self.assertNotEqual(0, revision)
                self.assertEqual([], entries)

    def test_catalog_sha_cache_is_isolated_by_canonical_package_path(self):
        with tempfile.TemporaryDirectory(prefix="igame-cache-isolation-") as temporary:
            base = Path(temporary)
            root_a = base / "root-a"
            root_b = base / "root-b"
            root_a.mkdir()
            root_b.mkdir()
            data_a = b"AAAAA"
            data_b = b"BBBBB"
            package_a = root_a / "same.tar.zst"
            package_b = root_b / "same.tar.zst"
            package_a.write_bytes(data_a)
            package_b.write_bytes(data_b)
            # Equal names, lengths and mtimes catch a cache keyed only by file
            # metadata instead of canonical path plus metadata.
            fixed_ns = 1_700_000_000_000_000_000
            os.utime(str(package_a), ns=(fixed_ns, fixed_ns))
            os.utime(str(package_b), ns=(fixed_ns, fixed_ns))
            shared_cache = base / "shared-catalog-cache.bin"

            with ServerProcess(
                self.server_executable,
                ["--root", root_a, "--catalog-cache", shared_cache],
                base,
            ) as server:
                unused_revision, entries_a = _fetch_all_catalog(
                    "127.0.0.1", server.port
                )
            with ServerProcess(
                self.server_executable,
                ["--root", root_b, "--catalog-cache", shared_cache],
                base,
            ) as server:
                unused_revision, entries_b = _fetch_all_catalog(
                    "127.0.0.1", server.port
                )

            self.assertEqual(1, len(entries_a))
            self.assertEqual(1, len(entries_b))
            self.assertEqual(hashlib.sha256(data_a).digest(), entries_a[0]["sha256"])
            self.assertEqual(hashlib.sha256(data_b).digest(), entries_b[0]["sha256"])
            self.assertNotEqual(entries_a[0]["sha256"], entries_b[0]["sha256"])

    def test_same_size_mtime_replacement_invalidates_live_and_persistent_cache(self):
        with tempfile.TemporaryDirectory(prefix="igame-cache-fingerprint-") as temporary:
            base = Path(temporary)
            root = base / "root"
            root.mkdir()
            package = root / "same-stat.tar.zst"
            cache = base / "catalog-cache.bin"
            fixed_ns = 1_700_000_000_000_000_000
            versions = [b"AAAAA", b"BBBBB", b"CCCCC"]
            package.write_bytes(versions[0])
            os.utime(str(package), ns=(fixed_ns, fixed_ns))

            with ServerProcess(
                self.server_executable,
                ["--root", root, "--catalog-cache", cache],
                base,
            ) as server:
                unused_revision, initial = _fetch_all_catalog(
                    "127.0.0.1", server.port
                )
                self.assertEqual(hashlib.sha256(versions[0]).digest(),
                                 initial[0]["sha256"])

                staged = root / "same-stat.tar.zst.tmp"
                staged.write_bytes(versions[1])
                os.utime(str(staged), ns=(fixed_ns, fixed_ns))
                _atomic_replace(staged, package)
                os.utime(str(package), ns=(fixed_ns, fixed_ns))
                unused_revision, refreshed = _fetch_all_catalog(
                    "127.0.0.1", server.port
                )
                self.assertEqual(hashlib.sha256(versions[1]).digest(),
                                 refreshed[0]["sha256"])
                self.assertNotEqual(initial[0]["version_token"],
                                    refreshed[0]["version_token"])

            # The second replacement happens while the server is down.  On
            # restart, the persistent index must reject the otherwise equal
            # filename/size/mtime tuple using the stable file fingerprint.
            staged = root / "same-stat.tar.zst.tmp"
            staged.write_bytes(versions[2])
            os.utime(str(staged), ns=(fixed_ns, fixed_ns))
            _atomic_replace(staged, package)
            os.utime(str(package), ns=(fixed_ns, fixed_ns))
            with ServerProcess(
                self.server_executable,
                ["--root", root, "--catalog-cache", cache],
                base,
            ) as server:
                unused_revision, restarted = _fetch_all_catalog(
                    "127.0.0.1", server.port
                )
                self.assertEqual(hashlib.sha256(versions[2]).digest(),
                                 restarted[0]["sha256"])
                self.assertNotEqual(refreshed[0]["version_token"],
                                    restarted[0]["version_token"])

    def test_stalled_partial_frame_is_closed_after_idle_deadline(self):
        with tempfile.TemporaryDirectory(prefix="igame-idle-deadline-") as temporary:
            base = Path(temporary)
            root = base / "root"
            root.mkdir()
            payload = b"idle timeout package"
            (root / "idle.tar.zst").write_bytes(payload)
            with ServerProcess(
                self.server_executable,
                ["--root", root, "--idle-timeout-seconds", "1"],
                base,
            ) as server:
                stalled = socket.create_connection(("127.0.0.1", server.port), timeout=2.0)
                try:
                    stalled.settimeout(0.2)
                    stalled.sendall(b"I")  # one byte of the 32-byte frame header
                    started = time.monotonic()
                    closed = False
                    while time.monotonic() - started < 4.0:
                        try:
                            received = stalled.recv(1)
                            if not received:
                                closed = True
                                break
                        except socket.timeout:
                            pass
                        except (ConnectionResetError, ConnectionAbortedError):
                            closed = True
                            break
                    self.assertTrue(closed, msg=server.read_log())
                    self.assertGreaterEqual(time.monotonic() - started, 0.75)
                finally:
                    stalled.close()

                # Closing a stalled peer must release the one-client server so
                # an ordinary catalog request can immediately proceed.
                unused_revision, entries = _fetch_all_catalog(
                    "127.0.0.1", server.port
                )
                self.assertEqual(["idle.tar.zst"], [
                    entry["package_id"] for entry in entries
                ])

    def test_legacy_file_mode_still_supports_info_and_get(self):
        with tempfile.TemporaryDirectory(prefix="igame-legacy-") as temporary:
            base = Path(temporary)
            package = base / "legacy-payload.bin"
            package_data = b"legacy protocol payload"
            package.write_bytes(package_data)
            with ServerProcess(
                self.server_executable,
                ["--file", package, "--id", "legacy-package"],
                base,
            ) as server:
                with ProtocolConnection("127.0.0.1", server.port) as client:
                    response_type, info = client.info("legacy-package")
                    self.assertEqual(INFO_RESPONSE, response_type)
                    self.assertEqual(len(package_data), info["file_size"])
                    self.assertEqual(hashlib.sha256(package_data).digest(), info["sha256"])
                    self.assertRegex(
                        info["version_token"], r"^[0-9a-f]{16}-[0-9a-f]{16}$"
                    )
                    response_type, chunk = client.get(
                        "legacy-package", info["version_token"], 7, 8
                    )
                    self.assertEqual(DATA_CHUNK, response_type)
                    self.assertEqual(7, chunk["offset"])
                    self.assertEqual(package_data[7:15], chunk["data"])

                with ProtocolConnection("127.0.0.1", server.port) as client:
                    response_type, response = client.catalog(
                        revision=0, cursor=0, force_refresh=True
                    )
                    self.assertEqual(ERROR_RESPONSE, response_type)
                    self.assertEqual(BAD_REQUEST, response["code"])

                # A rejected catalog request must not break the legacy endpoint.
                with ProtocolConnection("127.0.0.1", server.port) as client:
                    response_type, info = client.info("legacy-package")
                    self.assertEqual(INFO_RESPONSE, response_type)
                    self.assertEqual("legacy-package", info["package_id"])

    def test_invalid_root_and_legacy_option_combinations_fail_closed(self):
        with tempfile.TemporaryDirectory(prefix="igame-invalid-options-") as temporary:
            base = Path(temporary)
            root = base / "root"
            root.mkdir()
            regular_file = base / "file.bin"
            regular_file.write_bytes(b"file")

            cases = [
                (["--root", base / "missing"], "root"),
                (["--root", regular_file], "root"),
                (["--root", root, "--file", regular_file], "exclusive"),
                (["--file", regular_file, "--catalog-cache", base / "cache"], "root"),
            ]
            for arguments, expected_text in cases:
                completed = subprocess.run(
                    [str(self.server_executable)] + [str(value) for value in arguments],
                    cwd=str(base),
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    timeout=10.0,
                    text=True,
                )
                self.assertNotEqual(0, completed.returncode, msg=completed.stdout)
                self.assertIn(expected_text, completed.stdout.lower())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--server", required=True, type=Path)
    arguments, unittest_arguments = parser.parse_known_args()
    if not arguments.server.is_file():
        parser.error("--server must name the built iGameVisDataServer executable")
    CatalogIntegrationTests.server_executable = arguments.server.resolve()
    unittest.main(argv=[sys.argv[0]] + unittest_arguments)


if __name__ == "__main__":
    main()
