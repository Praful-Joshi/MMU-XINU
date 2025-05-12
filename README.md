# 📦 XINU MMU Project

### Project Overview

This project extends a barebones version of the Xinu operating system by adding full **demand paging** support, including an **MMU (Memory Management Unit)** implementation.  
It introduces paging-based virtual memory with dynamic allocation, a backing store system, page fault handling, and page replacement policies.

### Key Features

- **Demand Paging:**  
  Pages are loaded from the backing store on-demand when page faults occur.
  
- **Page Fault Handler:**  
  Custom ISR written to allocate page tables or physical frames at runtime.
  
- **Backing Store Management:**  
  Backing store (`bsm_tab`) tracks mappings of virtual pages to swap space.

- **Frame Table Management:**  
  Physical memory frames are tracked with metadata like owner process, page number, dirty status, etc.

- **Page Replacement Policy:**  
  - Implemented **Second Chance (SC)** algorithm for frame eviction.
  - Support for policy switching (`srpolicy()`/`grpolicy()`).

- **Integration with Process Management:**  
  - Private page directories per process.
  - Cleanup of page frames on process termination (`kill()`).
  - Save dirty pages back to backing store during context switches (`resched()`).

- **Low-Level Changes:**  
  - Modified system initialization to support paging (`sysinit()`).
  - Set up the Global Descriptor Table (GDT) and CR3 register.
  - Created a paging interrupt handler assembly stub (`pfintr.S`).

---

### Directory Structure

New&Modified_Files folder has all the code that we have added or modified.

| Folder | Description |
|:---|:---|
| `sys/` | Core Xinu kernel modules (scheduler, process management, etc.) |
| `paging/` | New MMU implementation: frame, backing store, page fault handling |
| `com/`, `tty/`, `mon/`, `lib/libxc/` | Existing Xinu drivers and libraries |
| `config/`, `compile/` | Build configuration and makefiles |
| `h/` | C header files including `paging.h` (new structures) |
| `misc/` | Testing and documentation materials |
| `TMP_pjgandh3_pjoshi7_byan4_svasude5/` | Temporary backup of early modified source files |

---

### Getting Started

1. **Build:**  
   Navigate to the `compile/` directory and run `make` to compile the OS.

2. **Run:**  
   Boot the generated binary in a PC emulator or test setup that supports Xinu.

3. **Test Paging:**  
   Programs using large memory spaces or artificially triggering page faults can be used to validate demand paging behavior.

---

### Authors

- Team Members:  
  - `pjgandh3`
  - `pjoshi7`
  - `byan4`
  - `svasude5`

---

### Notes

- **Current page replacement policy**: Second Chance (SC).
- **Max number of backing stores:** 8.
- **Max frames available:** 1024 frames of 4 KB each.
- Some files are intentionally backed up in `TMP_pjgandh3_pjoshi7_byan4_svasude5/` for archival reasons.

---