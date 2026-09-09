# iGameVis local package data server

This standalone C++17 server exposes either a catalog of pre-generated archives
or one legacy archive over TCP. It is deliberately isolated from the current
`CSTest` socket sample and never accepts filesystem paths from a client.

## Build

The directory is an independent CMake project; no top-level iGameVis change is
required.

```powershell
cmake -S Tools/DataServer -B out/build/DataServer-Release -G Ninja `
  -DCMAKE_BUILD_TYPE=Release
cmake --build out/build/DataServer-Release
ctest --test-dir out/build/DataServer-Release --output-on-failure
```

On Windows, run these commands in a Visual Studio developer environment.

To include the target in the main iGameVis build, the minimal integration is a
single line after `add_subdirectory(ThirdParty)` (or later):

```cmake
add_subdirectory(Tools/DataServer)
```

The directory reuses the existing `libzstd_static` target in that case. It adds
its own bundled-zstd subdirectory only when configured as a standalone project.

## Run a package catalog

Place complete archives directly below one directory, then start catalog mode:

```powershell
out/build/DataServer-Release/iGameVisDataServer.exe `
  --root D:/packages `
  --catalog-cache D:/iGameVis-server-cache/catalog-sha.idx
```

`--catalog-cache` is optional. The default is
`<root>/.igamevis-catalog-v2.idx`. Its parent directory must already exist.
The cache is written through a temporary file and atomic rename, is bound to
the canonical package root, and avoids rehashing unchanged packages after a
restart. A cache hit requires size, mtime and a platform file fingerprint to
match. On Windows the fingerprint contains the volume/file ID and
creation/change times; on POSIX it contains device/inode and ctime. Replacing a
file while preserving its filename, size and mtime therefore still causes a
new SHA-256 calculation.

Catalog mode has these rules:

- only direct children whose names end exactly in `.tar.zst` are considered;
- a candidate must be a regular non-symlink file and, on Windows, must not be a
  reparse point;
- subdirectories, partial files such as `*.tar.zst.part`, the SHA cache, and
  other filenames are not published;
- the logical package ID is the complete filename, for example
  `DRIVAER-CP-L04.tar.zst`; the GUI display name strips `.tar.zst`;
- filenames with leading or trailing ASCII whitespace are skipped because the
  GUI normalizes user-facing IDs before opening a package;
- every published entry has a stable SHA-256 digest. Startup and a manual Fetch
  may therefore take time when archives are new or changed;
- Fetch performs a synchronous rescan and atomically replaces the in-memory
  catalog only after every ready package has been checked.

An active GET connection pins its already-open archive version. If an operator
atomically publishes a replacement and another client refreshes the catalog,
the active connection continues reading the old bytes; new connections receive
the new token and content. On Windows, publishers should use `ReplaceFileW` for
this operation (`MoveFileExW`, used by Python `os.replace`, does not replace an
open destination even when it was opened with delete sharing). The pinned
server handle is read-only and grants only read and delete sharing: in-place
writes are rejected, while `ReplaceFileW` can atomically replace the directory
entry and the active handle continues reading the old file object.

## Run one legacy package

```powershell
out/build/DataServer-Release/iGameVisDataServer.exe `
  --file D:/packages/DRIVAER-CP-L04.tar.zst `
  --id DRIVAER-CP-L04 `
  --sha256 <optional-64-hex-digits>
```

Alternatively, create the package from a source directory and then immediately
serve it:

```powershell
out/build/DataServer-Release/iGameVisDataServer.exe `
  --pack D:/1000000000_surface/drivaer_cp/L04 `
  --file D:/packages/DRIVAER-CP-L04.tar.zst `
  --id DRIVAER-CP-L04 `
  --compression-level 3
```

Legacy-only options:

- `--file` selects legacy single-package mode and is mutually exclusive with
  `--root`. It is resolved to one canonical regular non-symlink file at startup.
- `--pack` optionally names a source directory. Its contents are packed at the
  archive root by `TarZstdArchive::Create()` before the server starts listening.
  The output must be outside the source directory.
- `--replace-package` permits `--pack` to replace an existing regular archive;
  without it, an existing output is an error.
- `--compression-level` selects zstd level 1 through 19 and defaults to 3.
- `--id` is the only logical package identifier accepted over the wire. It
  defaults to the archive filename and may not contain a path separator.
- The server always streams through the completed package once before listening,
  computes SHA-256 and publishes the raw 32-byte digest expected by
  `QCryptographicHash::result()`.
- `--sha256` is optional and acts as an expected 64-character hexadecimal value.
  A mismatch aborts startup; it is not trusted without verification.
- If the legacy file's size or mtime changes after startup, INFO reports the new
  fixed-width version token with an empty SHA field. GET can use that token, but
  restart the process so it can hash and publish the new content digest.

Options shared by both modes:

- Ctrl+C requests a clean stop. Accept/receive/send waits use short polling
  intervals. A connection with no socket progress is closed after 300 seconds
  by default so the single-client server cannot be held forever by a stalled
  peer. `--idle-timeout-seconds <1-86400>` changes that deadline; the default is
  intentionally long enough for the local client to hash a multi-gigabyte
  cached archive between INFO and GET.
- `--bind` defaults to `127.0.0.1`.
- `--port` defaults to `34567`.

The server handles one client at a time. A disconnected client can reconnect and
continue from the length of its local `.part` file.

## Browse and open packages in iGameVis

For catalog mode, choose **File > Remote Model Library...**. Enter the host,
port and a writable cache root, then click **Fetch**. The table shows display
name, UInt64 size, server mtime ticks and content version. Select exactly one row
and click **Open**; **Cancel** stops either the current fetch or package request.
The catalog client follows all pages automatically. It sends `GOODBYE` and
closes the catalog connection before enabling Open, allowing the single-client
server to accept the selected package's INFO/GET connection.

Each GUI selection uses an isolated directory at
`<cache-root>/packages/<sha256(endpoint + package-id)>`. Packages with similar
filenames, different logical IDs, or different server endpoints cannot share a
partial download or extracted cache. A per-namespace `.package.lock` prevents
two requests from mutating the same cache concurrently.

Legacy single-package mode remains available on the command line. The download,
SHA-256 verification and extraction run outside the GUI thread; after the
package is published to the cache, its root VTM is passed to the normal
`igQtFileLoader::OpenFile()` path.

```powershell
out/build/x64-Release/iGameVis.exe `
  --remote-package DRIVAER-CP-L04 `
  --remote-host 127.0.0.1 `
  --remote-port 34567 `
  --remote-cache D:/iGameVis-cs-cache
```

Within a package cache namespace, the transfer cache contains:

- `<archive>.part` and `<archive>.part.json` while a download is incomplete;
- the verified archive plus `<archive>.download.json` after atomic publication;
- `extracted/<package-id>-<version-key>/` after validated extraction.

Restarting the same command queries INFO, validates the server version token,
and resumes at the UInt64 length of the matching `.part`. A complete cached
archive is rechecked against the advertised SHA-256 before reuse. The extracted
cache is accepted only when it has one root VTM entry point and every referenced
dataset resolves to a regular file within the extracted package.

## Protocol v1

Every integer is unsigned big-endian unless explicitly described otherwise.
Every complete wire frame, including its header, is at most 4 MiB.

### Fixed 32-byte frame header

| Offset | Size | Field |
|---:|---:|---|
| 0 | 4 | magic: ASCII `IGPK` (`0x4947504B`) |
| 4 | 2 | protocol version: `1` |
| 6 | 2 | message type |
| 8 | 8 | request ID |
| 16 | 8 | payload size |
| 24 | 8 | reserved, must be zero |

The maximum payload is 4,194,272 bytes.

### Message types and payloads

1. `INFO_REQUEST`
   - `packageIdLength: u16`, `reserved: u16`, UTF-8 `packageId`.
2. `INFO_RESPONSE`
   - `fileSize: u64`.
   - `mtimeTicks: i64` encoded as its 64-bit two's-complement bit pattern. This is
     opaque client metadata from `std::filesystem::file_time_type`.
   - `maxFrameSize: u32`, `maxChunkSize: u32`.
   - four `u16` lengths for `packageId`, `fileName`, `versionToken`, `sha256`.
   - the first three UTF-8 strings in that order, followed by either zero bytes
     or the raw 32-byte SHA-256 digest.
3. `GET_REQUEST`
   - `packageIdLength: u16`, `versionTokenLength: u16`.
   - `requestedLength: u32`, `offset: u64`.
   - UTF-8 `packageId`, then UTF-8 `versionToken`.
   - `requestedLength` must be 1 through 4,194,256. The final response can be
     shorter at EOF.
4. `DATA_CHUNK`
   - `offset: u64`, `dataLength: u32`, IEEE CRC-32: `u32`, then `dataLength`
     bytes. The CRC covers only the data bytes.
5. `ERROR`
   - `errorCode: u32`, `messageLength: u16`, `currentTokenLength: u16`, UTF-8
     message, then the current token if available.
6. `GOODBYE`
   - empty payload; closes the client connection without stopping the server.
7. `CATALOG_REQUEST`
   - `revision: u64`, `cursor: u32`, `pageSize: u16`, `flags: u16`.
   - `pageSize` is 1 through 100. Flag bit 0 requests a synchronous refresh and
     is valid only on the first page (`cursor == 0`). Other flag bits are
     rejected.
   - a first-page request uses revision and cursor zero. Later pages echo the
     response revision and use its `nextCursor` without the refresh flag.
8. `CATALOG_RESPONSE`
   - `revision: u64`, `nextCursor: u32`, `entryCount: u16`, reserved zero: `u16`.
   - `nextCursor == 0xffffffff` marks the last page.
   - each entry starts with `fileSize: u64`, `mtimeTicks: i64`, followed by four
     `u16` UTF-8 string lengths for package ID, display name, filename and
     version token; then raw SHA-256 (exactly 32 bytes) and those four strings.
   - the version token is the lowercase 64-character SHA-256 hexadecimal text.
     Entries are sorted by their full package IDs.

Error codes are: bad request (1), package not found (2), stale version (3),
invalid range (4), I/O error (5), unsupported protocol (6), internal error (7).

In legacy mode the version token is the fixed-width hexadecimal representation
of `fileSize-mtimeTicks`. In catalog mode it is the content SHA-256 hexadecimal
text. When opening a new package/token transfer session, the server checks the
package's size and mtime before and immediately after opening it. Later chunks
use that pinned handle. A token mismatch returns `StaleVersion`; the client must
discard the old partial archive, request INFO again, and start at offset zero.

## Security and current scope

- The client supplies a logical package ID, never a path.
- Legacy mode opens only the canonical file supplied with `--file`. Catalog mode
  opens only a server-discovered canonical direct child of `--root`.
- There is no authentication or encryption. Keep the default loopback binding;
  do not expose this MVP to an untrusted network.
- One-client-at-a-time service is intentional for the first local C/S milestone.
- Whole-file SHA-256 is computed by the server before listening and validated by
  the client after download. This requires one sequential read of the package at
  server startup, but uses a fixed 4 MiB buffer.

## Tests

The standalone DataServer build registers protocol unit tests and black-box
server tests with CTest. The integration suite launches the real executable and
covers an empty catalog, two-package sorted pagination, add/remove refresh,
stale revisions, direct-child and symlink/reparse filtering, selected-package
INFO/GET, pinned-handle replacement during refresh, server SHA-cache isolation,
same-size/same-mtime replacement across refresh and restart, stalled-client
deadlines and legacy compatibility.

The Qt catalog client has a separate Qt Core-only test project. It exercises a
101-entry fake-server exchange, automatic paging, repeated Fetch on one client,
revision mismatch rejection, UInt64 sizes, `GOODBYE`/socket closure and
endpoint/package cache isolation:

```powershell
cmake -S Qt/tests -B out/build/RemoteCatalogClientTests -G Ninja `
  -DCMAKE_BUILD_TYPE=Release
cmake --build out/build/RemoteCatalogClientTests
ctest --test-dir out/build/RemoteCatalogClientTests --output-on-failure
```
