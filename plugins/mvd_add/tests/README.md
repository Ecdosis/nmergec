nmergec
=======

C based revision of NMergeNew (Java)

This is a revision of NMergeNew in the C language. The idea is to make 
it faster, improve handling of transpositions, and remove the limit on 
file size, current around 100K.

It's going to have a complete test suite built into it from the start. 
The idea is to call this from the Java REST service although the 
commandline interface will remain.

Handling of transpositions will exploit tadjacency as described in my 
blogpost 
http://multiversiondocs.blogspot.com.au/2012/03/better-way-to-do-transpositions.html
