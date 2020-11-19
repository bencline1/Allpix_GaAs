Quick Start
===========

This chapter serves as a swift introduction to Allpix² for users who
prefer to start quickly and learn the details while simulating. The
typical user should skip the next paragraphs and continue reading the
following chapters instead.

Allpix² is a generic simulation framework for pixel detectors. It
provides a modular, flexible and user-friendly structure for the
simulation of independent detectors in arbitrary configurations. The
framework currently relies on the Geant4[^1], ROOT[^2] and
Eigen3[^8] libraries which need to be installed and loaded before
using Allpix².

The minimal, default installation can be obtained by executing the
commands listed below. More detailed installation instructions can be
found in Chapter [Installation](installation.md).
```
    $ git clone https://gitlab.cern.ch/allpix-squared/allpix-squared
    $ cd allpix-squared
    $ mkdir build && cd build/
    $ cmake ..
    $ make install
    $ cd ..
```
The binary can then be executed with the provided example configuration
file as follows:
```
    $ bin/allpix -c examples/example.conf
```
Hereafter, the example configuration can be copied and adjusted to the
needs of the user. This example contains a simple setup of two test
detectors. It simulates the whole chain, starting from the passage of
the beam, the deposition of charges in the detectors, the carrier
propagation and the conversion of the collected charges to digitized
pixel hits. All generated data is finally stored on disk in ROOT TTrees
for later analysis.

After this quick start it is very much recommended to proceed to the
other chapters of this user manual. For quickly resolving common issues, Chapter [FAQs](faq.md) may be particularly useful.

[^1]:S. Agostinelli et al. “Geant4 - a simulation toolkit”. In: Nucl. Instr. Meth. A 506.3 (2003), pp. 250–303. issn: 0168-9002. doi: 10.1016/S0168-9002(03)01368-8.
[^2]:Rene Brun and Fons Rademakers. “ROOT - An Object Oriented Data Analysis Framework”. In: AIHENP’96 Workshop, Lausanne. Vol. 389. Sept. 1996, pp. 81–86.
[^8]:Gaël Guennebaud, Benoît Jacob, et al. Eigen v3. 2010. url: [http://eigen.tuxfamily.org](http://eigen.tuxfamily.org).