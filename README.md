# Purity Server: Overview

This C++ project implements a **Asynchronous server** using **Boost.Asio**, a custom binary protocol with **MMORPG standart** packet framing, SRP (Secure Remote Password) for authentication logic, and PostgreSQL integration.

## Key Components

## 📦 Packet Structure

Every TCP packet uses the **MMORPG** binary framing:

```
[2 bytes Length_BE][2 bytes Opcode_LE][Payload]
```

- Length_BE — Total length of [Opcode + Payload] in Big Endian.
- Opcode_LE — Packet opcode as Little Endian.
- Payload — Binary data, encoded with your custom ByteBuffer.

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

## 📦 Configuration via Environment Variables

The server automatically reads database connection settings from **environment variables**, with safe defaults if not set:

```cpp
std::string db_host = std::getenv("DB_URL") ? std::getenv("DB_URL") : "127.0.0.1";
std::string db_port = std::getenv("DB_PORT") ? std::getenv("DB_PORT") : "5432";
std::string db_user = std::getenv("DB_USER") ? std::getenv("DB_USER") : "postgres";
std::string db_password = std::getenv("DB_PASSWORD") ? std::getenv("DB_PASSWORD") : "postgres";
std::string db_name = std::getenv("DB_NAME") ? std::getenv("DB_NAME") : "postgres";
```

### ⚙️ Variables:
- **DB_URL** — PostgreSQL host address (default `127.0.0.1`)
- **DB_PORT** — PostgreSQL port (default `5432`)
- **DB_USER** — database user (default `postgres`)
- **DB_PASSWORD** — database password (default `postgres`)
- **DB_NAME** — database name (default `postgres`)

If an environment variable is not set, a safe fallback will be used. The log output shows exactly which values are applied.

Example for Linux:
```bash
export DB_URL=192.168.1.100
export DB_USER=myuser
export DB_PASSWORD=mypass
./server
```

This makes it easy to configure the server without changing the source code! ✅


---

## Requirements

**sudo apt install libssl-dev libpq-dev libpqxx-dev pkg-config**

## ⚡️Compilation

mkdir build && cd build

cmake ..

make -j 4             (or another threads count)

---