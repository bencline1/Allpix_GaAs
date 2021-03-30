/**
 * @file
 * @brief Definition of a module to deposit charges using Hans Bichsel's stragelling description
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

namespace allpix {

    /**
     * @brief Particle
     */
    class Particle {
    public:
        /**
         * @brief Type of particles
         */
        enum class Type : unsigned int {
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

        /**
         * Constructor for new particle
         * @param energy        Kinetic energy of the particle
         * @param pos           Position of generation
         * @param dir           Direction of motion
         * @param particle_type Type of particle
         * @param parent        ID of the parent particle, none (primary) if negative.
         */
        Particle(
            double energy, ROOT::Math::XYZPoint pos, ROOT::Math::XYZVector dir, Type type, double time = 0, int parent = -1)
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
        Type type() const { return type_; }

        /**
         * Helper to obtain particle rest mass in units of MeV
         * @return Particle rest mass in MeV
         */
        double mass() const { return mass_.at(static_cast<std::underlying_type<Type>::type>(type_)); };

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
        double energy_{};       // [MeV]
        Type type_{Type::NONE}; // particle type

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

    inline std::ostream& operator<<(std::ostream& os, const Particle::Type type) {
        os << static_cast<std::underlying_type<Particle::Type>::type>(type);
        return os;
    }

} // namespace allpix
