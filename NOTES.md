# Amortized analysis

Method for analysing a given algorithm's complexity or how much of a resource, especially time or memory, it takes to execute. The motivation for amortized analysis is that looking at the worst-case time per operation can be too pessimistic if the only way to produce an expensive operation is to set it up with a large number of cheap operations beforehand.

For short, the amortized cost of *n* operations is the total cost of the operations divided by *n*. [Examples](https://www.cs.cmu.edu/afs/cs/academic/class/15451-s07/www/lecture_notes/lect0206.pdf).

# Updating memory in C

Since `realloc()` only takes a pointer to the first byte of a block of memory, under the hood, the memory allocator maintains additional bookkeping information for each block of heap-allocated memory, including its size.

Given a pointer to some previously allocated memory, it can find this bookkeping information, which is necessary to be able to cleanly free it. It's this size metadata that `realloc()` updates.