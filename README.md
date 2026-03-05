# Micro-XRCE-DDS Test Suite

[![C](https://img.shields.io/badge/C-11-blue?logo=c)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![DDS](https://img.shields.io/badge/DDS-Micro--XRCE-orange)](https://micro-xrce-dds.readthedocs.io/)
[![License](https://img.shields.io/badge/License-Apache%202.0-green)](LICENSE)
[![Status](https://img.shields.io/badge/Status-Active-brightgreen)](README.md)

Exploration project for **Micro-XRCE-DDS** - a lightweight DDS implementation for IoT and embedded systems. Testing pub-sub patterns, message serialization, and topic routing.

## 🚀 Quick Start

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
