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

Facility Inspection("Inspection");              ///< 1 inspection line for check correctness of milk
Facility Reception[RECEPTION_LINES];            ///< 6 reception lines for pumping milk from tanker to reception silos
Facility Homogenizer("Homogenizer");
Facility Pasteurizer("Pasteurizer");
Facility UltraPasteurizer("UltraPasteurizer");
Facility SmallBottleLine("SmallBottleLine");

Store Clarifiers("Clarifiers", CLARIFICATION_LINES);
Store PasteurizedBottleLines("PasteurizedBottleLines", PASTEURIZED_BOTTLE_LINES);
Store UltraPasteurizedBottleLines("UltraPasteurizedBottleLines", ULTRA_PASTEURIZED_BOTTLE_LINES);
Store ReceptionSilos("ReceptionSilos", RECEPTION_SILOS_CAPACITY);    ///< 4 x 160 000 Liters
Store ProcessingSilos("ProccesingSilos", PROCESSING_SILOS_CAPACITY);
Store HomogenizationSilos("HomegenizationSilos", HOMOGENIZATION_SILOS_CAPACITY);
Store PasteurizationSilos("PasteurizationSilos", PASTEURIZATION_SILOS_CAPACITY);
Store UltraPasteurizationSilos("UltraPasteurizationSilos", PASTEURIZATION_SILOS_CAPACITY);

Histogram TankerTime("TankerTime", 0, 1000, 10);
Queue Auxiliary_Queues[4];

///< current month for modelling the frequency of dependencies on different periods of year
std::tuple<int, double, double, double> current_month;

///< custom statistics values
std::map<std::string, int> stats = {
        {"Infected Milk", 0},
};


unsigned long compute_seconds_of_year() {
    unsigned long seconds_of_year = 0;
    for (auto const &month : monthly_details) {
        seconds_of_year += std::get<0>(month.second) * DAY;
    }
    return seconds_of_year;
}

void print_stats(const std::string &file_name) {
    Inspection.Output();
    for (unsigned i = 0; i < RECEPTION_LINES; i++) {
        Reception[i].Output();
    }
    ReceptionSilos.Output();
    ProcessingSilos.Output();
    TankerTime.Output();
    Clarifiers.Output();
    Homogenizer.Output();
    Pasteurizer.Output();
    PasteurizationSilos.Output();
    UltraPasteurizer.Output();
    UltraPasteurizationSilos.Output();
    PasteurizedBottleLines.Output();
    SmallBottleLine.Output();
    UltraPasteurizedBottleLines.Output();

    for (std::pair<std::string, int> stats_item: stats) {
        std::ofstream file(file_name, std::ios_base::app | std::ios_base::out);
        file << stats_item.first << " " << stats_item.second << std::endl;
    }
}

class PasteurizedPackaging : public Process {
public:
    void Behavior() override {
        Enter(PasteurizedBottleLines, 1);
        Leave(PasteurizationSilos);

        if (!Auxiliary_Queues[2].Empty()) {
            auto tmp = (Process *) Auxiliary_Queues[2].GetFirst();
            tmp->Activate();
        }

        Wait(PASTEURIZED_PACKAGING_PERIOD * MINUTE);
        Leave(PasteurizedBottleLines, 1);
    }
};

class UltraPasteurizedPackaging : public Process {
public:
    void Behavior() override {
        if (Random() < 0.25) {
            Seize(SmallBottleLine);
            Leave(UltraPasteurizationSilos, 1);

            if (!Auxiliary_Queues[3].Empty()) {
                auto tmp = (Process *) Auxiliary_Queues[3].GetFirst();
                tmp->Activate();
            }

            Wait(SMALL_BOTTLE_PACKAGING_PERIOD);
            Release(SmallBottleLine);
        } else {
            Enter(UltraPasteurizedBottleLines, 1);
            Leave(UltraPasteurizationSilos, 1);

            if (!Auxiliary_Queues[3].Empty()) {
                auto tmp = (Process *) Auxiliary_Queues[3].GetFirst();
                DEBUG_PRINT("WAKE UP ULTRA! (%s) --- %d\n", tmp->Name(), UltraPasteurizationSilos.Free());
                tmp->Activate();
            }

            Wait(ULTRA_PASTEURIZED_PACKAGING_PERIOD * MINUTE);
            Leave(UltraPasteurizedBottleLines, 1);
        }

    }
};

class Pasteurization : public Process {
public:
    void Behavior() override {
        if (Random() < 0.5) {   ///< pasteurization line
            int busy = (Pasteurizer.Busy() ? 1 : 0);
            if (int(PasteurizationSilos.Free() - (Pasteurizer.QueueLen()+1+busy)) < 0) {
                Auxiliary_Queues[2].Insert(this);
                Passivate();
            }

            Seize(Pasteurizer);

            if (PasteurizationSilos.Free() <= 0) {
                DEBUG_PRINT("PASTEURIZER: %d\n", PasteurizationSilos.Free());
            }
            Leave(HomogenizationSilos, 1);

            if (!Auxiliary_Queues[2].Empty()) {
                auto tmp = (Process *) Auxiliary_Queues[2].GetFirst();
                tmp->Activate();
            }

            Wait(PASTEURIZATION_PERIOD * MINUTE);
            Enter(PasteurizationSilos, 1);
            Release(Pasteurizer);
            (new PasteurizedPackaging)->Activate();
        } else {                ///< ultra pasteurization line
            int busy = (UltraPasteurizer.Busy() ? 1 : 0);
            if (int(UltraPasteurizationSilos.Free() - (UltraPasteurizer.QueueLen()+1+busy)) < 0) {
                Auxiliary_Queues[3].Insert(this);
                Passivate();
            }

            Seize(UltraPasteurizer);

            if (UltraPasteurizationSilos.Free() <= 0) {
                DEBUG_PRINT("ULTRAPASTEURIZER: %d (%s)\n", UltraPasteurizationSilos.Free(), this->Name());
            }

            Leave(HomogenizationSilos, 1);

            if (!Auxiliary_Queues[1].Empty()) {
                auto tmp = (Process *) Auxiliary_Queues[1].GetFirst();
                tmp->Activate();
            }

            Wait(ULTRA_PASTEURIZATION_PERIOD * MINUTE);
            Enter(UltraPasteurizationSilos, 1);
            Release(UltraPasteurizer);
            (new UltraPasteurizedPackaging)->Activate();
        }

    }
};

class Homogenization : public Process {
public:
    void Behavior() override {
        int busy = (Homogenizer.Busy() ? 1 : 0);
        if (int(HomogenizationSilos.Free() - (Homogenizer.QueueLen()+1+busy)) < 0) {
            Auxiliary_Queues[1].Insert(this);
            Passivate();
        }

        Seize(Homogenizer);

        if (HomogenizationSilos.Free() <= 0) {
            DEBUG_PRINT("HOMOGENIZER %d\n", HomogenizationSilos.Free());
        }

        Leave(ProcessingSilos, 1);

        if (!Auxiliary_Queues[0].Empty()) {
            auto tmp = (Process *) Auxiliary_Queues[0].GetFirst();
            tmp->Activate();
        }

        Wait(0.1 * MINUTE);
        Enter(HomogenizationSilos, 1);
        Release(Homogenizer);
        (new Pasteurization)->Activate();
    }
};

class Clarification : public Process {
public:
    void Behavior() override {

        ///< content of ProcessingSilos with the currently processing milk
        if (int(ProcessingSilos.Free() - (Clarifiers.Used() + Clarifiers.QueueLen() + 1)) < 0) { ///< no free space in ProcessingSilos
            ///< must waiting on processing milk by next machines, that pumping from these silos
            Auxiliary_Queues[0].Insert(this);
            Passivate();
        }

        Enter(Clarifiers, 1);

        if (ProcessingSilos.Free() - Clarifiers.Used() <= 0) {
            DEBUG_PRINT("CLARIFICATORS: 0\n");
        }

        ///< milk is pumping to the Clarificator silos for its processing
        Leave(ReceptionSilos, 1);
        if (Clarifiers.Free() > (CLARIFICATION_LINES >> 1)) {   ///< first half of clarificators processing faster
            Wait(FAST_CLARIFICATION_PERIOD * MINUTE);
        } else {    ///< second half of clarificator processing slowly
            Wait(SLOW_CLARIFICATION_PERIOD * MINUTE);
        }
        ///< milk is pumping from the Clarificator silos to Processing Silos after its processing (clarification)
        Enter(ProcessingSilos, 1);
        Leave(Clarifiers, 1);
        ///< signal for next phase of whole process of developing of milk
        (new Homogenization)->Activate();
    }
};


class Tanker : public Process {
public:
    double Arrival = 0.0;

    void Behavior() override {
        Arrival = Time;
        Seize(Inspection);
        ///< removal period of obtained milk sample from tanker
        Wait(MINUTE);
        Release(Inspection);

        if (Random() > PROBABILITY_OF_INFECTED_MILK) { ///< successful inspection
            double brought_milk = Normal(std::get<2>(current_month), std::get<3>(current_month));
            int idx = 0;
            for (unsigned i = 0; i < RECEPTION_LINES; i++) {
                if (Reception[i].QueueLen() < Reception[idx].QueueLen()) {
                    idx = i;
                }
            }
            //DEBUG_PRINT("SELECT no. %d: %d\n", idx,  Reception[idx].QueueLen());
            Seize(Reception[idx]);

            while (brought_milk > 0) {
                Enter(ReceptionSilos, 1);
                Wait(PUMPING_FROM_TANKER_PERIOD * MINUTE); ///< 40 000 Liters per Hour => 1000 Litres per 1.5 Minute
                (new Clarification)->Activate();
                brought_milk--;
            }
            Release(Reception[idx]);
        } else {    ///< unsuccessful inspection -> tanker leaving the system
            stats.find("Infected Milk")->second++;
        }
        TankerTime(Time - Arrival);
    }

};


class Month : public Event {
private:
    unsigned month;

public:
    void Init(unsigned start_month) {
        month = start_month;
        Activate();
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

class FillingSilos : public Process {
public:
    void Behavior() override {
        Enter(ReceptionSilos, RECEPTION_SILOS_CAPACITY);
        for(unsigned i = 0; i < RECEPTION_SILOS_CAPACITY; i++) {
            (new Clarification)->Activate();
        }

        Enter(ProcessingSilos, PROCESSING_SILOS_CAPACITY);
        for (unsigned i = 0; i < PROCESSING_SILOS_CAPACITY; i++) {
            (new Homogenization)->Activate();
        }

        Enter(HomogenizationSilos, HOMOGENIZATION_SILOS_CAPACITY);
        for (unsigned i = 0; i < HOMOGENIZATION_SILOS_CAPACITY; i++) {
            (new Pasteurization)->Activate();
        }

        Enter(PasteurizationSilos, PASTEURIZATION_SILOS_CAPACITY);
        for (unsigned i = 0; i < PASTEURIZATION_SILOS_CAPACITY; i++) {
            (new PasteurizedPackaging)->Activate();
        }

        Enter(UltraPasteurizationSilos, PASTEURIZATION_SILOS_CAPACITY);
        for (unsigned i = 0; i < PASTEURIZATION_SILOS_CAPACITY; i++) {
            (new UltraPasteurizedPackaging)->Activate();
        }
    }
};


int main(int argc, char **argv) {
    std::map<std::string, unsigned> argv_map = process_args(argc, argv);
    std::cout << "Milk developing process" << std::endl;
    SetOutput("milk-app.out");
    std::srand(std::time(0));
    Init(0, argv_map.at("sim_period"));
    (new FillingSilos)->Activate();
    (new Generator)->Activate();
    (new Month)->Init(argv_map.at("start_month"));
    Run();
    print_stats("milk-app.out");
    return 0;
}