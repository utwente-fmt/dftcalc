![Compile and Test](https://github.com/utwente-fmt/dftcalc/workflows/Compile%20and%20Test/badge.svg?branch=master)

# DFTCalc: A Dynamic Fault Tree calculator for reliability and availability

DFTCalc calculates the failure probability of a DFT by making use of the compositional semantics of I/O-IMCs. In this process it docks on several state-of- the-art tools and languages: All leaves and gates of the input DFT are expressed by I/O-IMCs; CADP or DFTRES are used to efficiently compose the individual I/O-IMCs; and a model checker (Storm, Modest, MRMC, IMRMC, or IMCA) finally calculate the failure probability for a certain point in time. The following section describes how DFTCalc aligns all these tools and formats to orchestrate the analysis of a DFT.

DFTCalc takes as input a DFT in Galileo’s textual format. This intuitive format describes a DFT top-down from its root to the basic components. Each subtree is identified by a name, logically connected with other subtrees by gates, and then refined down to the basic components. On execution, DFTCalc processes a given DFT in various stages and analyzes the system’s reliability. The tool’s output is a quantification of this attribute which is expressed as either the failure probability for a given mission time or the mean time to failure.

## Supported systems

- GNU/Linux (tested on Fedora, Debian)
- MacOS X (tested on Catalina)
- Windows may work with use of cygwin (not tested)

## Build Dependencies

To build and run DFTCalc, you need some external libraries. Which ones
exactly depends on which options you wish to use. Below we list the
external libraries and tools required. The yaml-cpp library is always
needed. Either CADP or DFTRES is needed: DFTRES is always needed for
the --exact switch, CADP is needed for some unusual gates, in all other
cases you can decide which is desired.
Beyond that, you need at least one of MRMC, IMRMC, IMCA, Storm, or
Modest. For exact operation (the --exact switch), you need DFTRES and
IMRMC, Storm, or Modest.

**yaml-cpp**
Install using your favorite package manager, e.g. 'dnf install yaml-cpp-devel' on Fedora.

**CADP**
See the [CADP website](http://www.inrialpes.fr/vasy/cadp/) on how to obtain a license and download the CADP toolkit.

**MRMC Backend**
Download MRMC from [the MRMC homepage](http://www.mrmc-tool.org/),
and ensure that the 'mrmc' binary is in your PATH.

**Coral/imc2ctmdp**
Only needed for the MRMC backend. Since the 2015 version of CADP you have to compile imc2ctmdp yourself. You can get the code from [here] (https://github.com/buschko/imc2ctmdp). Set your 'CORAL' environment variable to the root of this installation (such that "$CORAL/bin" contains the imc2ctmdp binary).

**IMRMC Backend**
Download IMRMC from [the IMRMC homepage](https://www.ennoruijters.nl/imrmc.html)
and ensure that the 'imrmc' binary is in your PATH.

**IMCA Backend**
Download the most recent IMCA version from [github](https://github.com/utwente-fmt/imca) and visit for more information the [IMCA homepage](http://www-i2.informatik.rwth-aachen.de/imca/index.html). Set the 'IMCA' environment variable to the root of your IMCA installation.

**Storm Backend**
Download the most recent Storm version from [the Storm
homepage](http://www.stormchecker.org/) and ensure that the 'storm'
binary is in your PATH.

**Modest Backend**
Download the most recent Modest version from [the Modest
homepage](http://www.modestchecker.net/) and ensure that running the
'mcsta' command invokes the mcsta tool (e.g., by creating a shell script
'mcsta' in the PATH that starts mcsta).

**DFTRES Converter**
For the 'exact' mode, download and build the most recent version of
DFTRES from [github](https://github.com/utwente-fmt/DFTRES) and set an
environment variable 'DFTRES' to point to the generated jar file.

## Installation

By default, DFTCalc is installed into /opt/dft2lntroot. This can be
changed by passing the '-DDFTROOT=/your/preferred/path' option to cmake.

**Create Makefiles**
$ mkdir build && cd build && cmake ..

**Build**
$ make

**Install**
$ make install
