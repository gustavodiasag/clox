# Amortized analysis

Method for analysing a given algorithm's complexity or how much of a resource, especially time or memory, it takes to execute. The motivation for amortized analysis is that looking at the worst-case time per operation can be too pessimistic if the only way to produce an expensive operation is to set it up with a large number of cheap operations beforehand.

For short, the amortized cost of *n* operations is the total cost of the operations divided by *n*. [Examples](https://www.cs.cmu.edu/afs/cs/academic/class/15451-s07/www/lecture_notes/lect0206.pdf).

# Updating memory in C

Since `realloc()` only takes a pointer to the first byte of a block of memory, under the hood, the memory allocator maintains additional bookkeping information for each block of heap-allocated memory, including its size.

Given a pointer to some previously allocated memory, it can find this bookkeping information, which is necessary to be able to cleanly free it. It's this size metadata that `realloc()` updates.

# Value representation

For small fixed-size values like integers, many instruction sets store the value directly in the code stream right after the operational code. These are called **immediate instructions** because the bits for the value are immediately after the opcode.

However, that doesn't work well for large or variable-sized constants. Those are instead stored in a separate "constant data" region in the binary executable. Then, the instruction to load a constant has an address pointing to where the value is stored in that section. Most virtual machines do something similar, like the JVM's [constant pools](https://docs.oracle.com/javase/specs/jvms/se7/html/jvms-4.html#jvms-4.4).

# Register-based bytecode

Another family of bytecode architectures. In a register-based virtual machine a stack is also used, but there's the difference that instructions can read their inputs from anywhere in the stack and can store the outputs into specific stack slots. As an example, consider the Lox script below:

```
var a = 1;
var b = 2;
var c = a + b;
```

In a stack-based virtual machine, the last statement will compile to something like:

```
load <a>    // Read local variable a and push it onto the stack.
load <b>    // Read local variable b and push it onto the stack.
add         // Pop two values, add them and push the result.
store <c>   // Pop value and store in local variable c.
```

Which contains four separate instructions, four bytes to represent each of them, three bytes to represent each operand and three pushes and pops from the stack. Using a [register-based](https://en.wikipedia.org/wiki/Register_allocation) instruction set though, on which instructions can read from and store **directly into local variables**, there's only a single instruction to decode and dispatch, that can be represented with only four bytes and pushing and popping operations are no longer needed:

```
add <a> <b> <c> // Read values from a and b, add them and store in c.
```

# Structure padding in C

While allocating memory for a structure, the C compiler may adjust the amount of space used by it to optimize read operations on the processor, considering that it is able to obtain a whole word whose size is specified by the computer architecture. For that, one or more empty bytes are inserted or left empty between memory addresses which are allocated for structure members, resulting in an alignment of the data.

# Hashing terms

- **Collisions**: keys mapping to the same entries in a table.

- **Load factor**: the number of entries divided by the total size of the table.

- **Birthday paradox**: as the number of entries in the table increases, the chance of collision quickly rises.

- **Separate chaining**: instead of each bucket containing a single entry, it stores a collection of them.

- **Open adressing**: technique that handles collisions internally, all entries live direcly in the table.

- **Clustering**: overflow of buckets right next to each other due to similar key values.

- **Probing**: process of finding an available bucket.

- **Probe sequence**: the order that the buckets are examined.

# Tombstones

While deleting items from a hash table that uses open addressing to handle collisions, it may occur that the bucket with a key being deleted is part of one or more implicit probe sequences. Considering that a probe sequence reaches its end when an entry is empty, simply clearing it leaves trailing entries unreachable. To solve this, **tombstones** are used for deletion, so instead of clearing an entry, it is replaced by a sentinel. This way, if a tombstone is found while following a probe sequence during lookup, the process isn't stopped. Tombstones are also used during insertion so that the number of unused entries in the table's bucket array does not waste unnecessary space.

# First-class functions

A programming language is said to have **first-class functions** when functions in that language are treated like any other variable. For example, in such a language, a function can be passed as an ragument to other functions, can be returned by another function and can be assigned as a value to a variable. An example would be:

```js
const nums = [1, 2, 3, 4, 5]

const addOne = (n) => n + 1

const added = nums.map(addOne) // [2, 3, 4, 5, 6]
```