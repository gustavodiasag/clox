# Amortized analysis

Method for analysing a given algorithm's complexity or how much of a resource, especially time or memory, it takes to execute. The motivation for amortized analysis is that looking at the worst-case time per operation can be too pessimistic if the only way to produce an expensive operation is to set it up with a large number of cheap operations beforehand.

For short, the amortized cost of *n* operations is the total cost of the operations divided by *n*. [Examples](https://www.cs.cmu.edu/afs/cs/academic/class/15451-s07/www/lecture_notes/lect0206.pdf).

# Updating memory in C

Since `realloc()` only takes a pointer to the first byte of a block of memory, under the hood, the memory allocator maintains additional bookkeping information for each block of heap-allocated memory, including its size.

Given a pointer to some previously allocated memory, it can find this bookkeping information, which is necessary to be able to cleanly free it. It's this size metadata that `realloc()` updates.

# Value representation

For small fixed-size values like integers, many instruction sets store the value directly in the code stream right after the operational code. These are called **immediate instructions** because the bits for the value are immediately after the opcode.

However, that doesn't work well for large or variable-sized constants. Those are instead stored in a separate "constant data" region in the binary executable. Then, the instruction to load a constant has an address pointing to where the value is stored in that section. Most virtual machines do something similar, like the JVM's [constant pools](https://docs.oracle.com/javase/specs/jvms/se7/html/jvms-4.html#jvms-4.4).