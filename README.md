# C++ Template class implementation of Hash Array Mapped Trie

Do you want space-efficient and fast hash table? HAMT is just here for you. Based on the paper [Ideal Hash Trees by Phil Bagwell](http://lampwww.epfl.ch/papers/idealhashtrees.pdf), and as the title stated, it has really ideal features as a hash table as below.

## Features
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

## More information
 * [Ideal Hash Trees by Phil Bagwell](http://lampwww.epfl.ch/papers/idealhashtrees.pdf).
 * [Wikipedia on Hash array mapped trie](http://en.wikipedia.org/wiki/Hash_array_mapped_trie).
 * [Ideal Hash Tries: an implementation in C++](http://www.altdevblogaday.com/2011/03/22/ideal-hash-tries-an-implementation-in-c/).
