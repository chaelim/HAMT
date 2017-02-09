[![Build status](https://ci.appveyor.com/api/projects/status/qtyham872bkcbdwm/branch/dev?svg=true)](https://ci.appveyor.com/project/chaelim/hamt)

C++ Template class implementation of Hash Array Mapped Trie
================================

Do you want space-efficient and fast hash table? HAMT is just here for you. Based on the paper [Ideal Hash Trees by Phil Bagwell](http://lampwww.epfl.ch/papers/idealhashtrees.pdf), and as the title stated, it has really ideal features as a hash table as below.

Features
-------------------------
 * No initial root hash table required.
   _(Empty hash table just takes 8 bytes in 32 bit build or 12 bytes in 64 bit build.)_
 * No stop the world rehashing.
 * Faster and smaller.
 * Constant add/delete O(1) operations
 * C++ Template implementation can be easily used to any data type.
 * 32 bit hash key and 32 bit bitmap to index subhash array.
 * 32 bit integer and string (ANSI and Unicode) hash key templates are included.
 * Expected tree depth: ![equation](http://latex.codecogs.com/gif.latex?O%28%5Clog_%7B2%5EW%7D%28n%29%29).  
     w = 5  
     n : number of elements stored in the trie
 * Hamming weight of bitmap can be caculated using POPCNT(Population count) CPU intruction (introduced in Nehalem-base and Barcelona microarchitecture CPU). POPCNT can speed up overall performance about 10%.

Test program build notes
-------------------------
 * Open and compile Test\IdealHash.sln
 * To enable POPCNT CPU instruction, change 0 to 1 in "#define SSE42_POPCNT 0". POPCNT is SSE4 CPU instruction start supported since Intel Nehalem and AMD Barcelona.
 * References on POPCNT:
    - http://en.wikipedia.org/wiki/SSE4#POPCNT_and_LZCNT
    - http://developer.amd.com/community/blog/barcelona-processor-feature-advanced-bit-manipulation-abm/

More information
-------------------------
 * [Ideal Hash Trees by Phil Bagwell](http://lampwww.epfl.ch/papers/idealhashtrees.pdf).
 * [Wikipedia on Hash array mapped trie](http://en.wikipedia.org/wiki/Hash_array_mapped_trie).
 * [Ideal Hash Tries: an implementation in C++](http://www.altdevblogaday.com/2011/03/22/ideal-hash-tries-an-implementation-in-c/).
 * [POPCNT Instruction](http://en.wikipedia.org/wiki/SSE4#POPCNT_and_LZCNT)

