# InjectMessage
**Maintainer**: Simon Spannagel (simon.spannagel@cern.ch)  
**Status**: Functional  
**Output**: *configurable*

### Description
This module allows to generate random objects of different types and to dispatch them as messages to the framework.

In particular, this module can be used to generate random input data to the framework by specifying data types for which objects should be created and dispatched as messages.
This is especially useful for unit testing of individual modules that require input from previous simulation stages.

### Parameters
* `messages`: Array of object names (without `allpix::` prefix) to be generated and dispatched as messages.

### Usage
*Example how to use this module*

```ini
[InjectMessage]
messages = ["DepositedCharge", "MCParticle"]
```
