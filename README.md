nmergec
=======

C based revision of NMergeNew (Java)

This is a revision of NMergeNew in the C language. The idea is to make 
it faster, improve handling of transpositions, and remove the limit on 
file size, current around 100K.

The program has been split up into a number of dynamic libraries of 
plugins, one for each basic function. The MVD handling has also been 
split off from the main program, as it is needed by each plugin. This 
will make addition of new functionality easy.

nmergec has a complete test suite built into it from the start. 
So any modifications to the code can be quickly tested. 

The nmergec program and its modules will normally be called via JNI and 
the Java REST service, although the commandline interface will remain. 
It could also be turned into a PHP extension.

Handling of transpositions will exploit adjacency. So if two transposed 
sections are separated by a short non-matching section they can be 
assessed together, as if they were one, although the transpositions 
themselves will still be atomic. This should produce more natural 
transposition detection.
