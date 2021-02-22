# DepositionBichsel
**Maintainer**: Simon Spannagel (<simon.spannagel@desy.de>), Daniel Pitzl (<daniel.pitz@desy.de>)  
**Status**: Functional  
**Output**: DepositedCharge, MCParticle

### Description

[@bichsel], [@mazziotta]

### Parameters
* `temperature`:
* `delta_energy_cut`: Lower cut-off energy for the production of delta rays.
* `energy_threshold`: Energy threshold above which ionization is considered in inelastic scatterings. Defaults to 1.5 times the band gap energy at the given temperature.
* `source_energy`: Kinetic energy of the incoming particle.
* `data_paths`: Paths that should be searched for the tabulated data files in addition to the standard installation path.
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
