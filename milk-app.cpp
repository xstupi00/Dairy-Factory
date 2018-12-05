/**************************************************************
 * Project:     DNS Export
 * File:		main.cpp
 * Author:		Šimon Stupinský
 * University: 	Brno University of Technology
 * Faculty: 	Faculty of Information Technology
 * Course:	    Network Applications and Network Administration
 * Date:		27.11.2018
 * Last change:	27.11.2018
 *
 * Subscribe:
 *
 **************************************************************/

/**
 * @file    main.cpp
 * @brief
 */

#include "milk-app.h"

Facility Inspection("Inspection");
Facility ReceptionLines[RECEPTION_LINES];
Store ReceptionMilkSilos("ReceptionMilkSilos", RECEPTION_MILK_SILOS_CAPACITY);
Store ReceptionCreamSilos("ReceptionCreamSilos", RECEPTION_CREAM_SILOS_CAPACITY);

Queue ClarificatorsQueue;
Store ClarificationLinesFast("ClarificationLinesFast", CLARIFICATION_LINES >> 1);
Store ClarificationLinesSlow("ClarificationLinesSlow", CLARIFICATION_LINES >> 1);
Store ClarificationMilkSilos("ClarificationMilkSilos", CLARIFICATION_MILK_SILOS_CAPACITY);

Facility Homogenizer("Homogenizer");
Facility HomogenizerPump("HomogenizerPump");
Facility Pasteurizer("Pasteurizer");
Store PasteurizationBottleMachines("PasteurizationBottleMachines", PASTEURIZATION_BOTTLE_MACHINES);

Store FactoryStoreLiterBottle("FactoryStoreLiterBottle", INT32_MAX);
Store FactoryStoreLittleBottle("FactoryStoreLittleBottle", INT32_MAX);
Queue FluidMilkLines;

Store WholeMilkMachinesFast("WholeMilkBottleMachinesFast", WHOLE_MILK_MACHINES_FAST);
Store WholeMilkMachinesSlow("WholeMilkBottleMachinesSlow", WHOLE_MILK_MACHINES_SLOW);
Store LightMilkMachines("LightMilkMachines", LIGHT_MILK_MACHINES);
Store LactoFreeMilkMachines("LactoFreeMilkMachines", LACTO_FREE_MILK_MACHINES);
Store StrawberryFlavoredMilkMachines("StrawberryFlavoredMilkMachines", STRAWBERRY_FLAVORED_MILK_MACHINES);
Store CholesterolFreeMilkMachines("CholesterolFreeMilkMachines", CHOLESTEROL_FREE_MILK_MACHINES);
Store StrawberryFruitMilkMachinesSlow("StrawberryFruitMilkMachinesSlow", STRAWBERRY_FRUIT_MILK_MACHINES_SLOW);
Store StrawberryFruitMilkMachinesFast("StrawberryFruitMilkMachinesFast", STRAWBERRY_FRUIT_MILK_MACHINES_FAST);
Store MangoFrutiMilkMachines("MangoFruitMilkMachines", MANGO_FRUIT_MILK_MACHINES);

Store StandardizatorFast("StandardizatorFast", STANDARDIZATORS - 1);
Store StandardizatorSlow("StandardizatorSlow", STANDARDIZATORS - 2);
Store StandardizatorTank("StandardizatorTank", STANDARDIZATORS_TANKS_CAPACITY);
Store CreamPasteurizatorsFast("CreamPasteurizatorsFast", CREAM_PASTEURIZERS - 1);
Store CreamPasteurizatorsSlow("CreamPasteurizatorsSlow", CREAM_PASTEURIZERS - 2);
Store CreamBottleMachines("CreamBottleMachines", CREAM_BOTTLE_MACHINES);
Store CreamBottleTank("CreamBottleTank", CREAM_BOTTLE_TANKS_CAPACITY);
Queue StandardizatorsQueue;
Queue CreamPasteurizatorsQueue;
Queue CreamPackagingQueue;
Store FactoryCreamStore("FactoryCreamStore", INT32_MAX);

Histogram TankersTime("TankersTime", 100, 1000);


///< current month for modelling the frequency of dependencies on different periods of year
std::tuple<int, double, double, double> current_month;


///< custom statistics values
std::map<std::string, int> stats = {
        {"Received Milk", 0},
        {"Infected Milk", 0},
};

unsigned planned_production = 0;

unsigned long compute_seconds_of_year() {
    unsigned long seconds_of_year = 0;
    for (auto const &month : monthly_details) {
        seconds_of_year += std::get<0>(month.second) * DAY;
    }
    return seconds_of_year;
}

unsigned waiting_milk() {
    return HomogenizerPump.QueueLen() + WholeMilkMachinesSlow.QueueLen() + WholeMilkMachinesFast.QueueLen() +
           LightMilkMachines.QueueLen() + LactoFreeMilkMachines.QueueLen() + StrawberryFlavoredMilkMachines.QueueLen() +
           CholesterolFreeMilkMachines.QueueLen() + StrawberryFruitMilkMachinesSlow.QueueLen() +
           StrawberryFruitMilkMachinesFast.QueueLen() + MangoFrutiMilkMachines.QueueLen() + planned_production;
}

void ClarificationActivate() {
    if (not ClarificatorsQueue.Empty()) {
        auto tmp = (Process *) ClarificatorsQueue.GetFirst();
        tmp->Activate();
    }
}


class UltraPasteurizedMilk : public Process {
private:
    Store *ProcessingMachineFirst;
    Store *ProcessingMachineSecond;
    unsigned ProcessingPeriodFirst;
    unsigned ProcessingPeriodSecond;
    bool liter_presentation;

public:
    UltraPasteurizedMilk(Store *processing_machine_first, Store *processing_machine_second,
                         unsigned processing_period_first, unsigned processing_period_second, bool liter = true) {
        ProcessingMachineFirst = processing_machine_first;
        ProcessingMachineSecond = processing_machine_second;
        ProcessingPeriodFirst = processing_period_first;
        ProcessingPeriodSecond = processing_period_second;
        liter_presentation = liter;
    }

    void Behavior() override {
        Store *ProcessingMachine;
        double ProcessingPeriod;
        if (ProcessingMachineFirst->Free() or not ProcessingMachineSecond or not ProcessingMachineSecond->Free()) {
            ProcessingMachine = ProcessingMachineFirst;
            ProcessingPeriod = ProcessingPeriodFirst;
        } else {
            ProcessingMachine = ProcessingMachineSecond;
            ProcessingPeriod = ProcessingPeriodSecond;
        }

        Enter(*ProcessingMachine, 1);
        planned_production--;
        Leave(ClarificationMilkSilos, 1);
        ClarificationActivate();
        Wait(ProcessingPeriod);
        liter_presentation ? Enter(FactoryStoreLiterBottle, 1) : Enter(FactoryStoreLittleBottle, 1);
        Leave(*ProcessingMachine, 1);
    }
};


class PasteurizedMilk : public Process {
public:
    void Behavior() override {

        Seize(HomogenizerPump);
        planned_production--;
        Leave(ClarificationMilkSilos, 1);
        Wait(HOMOGENIZATION_PUMPING_SPEED);
        Release(HomogenizerPump);

        Seize(Homogenizer);
        ClarificationActivate();
        Wait(HOMOGENIZATION_PERIOD);
        Release(Homogenizer);

        Seize(Pasteurizer);
        Wait(PASTEURIZATION_PERIOD);
        Release(Pasteurizer);

        Enter(PasteurizationBottleMachines, 1);
        Wait(PASTEURIZATION_BOTTLE_MACHINES_SPEED);
        Enter(FactoryStoreLiterBottle, 1);
        Leave(PasteurizationBottleMachines);
    }
};

class FluidMilkGenerator : public Process {
public:
    void Behavior() override {
        int ClarificationMilkSilosCapacity = int(ClarificationMilkSilos.Used()) - int(waiting_milk());
        while (not FluidMilkLines.Empty() and ClarificationMilkSilosCapacity > 0) {
            auto tmp = (Process *) FluidMilkLines.GetFirst();
            tmp->Activate();
            planned_production++;
            ClarificationMilkSilosCapacity--;
        }
    }
};

class Clarification : public Process {
private:
    Store *Clarificator;
    double ClarificationPeriod;

public:
    void Init(bool is_slow) {
        if (not is_slow) {
            Clarificator = &ClarificationLinesFast;
            ClarificationPeriod = CLARIFICATION_PROCESSING_SPEED_FAST;
        } else {
            Clarificator = &ClarificationLinesSlow;
            ClarificationPeriod = CLARIFICATION_PROCESSING_SPEED_SLOW;
        }

        Activate();
    }

    void Behavior() override {
        for (;;) {
            if (ReceptionMilkSilos.Used() and
                ClarificationMilkSilos.Free() - (ClarificationLinesFast.Used() + ClarificationLinesSlow.Used())) {
                Enter(*Clarificator, 1);
                Leave(ReceptionMilkSilos, 1);
                Wait(ClarificationPeriod);
                Enter(ClarificationMilkSilos, 1);

                if (not FluidMilkLines.Empty()) {
                    (new FluidMilkGenerator)->Activate();
                }

                Leave(*Clarificator, 1);
            } else {
                ClarificatorsQueue.Insert(this);
                Passivate();
            }
        }
    }
};


class Tanker : public Process {
private:
    double Arrival = 0;

    int get_free_reception_link() {
        int idx = -1;
        if (Random() > PROBABILITY_INFECTED_MILK) {
            int index = 0;
            for (unsigned i = 0; i < RECEPTION_LINES; i++) {
                if (ReceptionLines[i].QueueLen() < ReceptionLines[index].QueueLen()) {
                    index = i;
                }
            }
            std::vector<std::tuple<int, int, int>> potentional_links;
            for (unsigned i = 0; i < RECEPTION_LINES; i++) {
                if (ReceptionLines[i].QueueLen() <= ReceptionLines[index].QueueLen()) {
                    long free_capacity = (i < RECEPTION_LINES - 2 ? ReceptionMilkSilos.Free()
                                                                  : ReceptionCreamSilos.Free());

                    potentional_links.emplace_back(
                            std::make_tuple(free_capacity - (ReceptionLines[i].QueueLen() * 25),
                                            not ReceptionLines[i].Busy(), i));
                }
            }

            std::vector<int> silos_max_capacities = {RECEPTION_MILK_SILOS_CAPACITY,
                                                     RECEPTION_CREAM_SILOS_CAPACITY};
            unsigned empty_silos = 0;
            std::vector<int> empty_silos_indexes;
            for (int &silo_max_capacity : silos_max_capacities) {
                auto it = std::find_if(potentional_links.begin(), potentional_links.end(),
                                       [&silo_max_capacity](const std::tuple<int, int, int> &potentional_link) {
                                           return std::get<0>(potentional_link) == silo_max_capacity;
                                       });
                if (it != potentional_links.end()) {
                    empty_silos++;
                    empty_silos_indexes.emplace_back(std::get<2>(*it));
                }
            }


            if (empty_silos > 0) {
                std::random_device random_device;
                std::mt19937 engine{random_device()};
                std::uniform_int_distribution<int> dist(0, int(empty_silos_indexes.size() - 1));
                idx = empty_silos_indexes[dist(engine)];
            } else {
                auto best_links = std::max_element(potentional_links.begin(), potentional_links.end());
                idx = std::get<2>(*best_links);
            }
        } else {
            stats.find("Infected Milk")->second++;
        }
        return idx;
    }

public:
    void Behavior() override {
        Arrival = Time;

        Seize(Inspection);
        Wait(INSPECTION_PERIOD);
        Release(Inspection);

        int idx = get_free_reception_link();
        if (idx != -1) {

            double brought_milk = Normal(std::get<2>(current_month), std::get<3>(current_month));
            stats.find("Received Milk")->second += brought_milk;
            Seize(ReceptionLines[idx]);

            if (idx < RECEPTION_LINES - 2) {
                while (brought_milk > 1) {
                    Enter(ReceptionMilkSilos, 1);
                    Wait(RECEPTION_MILK_PUMPING_SPEED);

                    if (ReceptionMilkSilos.Used() > 0.40 * RECEPTION_MILK_SILOS_CAPACITY) {
                        while (not ClarificatorsQueue.Empty()) {
                            auto tmp = (Process *) ClarificatorsQueue.GetFirst();
                            tmp->Activate();
                        }
                    }

                    brought_milk -= 1;
                }
            } else if (idx < RECEPTION_LINES) {
                while (brought_milk > 1) {
                    Enter(ReceptionCreamSilos, 1);
                    Wait(RECEPTION_CREAM_PUMPING_SPEED);

                    //if (ReceptionCreamSilos.Used() > 0.25 * RECEPTION_CREAM_SILOS_CAPACITY) {
                        while (not StandardizatorsQueue.Empty()) {
                            auto tmp = (Process *) StandardizatorsQueue.GetFirst();
                            tmp->Activate();
                        }
                    //}

                    brought_milk--;
                }
            }
            Release(ReceptionLines[idx]);
        }

        TankersTime(Time - Arrival);
    }
};

class CreamPackaging : public Process {
public:
    void Behavior() override {
        for (;;) {
            if (int(CreamBottleTank.Used()) - int(CreamBottleMachines.QueueLen()) > 0) {
                Enter(CreamBottleMachines, 1);
                Leave(CreamBottleTank, 1);

                if (not CreamPasteurizatorsQueue.Empty()) {
                    auto tmp = (Process *) CreamPasteurizatorsQueue.GetFirst();
                    tmp->Activate();
                }

                Wait(CREAM_BOTTLE_MACHINES_SPEED);
                Enter(FactoryCreamStore, 1);
                Leave(CreamBottleMachines, 1);
                break;
            } else {
                CreamPackagingQueue.Insert(this);
                Passivate();
            }
        }

    }
};

class CreamPasteurization : public Process {
    Store *CreamPasteurizator;
    double PasteurizationPeriod;

public:
    void Init(bool is_slow) {
        if (not is_slow) {
            CreamPasteurizator = &CreamPasteurizatorsFast;
            PasteurizationPeriod = CREAM_PASTEURIZATORS_FAST_SPEED;
        } else {
            CreamPasteurizator = &CreamPasteurizatorsSlow;
            PasteurizationPeriod = CREAM_PASTEURIZATORS_SLOW_SPEED;
        }
        Activate();
    }

    void Behavior() override {

        for (;;) {
            if (StandardizatorTank.Used() and
                CreamBottleTank.Free() - (CreamPasteurizatorsSlow.Used() + CreamPasteurizatorsFast.Used()) > 0) {
                Enter(*CreamPasteurizator, 1);
                Leave(StandardizatorTank, 1);

                if (not StandardizatorsQueue.Empty()) {
                    auto tmp = (Process *) StandardizatorsQueue.GetFirst();
                    tmp->Activate();
                }

                Wait(PasteurizationPeriod);
                Enter(CreamBottleTank, 1);

                if (not CreamPackagingQueue.Empty()) {
                    auto tmp = (Process *) CreamPackagingQueue.GetFirst();
                    tmp->Activate();
                }

                Leave(*CreamPasteurizator, 1);
            } else {
                CreamPasteurizatorsQueue.Insert(this);
                Passivate();
            }
        }
    }
};

class Standardization : public Process {
private:
    Store *Standardizator;
    double StandardizationPeriod;

public:
    void Init(bool is_slow) {
        if (not is_slow) {
            Standardizator = &StandardizatorFast;
            StandardizationPeriod = STANDARDIZATORS_FAST_SPEED;
        } else {
            Standardizator = &StandardizatorSlow;
            StandardizationPeriod = STANDARDIZATORS_SLOW_SPEED;
        }
        Activate();
    }

    void Behavior() override {

        for (;;) {
            if (int(ReceptionCreamSilos.Used()) > 0 and
                StandardizatorTank.Free() - (StandardizatorFast.Used() + StandardizatorSlow.Used()) > 0) {
                Enter(*Standardizator, 1);

                Leave(ReceptionCreamSilos, 1);

                Wait(StandardizationPeriod);
                Enter(StandardizatorTank, 1);

                if (not CreamPasteurizatorsQueue.Empty()) {
                    auto tmp = (Process *) CreamPasteurizatorsQueue.GetFirst();
                    tmp->Activate();
                }

                Leave(*Standardizator, 1);
            } else {
                StandardizatorsQueue.Insert(this);
                Passivate();
            }
        }
    }

};


void activate_processing_machine(ProcessingMachines processing_machine_id, bool flag = false) {
    switch (processing_machine_id) {
        case Pasteurized: {
            FluidMilkLines.Insert(new PasteurizedMilk);
            break;
        }
        case WholeMilk: {
            FluidMilkLines.Insert(new UltraPasteurizedMilk(WholeMilkMachinesFast, WholeMilkMachinesSlow,
                                                           WHOLE_MILK_MACHINES_SPEED_FAST,
                                                           WHOLE_MILK_MACHINES_SPEED_SLOW));
            break;
        }
        case LightMilk: {
            FluidMilkLines.Insert(new UltraPasteurizedMilk(LightMilkMachines, nullptr, LIGHT_MILK_MACHINES_SPEED, 0));
            break;
        }
        case LactoFree: {
            FluidMilkLines.Insert(
                    new UltraPasteurizedMilk(LactoFreeMilkMachines, nullptr, LACTO_FREE_MILK_MACHINES_SPEED, 0));
            break;
        }
        case StrawberryFlavored: {
            FluidMilkLines.Insert(new UltraPasteurizedMilk(StrawberryFlavoredMilkMachines, nullptr,
                                                           STRAWBERRY_FLAVORED_MILK_MACHINES_SPEED, 0));
            break;
        }
        case CholesterolFree: {
            FluidMilkLines.Insert(
                    new UltraPasteurizedMilk(CholesterolFreeMilkMachines, nullptr, CHOLESTEROL_FREE_MILK_MACHINES_SPEED,
                                             0));
            break;
        }
        case StrawberryFruits: {
            FluidMilkLines.Insert(
                    new UltraPasteurizedMilk(StrawberryFruitMilkMachinesFast, StrawberryFruitMilkMachinesSlow,
                                             STRAWBERRY_FRUIT_MILK_MACHINES_FAST_SPEED,
                                             STRAWBERRY_FRUIT_MILK_MACHINES_SLOW_SPEED, false));
            break;
        }
        case MangoFruits: {
            FluidMilkLines.Insert(
                    new UltraPasteurizedMilk(MangoFrutiMilkMachines, nullptr, MANGO_FRUIT_MILK_MACHINES_SPEED, 0,
                                             false));
            break;
        }
        case Cream: {
            (new CreamPackaging)->Activate();
            break;
        }
        case ClarificationMachine: {
            (new Clarification)->Init(flag);
            break;
        }
        case StandardizationMachine: {
            (new Standardization)->Init(flag);
            break;
        }
        case CreamPasteurizationMachine: {
            (new CreamPasteurization)->Init(flag);
            break;
        }
        default: {
            break;
        }
    }
}


class ProductionGenerators : public Event {
private:
    double production_probability;
    double current_month_id;
    unsigned ProcessingMachineID;
    std::pair<double, double> ProbabilityNum;

public:
    ProductionGenerators(unsigned processing_machine_id, std::pair<double, double> probability_num) {
        production_probability = 0;
        current_month_id = std::get<1>(current_month);
        ProcessingMachineID = processing_machine_id;
        ProbabilityNum = probability_num;
        Activate();
    }

    void Behavior() override {
        if (std::get<1>(current_month) != current_month_id) {
            current_month_id = std::get<1>(current_month);
            production_probability = Uniform(ProbabilityNum.first, ProbabilityNum.second);
        }

        activate_processing_machine(machines_mapping.at(ProcessingMachineID));
        Activate(Time + (HOUR / production_probability));
    }
};


class Initialization : public Event {
private:
    unsigned month = 0;

    void activate_machine(unsigned machines_count, unsigned machines_boundary, unsigned machine_id) {
        for (unsigned i = 0; i < machines_count; i++) {
            activate_processing_machine(machines_mapping.at(machine_id), i >= machines_boundary);
        }
    }

public:
    void Init(unsigned start_month) {
        month = start_month;

        activate_machine(CLARIFICATION_LINES, (CLARIFICATION_LINES >> 1), ClarificationMachine);
        activate_machine(STANDARDIZATORS, STANDARDIZATORS - 1, StandardizationMachine);
        activate_machine(CREAM_PASTEURIZERS, CREAM_PASTEURIZERS - 1, CreamPasteurizationMachine);

        Activate();
        (new FluidMilkGenerator)->Activate();

        for (unsigned i = 0; i < ProcessingMachineCount; i++) {
            new ProductionGenerators(i, ProductionDetails.at(machines_mapping.at(i)));
        }
    }

    void Behavior() override {
        current_month = monthly_details.at(month);
        (month + 1) % MONTH_COUNT ? month++ : month = 0;
        Activate(Time + std::get<0>(current_month) * DAY);
    }
};


class Generator : public Event {
    void Behavior() override {
        (new Tanker)->Activate();
        Activate(Time + Exponential(std::get<1>(current_month) * MINUTE));
    }
};


std::map<std::string, unsigned> process_args(int argc, char **argv) {
    const char *const short_opts = "hm:p:";     ///< short forms of arguments
    const option long_opts[] = {                ///< long forms of arguments
            {"month",  required_argument, nullptr, 'm'},
            {"period", required_argument, nullptr, 'p'},
            {nullptr, 0,                  nullptr, 0},
    };

    std::map<std::string, unsigned> argv_map = {
            {"start_month", January},
            {"sim_period",  0},
    };

    ///< default period for simulation is two months
    std::string period = "m";
    unsigned period_value = 2;

    int opt;
    while ((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
        switch (opt) {
            case 'h': {     ///< printing of the manual page
                std::cerr << help_str;
                exit(EXIT_SUCCESS);
            }
            case 'm': {     ///< starting month of the simulation
                argv_map.find("start_month")->second = map_months.at(optarg);
                break;
            }
            case 'p': {
                period = std::string(optarg);
                period_value = (unsigned) std::stoul(period.substr(0, std::string(optarg).size() - 1));
                    if (period.back() == 'h') {
                        argv_map.find("sim_period")->second = period_value * HOUR;
                    } else if (period.back() == 'd') {
                        argv_map.find("sim_period")->second = period_value * DAY;
                    } else if (period.back() == 'w') {
                        argv_map.find("sim_period")->second = period_value * WEEK;
                    } else if (period.back() == 'y') {
                        argv_map.find("sim_period")->second = period_value * unsigned(compute_seconds_of_year());
                    }
                break;
            }
            default: {      ///< unknown arguments
                std::cerr << "Wrong combinations of arguments!" << std::endl << std::endl;
                std::cerr << help_str;
                exit(EXIT_FAILURE);
            }
        }

        unsigned long long seconds = 0;
        if (period.back() == 'm') {
            for (auto it = monthly_details.find(argv_map.at("start_month")); period_value; period_value--) {
                seconds += std::get<0>(it->second) * DAY;
                (it->first + 1) % MONTH_COUNT ? it++ : it = monthly_details.begin();
            }
            argv_map.find("sim_period")->second = seconds;
        }
    }

    return argv_map;
}


void print_stats() {
    TankersTime.Output();
    ReceptionMilkSilos.Output();
    ReceptionCreamSilos.Output();
    ClarificationLinesFast.Output();
    ClarificationLinesSlow.Output();
    ClarificatorsQueue.Output();
    ClarificationMilkSilos.Output();
    HomogenizerPump.Output();
    Homogenizer.Output();
    Pasteurizer.Output();
    PasteurizationBottleMachines.Output();
    WholeMilkMachinesFast.Output();
    WholeMilkMachinesSlow.Output();
    LightMilkMachines.Output();
    LactoFreeMilkMachines.Output();
    StrawberryFlavoredMilkMachines.Output();
    CholesterolFreeMilkMachines.Output();
    StrawberryFruitMilkMachinesFast.Output();
    StrawberryFruitMilkMachinesSlow.Output();
    MangoFrutiMilkMachines.Output();
    FactoryStoreLiterBottle.Output();
    FactoryStoreLittleBottle.Output();
    FluidMilkLines.Output();

    StandardizatorFast.Output();
    StandardizatorSlow.Output();
    StandardizatorTank.Output();
    CreamPasteurizatorsFast.Output();
    CreamPasteurizatorsSlow.Output();
    CreamBottleTank.Output();
    CreamBottleMachines.Output();
    FactoryCreamStore.Output();

    for (auto &ReceptionLine : ReceptionLines) {
        ReceptionLine.Output();
    }

    DEBUG_PRINT("Received Milk: %d\n", stats.at("Received Milk"));
    DEBUG_PRINT("Rejected Milk: %d\n", stats.at("Infected Milk"));
}


int main(int argc, char **argv) {
    std::map<std::string, unsigned> argv_map = process_args(argc, argv);
    std::cout << "Milk developing process" << std::endl;
    SetOutput("milk-app.out");
    /* initialize random seed: */
    srand(time(nullptr));
    ReceptionLines[RECEPTION_LINES - 1].SetName("CreamReceptionLines");
    DEBUG_PRINT("PERIOD: %d\n", argv_map.at("sim_period"));
    Init(0, argv_map.at("sim_period"));
    (new Generator)->Activate();
    (new Initialization)->Init(argv_map.at("start_month"));
    Run();
    print_stats();
    return 0;
}