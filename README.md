=====================================================================================================================
DFTCalc: A Dynamic Fault Tree calculator for reliability and availability
=====================================================================================================================

DFTCalc calculates the failure probability of a DFT by making use of the compositional semantics of I/O-IMCs. In this process it docks on several state-of- the-art tools and languages: All leaves and gates of the input DFT are expressed by I/O-IMCs with the process algebra language Lotos NT; CADP is used to efficiently compose the individual I/O-IMCs; and the model checker mrmc or IMCA finally calculate the failure probability for a certain point in time. The following section describes how DFTCalc aligns all these tools and formats to orchestrate the analysis of a DFT.

DFTCalc takes as input a DFT in Galileo’s textual format. This intuitive format describes a DFT top-down from its root to the basic components. Each subtree is identified by a name, logically connected with other subtrees by gates, and then refined down to the basic components. On execution, DFTCalc processes a given DFT in various stages and analyzes the system’s reliability. The tool’s output is a quantification of this attribute which is expressed as either the failure probability for a given mission time or the mean time to failure.

=====================================================================================================================
Supported systems
=====================================================================================================================

- GNU/Linux (tested on Ubuntu, Debian)
- MacOS X (tested on 10.9, needs a seperate build of imc2ctmdp if MRMC is used)
- Windows may work with use of cygwin (not tested)

=====================================================================================================================
Build Dependencies
=====================================================================================================================

We list the external libraries and tools which are required to build this software.

**CADP**
See the [CADP website](http://www.inrialpes.fr/vasy/cadp/) on how to obtain a license and download the CADP toolkit.

**Coral/imc2ctmdp**
Download Coral containing a 64bit binary version of imc2ctmdp for Linux from [here](http://fmt.ewi.utwente.nl/tools/dftcalc/coral_64bit_cadp2009h.tar.gz). If you experience problems or need a source version of imc2ctmdp please contact us.

**MRMC Backend**
Download MRMC from [the MRMC homepage](http://www.mrmc-tool.org/).

**IMCA Backend**
Download the most recent IMCA version from [github](https://github.com/utwente-fmt/imca) and visit for more information the [IMCA homepage](http://www-i2.informatik.rwth-aachen.de/imca/index.html).

=====================================================================================================================
Installation
=====================================================================================================================

**Create Makefiles**
$ cmake

**Build**
$ make

**Install**
$ make install
