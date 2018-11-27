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

#include <cstring>
#include "milk-app.h"

Facility Inspection("Inspection");              ///< 1 inspection line for check correctness of milk
Facility Reception[RECEPTION_LINES];            ///< 6 reception lines for pumping milk from tanker to reception silos
Store ReceptionSilos("ReceptionSilos", RECEPTION_SILOS_CAPACITY);    ///< 4 x 160 000 Liters
Store ProcessingSilos("ProccesingSilog", PROCESSING_SILOS_CAPACITY);
Histogram TankerTime("TankerTime", 0, 1000, 10);

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

void print_stats(const std::string& file_name) {
    Inspection.Output();
    for(unsigned i = 0; i < RECEPTION_LINES; i++) {
        Reception[i].Output();
    }
    ReceptionSilos.Output();
    TankerTime.Output();

    for (std::pair<std::string, int> stats_item: stats) {
        std::ofstream file(file_name, std::ios_base::app | std::ios_base::out);
        file << stats_item.first << " " << stats_item.second << std::endl;
    }
}

class Tanker : public Process {
public:
    double Arrival = 0.0;

    void Behavior() override {
        Arrival = Time;
        Seize(Inspection);
        Wait(MINUTE);   ///< removal period of obtained milk sample from tanker
        Release(Inspection);

        if (Random() > 0.005) { ///< successful inspection
            double brought_milk = Normal(std::get<2>(current_month), std::get<3>(current_month));
            int idx=0;
            for (unsigned  i = 0; i < RECEPTION_LINES; i++)
                if (Reception[i].QueueLen() < Reception[idx].QueueLen())
                    idx = i;
            Seize(Reception[idx]);

            while (brought_milk > 0) {
                Enter(ReceptionSilos, 1);
                Wait(1.5 * MINUTE); ///< 40 000 Liters per Hour => 1000 Litres per 1.5 Minute
                brought_milk--;
            }
            Release(Reception[idx]);
        } else {    ///< unsuccesful inspection -> tanker leaving the system
            stats.find("Infected Milk")->second++;
        }
        TankerTime(Time - Arrival);
    }

};

class Processor : public Process {
    void Behavior() override {
        if (ReceptionSilos.Free() == RECEPTION_SILOS_CAPACITY) {
            Passivate();
        } else {
            Enter(ProcessingSilos, 1);
            Leave(ReceptionSilos, 1);
        }
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
        DEBUG_PRINT("EXPONETIAL: %g (TIME: %g)\n", Exponential(std::get<1>(current_month)), Time);
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


int main(int argc, char **argv) {
    std::map<std::string, unsigned> argv_map = process_args(argc, argv);
    std::cout << "Milk developing process" << std::endl;
    SetOutput("milk-app.out");
    std::srand(std::time(0));
    Init(0, argv_map.at("sim_period"));
    (new Generator)->Activate();
    (new Month)->Init(argv_map.at("start_month"));
    (new Processor)->Activate();
    Run();
    print_stats("milk-app.out");
    return 0;
}