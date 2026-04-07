# Node 3 implementation notes

## Main design choices

- BLE notifications are received in a callback.
- The callback only pushes raw 11-byte frames into a queue.
- The main loop processes queued frames and writes to the SD card.
- The logger keeps the file open and flushes periodically.

## Why this matters

Writing directly to SD inside a BLE callback is risky because SD access is relatively slow and blocking.
The queue-based approach is more robust and prevents missed frames.

## Current file naming behavior

The logger creates numbered files such as:

- `node3_log_000.csv`
- `node3_log_001.csv`
- `node3_log_002.csv`

This helps keep different tests separated without overwriting older data.
