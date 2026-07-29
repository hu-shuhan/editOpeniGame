# Metric 9.1: Adaptive Compression for Large-Scale CAE Simulation Data

## Metric Scope

| Item | Requirement |
|------|-------------|
| Error | Variable error in visual-feature regions no greater than 5% |
| Compression | CAE data compression ratio no less than 20% |
| Assessment | Qualified third-party evaluation |
| Deliverable | Open link library |

> The repository provides IGC encoding and decoding. Compliance with the error and compression thresholds requires fixed datasets, feature-region masks, and a third-party report.

## IGC Encoding

The encoder organizes geometry, topology, attributes, and parameters as payloads and compresses them with Zstandard before writing an `.igc` file.

| Path | Description |
|------|-------------|
| `iGameCore/Filters/MeshCodec/iGameMeshEncoderFilter.h` | Encoding and payload compression |
| `iGameCore/Filters/MeshCodec/SubCodec/iGameMeshCodecZSTD.h` | Zstandard codec |
| `iGameCore/IO/IGC/iGameIGCWriter.*` | IGC writer |
| `Examples/Filter/Compression/TestEncoder.cpp` | `testEncoder` |

```cpp
auto writer = iGame::IGCWriter::New();
writer->SetCodecControlParams(
    iGame::MeshEncoderFilter<iGame::EncodeOutputBinaryArray>::GenerateDefaultCodecParams(source));
writer->WriteToFile(source, "./Models/comp.igc");
```

## IGC Decoding

| Path | Description |
|------|-------------|
| `iGameCore/Filters/MeshCodec/iGameMeshDecoderFilter.h` | Decompression and data reconstruction |
| `iGameCore/IO/IGC/iGameIGCReader.*` | IGC reader |
| `Examples/Filter/Compression/TestDecoder.cpp` | `testDecoder` |

Run `testEncoder` first to create `comp.igc`, then run `testDecoder` to load and visualize the reconstructed data. `testLosslessEncode` provides a command-line round-trip verification entry.

## Assessment

```text
compressionRatio = (originalBytes - compressedBytes) / originalBytes
```

Assessment should fix input files, feature-region masks, variables, error definition, library version, build environment, and logs. Verify compression ratio, regional variable error, topology, attribute names, dimensions, attachment, and value ranges after decoding. The link-library release should include headers, libraries, runtime dependencies, CMake configuration, examples, and version information.
