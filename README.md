pfordelta_for_adios_query
=========================
A. INTRODUCTION
This software is an extension to the PForDelta algorithm, an efficient compression/decompression algorithm to encode a list of integers into a bit stream and decode a stream of bits into integers.
Besides the general implementation of the PForDelta, we have two additional components. 
First, we optimize the PForDelta with run-length encoding and bit expansion to improve the storage footprint of the compressed data and the decoding speed. 
Also, the output of the encode API is a bitmap, rather than a bit stream. We make this change to speed up the query processing, in which a set intersection or union operation might be performed.
The main idea is described in the paper [1]. 
Second, we enhance the APIs with spatial constraint, selection boxes. The output of the decode API is still a bitmap, but within a selection box. 
All these functions are tight to the query engine implemented in the ADIOS software package. And thus, they are not intended to general users.   

B. LIMITATION
The integers that can be encoded in this project are those 32-bit integers. We dont support to encode the integers with 64 bits.

C. COMPILATION AND INSTALLATION
This package is compiled successfully with g++ 4.8.2, and it requires boost package.
Before compiling the package, please ensure the install path of boost is correctly specified in the Makefile ( 1st line ).
In addition, to install the package, please specify the right path (2nd line in the Makefile) of the package is going to been installed. 
To compile it, just do the following command:  
make all
To install it after the success compilation, do the following command: 
make install 

D. REFERENCES
[1] X. Zou, S. Lakshminarasimhan, D. A. Boyuka II, S. Ranshous, et al. Fast set intersection through run-time bitmap construction over PForDelta compressed indexes. In Proc. Euro-Par, 2014.