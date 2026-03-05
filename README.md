<div align="center">

# XRCE Exploration

**Exploration & testing ground for Micro-XRCE-DDS — experimenting with pub-sub patterns, multiple message structures, and future bridges to FastDDS.**

[![C](https://img.shields.io/badge/C-11-blue?style=for-the-badge&logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![DDS](https://img.shields.io/badge/Micro--XRCE--DDS-orange?style=for-the-badge&logo=ros&logoColor=white)](https://micro-xrce-dds.readthedocs.io/)
[![FastCDR](https://img.shields.io/badge/FastCDR-Apache%202.0-blue?style=for-the-badge&logo=eprosima&logoColor=white)](https://github.com/eProsima/Fast-CDR)
[![License: MIT](https://img.shields.io/badge/License-MIT-red?style=for-the-badge)](LICENSE)
[![Status](https://img.shields.io/badge/Status-Exploring-brightgreen?style=for-the-badge)](README.md)

---

</div>

Exploration project for **Micro-XRCE-DDS** - a lightweight DDS implementation for IoT and embedded systems. Testing pub-sub patterns, message serialization, and topic routing.

## Branches

| Branch | Focus | Status |
|--------|-------|--------|
| **main** | Core Micro-XRCE-DDS implementation with basic pub-sub patterns | Stable |
| **dev_fast** | Experiments with FastDDS integration and multi-structure bridges | WIP |

Choose `main` for stable exploration, `dev_fast` for cutting-edge FastDDS bridge experiments.

## Quick Start

### Prerequisites
- Micro-XRCE-DDS Agent running on port 7400
- eProsima Fast CDR
- C compiler (gcc/clang)
- Make or CMake

### Build & Run

```bash
# With Make
make build
make run

# Or with CMake
mkdir build && cd build
cmake .. && make
./publisher &
./subscriber &
./subscriber2
```

## 📁 Project Structure

```
test_microdds/
├── include/           # Headers
│   └── HelloWorld.h  # Message structure
├── src/
│   ├── common/       # Serialization
│   ├── publisher/    # Producer apps
│   └── subscriber/   # Consumer apps
├── Makefile          # Build targets
├── CMakeLists.txt    # CMake config
└── README.md
```

## 📊 Architecture

- **publisher** - Generates random JSON messages continuously
- **publisher2** - Queue-based variant
- **subscriber** - Receives and forwards to intermediate topic
- **subscriber2** - Receives forwarded messages

Data flow: `publisher` → `HelloWorld topic` → `subscriber` → `ForwardedTopic` → `subscriber2`

## 🔧 Start Agent

```bash
./MicroXRCEAgent udp4 -p 7400
```

## 📚 Links

- [Micro-XRCE-DDS Docs](https://micro-xrce-dds.readthedocs.io/)
- [Fast CDR](https://github.com/eProsima/Fast-CDR)
- [DDS Standard](https://www.omg.org/spec/DDS/)

---

**Version**: 1.0  
**Last Updated**: March 2026
