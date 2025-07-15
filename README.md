# Purity Server: Overview

This C++ project implements a **Asynchronous server** using **Boost.Asio**, a custom binary protocol with **big-endian** packet framing, SRP (Secure Remote Password) for authentication logic, and PostgreSQL integration.

## Key Components

### ✅ **Handlers** (`Handlers.cpp`)
- Contains a **dispatch system** that routes packets by opcode.
- Handles `PING`, `CHAT_MESSAGE` and `ACCOUNT_LOOKUP_BY_NAME` opcodes.
- Runs `ACCOUNT_LOOKUP_BY_NAME` as a coroutine to execute blocking DB logic in a safe worker thread pool.

### ✅ **ClientSession** (`ClientSession.cpp`)
- Owns a TCP socket and buffers.
- Fully **thread-safe**: `send_packet()` posts safely back to the I/O thread.
- Reads framed packets `[2 size][2 opcode][payload]`.
- Uses a `MessageBuffer` for incremental reading.
- Manages its own lifetime via `shared_from_this()`.

### ✅ **Server** (`Server.cpp`)
- Accepts new connections asynchronously.
- Manages active sessions with a `std::mutex`.
- Cleanly stops all sessions and the DB pool on shutdown.

### ✅ **SRP (Secure Remote Password)** (`SRP.cpp`)
- Implements RFC 5054 style ephemeral key generation and proof.
- Supports fake challenges to mitigate timing attacks.
- Uses OpenSSL BIGNUMs with proper lifetime management.

## Technical Highlights

- **All packets are big-endian.**
- Safe async write queue for each client.
- `Handlers` run CPU-bound DB calls in dedicated threads but serialize back replies to the main I/O thread.
- `SRP` is prepared for PvPGN-like proof-of-concept authentication.
- All socket writes are guarded against race conditions.

## Security & Stability

- All OpenSSL BIGNUMs are freed properly.
- DB pool handles broken connections.
- Sessions auto-cleanup on error or disconnect.
- No leaks in the async read/write chain.

## How to Extend

- Add new opcodes in `Handlers.cpp`.
- Add new prepared statements in your DB class.
- Extend `SRP` for real client proof validation.

---

## Requirements

**sudo apt install libssl-dev libpq-dev libpqxx-dev pkg-config**

## ⚡️Compilation

mkdir build && cd build

cmake ..

make -j 4             (or another threads count)

---