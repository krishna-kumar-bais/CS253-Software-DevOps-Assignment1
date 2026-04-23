# CS253 Assignment 1: Memory-Efficient Versioned File Indexer

## Overview

This program implements a memory-efficient versioned file indexer in C++. It processes large text files incrementally using a fixed-size buffer and supports three types of analytical queries: word count, top-K frequent words, and word frequency difference between two versions.

---

## Files

| File | Description |
|------|-------------|
| `rollnumber_firstname.cpp` | Main C++ source code |
| `rollnumber_firstname.md` | This README file |
| `rollnumber_firstname.pdf` | Assignment report |
| `rollnumber_firstname.jpg` | Execution screenshot |

---

## Design Overview

The solution uses object-oriented design with the following classes:

### `Timer<Clock>` (Class Template)
A generic timer class templated on a clock type (e.g., `std::chrono::high_resolution_clock`). Used to measure total execution time of a query.

### `FileStreamReader`
Handles buffered reading of a file in fixed-size chunks. Opens the file in binary mode and reads `bufferSize` bytes at a time, ensuring the entire file is never loaded into memory.

### `Tokenizer`
Processes raw character buffers into lowercase alphanumeric tokens. Maintains a `carry` string to correctly handle tokens that are split across buffer boundaries.

### `VersionedIndex`
Stores a word-frequency map (`unordered_map`) for a single file version. Provides overloaded `count()` methods and a `topK()` method for sorted results.

### `QueryEngine`
Manages multiple `VersionedIndex` instances keyed by version name. Provides safe lookup with exception handling.

### `Query` (Abstract Base Class)
Defines the interface for all query types via pure virtual `execute()` and `description()` methods.

### `WordQuery`, `DiffQuery`, `TopKQuery` (Derived Classes)
Concrete implementations of `Query`, each handling one query type with runtime polymorphism.

---

## C++ Features Demonstrated

| Feature | Where Used |
|---------|-----------|
| **Template** | `Timer<Clock>` class |
| **Inheritance** | `WordQuery`, `DiffQuery`, `TopKQuery` extend `Query` |
| **Runtime Polymorphism** | `query->execute(engine)` via virtual dispatch |
| **Function Overloading** | `VersionedIndex::count(word)` and `count(word, bool)` |
| **Exception Handling** | `try/catch` in `main`, `throw` in `QueryEngine::get()` and `FileStreamReader` |
| **Smart Pointers** | `unique_ptr<Query>` for query ownership |

---

## Compilation

```bash
g++ -O2 rollnumber_firstname.cpp -o analyzer
```

Requires C++14 or later.

---

## Usage

### Word Count Query
```bash
./analyzer --file <path> --version <name> --buffer <kb> --query word --word <token>
```

### Top-K Query
```bash
./analyzer --file <path> --version <name> --buffer <kb> --query top --top <k>
```

### Difference Query
```bash
./analyzer --file1 <path1> --version1 <name1> --file2 <path2> --version2 <name2> --buffer <kb> --query diff --word <token>
```

**Buffer size must be between 256 and 1024 KB.**

---

## Example Commands and Output

### Word Count
```bash
$ ./analyzer --file test_logs.txt --version v1 --buffer 256 --query word --word error
Version: v1
Count: 605079
Buffer Size (KB): 256
Execution Time (s): 1.31127
```

### Top-10 Frequent Words
```bash
$ ./analyzer --file test_logs.txt --version v1 --buffer 256 --query top --top 10
Top-10 words in version v1:
devops 1209558
debug 605150
error 605079
...
Buffer Size (KB): 256
Execution Time (s): 1.30456
```

### Difference Query
```bash
$ ./analyzer --file1 test_logs.txt --version1 v1 --file2 verbose_logs.txt --version2 v2 --buffer 256 --query diff --word error
Difference (v2 - v1): -495377
Buffer Size (KB): 256
Execution Time (s): 2.7623
```

---

## Memory Efficiency

- Only the fixed-size buffer and the word-frequency map (growing with unique words only) are kept in memory.
- The entire file is never stored in memory.
- The `carry` string in `Tokenizer` ensures correct tokenization across buffer boundaries.

---

## Constraints

- Buffer size: 256 KB – 1024 KB (validated at runtime; throws exception otherwise)
- Words: contiguous sequences of alphanumeric characters, matched case-insensitively
- All input is provided via command-line arguments only
