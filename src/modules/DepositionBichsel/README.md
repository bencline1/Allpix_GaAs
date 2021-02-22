# DepositionBichsel
**Maintainer**: Simon Spannagel (<simon.spannagel@desy.de>), Daniel Pitzl (<daniel.pitz@desy.de>)  
**Status**: Functional  
**Output**: DepositedCharge, MCParticle

### Description

[@bichsel], [@mazziotta]

### Parameters
* `temperature`:
* `delta_energy_cut`
* `energy_threshold`
* `source_energy`
* `data_paths`
* `fast`
* `output_plots`

### Usage

```toml
[DepositionBichsel]
number_of_steps = 100
position = -10um 10um 0um
model = "fixed"
source_type = "mip"
```


[@bichsel]: http://prola.aps.org/abstract/RMP/v60/i3/p663_1
[@mazziotta]: https://doi.org/10.1016/j.nima.2004.05.127
