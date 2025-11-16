# Safety Marking Updater

This tool finds legacy safety-critical comment blocks in C++ source and replaces them with a new style using a configurable mapping file. It also classifies comment blocks (license header, safety warning, safety-critical, general) for inspection.

## How It Works
- Reads mappings from `markings.cfg` (or a custom config path). Each mapping defines an `old` block and the `new` block to replace it with.
- Scans the target file line-by-line, matching any configured `old` block (whitespace-insensitive) and emitting the corresponding `new` block while preserving indentation.
- Optionally strips nested markers derived from the config (keeps only the outermost pairs) and warns if unmatched start/end markers remain.
- Writes the updated file in place and reports timing and any warnings.

Diagram
```
participant User
participant Tool
participant Config
participant Analyzer
participant File

User->Tool: invoke replace_code <file> [config]
Tool->Config: read mappings
Tool->File: read lines
Tool->Tool: apply mappings (replace old blocks with new blocks)
Tool->File: write updated lines
Tool->Analyzer: analyze updated lines
Analyzer-->Tool: comment blocks + kinds
Tool-->User: summary of updated blocks
```

## Build

```bash
mkdir -p build && cd build
cmake ..
cmake --build .
```

## Run

```bash
# Use default config (markings.cfg)
./build/replace_code ./examples/example.cpp

# Use a custom config
./build/replace_code ./examples/example.cpp -c ./markings.cfg
```

## Config Format (markings.cfg)
- Blocks are defined with markers:
  - `BEGIN` starts an old block
  - `NEW` starts the replacement block
  - `END` ends the mapping
- Lines are matched after trimming leading/trailing whitespace; indentation is re-applied from the source file.

Example:
```
# Legacy single-line warning -> block comment
BEGIN
// This File Contains Safety-Critical Code
NEW
/*
*   WARNING: This File Contains Safety-Critical Code 
*/ 
END

# ASCII art begin -> simple marker
BEGIN
/*******************************************/
/*******************************************/
/**                                       **/
/**    BEGIN SAFETY CRITICAL SECTION      **/
/**                                       **/
/*******************************************/
/*******************************************/
NEW
// Safety-Critical Section Start
END
```

## What It Detects
- License headers, safety file warnings, safety-critical section markers, and general comments are identified after the update for reporting.
- Nested markers can be stripped when `--strip-nested` is passed; unmatched start/end tokens are reported as warnings.

## Notes
- Updates are applied in place; create your own backup if needed.
- Matching is tolerant of surrounding whitespace but otherwise exact per line. Update `markings.cfg` to support additional patterns.
- Set `--strip-nested` to collapse nested start/end markers based on your config; the tool will flag unmatched markers.
