/**
 * @file
 * @brief Definition of shell ionization mechanisms following Mazziotta
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <array>
#include <stack>

#include "core/utils/distributions.h"
#include "core/utils/prng.h"

#include <Math/Vector3D.h>

namespace allpix {

    /**
     * @brief Class to calculate ionization and photoabsorption in different shells
     */
    class PhotoAbsorptionIonizer {
    public:
        /**
         * @brief Constructor of ionizer class, also calculates Auger probability integrals
         * @param random_engine pseudo-random number generator to be used
         */
        PhotoAbsorptionIonizer(RandomNumberGenerator& random_generator);

        /**
         * @brief Function to calculate electron-hole pairs and their energy from ionization
         *
         * @param  energy_gamma Energy of the incoming (virtual) photon interacting with the shells
         * @return              List of e-h pairs with the value being their respective energy
         */
        std::stack<double> getIonization(double energy_gamma);

    private:
        /**
         * Helper to calculate shell transition process
         *
         * @param energy_auger   Emission energy for Auger electron from the shell in question
         * @param veh            Stack of e-h pairs to be amended
         */
        void transition(double energy_auger, std::stack<double>& veh);

        /**
         * Random number generator and distributions
         */
        RandomNumberGenerator* random_engine_{nullptr};
        allpix::uniform_real_distribution<double> uniform_dist_{0, 1};
        std::vector<double> intervals{-1, 0, 1};
        std::vector<double> probabilities{1, 0, 1};
        allpix::piecewise_linear_distribution<double> triangular_dist_{
            intervals.begin(), intervals.end(), probabilities.begin()};

        /**
         * Helper to obtain pseudo-random number from uniform distrioution [0, 1]
         * @return Random number from uniform distribution
         */
        double uniform();

        /**
         * Helper to obtain pseudo-random number from inverse triangular distrioution [-1, 1]
         * @return Random number from inverse triangular distribution
         */
        double triangular();

        /**
         * Definition of shells
         *
         * Shells are enumerated using the following indices:
         * 0: (unused)
         * 1: Valence band, upper edge
         * 2: M shell
         * 3: L shell
         * 4: K shell
         */

        /**
         * Number of possible transitions to the respective shells
         */
        const std::array<int, 5> nvac{{0, 0, 2, 2, 9}};

        /**
         * Energies of the respective shells
         */
        const std::array<double, 5> energy_shell{{
            0,
            12.0,
            99.2,
            148.7,
            1839.0,
        }};
        // Short-hand for valence band edge energy
        const double energy_valence_ = energy_shell[1];

        /**
         * Probability integrals for Auger electron emission for each of the shells.
         * First index indicates shell (2: L23, 3: L1, 4: K), second index enumerates transition processes
         */
        double auger_prob_integral[5][10];

        /**
         * Auger electron emission energy for each of the shells.
         * First index indicates shell (2: L23, 3: L1, 4: K), second index enumerates transition processes
         */
        double auger_energy[5][10];

        /**
         * Probabilities for photoabsorption at different energies in each of the shells M, L23, L1 and K
         *
         * Extrapolated from Fig. 1 in
         * G.W. Fraser, et al., Nucl. Instr. and Meth. A 350 (1994) 368
         * https://doi.org/10.1016/0168-9002(94)91185-1
         */
        const std::array<double, 14> EPP{
            {0.0, 40.0, 50.0, 99.2, 99.2, 148.7, 148.7, 150.0, 300.0, 500.0, 1000.0, 1839.0, 1839.0, 2000.0}};
        const std::array<double, 14> PM{{0, 1.0, 1.0, 1.0, 0.03, 0.03, 0.02, 0.02, 0.02, 0.02, 0.03, 0.05, 0.0, 0.0}};
        const std::array<double, 14> PL23{{0, 0.0, 0.0, 0.0, 0.97, 0.92, 0.88, 0.88, 0.83, 0.70, 0.55, 0.39, 0.0, 0.0}};
        const std::array<double, 14> PL1{{0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.1, 0.1, 0.15, 0.28, 0.42, 0.56, 0.08, 0.08}};
        const std::array<double, 14> PK{{0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.92, 0.92}};
    };
} // namespace allpix
