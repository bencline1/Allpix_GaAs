/**
 * @file
 * @brief Definition of a module to deposit charges using Hans Bichsel's stragelling description
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <array>
#include <random>

#include "core/module/Module.hpp"

#include <TH1I.h>
#include <TProfile.h>

namespace allpix {
#define HEPS_ENTRIES 1251
#define N2 64

    /**
     * @brief Type of particles
     */
    enum class ParticleType : unsigned int {
        NONE = 0, ///< No particle
        PROTON,
        PION,
        KAON,
        ELECTRON,
        MUON,
        HELIUM,
        LITHIUM,
        CARBON,
        IRON,
    };

    inline std::ostream& operator<<(std::ostream& os, const ParticleType type) {
        os << static_cast<std::underlying_type<ParticleType>::type>(type);
        return os;
    }

    /**
     * @brief Particle
     */
    class Particle {
    public:
        /**
         * Constructor for new particle
         * @param energy        Kinetic energy of the particle
         * @param pos           Position of generation
         * @param dir           Direction of motion
         * @param particle_type Type of particle
         * @param parent        ID of the parent particle, none (primary) if negative.
         */
        Particle(double energy, ROOT::Math::XYZPoint pos, ROOT::Math::XYZVector dir, ParticleType type, int parent = -1)
            : position_start_(pos), position_end_(std::move(pos)), direction_(std::move(dir)), parent_id_(parent),
              energy_(energy), type_(type) {
            update();
        };

        Particle() = delete;

        ROOT::Math::XYZPoint position() const { return position_end_; }
        void setPosition(ROOT::Math::XYZPoint pos) { position_end_ = pos; }

        ROOT::Math::XYZPoint position_start() const { return position_start_; }

        ROOT::Math::XYZVector direction() const { return direction_; }
        void setDirection(ROOT::Math::XYZVector dir) { direction_ = dir; }

        int getParentID() { return parent_id_; }

        /**
         * @ Helper to obtain the relativistiv kinetic energy of the particle
         * @return Particle's relativistic kinetic energy
         */
        double E() const { return energy_; }
        void setE(double energy) {
            energy_ = energy;
            update();
        }
        ParticleType type() const { return type_; }

        /**
         * Helper to obtain particle rest mass in units of MeV
         * @return Particle rest mass in MeV
         */
        double mass() const { return mass_.at(static_cast<std::underlying_type<ParticleType>::type>(type_)); };

        double gamma() const { return gamma_; }

        double betasquared() const { return betasquared_; }

        double momentum() const { return momentum_; }

        double velocity() const { return velocity_; }

    private:
        ROOT::Math::XYZPoint position_start_;
        ROOT::Math::XYZPoint position_end_;
        ROOT::Math::XYZVector direction_;
        int parent_id_{};

        // Relativistic kinetic energy
        double energy_{};                       // [MeV]
        ParticleType type_{ParticleType::NONE}; // particle type

        void update() {
            gamma_ = energy_ / mass() + 1.0;
            double betagamma = sqrt(gamma_ * gamma_ - 1.0); // bg = beta*gamma = p/m
            betasquared_ = betagamma * betagamma / (1 + betagamma * betagamma);
            momentum_ = mass() * betagamma;              // [MeV/c]
            velocity_ = betagamma / gamma_ * 299.792458; // in base units [mm/ns]
        }

        double gamma_{};
        double betasquared_{};
        double momentum_{};
        double velocity_{};

        std::vector<double> mass_{
            0,
            938.2723,   // proton
            139.578,    // pion
            493.67,     // K
            0.51099906, // e
            105.65932   // mu
        };
    };

    /**
     * @brief Deposited clusters of electron-hole pairs generated via ionization
     */
    class Cluster {
    public:
        /**
         * @ brief default constructor
         */
        Cluster() = delete;

        /**
         * Constructor for e/h pair cluster
         * @param eh_pairs Number of electron-hole pairs
         * @param pos      Position of the cluster in local coordinates
         * @param energy   Energy of the generating particle
         */
        Cluster(int eh_pairs, ROOT::Math::XYZPoint pos, double energy, size_t particle_id)
            : neh(eh_pairs), position(pos), E(energy), particle_id_(particle_id){};
        int neh;
        ROOT::Math::XYZPoint position;
        double E; // [eV] generating particle
        size_t particle_id_{};
    };

    /**
     * @ingroup Modules
     * @brief Module to deposit charges at predefined positions in the sensor volume
     *
     * This module can deposit charge carriers at defined positions inside the sensitive volume of the detector
     */
    class DepositionBichselModule : public Module {
    public:
        /**
         * @brief Constructor for a module to deposit charges at a specific point in the detector's active sensor volume
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        DepositionBichselModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Deposit charge carriers for every simulated event
         */
        void run(unsigned int) override;

        /**
         * @brief Initialize the histograms
         */
        void init() override;

        /**
         * @brief Write statistical summary
         */
        void finalize() override;

    private:
        std::mt19937_64 random_generator_;
        std::shared_ptr<const Detector> detector_;

        Messenger* messenger_;

        std::vector<std::string> data_paths_;
        std::ifstream open_data_file(const std::string& file_name);

        std::vector<Cluster> stepping(Particle init, unsigned iev, double depth, unsigned& ndelta);

        void update_elastic_collision_parameters(double& inv_collision_length_elastic,
                                                 double& screening_parameter,
                                                 const Particle& particle) const;

        void create_output_plots(unsigned int event_num, const std::vector<Cluster>& clusters);

        using table = std::array<double, HEPS_ENTRIES>;
        table E, dE;
        table dielectric_const_real;
        table dielectric_const_imag;
        table dfdE;
        table oscillator_strength_ae;
        table xkmn;

        // FIXME possible config parameters
        bool fast_{};
        // delta ray range: 1 um at 10 keV (Mazziotta 2004)
        double explicit_delta_energy_cut_{};
        ParticleType particle_type_{};
        double temperature_{};
        bool output_plots_{};
        bool output_event_displays_{};
        double initial_energy_{};
        double energy_threshold_{};

        // Constants
        const double electron_mass_ = 0.51099906; // e mass [MeV]
        const double rydberg_constant_ = 13.6056981;
        const double fac_ = 8.0 * M_PI * rydberg_constant_ * rydberg_constant_ * pow(0.529177e-8, 2) / electron_mass_ / 1e6;
        const double zi_ = 1.0;

        // = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
        // silicon:

        const double atomic_number_ = 14.0;                          // ZA = atomic number of absorber, Si
        const double atomic_weight = 28.086;                         // AW = atomic weight of absorber
        const double density = 2.329;                                // rho= density of absorber material
        const double radiation_length = 9.36;                        // [cm]
        const double atnu_ = 6.0221367e23 * density / atomic_weight; // atnu = # of atoms per cm**3

        // Histograms:
        TProfile *elvse, *invse;
        TH1I *hstep5, *hstep0, *hzz, *hde0, *hde1, *hde2, *hdel, *htet, *hnprim, *hlogE, *hlogn, *hscat, *hncl, *htde,
            *htde0, *htde1, *hteh, *hq0, *hrms;

        /**
         * Reading HEPS.TAB data file
         *
         * HEPS.TAB is the table of the dielectric constant for solid Si, epsilon = ep(1,j) + i*ep(2,j), as a function of
         * energy loss E(j), section II.E in RMP, and rim is Im(-1/epsilon), Eq. (2.17), p.668. Print statements are included
         * to check that the file is read correctly.
         */
        void read_hepstab();

        /**
         * Reading MACOM.TAB data file
         *
         * MACOM.TAB is the table of the integrals over momentum transfer K of the generalized oscillator strength, summed
         * for all shells, i.e. the A(E) of Eq. (2.11), p. 667 of RMP
         */
        void read_macomtab();

        /**
         * Reading EMERC.TAB data file
         *
         * EMERC.TAB is the table of the integral over K of generalized oscillator strength for E < 11.9 eV with
         * Im(-1/epsilon) from equations in the Appendix of Emerson et al., Phys Rev B7, 1798 (1973) (also see CCS-63)
         */
        void read_emerctab();

        double gena1();
        double gena2();
    };
} // namespace allpix
