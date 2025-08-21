# OrtaVM Documentation

## Contents
- [XBin Format](#xbin-format)
- [File Structure](#file-structure)
- [Data Types](#data-types)
- [Operand Encoding](#operand-encoding)

## XBin Format

XBin is a compact yet powerful binary format designed for OrtaVM bytecode storage and execution. It provides efficient serialization of virtual machine programs with optimal space usage through variable-sized operand encoding.

### Key Features
- **Compact Storage**: Variable-sized integer encoding minimizes file size
- **Type Safety**: Explicit operand type identification
- **Metadata Support**: Embedded program metadata and debugging information
- **Label Resolution**: Built-in support for jump targets and references

## File Structure

The XBin format follows a sequential binary layout with the following components:

### 1. Header Section
| Field | Size | Type | Description                                |
|-------|------|------|--------------------------------------------|
| Meta | 13 bytes | `OrtaMeta` | VM metadata including flags and magic      |
| Filename Length | `sizeof(size_t)` | `size_t` | Length of source filename                  |
| Filename | Variable | `char[]` | Original source filename (null-terminated) |

### 2. Instructions Section
| Field | Size | Type | Description |
|-------|------|------|-------------|
| Instruction Count | `sizeof(size_t)` | `size_t` | Total number of instructions |

For each instruction:

| Field | Size | Type | Description |
|-------|------|------|-------------|
| OpCode | 1 byte | `unsigned char` | Instruction operation code |
| Line | 4 bytes | `unsigned int` | Source line number for debugging |
| Operands Count | `sizeof(size_t)` | `size_t` | Number of operands for this instruction |

### 3. Operands Encoding
Each operand is prefixed with a type identifier:

#### Register Operands (`'R'`)
| Field | Size | Type | Description |
|-------|------|------|-------------|
| Type | 1 byte | `char` | `'R'` identifier |
| Register ID | `sizeof(XRegisters)` | `XRegisters` | Register enumeration value |

#### Numeric Operands (`'N'`)
| Field | Size   | Type | Description |
|-------|--------|------|-------------|
| Type | 1 byte | `char` | `'N'` identifier |
| Size | 1 byte | `unsigned char` | Size of the encoded value (1, 2, 4, or 8 bytes) |
| Value | Varies | Signed/Unsigned | The actual numeric value |

#### String Operands (`'S'`)
| Field | Size             | Type | Description |
|-------|------------------|------|-------------|
| Type | 1 byte           | `char` | `'S'` identifier |
| Length | `sizeof(size_t)` | `size_t` | String length |
| Data | Varies           | `char[]` | String content |

### 4. Labels Section
| Field | Size | Type | Description |
|-------|------|------|-------------|
| Labels Count | `sizeof(size_t)` | `size_t` | Total number of labels |

For each label:

| Field | Size             | Type | Description |
|-------|------------------|------|-------------|
| Name Length | `sizeof(size_t)` | `size_t` | Label name length |
| Name | Varies           | `char[]` | Label name string |
| Address | `sizeof(size_t)` | `size_t` | Instruction address/offset |

## Data Types

### Numeric Value Optimization
The format uses intelligent size optimization for numeric values based on their range:

| Value Range | Storage Type | Size |
|-------------|--------------|------|
| `SCHAR_MIN` to `SCHAR_MAX` | `signed char` | 1 byte |
| `0` to `UCHAR_MAX` | `unsigned char` | 1 byte |
| `SHRT_MIN` to `SHRT_MAX` | `short` | 2 bytes |
| `0` to `USHRT_MAX` | `unsigned short` | 2 bytes |
| `INT_MIN` to `INT_MAX` | `int` | 4 bytes |
| `0` to `UINT_MAX` | `unsigned int` | 4 bytes |
| All other values | `int64_t`/`uint64_t` | 8 bytes |

### VM Flags
The metadata section includes flags that indicate required VM capabilities:
- `FLAG_STACK`: Program uses stack operations (`IPUSH`, `IPOP`)
- `FLAG_MEMORY`: Program uses memory operations (`IALLOC`, `IREADMEM`, `IWRITEMEM`)
- `FLAG_XCALL`: Program uses external calls (`IXCALL`)
- `FLAG_EXTERNAL_LIBRARY`: Program uses external libraries (`IXCALL 3`)

## Operand Encoding

### Register Encoding
Registers are encoded using their enumeration values from the `XRegisters` enum, providing type-safe register identification.

### Numeric Encoding
Numbers are automatically sized to use the minimum required bytes:
- Negative values use signed types
- Non-negative values use unsigned types when possible
- Values are cast to the appropriate size during encoding

### String Encoding
Strings are stored with explicit length prefixes, allowing for efficient parsing without null-termination requirements.
