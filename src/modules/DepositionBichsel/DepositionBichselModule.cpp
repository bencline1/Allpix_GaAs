#include "DepositionBichselModule.hpp"
#include "MazziottaIonizer.hpp"

#include <fstream>
#include <sstream>
#include <stack>

#include "core/messenger/Messenger.hpp"
#include "core/utils/file.h"
#include "core/utils/log.h"
#include "objects/DepositedCharge.hpp"
#include "objects/MCParticle.hpp"

using namespace allpix;

DepositionBichselModule::DepositionBichselModule(Configuration& config,
                                                 Messenger* messenger,
                                                 std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)), messenger_(messenger) {

    // Seed the random generator with the global seed
    random_generator_.seed(getRandomSeed());

    config_.setDefault("source_position", ROOT::Math::XYZPoint(0., 0., 0.));
    config_.setDefault<double>("temperature", 293.15);
    config_.setDefault("delta_energy_cut", 0.009);
    config_.setDefault<bool>("fast", true);
    config_.setDefault<bool>("output_plots", false);
    temperature_ = config_.get<double>("temperature");
    explicit_delta_energy_cut_ = config_.get<double>("delta_energy_cut");
    fast_ = config_.get<bool>("fast");
    output_plots_ = config_.get<bool>("output_plots");

    initial_energy_ = config_.get<double>("source_energy");

    // EGAP = GAP ENERGY IN eV
    // EMIN = THRESHOLD ENERGY (ALIG ET AL., PRB22 (1980), 5565)
    energy_threshold_ =
        config_.get<double>("energy_threshold", 1.5 * 1.17 - 4.73e-4 * temperature_ * temperature_ / (636 + temperature_));

    // FIXME make sure particle exists
    particle_type_ = static_cast<ParticleType>(config_.get<unsigned int>("particle_type", 4));

    // Register lookup paths for cross-section and oscillator strength data files:
    if(config_.has("data_paths")) {
        auto extra_paths = config_.getPathArray("data_paths", true);
        data_paths_.insert(data_paths_.end(), extra_paths.begin(), extra_paths.end());
        LOG(TRACE) << "Registered data paths from configuration.";
    }
    if(path_is_directory(ALLPIX_BICHSEL_DATA_DIRECTORY)) {
        data_paths_.emplace_back(ALLPIX_BICHSEL_DATA_DIRECTORY);
        LOG(TRACE) << "Registered data path: " << ALLPIX_BICHSEL_DATA_DIRECTORY;
    }
    const char* data_dirs_env = std::getenv("XDG_DATA_DIRS");
    if(data_dirs_env == nullptr || strlen(data_dirs_env) == 0) {
        data_dirs_env = "/usr/local/share/:/usr/share/:";
    }
    std::vector<std::string> data_dirs = split<std::string>(data_dirs_env, ":");
    for(auto data_dir : data_dirs) {
        if(data_dir.back() != '/') {
            data_dir += "/";
        }

        data_dir += std::string(ALLPIX_PROJECT_NAME) + std::string("/data");
        if(path_is_directory(data_dir)) {
            data_paths_.emplace_back(data_dir);
            LOG(TRACE) << "Registered global data path: " << data_dir;
        }
    }
}

void DepositionBichselModule::init() {

    // Booking histograms:
    if(output_plots_) {
        auto model = detector_->getModel();
        auto depth = model->getSensorSize().z();

        elvse = new TProfile("elvse", "elastic mfp;log_{10}(E_{kin}[MeV]);elastic mfp [#mum]", 140, -3, 4);
        invse = new TProfile("invse", "inelastic mfp;log_{10}(E_{kin}[MeV]);inelastic mfp [#mum]", 140, -3, 4);

        hstep5 = new TH1I("step5", "step length;step length [#mum];steps", 500, 0, 5);
        hstep0 = new TH1I("step0", "step length;step length [#mum];steps", 500, 0, 0.05);
        hzz = new TH1I("zz", "z;depth z [#mum];steps", depth, 0, depth);

        hde0 = new TH1I("de0", "step E loss;step E loss [eV];steps", 200, 0, 200);
        hde1 = new TH1I("de1", "step E loss;step E loss [eV];steps", 100, 0, 5000);
        hde2 = new TH1I("de2", "step E loss;step E loss [keV];steps", 200, 0, 20);
        hdel = new TH1I("del", "log step E loss;log_{10}(step E loss [eV]);steps", 140, 0, 7);
        htet = new TH1I("tet", "delta emission angle;delta emission angle [deg];inelasic steps", 180, 0, 90);
        hnprim = new TH1I("nprim", "primary eh;primary e-h;scatters", 21, -0.5, 20.5);
        hlogE = new TH1I("logE", "log Eeh;log_{10}(E_{eh}) [eV]);eh", 140, 0, 7);
        hlogn = new TH1I("logn", "log neh;log_{10}(n_{eh});clusters", 80, 0, 4);
        hscat = new TH1I("scat", "elastic scattering angle;scattering angle [deg];elastic steps", 180, 0, 180);
        hncl = new TH1I("ncl", "clusters;e-h clusters;tracks", 4 * depth * 5, 0, 4 * depth * 5);

        double lastbin = initial_energy_ < 1.1 ? 1.05 * initial_energy_ * 1e3 : 5 * 0.35 * depth; // 350 eV/micron
        htde = new TH1I("tde", "sum E loss;sum E loss [keV];tracks / keV", std::max(100, int(lastbin)), 0, int(lastbin));
        htde0 = new TH1I(
            "tde0", "sum E loss, no delta;sum E loss [keV];tracks, no delta", std::max(100, int(lastbin)), 0, int(lastbin));
        htde1 = new TH1I("tde1",
                         "sum E loss, with delta;sum E loss [keV];tracks, with delta",
                         std::max(100, int(lastbin)),
                         0,
                         int(lastbin));

        hteh = new TH1I("teh",
                        "total e-h;total charge [ke];tracks",
                        std::max(100, int(50 * 0.1 * depth)),
                        0,
                        std::max(1, int(10 * 0.1 * depth)));
        hq0 = new TH1I("q0",
                       "normal charge;normal charge [ke];tracks",
                       std::max(100, int(50 * 0.1 * depth)),
                       0,
                       std::max(1, int(10 * 0.1 * depth)));
        hrms = new TH1I("rms", "RMS e-h;charge RMS [e];tracks", 100, 0, 50 * depth);
    }
    // = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
    // INITIALIZE ENERGY BINS

    double u = log(2.0) / N2;
    double um = exp(u);
    int ken = static_cast<int>(log(1839.0 / 1.5) / u);
    double Emin = 1839.0 / pow(2, ken / N2); // integer division intended

    // EMIN is chosen to give an E-value exactly at the K-shell edge, 1839 eV
    E[1] = Emin;
    for(size_t j = 2; j < E.size(); ++j) {
        E[j] = E[j - 1] * um; // must agree with heps.tab
        dE[j - 1] = E[j] - E[j - 1];
    }

    LOG(DEBUG) << std::endl << "  n2 " << N2 << ", Emin " << Emin << ", um " << um << ", E[nume] " << E.back();

    // = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
    // READ DIELECTRIC CONSTANTS
    read_hepstab();

    //= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
    // READ INTEGRAL OVER MOMENTUM TRANSFER OF THE GENERALIZED OSCILLATOR STRENGTH
    read_macomtab();

    read_emerctab();
}

void DepositionBichselModule::run(unsigned int event) {
    std::uniform_real_distribution<double> unirnd(0, 1);

    // defaults:
    auto model = detector_->getModel();
    auto depth = model->getSensorSize().z();

    // double depth = 285; // [mu] pixel depth
    double pitch = 25 * 1e-3; // [mu] pixels size
    double angle = 999;       // flag

    double turn = atan(pitch / depth); // [rad] default
    if(fabs(angle) < 91) {
        turn = angle / 180 * M_PI;
    }

    double width = depth * tan(turn); // [mu] projected track, default: pitch

    LOG(TRACE) << "  particle type     " << particle_type_;
    LOG(TRACE) << "  kinetic energy    " << initial_energy_ << " MeV";
    LOG(TRACE) << "  pixel pitch       " << pitch << " um";
    LOG(TRACE) << "  pixel depth       " << depth << " mm";
    LOG(TRACE) << "  incident angle    " << turn * 180 / M_PI << " deg";
    LOG(TRACE) << "  track width       " << width << " um";
    LOG(TRACE) << "  temperature       " << temperature_ << " K";

    LOG(INFO) << event;

    // put track on std::stack:
    double xm = pitch * (unirnd(random_generator_) - 0.5);       // [mu] -p/2..p/2 at track mid
    ROOT::Math::XYZPoint pos((xm - 0.5 * width), 0, -depth / 2); // local coord: z [-d/2, +d/2]
    ROOT::Math::XYZVector dir(sin(turn), 0, cos(turn));
    Particle initial(initial_energy_, pos, dir, particle_type_); // beam particle is first "delta"
    // x : entry point is left;
    // y :  [cm]
    // z :  pixel from 0 to depth [cm]

    unsigned ndelta = 0; // number of deltas generated

    auto clusters = stepping(std::move(initial), event, depth, ndelta);
}

std::vector<Cluster> DepositionBichselModule::stepping(Particle init, unsigned iev, double depth, unsigned& ndelta) {

    MazziottaIonizer ionizer(&random_generator_);
    std::uniform_real_distribution<double> unirnd(0, 1);

    std::vector<MCParticle> mcparticles;

    std::vector<Cluster> clusters;
    std::stack<Particle> deltas;
    deltas.push(init);
    // Statistics:
    unsigned nsteps = 0; // number of steps for full event
    unsigned nscat = 0;  // elastic scattering
    unsigned nloss = 0;  // ionization
    double total_energy_loss = 0.0;
    unsigned nehpairs = 0;
    unsigned sumeh2 = 0;

    while(!deltas.empty()) {
        double Ekprev = 9e9; // update flag for next delta

        auto particle = deltas.top();
        deltas.pop();
        LOG(TRACE) << "Picked up particle of type " << particle.type();

        auto nlast = E.size() - 1;

        double xm0 = 1;
        double xlel = 1;
        double gn = 1;
        table totsig;

        LOG(DEBUG) << "  delta " << Units::display(particle.E(), {"keV", "MeV", "GeV"}) << ", cost "
                   << particle.direction().Z() << ", u " << particle.direction().X() << ", v " << particle.direction().Y()
                   << ", z " << particle.position().Z();

        while(1) { // steps
            LOG(TRACE) << "Stepping...";
            if(particle.E() < 0.9 * Ekprev) { // update
                LOG(TRACE) << "Updating...";
                // Emax = maximum energy loss, see Uehling, also Sternheimer & Peierls Eq.(53)
                double Emax =
                    particle.mass() * (particle.gamma() * particle.gamma() - 1) /
                    (0.5 * particle.mass() / electron_mass_ + 0.5 * electron_mass_ / particle.mass() + particle.gamma());

                // maximum energy loss for incident electrons
                if(particle.type() == ParticleType::ELECTRON) {
                    Emax = 0.5 * particle.E();
                }
                Emax = 1e6 * Emax; // eV

                // Define parameters and calculate Inokuti"s sums,
                // S ect 3.3 in Rev Mod Phys 43, 297 (1971)

                double zi = 1.0;
                double dec = zi * zi * atnu * fac_ / particle.betasquared();
                double EkeV = particle.E() * 1e6; // [eV]

                // Generate collision spectrum sigma(E) from df/dE, epsilon and AE.
                // sig(*,j) actually is E**2 * sigma(E)

                std::array<double, 6> tsig;
                tsig.fill(0);
                table H;

                // Statistics:
                double stpw = 0;

                std::array<table, 6> sig;
                for(unsigned j = 1; j < E.size(); ++j) {

                    if(E[j] > Emax) {
                        break;
                    }

                    // Eq. (3.1) in RMP and red notebook CCS-33, 39 & 47
                    double Q1 = rydberg_constant_;
                    if(E[j] < 11.9) {
                        Q1 = pow(xkmn[j], 2) * rydberg_constant_;
                    } else if(E[j] < 100.0) {
                        Q1 = pow(0.025, 2) * rydberg_constant_;
                    }

                    double qmin =
                        E[j] * E[j] / (2 * electron_mass_ * 1e6 * particle.betasquared()); // twombb = 2 m beta**2 [eV]
                    if(E[j] < 11.9 && Q1 < qmin) {
                        sig[1][j] = 0;
                    } else {
                        sig[1][j] = E[j] * dfdE[j] * log(Q1 / qmin);
                    }
                    // longitudinal excitation, Eq. (46) in Fano; Eq. (2.9) in RMP

                    double epbe = std::max(1 - particle.betasquared() * dielectric_const_real[j], 1e-20); // Fano Eq. (47)
                    double sgg = E[j] * dfdE[j] * (-0.5) *
                                 log(epbe * epbe + pow(particle.betasquared() * dielectric_const_imag[j], 2));

                    double thet = atan(dielectric_const_imag[j] * particle.betasquared() / epbe);
                    if(thet < 0) {
                        thet = thet + M_PI; // plausible-otherwise I"d have a jump
                    }
                    // Fano says [p 21]: "arctan approaches pi for betasq*eps1 > 1 "

                    double sgh = 0.0092456 * E[j] * E[j] * thet *
                                 (particle.betasquared() - dielectric_const_real[j] / (pow(dielectric_const_real[j], 2) +
                                                                                       pow(dielectric_const_imag[j], 2)));

                    sig[2][j] = sgg;
                    sig[3][j] = sgh; // small, negative

                    // uef from  Eqs. 9 & 2 in Uehling, Ann Rev Nucl Sci 4, 315 (1954)
                    double uef = 1 - E[j] * particle.betasquared() / Emax;
                    if(particle.type() == ParticleType::ELECTRON) {
                        uef = 1 + pow(E[j] / (EkeV - E[j]), 2) +
                              pow((particle.gamma() - 1) / particle.gamma() * E[j] / EkeV, 2) -
                              (2 * particle.gamma() - 1) * E[j] / (particle.gamma() * particle.gamma() * (EkeV - E[j]));
                    }
                    // there is a factor of 2 because the integral was over d(lnK) rather than d(lnQ)
                    sig[4][j] = 2 * oscillator_strength_ae[j] * uef;

                    sig[5][j] = 0;
                    for(unsigned i = 1; i <= 4; ++i) {
                        sig[5][j] += sig[i][j]; // sum

                        // divide by E**2 to get the differential collision cross section sigma
                        // Tsig = integrated total collision cross section
                        tsig[i] = tsig[i] + sig[i][j] * dE[j] / (E[j] * E[j]);
                    }                                             // i
                    tsig[5] += sig[5][j] * dE[j] / (E[j] * E[j]); // running sum

                    double HE2 = sig[5][j] * dec;
                    H[j] = HE2 / (E[j] * E[j]);
                    stpw += H[j] * E[j] * dE[j]; // dE/dx
                    nlast = j;
                }
                xm0 = tsig[5] * dec; // 1/path

                // Statistics:
                double sst = H[1] * dE[1]; // total cross section (integral)

                totsig[1] = H[1] * dE[1]; // running integral
                for(unsigned j = 2; j <= nlast; ++j) {
                    totsig[j] = totsig[j - 1] + H[j] * dE[j];
                    sst += H[j] * dE[j];
                }

                // NORMALIZE running integral:
                for(unsigned j = 1; j <= nlast; ++j) {
                    totsig[j] /= totsig[nlast]; // norm
                }

                // elastic:
                if(particle.type() == ParticleType::ELECTRON) {
                    // gn = 2*2.61 * pow( atomic_number, 2.0/3.0 ) / EkeV; // Mazziotta
                    gn = 2 * 2.61 * pow(atomic_number, 2.0 / 3.0) / (particle.momentum() * particle.momentum()) *
                         1e-6;            // Moliere
                    double E2 = 14.4e-14; // [MeV*cm]
                    double FF = 0.5 * M_PI * E2 * E2 * atomic_number * atomic_number / (particle.E() * particle.E());
                    double S0EL = 2 * FF / (gn * (2 + gn));
                    // elastic total cross section  [cm2/atom]
                    xlel = atnu * S0EL; // ATNU = N_A * density / A = atoms/cm3
                } else {
                    double getot = particle.E() + particle.mass();
                    xlel = std::min(2232.0 * radiation_length *
                                        pow(particle.momentum() * particle.momentum() / (getot * zi), 2),
                                    10.0 * radiation_length);
                    // units ?
                }

                if(output_plots_) {
                    elvse->Fill(log(particle.E()) / log(10), 1e4 / xlel);
                    invse->Fill(log(particle.E()) / log(10), 1e4 / xm0);
                }

                Ekprev = particle.E();

                LOG(TRACE) << "  ev " << iev << " type " << particle.type() << ", Ekin " << particle.E() * 1e3 << " keV"
                           << ", beta " << sqrt(particle.betasquared()) << ", gam " << particle.gamma() << std::endl
                           << "  Emax " << Emax << ", nlast " << nlast << ", Elast " << E[nlast] << ", norm "
                           << totsig[nlast] << std::endl
                           << "  inelastic " << 1e4 / xm0 << "  " << 1e4 / sst << ", elastic " << 1e4 / xlel << " um"
                           << ", mean dE " << stpw * depth * 1e-3 << " keV";
            } // update

            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            // step:

            double tlam = 1 / (xm0 + xlel);                                // [cm] TOTAL MEAN FREE PATH (MFP)
            double step = -log(1 - unirnd(random_generator_)) * tlam * 10; // exponential step length, in MM

            // Update position after step
            particle.setPosition(
                ROOT::Math::XYZPoint(ROOT::Math::XYZVector(particle.position()) + step * particle.direction()));

            if(particle.E() < 1) {
                LOG(TRACE) << "step " << step << ", z " << particle.position().Z();
            }

            if(output_plots_) {
                hstep5->Fill(step);
                hstep0->Fill(step);
                hzz->Fill(particle.position().Z());
            }

            // Outside the sensor
            if(!detector_->isWithinSensor(particle.position())) {
                LOG(INFO) << "Left the sensor at " << Units::display(particle.position(), {"mm", "um"});
                break;
            }

            ++nsteps;

            // INELASTIC (ionization) PROCESS
            if(unirnd(random_generator_) > tlam * xlel) {
                LOG(TRACE) << "Inelastic scattering";
                ++nloss;

                // GENERATE VIRTUAL GAMMA:
                double yr = unirnd(random_generator_); // inversion method
                unsigned je = 2;
                for(; je <= nlast; ++je) {
                    if(yr < totsig[je]) {
                        break;
                    }
                }

                double energy_gamma = E[je - 1] + (E[je] - E[je - 1]) * unirnd(random_generator_); // [eV]

                if(output_plots_) {
                    hde0->Fill(energy_gamma); // M and L shells
                    hde1->Fill(energy_gamma); // K shell
                    hde2->Fill(energy_gamma * 1e-3);
                    hdel->Fill(log(energy_gamma) / log(10));
                }

                double residual_kin_energy = particle.E() - energy_gamma * 1E-6; // [ MeV]

                // cut off for further movement: [MeV]
                if(residual_kin_energy < explicit_delta_energy_cut_) {
                    energy_gamma = particle.E() * 1E6;                 // [eV]
                    residual_kin_energy = particle.E() - energy_gamma; // zero
                    // LOG(TRACE) << "LAST ENERGY LOSS" << energy_gamma << residual_kin_energy
                }

                total_energy_loss += energy_gamma; // [eV]

                // emission angle from delta:

                // PRIMARY SCATTERING ANGLE:
                // SINT = SQRT(ER*1.E-6/EK) ! Mazziotta = deflection angle
                // COST = SQRT(1.-SINT*SINT)
                // STORE INFORMATION ABOUT DELTA-RAY:
                // SINT = COST ! flip
                // COST = SQRT(1.-SINT**2) ! sqrt( 1 - ER*1e-6 / particle.E() ) ! wrong

                // double cost = sqrt( energy_gamma / (2*electron_mass_ev + energy_gamma) ); // M. Swartz
                double cost = sqrt(energy_gamma / (2 * electron_mass_ * 1e6 + energy_gamma) *
                                   (particle.E() + 2 * electron_mass_) / particle.E());
                // Penelope, Geant4
                double sint = 0;
                if(cost * cost <= 1) {
                    sint = sqrt(1 - cost * cost); // mostly 90 deg
                }
                double phi = 2 * M_PI * unirnd(random_generator_);

                // G4PenelopeIonisationModel.cc

                // rb = kineticEnergy + 2*electron_mass_c2;

                // kineticEnergy1 = kineticEnergy - deltaE;
                // cosThetaPrimary = sqrt( kineticEnergy1 * rb /
                // ( kineticEnergy * ( rb - deltaE ) ) );

                // cosThetaSecondary = sqrt( deltaE * rb /
                // ( kineticEnergy * ( deltaE + 2*electron_mass_c2 ) ) );

                // penelope.f90
                // Energy and scattering angle ( primary electron ).
                // EP = E - DE
                // TME = 2 * ME
                // RB = E + TME
                // CDT = SQRT( EP * RB / ( E * ( RB - DE ) ) )

                // emission angle of the delta ray:
                // CDTS = SQRT( DE * RB / ( E * ( DE + TME ) ) ) // like Geant

                std::vector<double> din{sint * cos(phi), sint * sin(phi), cost};

                if(output_plots_) {
                    htet->Fill(180 / M_PI * asin(sint)); // peak at 90, tail to 45, elastic forward
                }

                // transform into detector system:
                double cz = particle.direction().Z(); // delta direction
                double sz = sqrt(1 - cz * cz);
                double phif = atan2(particle.direction().Y(), particle.direction().X());
                ROOT::Math::XYZVector delta_direction(cz * cos(phif) * din[0] - sin(phif) * din[1] + sz * cos(phif) * din[2],
                                                      cz * sin(phif) * din[0] + cos(phif) * din[1] + sz * sin(phif) * din[2],
                                                      -sz * din[0] + cz * din[2]);

                // GENERATE PRIMARY e-h:
                std::stack<double> veh;
                if(energy_gamma > energy_threshold_) {
                    veh = ionizer.getIonization(energy_gamma);
                }

                if(output_plots_) {
                    hnprim->Fill(static_cast<double>(veh.size()));
                }

                double sumEeh{0};
                unsigned neh{0};

                // PROCESS e and h
                while(!veh.empty()) {
                    double Eeh = veh.top();
                    veh.pop();

                    if(output_plots_) {
                        hlogE->Fill(Eeh > 1 ? log(Eeh) / log(10) : 0);
                    }

                    if(Eeh > explicit_delta_energy_cut_ * 1e6) {
                        // Put new delta on std::stack:
                        deltas.emplace(Eeh * 1E-6, particle.position(), delta_direction, ParticleType::ELECTRON);

                        ++ndelta;
                        total_energy_loss -= Eeh; // [eV], avoid double counting

                        continue; // next ieh
                    }             // new delta

                    sumEeh += Eeh;

                    // slow down low energy e and h: 95% of CPU time
                    while(!fast_ && Eeh > energy_threshold_) {
                        // for e and h
                        double p_ionization =
                            1 / (1 + aaa * 105 / 2 / M_PI * sqrt(Eeh - eom0) / pow(Eeh - energy_threshold_, 3.5));

                        if(unirnd(random_generator_) < p_ionization) { // ionization
                            ++neh;
                            double E1 = gena1() * (Eeh - energy_threshold_);
                            double E2 = gena2() * (Eeh - energy_threshold_ - E1);

                            if(E1 > energy_threshold_) {
                                veh.push(E1);
                            }
                            if(E2 > energy_threshold_) {
                                veh.push(E2);
                            }

                            Eeh = Eeh - E1 - E2 - energy_threshold_;
                        } else {
                            Eeh -= eom0; // phonon emission
                        }
                    } // slow: while Eeh
                }     // while veh

                if(fast_) {
                    std::poisson_distribution<unsigned int> poisson(sumEeh / 3.645);
                    neh = poisson(random_generator_);
                }

                nehpairs += neh;
                sumeh2 += neh * neh;

                LOG(TRACE) << "  dE " << energy_gamma << " eV, neh " << neh;

                // Store charge cluster:
                if(neh > 0) {
                    clusters.emplace_back(neh, particle.position(), energy_gamma);

                    if(output_plots_) {
                        hlogn->Fill(log(neh) / log(10));
                    }
                }

                // Update particle energy
                particle.setE(particle.E() - energy_gamma * 1E-6); // [MeV]

                if(particle.E() < 1) {
                    LOG(TRACE) << "    Ek " << particle.E() * 1e3 << " keV, z " << particle.position().Z() << ", neh " << neh
                               << ", steps " << nsteps << ", ion " << nloss << ", elas " << nscat << ", cl "
                               << clusters.size();
                }

                if(particle.E() < 1E-6 || residual_kin_energy < 1E-6) {
                    LOG(INFO) << "Absorbed at " << Units::display(particle.position(), {"mm", "um"});
                    break;
                }

                // For electrons, update elastic cross section at new energy
                if(particle.type() == ParticleType::ELECTRON) {
                    // gn = 2*2.61 * pow( atomic_number, 2.0/3.0 ) / (particle.E()*1E6); // Mazziotta
                    double pmom = sqrt(particle.E() * (particle.E() + 2 * particle.mass())); // [MeV/c] 2nd binomial
                    gn = 2 * 2.61 * pow(atomic_number, 2.0 / 3.0) / (pmom * pmom) * 1e-6;    // Moliere
                    double E2 = 14.4e-14;                                                    // [MeV*cm]
                    double FF = 0.5 * M_PI * E2 * E2 * atomic_number * atomic_number / (particle.E() * particle.E());
                    double S0EL = 2 * FF / (gn * (2 + gn));
                    // elastic total cross section  [cm2/atom]
                    xlel = atnu * S0EL; // ATNU = N_A * density / A = atoms/cm3
                }

            } else { // ELASTIC SCATTERING: Chaoui 2006
                LOG(TRACE) << "Elastic scattering";
                ++nscat;

                double r = unirnd(random_generator_);
                double cost = 1 - 2 * gn * r / (2 + gn - 2 * r);
                double sint = sqrt(1 - cost * cost);
                double phi = 2 * M_PI * unirnd(random_generator_);
                std::vector<double> din{sint * cos(phi), sint * sin(phi), cost};

                if(output_plots_) {
                    hscat->Fill(180 / M_PI * asin(sint)); // forward peak, tail to 90
                }

                // Change direction of particle:
                double cz = particle.direction().Z(); // delta direction
                double sz = sqrt(1.0 - cz * cz);
                double phif = atan2(particle.direction().Y(), particle.direction().X());
                particle.setDirection(
                    ROOT::Math::XYZVector(cz * cos(phif) * din[0] - sin(phif) * din[1] + sz * cos(phif) * din[2],
                                          cz * sin(phif) * din[0] + cos(phif) * din[1] + sz * sin(phif) * din[2],
                                          -sz * din[0] + cz * din[2]));
            } // elastic
        }     // while steps

        // Start and end position of MCParticle:
        auto start_global = detector_->getGlobalPosition(particle.position_start());
        auto end_global = detector_->getGlobalPosition(particle.position());

        // Finished treating this particle, let's store it:
        // Create MCParticle:
        // FIXME time missing.
        // FIXME references between particles missing.
        mcparticles.emplace_back(particle.position_start(),
                                 start_global,
                                 particle.position(),
                                 end_global,
                                 static_cast<std::underlying_type<ParticleType>::type>(particle.type()),
                                 0.,
                                 0.);
        LOG(DEBUG) << "Generated MCParticle with start " << Units::display(start_global, {"um", "mm"}) << " and end "
                   << Units::display(end_global, {"um", "mm"}) << " in detector " << detector_->getName();
        LOG(DEBUG) << "                    local start " << Units::display(particle.position_start(), {"um", "mm"})
                   << " and end " << Units::display(particle.position(), {"um", "mm"});
    } // while deltas

    LOG(DEBUG) << "  steps " << nsteps << ", ion " << nloss << ", elas " << nscat << ", dE " << total_energy_loss * 1e-3
               << " keV"
               << ", eh " << nehpairs << ", cl " << clusters.size();

    if(output_plots_) {
        hncl->Fill(static_cast<double>(clusters.size()));
        htde->Fill(total_energy_loss * 1e-3); // [keV] energy conservation - binding energy
        if(ndelta) {
            htde1->Fill(total_energy_loss * 1e-3); // [keV]
        } else {
            htde0->Fill(total_energy_loss * 1e-3); // [keV]
        }
        hteh->Fill(nehpairs * 1e-3); // [ke]
        hq0->Fill(nehpairs * 1e-3);  // [ke]
        hrms->Fill(sqrt(sumeh2));
    }
    return clusters;
}

std::ifstream DepositionBichselModule::open_data_file(const std::string& file_name) {

    std::string file_path{};
    for(auto& path : data_paths_) {
        // Check if file or directory
        if(allpix::path_is_directory(path)) {
            std::vector<std::string> sub_paths = allpix::get_files_in_directory(path);
            for(auto& sub_path : sub_paths) {
                auto name = allpix::get_file_name_extension(sub_path);

                // Accept only with correct suffix and file name
                std::string suffix(ALLPIX_BICHSEL_DATA_SUFFIX);
                if(name.first != file_name || name.second != suffix) {
                    continue;
                }

                file_path = sub_path;
                break;
            }
        } else {
            // Always a file because paths are already checked
            file_path = path;
            break;
        }
    }

    LOG(TRACE) << "Reading data file " << file_path;
    std::ifstream file(file_path);
    if(file.bad() || !file.is_open()) {
        throw ModuleError("Error opening data file \"" + file_name + "\"");
    }

    return file;
}

void DepositionBichselModule::read_hepstab() {
    auto heps = open_data_file("HEPS");

    std::string line;
    getline(heps, line);
    std::istringstream header(line);

    int n2t;
    size_t numt;
    header >> n2t >> numt;

    LOG(DEBUG) << "HEPS.TAB: n2t " << n2t << ", numt " << numt;
    if(N2 != n2t) {
        LOG(WARNING) << "HEPS: n2 & n2t differ";
    }
    if(E.size() - 1 != numt) {
        LOG(WARNING) << "HEPS: nume & numt differ";
    }
    if(numt > E.size() - 1) {
        numt = E.size() - 1;
    }

    unsigned jt = 1;
    while(!heps.eof() && jt < numt) {
        getline(heps, line);
        std::istringstream tokenizer(line);

        double etbl, rimt;
        tokenizer >> jt >> etbl >> dielectric_const_real[jt] >> dielectric_const_imag[jt] >> rimt;

        // The dipole oscillator strength df/dE is calculated, essentially Eq. (2.20)
        dfdE[jt] = rimt * 0.0092456 * E[jt];
    }

    LOG(INFO) << "Read " << jt << " data lines from HEPS.TAB";
    // MAZZIOTTA: 0.0 at 864
    // EP( 2, 864 ) = 0.5 * ( EP(2, 863) + EP(2, 865) )
    // RIM(864) = 0.5 * ( RIM(863) + RIM(865) )
    // DFDE(864) = RIM(864) * 0.0092456 * E(864)
    // DP: fixed in HEPS.TAB
}

void DepositionBichselModule::read_macomtab() {
    auto macom = open_data_file("MACOM");

    std::string line;
    getline(macom, line);
    std::istringstream header(line);

    int n2t;
    size_t numt;
    header >> n2t >> numt;

    auto nume = E.size() - 1;
    LOG(DEBUG) << "MACOM.TAB: n2t " << n2t << ", numt " << numt;
    if(N2 != n2t)
        LOG(WARNING) << "MACOM: n2 & n2t differ";
    if(nume != numt)
        LOG(WARNING) << "MACOM: nume & numt differ";
    if(numt > nume)
        numt = nume;

    unsigned jt = 1;
    while(!macom.eof() && jt < numt) {
        getline(macom, line);
        std::istringstream tokenizer(line);

        double etbl;
        tokenizer >> jt >> etbl >> oscillator_strength_ae[jt];
    }
    LOG(INFO) << "Read " << jt << " data lines from MACOM.TAB";
}

void DepositionBichselModule::read_emerctab() {
    auto emerc = open_data_file("EMERC");

    std::string line;
    getline(emerc, line); // header lines
    getline(emerc, line);
    getline(emerc, line);
    getline(emerc, line);

    unsigned jt = 1;
    while(!emerc.eof() && jt < 200) {
        getline(emerc, line);
        std::istringstream tokenizer(line);

        double etbl;
        tokenizer >> jt >> etbl >> oscillator_strength_ae[jt] >> xkmn[jt];
    }
    LOG(INFO) << "Read " << jt << " data lines from EMERC.TAB";
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double DepositionBichselModule::gena1() {
    std::uniform_real_distribution<double> uniform_dist(0, 1);

    double r1 = 0, r2 = 0, alph1 = 0;
    do {
        r1 = uniform_dist(random_generator_);
        r2 = uniform_dist(random_generator_);
        alph1 = 105. / 16. * (1. - r1) * (1 - r1) * sqrt(r1); // integral = 1, max = 1.8782971
    } while(alph1 > 1.8783 * r2);                             // rejection method

    return r1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double DepositionBichselModule::gena2() {
    std::uniform_real_distribution<double> uniform_dist(0, 1);

    double r1 = 0, r2 = 0, alph2 = 0;
    do {
        r1 = uniform_dist(random_generator_);
        r2 = uniform_dist(random_generator_);
        alph2 = 8 / M_PI * sqrt(r1 * (1 - r1));
    } while(alph2 > 1.27324 * r2); // rejection method

    return r1;
}
