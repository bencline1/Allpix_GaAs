# DepositionBichsel
**Maintainer**: Simon Spannagel (<simon.spannagel@desy.de>), Daniel Pitzl (<daniel.pitz@desy.de>)  
**Status**: Functional  
**Output**: DepositedCharge, MCParticle

### Description

[@bichsel], [@mazziotta], elastic scattering [@chaoui], photoabsorption [@fraser]


`fast` mode: number of charge carriers created is drawn from Poisson distribution around available energy divided by charge creation energy.
Resulting number of charge carriers is fluctuated using Gaussian distribution with charge carriers as mean and charge carriers times Fano factor as width.


Tracking between multiple detectors is peformed in vacuum, i.e. no additional scattering in other materials than the sensors is considered.
The intersection between a particle track and the sensor volume is performed using the Liang-Barsky line-clipping algorithm [@lineclipping].

### Parameters
* `temperature`:
* `delta_energy_cut`: Lower cut-off energy for the production of delta rays. Defaults to `9keV`.
* `energy_threshold`: Energy threshold above which ionization is considered in inelastic scatterings. Defaults to 1.5 times the band gap energy at the given temperature.
* `charge_creation_energy_`: Defaults to `3.64eV`
* `fano_factor`: Fano factor for charge carrier generation fluctuation, defaults to `0.115`.
* `data_paths`: Paths that should be searched for the tabulated data files in addition to the standard installation path.
* `fast`
* `particle_type`
* `source_position`
* `source_energy`: Kinetic energy of the incoming particle.
* `source_energy_spread`
* `beam_direction`
* `beam_size`
* `beam_divergence`
*

#### Plotting
* `output_plots`
* `output_event_displays`
* `output_plots_use_equal_scaling`
* `output_plots_align_pixels`
* `output_plots_theta`
* `output_plots_phi`

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
[@chaoui]: https://doi.org/10.1088/0953-8984/18/45/016
[@fraser]: https://doi.org/10.1016/0168-9002(94)91185-1
[@lineclipping]: Liang, Y. D., and Barsky, B., "A New Concept and Method for Line Clipping", ACM Transactions on Graphics, 3(1):1–22
