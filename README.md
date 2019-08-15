# Operating Systems - CS330 Assignments

This repo was created as a part of Operating Systems course. We were supposed to use [Gem5 Architectural Simulator](http://gem5.org/Main_Page) to create some basic functionalities of an Operating System. The course was undertaken under Prof. Debadatta Mishra.

For the first assignment, I implemented Page Table Walking and the whole logic behind it. We used multi-level page tables and learned the whole concept of virtual to physical memory mappings.

For the second assignment, I worked on privileged mode expceptions (like div-by-zero, page-faults) and operations like `write`, `expand` and `shrink` for memory pages.

For the third assignment, I implemented various signals (like `SIGSEGV`, `SIGFPE`, `SIGALRM`), syscalls like `sleep()` and `clone()` and a round-robin scheduling policy for the newly cloned process.

The final assignment happened in user-land and I worked on implemented a FUSE-based file system and opted for an [ext-2 like design](https://www.nongnu.org/ext2-doc/ext2.html).

--

AUTHOR: Mayank Sharma
## Assignment 1
