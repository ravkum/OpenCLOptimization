Pre-requisites:
1) On BaldEagle machine: 
	1) Install Visual Studio 2012.
	2) Install AMD Catalyst driver.
	3) install AMD APP SDK 2.9.1. 
	

For Linux
=========================
1. cmake .
2. make
3. Run the gaussianFilter binary created in bin/x86_64/Release


Fow Windows
==========================

Steps to compile the project:
1) Open the .sln file in Visual Studio 2012.
2) Change the configuration to Relase | x64.
3) Build the project.


Steps to run the exe:
1) Goto "gaussianFilter -> bin -> Release -> x86_64"
2) Run gaussianFilter.exe.



Exe command line options:
=======================================
1) -i (input image path)
2) -combinedKernel (0 | 1) //0 - Runs two separate kernels for Gaussian and Enhance filter one after other
			// 1 - Runs a combined kernel generating both Gaussian and ENhance filter outputs
3) -zeroCopy (0 | 1) //0 (default) - Device buffer, 1 - zero copy buffer
4) -filtSize (filterSize 3 | 5)
5) -useLds (0 | 1) 	//LDS memory to be used in the kernel or not?
6) -reduceOverhead (0 | 1) : Shows overhead caused by a blocking call after every kernel enqueue.
7) -useIntrinsics (0 | 1) : Uses intrinsics in the kernel.
8) -h  - Prints this help


Example: 
1) To run 5X5 filters on 16 bit/channel input image, run:
 
	gaussianFilter.exe -i Nature_1600x1200.bmp -filtSize 5 -bitWidth 16 -useLds 0 -reduceOverhead 1

2) To run 3X3 filters on 16 bit/channel input image, run:
 
	gaussianFilter.exe -i Nature_1600x1200.bmp -filtSize 3 -bitWidth 16 -useLds 0 -reduceOverhead 1
