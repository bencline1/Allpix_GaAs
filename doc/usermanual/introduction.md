Introduction
============

is a generic simulation framework for silicon tracker and vertex detectors written in modern , following the 11 and 14 standards. The goal of the framework is to provide an easy-to-use package for simulating the performance of Silicon detectors, starting with the passage of ionizing radiation through the sensor and finishing with the digitization of hits in the readout chip.

The framework builds upon other packages to perform tasks in the simulation chain, most notably Geant4  for the deposition of charge carriers in the sensor and ROOT  for producing histograms and storing the produced data. The core of the framework focuses on the simulation of charge transport in semiconductor detectors and the digitization to hits in the frontend electronics.

is designed as a modular framework, allowing for an easy extension to more complex and specialized detector simulations. The modular setup also allows to separate the core of the framework from the implementation of the algorithms in the modules, leading to a framework which is both easier to understand and to maintain. Besides modularity, the framework was designed with the following main design goals in mind:

1.  Reflect the physics

    -   A run consists of several sequential events. A single event here refers to an independent passage of one or multiple particles through the setup

    -   Detectors are treated as separate objects for particles to pass through

    -   All relevant information must be contained at the end of processing every single event (sequential events)

2.  Ease of use (user-friendly)

    -   Simple, intuitive configuration and execution (“does what you expect”)

    -   Clear and extensive logging and error reporting capabilities

    -   Implementing a new module should be feasible without knowing all details of the framework

3.  Flexibility

    -   Event loop runs sequence of modules, allowing for both simple and complex user configurations

    -   Possibility to run multiple different modules on different detectors

    -   Limit flexibility for the sake of simplicity and ease of use

has been designed following some ideas previously implemented in the AllPix  project. Originally written as a Geant4 user application, AllPix has been successfully used for simulating a variety of different detector setups.

Scope of this Manual
--------------------

This document is meant to be the primary User’s Guide for . It contains both an extensive description of the user interface and configuration possibilities, and a detailed introduction to the code base for potential developers. This manual is designed to:

-   Guide new users through the installation;

-   Introduce new users to the toolkit for the purpose of running their own simulations;

-   Explain the structure of the core framework and the components it provides to the simulation modules;

-   Provide detailed information about all modules and how to use and configure them;

-   Describe the required steps for adding new detector models and implementing new simulation modules.

Within the scope of this document, only an overview of the framework can be provided and more detailed information on the code itself can be found in the Doxygen reference manual  available online. No programming experience is required from novice users, but knowledge of (modern) will be useful in the later chapters and might contribute to the overall understanding of the mechanisms.

Support and Reporting Issues
----------------------------

As for most of the software used within the high-energy particle physics community, only limited support on best-effort basis for this software can be offered. The authors are, however, happy to receive feedback on potential improvements or problems arising. Reports on issues, questions concerning the software as well as the documentation and suggestions for improvements are very much appreciated. These should preferably be brought up on the issues tracker of the project which can be found in the repository .

Contributing Code
-----------------

is a community project that benefits from active participation in the development and code contributions from users. Users and prospective developers are encouraged to discuss their needs either via the issue tracker of the repository , the forum  or the developer’s mailing list to receive ideas and guidance on how to implement a specific feature. Getting in touch with other developers early in the development cycle avoids spending time on features which already exist or are currently under development by other users.

The repository contains a few tools to facilitate contributions and to ensure code quality as detailed in Chapter [ch:testing].
