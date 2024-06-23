## Multi-Level Page Tables Simulation

## Project Overview

This project simulates an operating system that manages a multi-level (trie-based) page table, designed for a 64-bit x86-like CPU. The project involves creating and querying virtual memory mappings in a page table.

## Files

- `os.c`: Contains helper functions simulating OS functionality.
- `os.h`: Header file with function prototypes and constants.
- `pt.c`: Your implementation of page table management functions.

## Functions

### 1. Create/Destroy Virtual Memory Mappings

void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn);

- **pt**: Physical page number of the page table root.
- **vpn**: Virtual page number to map/unmap.
- **ppn**: Physical page number to map to `vpn`, or `NO_MAPPING` to unmap.

### 2. Check Address Mapping

uint64_t page_table_query(uint64_t pt, uint64_t vpn);

- **pt**: Physical page number of the page table root.
- **vpn**: Virtual page number to query.

Returns the physical page number mapped to `vpn`, or `NO_MAPPING` if no mapping exists.

## Compilation

Compile the project using the following command:

gcc -O3 -Wall -std=c11 os.c pt.c -o page_table_sim

## Usage

Modify the `main()` function in `os.c` to thoroughly test your implementation.

## (This assignment is part of the Operating Systems course at Tel Aviv University).

