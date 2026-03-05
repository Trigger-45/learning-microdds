# XRCE Playground - Multi-Structure Edition

[![C](https://img.shields.io/badge/C-11-blue?logo=c)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![DDS](https://img.shields.io/badge/DDS-Micro--XRCE-orange)](https://micro-xrce-dds.readthedocs.io/)
[![License](https://img.shields.io/badge/License-Apache%202.0-green)](LICENSE)
[![Status](https://img.shields.io/badge/Status-Exploring-yellow)](README.md)

Exploration project for **Micro-XRCE-DDS** with multiple message structures. Experimenting with different data types and IDL definitions to understand **bridges between Micro-XRCE-DDS and FastDDS**.

> **Experiment**: Testing 2 message structures (HelloWorld + SensorData) and IDL schema for potential future FastDDS interop.

## What's Being Explored

- Multiple message types in Micro-XRCE-DDS
- IDL schema definition (fastdds.idl) 
- How to structure data for cross-middleware communication
- Transitioning between Micro-XRCE and full DDS systems

## Quick Start

### Prerequisites
- Micro-XRCE-DDS Agent running on port 7400
- eProsima Fast CDR
- C compiler (gcc/clang)


## Project Structure

```
test_microdds/
├── include/           # Headers
│   ├── HelloWorld.h  # Message type 1
│   └── SensorData.h  # Message type 2 (experimental)
├── src/
│   ├── common/       # Serialization for both types
│   ├── publisher/    # Producer app
│   └── subscriber/   # Consumer apps
├── fastdds.idl       # Schema for future FastDDS bridge
└── README.md
```

## Components

- **publisher** - Sends messages via Micro-XRCE-DDS
- **subscriber1** - Receives type 1 messages
- **subscriber2** - Receives type 2 messages (SensorData test)

## Special: IDL Definition

```idl
// fastdds.idl - For potential FastDDS bridge later
struct SensorData {
  unsigned long sensor_id;
  double temperature;
  double humidity;
  long timestamp;
};
```

This allows future exploration of communicating between Micro-XRCE and FastDDS systems.

## Start Micro-XRCE Agent

```bash
./MicroXRCEAgent udp4 -p 7400
```

## Links

- [Micro-XRCE-DDS Docs](https://micro-xrce-dds.readthedocs.io/)
- [Fast DDS](https://fast-dds.docs.eprosima.com/)
- [Fast CDR](https://github.com/eProsima/Fast-CDR)

---

**Version**: 1.0 (Multi-structure exploration)  
**Last Updated**: March 2026
