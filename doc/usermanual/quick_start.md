Quick Start
===========

This chapter serves as a swift introduction to for users who prefer to start quickly and learn the details while simulating. The typical user should skip the next paragraphs and continue reading the following chapters instead.

is a generic simulation framework for pixel detectors. It provides a modular, flexible and user-friendly structure for the simulation of independent detectors in arbitrary configurations. The framework currently relies on the Geant4 , ROOT  and Eigen3  libraries which need to be installed and loaded before using .

The minimal, default installation can be obtained by executing the commands listed below. More detailed installation instructions can be found in Chapter [ch:installation].

    $ git clone https://gitlab.cern.ch/allpix-squared/allpix-squared
    $ cd allpix-squared
    $ mkdir build && cd build/
    $ cmake ..
    $ make install
    $ cd ..

The binary can then be executed with the provided example configuration file as follows:

    $ bin/allpix -c examples/example.conf

Hereafter, the example configuration can be copied and adjusted to the needs of the user. This example contains a simple setup of two test detectors. It simulates the whole chain, starting from the passage of the beam, the deposition of charges in the detectors, the carrier propagation and the conversion of the collected charges to digitized pixel hits. All generated data is finally stored on disk in ROOT TTrees for later analysis.

After this quick start it is very much recommended to proceed to the other chapters of this user manual. For quickly resolving common issues, the in Chapter [ch:faq] may be particularly useful.
