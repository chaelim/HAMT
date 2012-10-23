# Implementation of Hash Array Mapped Trie

C++ Template class for HAMT (Hash Array Mapped Trie)

## Features
 * No initial root hash table required.
   _(Empty hash table just takes 8 bytes in 32 bit build or 12 bytes in 64 bit build.)_
 * No stop the world rehashing.
 * Faster and smaller.
 * Constant add/delete O(1) operations
 * C++ Template implementation can be easily used to any datatype.
 * 32 bit integer and string (ANSI and Unicode) hash key templates are included.

## References
 * [Ideal Hash Trees by Phil Bagwell](http://lampwww.epfl.ch/papers/idealhashtrees.pdf).
 * [Ideal Hash Tries: an implementation in C++](http://www.altdevblogaday.com/2011/03/22/ideal-hash-tries-an-implementation-in-c/).
