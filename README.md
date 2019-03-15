# DFTCalc: A Dynamic Fault Tree calculator for reliability and availability

DFTCalc calculates the failure probability of a DFT by making use of the compositional semantics of I/O-IMCs. In this process it docks on several state-of- the-art tools and languages: All leaves and gates of the input DFT are expressed by I/O-IMCs with the process algebra language Lotos NT; CADP is used to efficiently compose the individual I/O-IMCs; and a model checker (Storm, MRMC, IMRMC, or IMCA) finally calculate the failure probability for a certain point in time. The following section describes how DFTCalc aligns all these tools and formats to orchestrate the analysis of a DFT.

DFTCalc takes as input a DFT in Galileo’s textual format. This intuitive format describes a DFT top-down from its root to the basic components. Each subtree is identified by a name, logically connected with other subtrees by gates, and then refined down to the basic components. On execution, DFTCalc processes a given DFT in various stages and analyzes the system’s reliability. The tool’s output is a quantification of this attribute which is expressed as either the failure probability for a given mission time or the mean time to failure.

## Supported systems

- GNU/Linux (tested on Ubuntu, Debian)
- MacOS X (tested on 10.9, needs a seperate build of imc2ctmdp if MRMC is used)
- Windows may work with use of cygwin (not tested)

## Build Dependencies

To build and run DFTCalc, you need some external libraries. Which ones
exactly depends on which options you wish to use. Below we list the
external libraries and tools required. In any operation you require
CADP. Beyond that, you need either MRMC, IMRMC, IMCA, or Storm. For
exact operation (the --exact switch), you need IMRMC and DFTRES.

**CADP**
See the [CADP website](http://www.inrialpes.fr/vasy/cadp/) on how to obtain a license and download the CADP toolkit.

**MRMC Backend**
Download MRMC from [the MRMC homepage](http://www.mrmc-tool.org/),
and ensure that the 'mrmc' binary is in your PATH.

**Coral/imc2ctmdp**
Only needed for the MRMC backend. Since the 2015 version of CADP you have to compile imc2ctmdp yourself. You can get the code from [here] (https://github.com/buschko/imc2ctmdp).

**IMRMC Backend**
Download IMRMC from [the IMRMC homepage](https://www.ennoruijters.nl/imrmc.html)
and ensure that the 'imrmc' binary is in your PATH.

**IMCA Backend**
Download the most recent IMCA version from [github](https://github.com/utwente-fmt/imca) and visit for more information the [IMCA homepage](http://www-i2.informatik.rwth-aachen.de/imca/index.html). Ensure that the 'imca' binary is in your PATH.

**Storm Backend**
Download the most recent Storm version from [the Storm
homepage](http://www.stormchecker.org/) and ensure that the 'storm'
binary is in your PATH.

**DFTRES Converter**
For the 'exact' mode, download and build the most recent version of
DFTRES from [github](https://github.com/utwente-fmt/DFTRES) and set an
environment variable 'DFTRES' to point to the generated jar file.

## Installation

**Create Makefiles**
$ mkdir build && cd build && cmake ..

**Build**
$ make

**Install**
$ make install
