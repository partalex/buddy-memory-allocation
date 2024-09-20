# Buddy Memory Allocation System for Linux

## Overview

This project implements a **Buddy Memory Allocation** system for Linux using **C++** and **semaphores**. The buddy memory allocator is a dynamic memory allocation technique that efficiently manages memory by splitting blocks into smaller, equal-sized pieces until it finds a suitable size for the memory request. Semaphores are used to ensure thread-safe memory management, especially in multithreaded environments.

### Features

- **Buddy Memory Allocation**: Efficiently handles memory allocation and deallocation using the buddy system.
- **Semaphore Usage**: Ensures synchronization across threads for safe memory operations.
- **Linux Support**: Built specifically for Linux environments.

## Requirements

- **C++ Compiler**: Ensure your compiler supports the C++11 standard or later.
- **POSIX Semaphores**: Used for thread synchronization.
- **Linux OS**: The implementation is designed to work on Linux systems.
