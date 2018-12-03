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
Store ReceptionDerivativesSilos("ReceptionDerivativesSilos", RECEPTION_DERIVATIVES_SILOS_CAPACITY);
Store ReceptionCreamSilos("ReceptionCreamSilos", RECEPTION_CREAM_SILOS_CAPACITY);

Queue ClarificatorsQueue;
Store ClarificationLinesFast("ClarificationLinesFast", CLARIFICATION_LINES >> 1);
Store ClarificationLinesSlow("ClarificationLinesSlow", CLARIFICATION_LINES >> 1);
Store ClarificationMilkSilos("ClarificationMilkSilos", CLARIFICATION_MILK_SILOS_CAPACITY);
Store ClarificationCreamSilos("ClarificationCreamSilos", CLARIFICATION_CREAM_SILOS_CAPACITY);

Facility Homogenizer("Homogenizer");
Facility HomogenizerPump("HomogenizerPump");
Facility Pasteurizer("Pasteurizer");
Store PasteurizationBottleMachines("PasteurizationBottleMachines", PASTEURIZATION_BOTTLE_MACHINES);

Store FactoryStore("FactoryStore", INT32_MAX);
Queue FluidMilkLines;

Store WholeMilkMachinesFast("WholeMilkBottleMachinesFast", WHOLEMILK_MACHINES_FAST);
Store WholeMilkMachinesSlow("WholeMilkBottleMachinesSlow", WHOLEMILK_MACHINES_SLOW);
Store LightMilkMachines("LightMilkMachines", LIGHTMILK_MACHINES);
Store LactoFreeMilkMachines("LactoFreeMilkMachines", LACTOFREETMILK_MACHINES);
Store StrawberryFlavoredMilkMachines("StrawberryFlavoredMilkMachines", STRAWBERRY_FLAVORED_MILK_MACHINES);
Store CholesterolFreeMilkMachines("CholesterolFreeMilkMachines", CHOLESTEROL_FREE_MILK_MACHINES);

Histogram TankersTime("TankersTime", 100, 1000);


///< current month for modelling the frequency of dependencies on different periods of year
std::tuple<int, double, double, double> current_month;


///< custom statistics values
std::map<std::string, int> stats = {
        {"Received Milk", 0},
        {"Infected Milk", 0},
};

unsigned long compute_seconds_of_year() {
    unsigned long seconds_of_year = 0;
    for (auto const &month : monthly_details) {
        seconds_of_year += std::get<0>(month.second) * DAY;
    }
    return seconds_of_year;
}

void ClarificationActivate() {
    if (!ClarificatorsQueue.Empty()) {
        auto tmp = (Process *) ClarificatorsQueue.GetFirst();
        tmp->Activate();
    }
}

bool IsNotFreeCapacity() {
    return ((int(ClarificationMilkSilos.Used()) -
             (int(HomogenizerPump.Busy()) + int(Homogenizer.Busy()) + int(WholeMilkMachinesSlow.Used()) +
              int(WholeMilkMachinesFast.Used()) + int(LightMilkMachines.Used()) + int(LactoFreeMilkMachines.Used()) +
              int(StrawberryFlavoredMilkMachines.Used()) + int(CholesterolFreeMilkMachines.Used()))) <=
            1);
}

class StrawberryFlavoredMilk : public Process {
public:
    void Behavior() override {
        for (;;) {
            if (IsNotFreeCapacity() or not StrawberryFlavoredMilkMachines.Free()) {
                FluidMilkLines.Insert(this);
                this->Passivate();
            }

            if (StrawberryFlavoredMilkMachines.Free()) {
                Enter(StrawberryFlavoredMilkMachines, 1);
                Leave(ClarificationMilkSilos, 1);
                ClarificationActivate();
                Wait(STRAWBERRY_FLAVORED_MILK_MACHINES_SPEED);
                Enter(FactoryStore, 1);
                Leave(StrawberryFlavoredMilkMachines, 1);
                break;
            } else {
                DEBUG_PRINT("BUG Strawberry!!!\n");
            }
        }
    }
};

class CholesterolFreeMilk : public Process {
public:
    void Behavior() override {
        for (;;) {
            if (IsNotFreeCapacity() or not CholesterolFreeMilkMachines.Free()) {
                FluidMilkLines.Insert(this);
                this->Passivate();
            }

            if (CholesterolFreeMilkMachines.Free()) {
                Enter(CholesterolFreeMilkMachines, 1);
                Leave(ClarificationMilkSilos, 1);
                ClarificationActivate();
                Wait(CHOLESTEROL_FREE_MILK_MACHINES_SPEED);
                Enter(FactoryStore, 1);
                Leave(CholesterolFreeMilkMachines, 1);
                break;
            } else {
                DEBUG_PRINT("BUG Cholesterol!!!\n");
            }
        }

    }
};

class LactoFreeMilk : public Process {
public:
    void Behavior() override {
        for (;;) {
            if (IsNotFreeCapacity() or not LactoFreeMilkMachines.Free()) {
                FluidMilkLines.Insert(this);
                this->Passivate();
            }

            if (LactoFreeMilkMachines.Free()) {
                Enter(LactoFreeMilkMachines, 1);
                Leave(ClarificationMilkSilos, 1);
                ClarificationActivate();
                Wait(LACTOFREEMILK_MACHINES_SPEED);
                Enter(FactoryStore, 1);
                Leave(LactoFreeMilkMachines, 1);
                break;
            } else {
                DEBUG_PRINT("BUG Lacto!!!\n");
            }
        }
    }
};

class LightMilk : public Process {
public:
    void Behavior() override {
        for (;;) {
            if (IsNotFreeCapacity() or not LightMilkMachines.Free()) {
                FluidMilkLines.Insert(this);
                this->Passivate();
            }

            if (LightMilkMachines.Free()) {
                Enter(LightMilkMachines, 1);
                Leave(ClarificationMilkSilos, 1);
                ClarificationActivate();
                Wait(LIGHTMILK_MACHINES_SPEED);
                Enter(FactoryStore, 1);
                Leave(LightMilkMachines, 1);
                break;
            } else {
                DEBUG_PRINT("BUG Light!!!\n");
            }
        }
    }
};

class WholeMilk : public Process {
public:
    void Behavior() override {

        for (;;) {
            if (IsNotFreeCapacity() or
                (not WholeMilkMachinesFast.Free() and not WholeMilkMachinesSlow.Free())) {
                FluidMilkLines.Insert(this);
                this->Passivate();
            }

            if (WholeMilkMachinesFast.Free()) {
                Enter(WholeMilkMachinesFast, 1);
                Leave(ClarificationMilkSilos, 1);
                ClarificationActivate();
                Wait(WHOLEMILK_MACHINES_SPEED_FAST);
                Enter(FactoryStore, 1);
                Leave(WholeMilkMachinesFast, 1);
                break;
            } else if (WholeMilkMachinesSlow.Free()) {
                Enter(WholeMilkMachinesSlow, 1);
                Leave(ClarificationMilkSilos, 1);
                ClarificationActivate();
                Wait(WHOLEMILK_MACHINES_SPEED_SLOW);
                Enter(FactoryStore, 1);
                Leave(WholeMilkMachinesSlow, 1);
                break;
            } else {
                DEBUG_PRINT("BUG Whole!!!\n");
            }
        }

    }
};


class PasteurizedMilk : public Process {
public:
    void Behavior() override {

        if (IsNotFreeCapacity()) {
            FluidMilkLines.Insert(this);
            this->Passivate();
        }

        Seize(HomogenizerPump);
        Wait(HOMOGENIZATION_PUMPING_SPEED);
        Release(HomogenizerPump);

        Seize(Homogenizer);
        Leave(ClarificationMilkSilos, 1);
        ClarificationActivate();
        Wait(HOMOGENIZATION_PERIOD);
        Release(Homogenizer);

        Seize(Pasteurizer);
        Wait(PASTEURIZATION_PERIOD);
        Release(Pasteurizer);

        Enter(PasteurizationBottleMachines, 1);
        Wait(PASTEURIZATION_BOTTLE_MACHINES_SPEED);
        Enter(FactoryStore, 1);
        Leave(PasteurizationBottleMachines);
    }
};

class Clarification : public Process {
private:
    bool is_fast = false;

public:
    void Init(bool is_slow) {
        this->is_fast = not is_slow;
        this->Activate();
    }

    void Behavior() override {

        for (;;) {
            if (this->is_fast and ReceptionMilkSilos.Used() and
                ClarificationMilkSilos.Free() - (ClarificationLinesFast.Used() + ClarificationLinesSlow.Used())) {
                Enter(ClarificationLinesFast, 1);
                Leave(ReceptionMilkSilos, 1);
                Wait(CLARIFICATION_PROCESSING_SPEED_FAST);
                Enter(ClarificationMilkSilos, 1);

                if (!FluidMilkLines.Empty()) {
                    auto tmp = (Process *) FluidMilkLines.GetFirst();
                    tmp->Activate();
                }

                Leave(ClarificationLinesFast, 1);
            } else if (!this->is_fast and ReceptionMilkSilos.Used() and
                       ClarificationMilkSilos.Free() -
                       (ClarificationLinesFast.Used() - ClarificationLinesSlow.Used())) {
                Enter(ClarificationLinesSlow, 1);
                Leave(ReceptionMilkSilos, 1);
                Wait(CLARIFICATION_PROCESSING_SPEED_SLOW);
                Enter(ClarificationMilkSilos, 1);

                if (!FluidMilkLines.Empty()) {
                    auto tmp = (Process *) FluidMilkLines.GetFirst();
                    tmp->Activate();
                }

                Leave(ClarificationLinesSlow, 1);
            } else {
                ClarificatorsQueue.Insert(this);
                this->Passivate();
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
                    long free_capacity = 0;
                    if (i < 5) free_capacity = ReceptionMilkSilos.Free();
                    else if (i == 5) free_capacity = ReceptionDerivativesSilos.Free();
                    else if (i == 6) free_capacity = ReceptionCreamSilos.Free();

                    potentional_links.emplace_back(
                            std::make_tuple(free_capacity - (ReceptionLines[i].QueueLen() * 25),
                                            not ReceptionLines[i].Busy(), i));
                }
            }

            std::vector<int> silos_max_capacities = {RECEPTION_MILK_SILOS_CAPACITY,
                                                     RECEPTION_DERIVATIVES_SILOS_CAPACITY,
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

            /*for (auto &a: empty_silos_indexes) {
                DEBUG_PRINT("Empty Silos: %d\n", a);
            }

            for (auto &a: potentional_links) {
                DEBUG_PRINT("Index: %d -- %d -- %d\n", std::get<0>(a), std::get<1>(a), std::get<2>(a));
            }*/
        } else {
            stats.find("Infected Milk")->second++;
        }
        return idx;
    }

public:
    void Behavior() override {
        Arrival = Time;
        //DEBUG_PRINT("TANKER: %g (Capacity: %d)\n", Time, ReceptionMilkSilos.Used());

        Seize(Inspection);
        Wait(INSPECTION_PERIOD);
        Release(Inspection);

        int idx = get_free_reception_link();
        if (idx != -1) {
            /*DEBUG_PRINT("WINNER = %d\n", idx);
            DEBUG_PRINT("----------------\n");*/

            double brought_milk = Normal(std::get<2>(current_month), std::get<3>(current_month));
            stats.find("Received Milk")->second += brought_milk;
            Seize(ReceptionLines[idx]);
            if (idx < RECEPTION_LINES - 2) {
                while (brought_milk > 1) {
                    Enter(ReceptionMilkSilos, 1);
                    Wait(RECEPTION_MILK_PUMPING_SPEED);

                    if (ClarificatorsQueue.Length()) {
                        auto tmp = (Process *) ClarificatorsQueue.GetFirst();
                        tmp->Activate();
                    }

                    brought_milk -= 1;
                    //(new Clarification)->Activate();
                }
            } else if (idx == RECEPTION_LINES - 2) {
                while (brought_milk > 1) {
                    Enter(ReceptionDerivativesSilos, 1);
                    Wait(RECEPTION_MILK_PUMPING_SPEED);
                    brought_milk -= 1;
                }

            } else if (idx == RECEPTION_LINES - 1) {
                while (brought_milk > 1) {
                    Enter(ReceptionCreamSilos, 1);
                    Wait(RECEPTION_CREAM_PUMPING_SPEED);
                    brought_milk--;
                }
            }
            Release(ReceptionLines[idx]);
        }

        TankersTime(Time - Arrival);
    }
};


class PasteurizedGenerator : public Event {
private:
    double milk_production = Uniform(18.1872, 21.7134);

public:
    void Behavior() override {
        (new PasteurizedMilk)->Activate();
        Activate(Time + (HOUR / this->milk_production));
    }
};


class WholeMilkGenerator : public Event {
private:
    double whole_milk = Uniform(18.0690, 21.1320);

public:
    void Behavior() override {
        (new WholeMilk)->Activate();
        Activate(Time + (HOUR / this->whole_milk));
    }
};

class LightMilkGenerator : public Event {
private:
    double light_milk = Uniform(12.1056, 14.1452);

public:
    void Behavior() override {
        (new LightMilk)->Activate();
        Activate(Time + (HOUR / this->light_milk));
    }
};

class LactoFreeMilkGenerator : public Event {
private:
    double lacto_free_milk = Uniform(12.1056, 14.1452);

public:
    void Behavior() override {
        (new LactoFreeMilk)->Activate();
        Activate(Time + (HOUR / this->lacto_free_milk));
    }
};

class StrawberryFlavoredMilkGenerator : public Event {
private:
    double strawberry_flavored_milk = Uniform(12.1056, 14.1452);

public:
    void Behavior() override {
        (new StrawberryFlavoredMilk)->Activate();
        Activate(Time + (HOUR / this->strawberry_flavored_milk));
    }
};

class CholesterolFreeMilkGenerator : public Event {
private:
    double cholesterol_free_milk = Uniform(8.0704, 9.6368);
    //double strawberry_with_fruits = Uniform(5.0440, 6.0230);
    //double mango_with_fruits = Uniform(3.5308, 4.2161);

public:
    void Behavior() override {
        (new CholesterolFreeMilk)->Activate();
        Activate(Time + (HOUR / this->cholesterol_free_milk));
    }
};


class Initialization : public Event {
private:
    unsigned month = 0;

public:
    void Init(unsigned start_month) {
        //ClarificationLinesSlow.SetQueue(&ClarificationQueue);
        //ClarificationLinesFast.SetQueue(&ClarificationQueue);
        for (unsigned i = 0; i < CLARIFICATION_LINES; i++) {
            i < (CLARIFICATION_LINES >> 1) ? (new Clarification)->Init(false) : (new Clarification)->Init(true);
        }

        month = start_month;
        Activate();
        (new PasteurizedGenerator)->Activate();
        (new WholeMilkGenerator)->Activate();
        (new LightMilkGenerator)->Activate();
        (new LactoFreeMilkGenerator)->Activate();
        (new StrawberryFlavoredMilkGenerator)->Activate();
        (new CholesterolFreeMilkGenerator)->Activate();
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
        //DEBUG_PRINT("EXPONETIAL: %g (TIME: %g)\n", Exponential(std::get<1>(current_month)), Time);
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
                switch (period.back()) {
                    case 'h': {
                        period_value = (unsigned) std::stoul(period.substr(0, period.size() - 1));
                        argv_map.find("sim_period")->second = period_value * HOUR;
                        break;
                    }
                    case 'd': {
                        period_value = (unsigned) std::stoul(period.substr(0, period.size() - 1));
                        argv_map.find("sim_period")->second = period_value * DAY;
                        break;
                    }
                    case 'w': {
                        period_value = (unsigned) std::stoul(period.substr(0, period.size() - 1));
                        DEBUG_PRINT("WEEK: %d\n", period_value);
                        argv_map.find("sim_period")->second = period_value * WEEK;
                        break;
                    }
                    case 'm': {
                        period_value = (unsigned) std::stoul(period.substr(0, period.size() - 1));
                        break;
                    }
                    case 'y': {
                        period_value = (unsigned) std::stoul(period.substr(0, period.size() - 1));
                        argv_map.find("sim_period")->second = period_value * compute_seconds_of_year();
                        break;
                    }
                    default: {
                    }
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
    ReceptionDerivativesSilos.Output();
    ReceptionCreamSilos.Output();
    ClarificationLinesFast.Output();
    ClarificationLinesSlow.Output();
    ClarificatorsQueue.Output();
    ClarificationMilkSilos.Output();
    ClarificationCreamSilos.Output();
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
    FactoryStore.Output();
    FluidMilkLines.Output();

    for (auto &ReceptionLine : ReceptionLines) {
        ReceptionLine.Output();
    }

    //for (auto &stat : stats) {
    //    std::cout << stat.first << " = " << stat.second << std::endl;
    //}
}


int main(int argc, char **argv) {
    std::map<std::string, unsigned> argv_map = process_args(argc, argv);
    std::cout << "Milk developing process" << std::endl;
    SetOutput("milk-app.out");
    std::srand(std::time(0));
    ReceptionLines[RECEPTION_LINES - 1].SetName("CreamReceptionLines");
    DEBUG_PRINT("PERIOD: %d\n", argv_map.at("sim_period"));
    Init(0, argv_map.at("sim_period"));
    (new Generator)->Activate();
    (new Initialization)->Init(argv_map.at("start_month"));
    Run();
    print_stats();
    return 0;
}