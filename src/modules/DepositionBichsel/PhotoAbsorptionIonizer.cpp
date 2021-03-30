/**
 * @file
 * @brief Implementation of ionization and shell transitions
 * @remarks Based on code from Daniel Pitzel and - in turn - H. Bichsel and M. Mazziotta.
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "PhotoAbsorptionIonizer.hpp"

#include "core/module/exceptions.h"
#include "core/utils/log.h"

using namespace allpix;

PhotoAbsorptionIonizer::PhotoAbsorptionIonizer(RandomNumberGenerator& random_generator) : random_engine_(&random_generator) {

    // Shell energy and probability integral initialization
    for(unsigned n = 1; n <= 4; ++n) {
        for(unsigned i = 1; i <= 9; ++i) {
            auger_prob_integral[n][i] = 0;
            auger_energy[n][i] = 0;
        }
    }

    /**
     * Probability integrals for Auger emissions from different shells through different processes
     */
    // K shell
    auger_prob_integral[4][1] = 0.1920;
    auger_prob_integral[4][2] = 0.3885 + auger_prob_integral[4][1];
    auger_prob_integral[4][3] = 0.2325 + auger_prob_integral[4][2];
    auger_prob_integral[4][4] = 0.0720 + auger_prob_integral[4][3];
    auger_prob_integral[4][5] = 0.0030 + auger_prob_integral[4][4];
    auger_prob_integral[4][6] = 0.1000 + auger_prob_integral[4][5];
    auger_prob_integral[4][7] = 0.0040 + auger_prob_integral[4][6];
    auger_prob_integral[4][8] = 0.0070 + auger_prob_integral[4][7];
    auger_prob_integral[4][9] = 0.0010 + auger_prob_integral[4][8];

    // L1 shell
    auger_prob_integral[3][1] = 0.0250;
    auger_prob_integral[3][2] = 0.9750 + auger_prob_integral[3][1];

    // L23 shell
    auger_prob_integral[2][1] = 0.9990;
    auger_prob_integral[2][2] = 0.0010 + auger_prob_integral[2][1];

    /**
     * Auger electron emission energies for the different shells and processes:
     */
    // K shell
    auger_energy[4][1] = 1541.6;
    auger_energy[4][2] = 1591.1;
    auger_energy[4][3] = 1640.6;
    auger_energy[4][4] = 1690.3;
    auger_energy[4][5] = 1690.3;
    auger_energy[4][6] = 1739.8;
    auger_energy[4][7] = 1739.8;
    auger_energy[4][8] = 1839.0;
    auger_energy[4][9] = 1839.0;

    // L1 shell
    auger_energy[3][1] = 148.7;
    auger_energy[3][2] = 49.5;

    // L23 shell
    auger_energy[2][1] = 99.2;
    auger_energy[2][2] = 0.0;
}

std::stack<double> PhotoAbsorptionIonizer::getIonization(double energy_gamma) {

    std::stack<double> veh;

    unsigned is{};
    if(energy_gamma <= energy_valence_) {
        is = 0;
    } else if(energy_gamma <= EPP[3]) {
        is = 1;
    } else {
        std::array<double, 5> PV;
        if(energy_gamma > EPP[13]) {
            PV[1] = PM[13];
            PV[2] = PL23[13];
            PV[3] = PL1[13];
            PV[4] = PK[13];
        } else {
            unsigned iep = 3;
            // Find relevant energy bin for the gamma energy we're dealing with
            for(; iep < 13; ++iep) {
                if(energy_gamma > EPP[iep] && energy_gamma <= EPP[iep + 1]) {
                    break;
                }
            }

            // Interpolate probabilities for the given energy
            PV[1] = PM[iep] + (PM[iep + 1] - PM[iep]) / (EPP[iep + 1] - EPP[iep]) * (energy_gamma - EPP[iep]);
            PV[2] = PL23[iep] + (PL23[iep + 1] - PL23[iep]) / (EPP[iep + 1] - EPP[iep]) * (energy_gamma - EPP[iep]);
            PV[3] = PL1[iep] + (PL1[iep + 1] - PL1[iep]) / (EPP[iep + 1] - EPP[iep]) * (energy_gamma - EPP[iep]);
            PV[4] = PK[iep] + (PK[iep + 1] - PK[iep]) / (EPP[iep + 1] - EPP[iep]) * (energy_gamma - EPP[iep]);
        }

        // Calculate integral of probabilities for normalization
        double PPV = PV[1] + PV[2] + PV[3] + PV[4];

        PV[2] = PV[1] + PV[2];
        PV[3] = PV[2] + PV[3];
        PV[4] = PV[3] + PV[4];

        double rs = uniform();
        unsigned iv = 1;
        for(; iv <= 4; ++iv) {
            PV[iv] = PV[iv] / PPV; // normalize
            if(PV[iv] > rs) {
                break;
            }
        }
        // Restrict to 4 maximum
        is = std::min(iv, 4u);
    }

    LOG(TRACE) << "Shells for " << energy_gamma << " eV, energy_valence " << energy_valence_ << ", is " << is;

    // PROCESSES:

    // PHOTOABSORPTION IN VALENCE BAND
    if(is <= 1) {
        LOG(TRACE) << "Process: photoabsorption in valence band";
        if(energy_gamma < 0.1) {
            return veh;
        }

        double rv = uniform();
        if(energy_gamma < energy_valence_) {
            veh.push(rv * energy_gamma);
            veh.push((1 - rv) * energy_gamma);
        } else {
            veh.push(rv * energy_valence_);
            veh.push(energy_gamma - rv * energy_valence_);
        }
        return veh;
    }

    // PHOTOABSORPTION IN AN INNER SHELL
    double Ephe = energy_gamma - energy_shell[is];
    LOG(TRACE) << "Process: photoabsorption in an inner shell";
    if(Ephe <= 0) {
        LOG(DEBUG) << "shells: photoelectron with negative energy " << energy_gamma << ", shell " << is << " at "
                   << energy_shell[is] << " eV";
        return veh;
    }

    // PRIMARY PHOTOELECTRON:
    veh.push(Ephe);

    // AUGER ELECTRONS:
    double raug = uniform();

    int ks = 1;
    if(is <= 1) {
        ks = 1;
    } else if(is <= 3) {
        if(raug > auger_prob_integral[is][1]) {
            ks = 2;
        }
    } else {
        if(raug >= auger_prob_integral[is][1]) {
            for(int js = 2; js <= nvac[is]; ++js) {
                if(raug >= auger_prob_integral[is][js - 1] && raug < auger_prob_integral[is][js]) {
                    ks = js;
                }
            }
        }
    }

    if(is == 2) {
        // L23-SHELL VACANCIES
        if(ks == 1) {
            // TRANSITION L23 M M
            transition(auger_energy[2][1], veh);
        }
    } else if(is == 3) {
        // L1-SHELL VACANCIES
        if(ks == 2) {
            // TRANSITION L1 L23 M
            double energy = energy_valence_ * uniform();
            veh.push(energy);
            veh.push(auger_energy[is][ks] - energy);

            if(uniform() <= auger_prob_integral[2][1]) {
                // TRANSITION L23 M M
                transition(auger_energy[2][1], veh);
            }
        } else {
            // TRANSITION L1 M M
            transition(auger_energy[3][1], veh);
        }
    } else if(is == 4) {
        // K-SHELL VACANCIES
        if(ks >= 8) {
            // TRANSITION K M M
            transition(auger_energy[is][ks], veh);
        } else if(ks == 6 || ks == 7) {
            // TRANSITION K L23 M
            double energy = energy_valence_ * uniform();
            veh.push(energy);
            veh.push(auger_energy[is][ks] - energy); // adjust for energy conservation

            if(uniform() <= auger_prob_integral[2][1]) {
                // TRANSITION L23 M M
                transition(auger_energy[2][1], veh);
            }
        } else if(ks == 4 || ks == 5) {
            // TRANSITION K L1 M
            double energy = energy_valence_ * uniform();
            veh.push(energy);
            veh.push(auger_energy[is][ks] - energy); // adjust for energy conservation

            if(uniform() <= auger_prob_integral[3][1]) {
                // TRANSITION L1 M M
                transition(auger_energy[3][1], veh);
            } else {
                // TRANSITION L1 L23 M
                energy = energy_valence_ * uniform();
                veh.push(energy);
                veh.push(auger_energy[3][2] - energy);

                if(uniform() <= auger_prob_integral[2][1]) {
                    // TRANSITION L23 M M
                    transition(auger_energy[2][1], veh);
                }
            }
        } else if(ks == 3) {
            // TRANSITION K L23 L23
            veh.push(auger_energy[is][ks]); // default

            if(uniform() <= auger_prob_integral[2][1]) {
                // TRANSITION L23 M M
                transition(auger_energy[2][1], veh);
            }

            if(uniform() <= auger_prob_integral[2][1]) {
                // TRANSITION L23 M M
                transition(auger_energy[2][1], veh);
            }
        } else if(ks == 2) {
            // TRANSITION K L1 L23
            veh.push(auger_energy[is][ks]); // default

            // L23-SHELL VACANCIES
            if(uniform() <= auger_prob_integral[2][1]) {
                // TRANSITION L23 M M
                transition(auger_energy[2][1], veh);
            }

            // L1-SHELL VACANCIES
            if(uniform() > auger_prob_integral[3][1]) {
                // TRANSITION L1 L23 M
                double energy = energy_valence_ * uniform();
                veh.push(energy);
                veh.push(auger_energy[3][2] - energy);

                if(uniform() <= auger_prob_integral[2][1]) {
                    // TRANSITION L23 M M
                    transition(auger_energy[2][1], veh);
                }
            } else {
                // TRANSITION L1 M M
                transition(auger_energy[3][1], veh);
            }
        } else if(ks == 1) {
            // TRANSITION K L1 L1
            veh.push(auger_energy[is][ks]); // default

            // L1-SHELL VACANCIES
            if(uniform() > auger_prob_integral[3][1]) {
                // TRANSITION L1 L23 M
                double energy = energy_valence_ * uniform();
                veh.push(energy);
                veh.push(auger_energy[3][2] - energy);

                // L23-SHELL VACANCIES
                if(uniform() <= auger_prob_integral[2][1]) {
                    // TRANSITION L23 M M
                    transition(auger_energy[2][1], veh);
                }
            } else {
                // TRANSITION L1 M M
                transition(auger_energy[3][1], veh);
            }

            // L1-SHELL VACANCIES
            if(uniform() > auger_prob_integral[3][1]) {
                // TRANSITION L1 L23 M
                double energy = energy_valence_ * uniform();
                veh.push(energy);
                veh.push(auger_energy[3][2] - energy);

                // L23-SHELL
                if(uniform() <= auger_prob_integral[2][1]) {
                    // TRANSITION L23 M M
                    transition(auger_energy[2][1], veh);
                }
            } else {
                // TRANSITION L1 M M
                transition(auger_energy[3][1], veh);
            }
        } // ks
    }     // is 4

    return veh;
}

void PhotoAbsorptionIonizer::transition(double energy_auger, std::stack<double>& veh) {

    // AUGER ELECTRON
    double energy = (1 + triangular()) * energy_valence_; // 0..2*Ev
    veh.push(energy_auger - energy);

    // ASSIGN ENERGIES TO THE HOLES
    // holes share the Auger energy but both stay below valence band edge (12 eV)
    std::uniform_real_distribution<double> share{std::max(0., energy - energy_valence_), std::min(energy, energy_valence_)};
    double hole_energy = share(*random_engine_);
    veh.push(hole_energy);
    veh.push(energy - hole_energy);
}

double PhotoAbsorptionIonizer::uniform() {
    if(random_engine_ == nullptr) {
        throw ModuleError("Missing random number generator");
    }
    return uniform_dist_(*random_engine_);
}

double PhotoAbsorptionIonizer::triangular() {
    if(random_engine_ == nullptr) {
        throw ModuleError("Missing random number generator");
    }
    return triangular_dist_(*random_engine_);
}