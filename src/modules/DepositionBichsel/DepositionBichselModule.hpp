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

#include "core/geometry/GeometryManager.hpp"
#include "core/module/Module.hpp"

#include <TH1D.h>
#include <TH1I.h>
#include <TH2I.h>
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
        Particle(double energy,
                 ROOT::Math::XYZPoint pos,
                 ROOT::Math::XYZVector dir,
                 ParticleType type,
                 double time = 0,
                 int parent = -1)
            : position_start_(pos), position_end_(std::move(pos)), direction_(std::move(dir)), time_(time),
              parent_id_(parent), energy_(energy), type_(type) {
            update();
        };

        Particle() = delete;

        ROOT::Math::XYZPoint position() const { return position_end_; }
        void step(double step) {
            position_end_ = ROOT::Math::XYZPoint(ROOT::Math::XYZVector(position()) + step * direction());
            time_ += step / velocity_;
        }

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

        double time() const { return time_; }

    private:
        ROOT::Math::XYZPoint position_start_;
        ROOT::Math::XYZPoint position_end_;
        ROOT::Math::XYZVector direction_;
        double time_{};
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
     * @ingroup Modules
     * @brief Module to deposit charges at predefined positions in the sensor volume
     *
     * This module can deposit charge carriers at defined positions inside the sensitive volume of the detector
     */
    class DepositionBichselModule : public Module {

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
             * @param particle_id ID of the MCParticle this cluster was generated by
             * @param time     Time in local coordinates at which this cluster has been generated
             */
            Cluster(int eh_pairs, ROOT::Math::XYZPoint pos, size_t particle_id, double time)
                : neh_(eh_pairs), position_(pos), particle_id_(particle_id), time_(time){};

            /**
             * Get number of electron-hole pairs
             * @return Number of electron-hole pairs
             */
            int ehpairs() const { return neh_; }

            /**
             * Get position of cluster in local coordinates
             * @return Position of cluster
             */
            ROOT::Math::XYZPoint position() const { return position_; }

            /**
             * Get ID of the particle which created this cluster
             * @return Particle ID
             */
            size_t particleID() const { return particle_id_; };

            /**
             * Get time in local coordinates at which this cluster was created
             * @return Local time of creation
             */
            double time() const { return time_; }

        private:
            int neh_;
            ROOT::Math::XYZPoint position_;
            size_t particle_id_{};
            double time_{};
        };

    public:
        /**
         * @brief Constructor for module to simulate Bichsel's straggeling in silicon
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        DepositionBichselModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager);

        /**
         * @brief Initialize the histograms
         */
        void initialize() override;

        /**
         * @brief Deposit charge carriers for every simulated event
         */
        void run(Event*) override;

        /**
         * @brief Write statistics summary
         */
        void finalize() override;

    private:
        GeometryManager* geo_manager_;
        Messenger* messenger_;

        /**
         * Stepping function for individual detector. This function performs all stepping and ionization in the material and
         * dispatches messages for the detector with MCParticles and DepositedCharges.
         *
         * @param  primary Incoming particle entering the sensor (primary MCParticle), local coordinates
         * @param  detector Detector to operate on
         * @return          List of outgoing particles leaving the sensor, local coordinates
         */
        std::deque<Particle> stepping(Particle primary,
                                      const std::shared_ptr<const Detector>& detector,
                                      RandomNumberGenerator& random_generator); // NOLINT

        void update_elastic_collision_parameters(double& inv_collision_length_elastic,
                                                 double& screening_parameter,
                                                 const Particle& particle) const;

        /**
         * Plotting of event displays
         * @param event_num Event number
         * @param detector  Detector to generate the plot for
         */
        void create_output_plots(uint64_t event_num, const std::shared_ptr<const Detector>& detector);

        using table = std::array<double, HEPS_ENTRIES>;
        table E, dE;
        table dielectric_const_real;
        table dielectric_const_imag;
        table dfdE;
        table oscillator_strength_ae;
        table xkmn;

        // Source parameters:
        ROOT::Math::XYZPoint source_position_{};
        double source_energy_{};
        double source_energy_spread_{};
        ROOT::Math::XYZVector beam_direction_{};
        double beam_size_{};
        ROOT::Math::XYVector beam_divergence_{};
        ParticleType particle_type_{};

        // COnfig parameter for data file paths:
        std::vector<std::string> data_paths_;

        // Config parameters for the stepping algorithm
        bool fast_{};
        double explicit_delta_energy_cut_{};
        double energy_threshold_{};

        // Plotting configuration
        bool output_plots_{};
        bool output_event_displays_{};
        std::map<std::string, std::vector<Cluster>> clusters_plotting_;

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

        // Histograms - global:
        Histogram<TH1D> source_energy;

        // Histograms - per detector:
        std::map<std::string, TDirectory*> directories;
        std::map<std::string, Histogram<TProfile>> elvse, invse;
        std::map<std::string, Histogram<TH1I>> hstep5, hstep0, hzz, hde0, hde1, hde2, hdel, htet, hnprim, hlogE, hlogn,
            hscat, hncl, htde, htde0, htde1, hteh, hq0, hrms;
        std::map<std::string, Histogram<TH2I>> h2xy, h2zx, h2zr;

        /**
         * Helper function to find and open data files with cross section tables
         * @param  file_name Name of the file to be looked up and opened
         * @return           File handle to correctly found and opened file
         * @throws ModuleError if the requested file could not be found or opened
         */
        std::ifstream open_data_file(const std::string& file_name);

        /**
         * Reading HEPS.TAB data file
         * HEPS.TAB is the table of the dielectric constant for solid Si, epsilon = ep(1,j) + i*ep(2,j), as a function of
         * energy loss E(j), section II.E in RMP, and rim is Im(-1/epsilon), Eq. (2.17), p.668. Print statements are included
         * to check that the file is read correctly.
         */
        void read_hepstab();

        /**
         * Reading MACOM.TAB data file
         * MACOM.TAB is the table of the integrals over momentum transfer K of the generalized oscillator strength, summed
         * for all shells, i.e. the A(E) of Eq. (2.11), p. 667 of RMP
         */
        void read_macomtab();

        /**
         * Reading EMERC.TAB data file
         * EMERC.TAB is the table of the integral over K of generalized oscillator strength for E < 11.9 eV with
         * Im(-1/epsilon) from equations in the Appendix of Emerson et al., Phys Rev B7, 1798 (1973) (also see CCS-63)
         */
        void read_emerctab();

        double gena1(RandomNumberGenerator& random_generator);
        double gena2(RandomNumberGenerator& random_generator);
    };
} // namespace allpix
