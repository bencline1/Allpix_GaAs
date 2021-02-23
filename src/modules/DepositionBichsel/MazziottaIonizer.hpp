/**
 * @file
 * @brief Definition of shell ionization mechanisms following Mazziotta
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <array>
#include <random>
#include <stack>

#include <Math/Vector3D.h>

namespace allpix {

    /**
     * @brief Class to calculate ionization and photoabsorption in different shells
     */
    class MazziottaIonizer {
    public:
        /**
         * @brief Constructor of ionizer class, also calculates Auger probability integrals
         * @param random_engine pseudo-random number generator to be used
         */
        MazziottaIonizer(std::mt19937_64* random_engine);

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
         * @param energy_auger   [description]
         * @param veh            [description]
         */
        void transition(double energy_auger, std::stack<double>& veh);

        std::mt19937_64* random_engine_{nullptr};
        std::uniform_real_distribution<double> uniform_dist_{0, 1};
        double uniform() {
            if(random_engine_ == nullptr) {
                exit(1);
            }
            return uniform_dist_(*random_engine_);
        };

        std::vector<double> intervals{-1, 0, 1};
        std::vector<double> probabilities{1, 0, 1};
        std::piecewise_linear_distribution<double> triangular_dist_{
            intervals.begin(), intervals.end(), probabilities.begin()};
        double triangular() {
            if(random_engine_ == nullptr) {
                exit(1);
            }
            return triangular_dist_(*random_engine_);
        };

        // Shells
        // Possible transitions to this shell:
        const std::array<int, 5> nvac{{0, 0, 2, 2, 9}};
        // [1]: valence band upper edge (holes live below)
        // [2]: M, [3]: L, [4]: K shells
        const std::array<double, 5> energy_shell{{
            0,
            12.0,
            99.2,
            148.7,
            1839.0,
        }};
        const double energy_valence_ = energy_shell[1]; // 12.0 eV

        double auger_prob_integral[5][10];
        double auger_energy[5][10];

        // EPP(I) = VALORI DI ENERGIA PER TABULARE LE PROBABILITA" DI FOTOASSORBIMENTO NELLE VARIE SHELL
        // PM, PL23, PL1, PK = PROBABILITA" DI ASSORBIMENTO DA PARTE DELLE SHELL M,L23,L1 E K

        // VALORI ESTRAPOLATI DA FRASER
        const std::array<double, 14> EPP{
            {0.0, 40.0, 50.0, 99.2, 99.2, 148.7, 148.7, 150.0, 300.0, 500.0, 1000.0, 1839.0, 1839.0, 2000.0}};
        const std::array<double, 14> PM{{0, 1.0, 1.0, 1.0, 0.03, 0.03, 0.02, 0.02, 0.02, 0.02, 0.03, 0.05, 0.0, 0.0}};
        const std::array<double, 14> PL23{{0, 0.0, 0.0, 0.0, 0.97, 0.92, 0.88, 0.88, 0.83, 0.70, 0.55, 0.39, 0.0, 0.0}};
        const std::array<double, 14> PL1{{0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.1, 0.1, 0.15, 0.28, 0.42, 0.56, 0.08, 0.08}};
        const std::array<double, 14> PK{{0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.92, 0.92}};
    };
} // namespace allpix
