nmergec
=======

C based revision of NMergeNew (Java)

This is a revision of NMergeNew in the C language. 

The planned improvements include:

Simplify the alignment process by splitting up the existing functionality of 
nmerge into a series of plugins. The only tasks that the core nmerge program 
will perform will be to load and save MVD files, and to manage the plugins, of 
course.

Rewrite the entire program from scratch in the C language. This should overcome 
memory problems with Java by dynamically allocating memory only as needed, 
instead of wastefully at present. Also C gives the program great longevity and 
portability as well as speed. Also provide language wrappers so it can be called 
natively in PHP (as an extension) and Java (via JNI).

Use multi-threading to improve performance. Individual sub-alignments and 
building of suffix trees can carry on simultaneously.

Transpositions can be computed using a technique that exploits adjacency of 
short transposed sections. In this way even transpositions containing minor 
revisions can be detected. This should improve alignment quality.

Alignment will be by shuffling the fragments of the MVD, not by pasting in 
differences into a explicit variant graph. This should greatly improve the 
program's simplicity.

Changing the MVD file format so that versions and groups are merged into 
version-IDs. This should make version specification simpler by using a 
hierarchical naming system based on paths like /folios/F1, or /C376/add0, rather 
than on tables of separate groups and versions.

Change the default text encoding from UTF-8 to UTF-16. This will allow easy 
comparison between Chinese and other languages like Bengali, which split almost 
all characters across byte-boundaries.

Provide a test-suite to verify every aspect of the program as it is being 
written and to insulate it from damage if any changes are made later.

