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
Store CreamPasteurizatorsFast("CreamPasteurizatorsFast", CREAM_PASTEURIZERS_FAST);
Store CreamPasteurizatorsSlow("CreamPasteurizatorsSlow", CREAM_PASTEURIZERS_SLOW);
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
    if (!ClarificatorsQueue.Empty()) {
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
        this->ProcessingMachineFirst = processing_machine_first;
        this->ProcessingMachineSecond = processing_machine_second;
        this->ProcessingPeriodFirst = processing_period_first;
        this->ProcessingPeriodSecond = processing_period_second;
        this->liter_presentation = liter;
    }

    void Behavior() override {
        Store *ProcessingMachine;
        double ProcessingPeriod;
        if (this->ProcessingMachineFirst->Free() or not this->ProcessingMachineSecond or
            not this->ProcessingMachineSecond->Free()) {
            ProcessingMachine = this->ProcessingMachineFirst;
            ProcessingPeriod = this->ProcessingPeriodFirst;
        } else {
            ProcessingMachine = this->ProcessingMachineSecond;
            ProcessingPeriod = this->ProcessingPeriodSecond;
        }

        Enter(*ProcessingMachine, 1);
        planned_production--;
        Leave(ClarificationMilkSilos, 1);
        ClarificationActivate();
        Wait(ProcessingPeriod);
        this->liter_presentation ? Enter(FactoryStoreLiterBottle, 1) : Enter(FactoryStoreLittleBottle, 1);
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
        //DEBUG_PRINT("END = %d (%d) || %d || %d\n", ClarificationMilkSilos.Used(), ClarificationMilkSilosCapacity,
        //            FluidMilkLines.Length(), waiting_milk());
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

        this->Activate();
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

                    if (not ClarificatorsQueue.Empty() and
                        (ReceptionMilkSilos.Used() > 0.25 * RECEPTION_MILK_SILOS_CAPACITY or
                         not FluidMilkLines.Empty())) {
                        while (not ClarificatorsQueue.Empty()) {
                            auto tmp = (Process *) ClarificatorsQueue.GetFirst();
                            tmp->Activate();
                        }
                    } else {
                        //DEBUG_PRINT("ACTIVATION: %d\n", FluidMilkLines.Length());
                    }

                    brought_milk -= 1;
                }
            } else if (idx == RECEPTION_LINES - 2) {
                while (brought_milk > 1) {
                    Enter(ReceptionDerivativesSilos, 1);
                    Wait(RECEPTION_MILK_PUMPING_SPEED);

                    if (not StandardizatorsQueue.Empty() and
                        ReceptionDerivativesSilos.Used() > 0.25 * RECEPTION_DERIVATIVES_SILOS_CAPACITY) {
                        auto tmp = (Process *) StandardizatorsQueue.GetFirst();
                        tmp->Activate();
                    }

                    brought_milk -= 1;
                }

            } else if (idx == RECEPTION_LINES - 1) {
                while (brought_milk > 1) {
                    Enter(ReceptionCreamSilos, 1);
                    Wait(RECEPTION_CREAM_PUMPING_SPEED);

                    if (not StandardizatorsQueue.Empty() and
                        ReceptionCreamSilos.Used() > 0.25 * RECEPTION_CREAM_SILOS_CAPACITY) {
                        auto tmp = (Process *) StandardizatorsQueue.GetFirst();
                        tmp->Activate();
                    }

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
            if (int(CreamBottleTank.Used() - (CreamBottleMachines.Used() + CreamBottleMachines.QueueLen())) > 0) {
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
                this->Passivate();
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
            this->CreamPasteurizator = &CreamPasteurizatorsFast;
            this->PasteurizationPeriod = CREAM_PASTEURIZATORS_FAST_SPEED;
        } else {
            this->CreamPasteurizator = &CreamPasteurizatorsSlow;
            this->PasteurizationPeriod = CREAM_PASTEURIZATORS_SLOW_SPEED;
        }

        this->Activate();
    }

    void Behavior() override {

        for (;;) {
            if (StandardizatorTank.Used() and
                CreamBottleTank.Free() - (CreamPasteurizatorsSlow.Used() + CreamPasteurizatorsFast.Used()) > 0) {
                Enter(*this->CreamPasteurizator, 1);
                Leave(StandardizatorTank, 1);

                if (not StandardizatorsQueue.Empty()) {
                    auto tmp = (Process *) StandardizatorsQueue.GetFirst();
                   tmp->Activate();
                }

                Wait(this->PasteurizationPeriod);
                Enter(CreamBottleTank, 1);

                if (not CreamPackagingQueue.Empty()) {
                    auto tmp = (Process *) CreamPackagingQueue.GetFirst();
                    tmp->Activate();
                }

                Leave(*this->CreamPasteurizator, 1);
            } else {
                CreamPasteurizatorsQueue.Insert(this);
                this->Passivate();
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
            this->Standardizator = &StandardizatorFast;
            this->StandardizationPeriod = STANDARDIZATORS_FAST_SPEED;
        } else {
            this->Standardizator = &StandardizatorSlow;
            this->StandardizationPeriod = STANDARDIZATORS_SLOW_SPEED;
        }

        this->Activate();
    }

    void Behavior() override {

        for (;;) {
            if ((ReceptionCreamSilos.Used() or ReceptionDerivativesSilos.Used())
                and StandardizatorTank.Free() -
                    (StandardizatorFast.Used() + StandardizatorSlow.Used()) > 0) {
                Enter(*this->Standardizator, 1);

                if (ReceptionCreamSilos.Used() * 0.53 > ReceptionDerivativesSilos.Used()) {
                    Leave(ReceptionCreamSilos, 1);
                } else {
                    Leave(ReceptionDerivativesSilos, 1);
                }

                Wait(this->StandardizationPeriod);
                Enter(StandardizatorTank, 1);

                if (not CreamPasteurizatorsQueue.Empty()) {
                    auto tmp = (Process *) CreamPasteurizatorsQueue.GetFirst();
                    tmp->Activate();
                }

                Leave(*this->Standardizator, 1);
            } else {
                StandardizatorsQueue.Insert(this);
                this->Passivate();
            }
        }
    }

};


class PasteurizedGenerator : public Event {
private:
    double milk_production = Uniform(18.1872, 21.7134);

public:
    void Behavior() override {
        //DEBUG_PRINT("1: %g\n", milk_production);
        FluidMilkLines.Insert(new PasteurizedMilk);
        Activate(Time + (HOUR / milk_production));
    }
};


class WholeMilkGenerator : public Event {
private:
    double whole_milk = Uniform(18.0690, 21.1320);

public:
    void Behavior() override {
        //DEBUG_PRINT("2: %g\n", whole_milk);
        FluidMilkLines.Insert(new UltraPasteurizedMilk(WholeMilkMachinesFast, WholeMilkMachinesSlow,
                                                       WHOLE_MILK_MACHINES_SPEED_FAST,
                                                       WHOLE_MILK_MACHINES_SPEED_SLOW));
        Activate(Time + (HOUR / whole_milk));
    }
};


class LightMilkGenerator : public Event {
private:
    double light_milk = Uniform(12.1056, 14.1452);

public:
    void Behavior() override {
        //DEBUG_PRINT("3: %g\n", light_milk);
        FluidMilkLines.Insert(new UltraPasteurizedMilk(LightMilkMachines, nullptr, LIGHT_MILK_MACHINES_SPEED, 0));
        Activate(Time + (HOUR / light_milk));
    }
};

class LactoFreeMilkGenerator : public Event {
private:
    double lacto_free_milk = Uniform(12.1056, 14.1452);

public:
    void Behavior() override {
        //DEBUG_PRINT("4: %g\n", lacto_free_milk);
        FluidMilkLines.Insert(
                new UltraPasteurizedMilk(LactoFreeMilkMachines, nullptr, LACTO_FREE_MILK_MACHINES_SPEED, 0));
        Activate(Time + (HOUR / lacto_free_milk));
    }
};

class StrawberryFlavoredMilkGenerator : public Event {
private:
    double strawberry_flavored_milk = Uniform(12.1056, 14.1452);

public:
    void Behavior() override {
        //DEBUG_PRINT("5: %g\n", strawberry_flavored_milk);
        FluidMilkLines.Insert(
                new UltraPasteurizedMilk(StrawberryFlavoredMilkMachines, nullptr,
                                         STRAWBERRY_FLAVORED_MILK_MACHINES_SPEED, 0));
        Activate(Time + (HOUR / strawberry_flavored_milk));
    }
};

class CholesterolFreeMilkGenerator : public Event {
private:
    double cholesterol_free_milk = Uniform(8.0704, 9.6368);

public:
    void Behavior() override {
        //DEBUG_PRINT("6: %g\n", cholesterol_free_milk);
        FluidMilkLines.Insert(
                new UltraPasteurizedMilk(CholesterolFreeMilkMachines, nullptr, CHOLESTEROL_FREE_MILK_MACHINES_SPEED,
                                         0));
        Activate(Time + (HOUR / cholesterol_free_milk));
    }
};

class StrawberryFruitsMilkGenerator : public Event {
private:
    double strawberry_with_fruits = Uniform(5.0440, 6.0230);

public:
    void Behavior() override {
        //DEBUG_PRINT("7: %g\n", strawberry_with_fruits);
        FluidMilkLines.Insert(
                new UltraPasteurizedMilk(StrawberryFruitMilkMachinesFast, StrawberryFruitMilkMachinesSlow,
                                         STRAWBERRY_FRUIT_MILK_MACHINES_FAST_SPEED,
                                         STRAWBERRY_FRUIT_MILK_MACHINES_SLOW_SPEED, false));
        Activate(Time + (HOUR / strawberry_with_fruits));
    }
};

class MangoFruitsMilkGenerator : public Event {
private:
    double mango_with_fruits = Uniform(3.5308, 4.2161);

public:
    void Behavior() override {
        //DEBUG_PRINT("8: %g\n", mango_with_fruits);
        FluidMilkLines.Insert(
                new UltraPasteurizedMilk(MangoFrutiMilkMachines, nullptr, MANGO_FRUIT_MILK_MACHINES_SPEED, 0, false));
        Activate(Time + (HOUR / mango_with_fruits));
    }
};

class CreamGenerator : public Event {
private:
    double cream_production = Uniform(60.2054, 69.5887);

public:
    void Behavior() override {
        (new CreamPackaging)->Activate();
        Activate(Time + (HOUR / this->cream_production));
    }
};


class Initialization : public Event {
private:
    unsigned month = 0;

public:
    void Init(unsigned start_month) {
        for (unsigned i = 0; i < CLARIFICATION_LINES; i++) {
            i < (CLARIFICATION_LINES >> 1) ? (new Clarification)->Init(false) : (new Clarification)->Init(true);
        }

        for (unsigned i = 0; i < STANDARDIZATORS; i++) {
            i < (STANDARDIZATORS - 1) ? (new Standardization)->Init(false) : (new Standardization)->Init(true);
        }

        for (unsigned i = 0; i < CREAM_PASTEURIZERS_SLOW + CREAM_PASTEURIZERS_SLOW; i++) {
            i < (CREAM_PASTEURIZERS_SLOW + CREAM_PASTEURIZERS_FAST - 1) ? (new CreamPasteurization)->Init(false)
                                                                        : (new CreamPasteurization)->Init(true);
        }

        month = start_month;
        Activate();
        (new PasteurizedGenerator)->Activate();
        (new WholeMilkGenerator)->Activate();
        (new LightMilkGenerator)->Activate();
        (new LactoFreeMilkGenerator)->Activate();
        (new StrawberryFlavoredMilkGenerator)->Activate();
        (new CholesterolFreeMilkGenerator)->Activate();
        (new FluidMilkGenerator)->Activate();
        (new StrawberryFruitsMilkGenerator)->Activate();
        (new MangoFruitsMilkGenerator)->Activate();
        (new CreamGenerator)->Activate();
    }

    void Behavior() override {
        current_month = monthly_details.at(month);
        (month + 1) % MONTH_COUNT ? month++ : month = 0;
        Activate(Time + std::get<0>(current_month) * DAY);
    }
};


class Production : public Process {
    void Behavior() override {
        Enter(ReceptionCreamSilos, RECEPTION_CREAM_SILOS_CAPACITY);
        Enter(ReceptionMilkSilos, RECEPTION_MILK_SILOS_CAPACITY);
        Enter(ReceptionDerivativesSilos, RECEPTION_DERIVATIVES_SILOS_CAPACITY);
        Enter(ClarificationMilkSilos, CLARIFICATION_MILK_SILOS_CAPACITY);
        Enter(CreamBottleTank, CREAM_BOTTLE_TANKS_CAPACITY);
        Enter(StandardizatorTank, STANDARDIZATORS_TANKS_CAPACITY);

        std::default_random_engine generator;

        double mu = 229537.93;
        double sigma = 69776.62;
        double normal_std = sqrt(log(1 + (sigma / mu) * (sigma / mu)));
        double normal_mean = log(mu) - normal_std * normal_std / 2;
        std::lognormal_distribution<double> WholeMilk(normal_mean, normal_std);
        double whole_milk = WholeMilk(generator);
        DEBUG_PRINT("Whole Milk: %.4f\n", whole_milk/31);

        std::weibull_distribution<double> WeibullLight(10.119, 36666.36);
        double light_milk = WeibullLight(generator);
        DEBUG_PRINT("Light Milk: %.4f\n", light_milk/31);

        double drink_yogurt_strawberry_coco = Triag(25967.76, 40661.22, 221153);
        DEBUG_PRINT("Drink Milk Strawberry-Coconut: %.4f\n", drink_yogurt_strawberry_coco/31);

        mu = 130621.49;
        sigma = 47559.78;
        normal_std = sqrt(log(1 + (sigma / mu) * (sigma / mu)));
        normal_mean = log(mu) - normal_std * normal_std / 2;
        std::lognormal_distribution<double> DrinkMilk(normal_mean, normal_std);
        double drink_yogurt_pineapple_coco = DrinkMilk(generator);
        DEBUG_PRINT("Drink Milk PineApple-Coconut: %.4f\n", drink_yogurt_pineapple_coco/31);

        double strawberry_flavoured = Normal(63342.21, 16008);
        DEBUG_PRINT("Strawberry Flavoured: %.4f\n", strawberry_flavoured/31);

        double ultra_strawberry_milk = Triag(0.65235, 0.65235, 35401.05);
        DEBUG_PRINT("Ultra Strawberry: %.4f\n", ultra_strawberry_milk/31);

        double ultra_mango_milk = Triag(0.00276, 0.00276, 24286.53);
        DEBUG_PRINT("Ultra Mango: %.4f\n", ultra_mango_milk/31);

        double cereal_yogurt_strawberry_nut = Uniform(1775.45, 3791.46);
        //DEBUG_PRINT("Cereal Yogurt Strawberry-Nut: %.4f\n", cereal_yogurt_strawberry_nut/3);

        double cereal_yogurt_peach_nut = Triag(0.02711, 0.02717, 11662.38278);
        //DEBUG_PRINT("Cereal Yogurt Peach-Nut: %.4f\n", cereal_yogurt_peach_nut/3);

        std::weibull_distribution<double> WeibullSmallCream(32.022, 1467150.23);
        double small_cream = WeibullSmallCream(generator);
        DEBUG_PRINT("Small Cream: %.4f\n", small_cream/30);

        std::weibull_distribution<double> WeibullBigCream(7.0071, 858032.40);
        double big_cream = WeibullBigCream(generator);
        DEBUG_PRINT("Big Cream: %.4f\n", big_cream/30);

        std::weibull_distribution<double> WeibullYogurtS(9.970, 201340);
        double yogurt_strawberry = WeibullYogurtS(generator);
        //DEBUG_PRINT("Yogurt Strawberry: %.4f\n", yogurt_strawberry/3);

        double yogurt_peach = Normal(831686, 182164.92);
        //DEBUG_PRINT("Yogurt Peach: %.4f\n", yogurt_peach/3);

        double sum = (whole_milk + light_milk + drink_yogurt_pineapple_coco + drink_yogurt_strawberry_coco +
                      strawberry_flavoured + ultra_mango_milk + ultra_strawberry_milk +
                      cereal_yogurt_peach_nut + cereal_yogurt_strawberry_nut + small_cream + big_cream +
                      yogurt_peach + yogurt_strawberry) / 30;

        //DEBUG_PRINT("SUM = %.4f\n", sum );

        //DEBUG_PRINT("-----------------\n");
        //Activate(Time + (DAY * 7) + 1);
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

    //for (auto &stat : stats) {
    //    std::cout << stat.first << " = " << stat.second << std::endl;
    //}
    DEBUG_PRINT("Received Milk: %d\n", stats.at("Received Milk"));
    DEBUG_PRINT("Rejected Milk: %d\n", stats.at("Infected Milk"));
}


int main(int argc, char **argv) {
    std::map<std::string, unsigned> argv_map = process_args(argc, argv);
    std::cout << "Milk developing process" << std::endl;
    SetOutput("milk-app.out");
    std::srand(std::time(0));
    ReceptionLines[RECEPTION_LINES - 1].SetName("CreamReceptionLines");
    DEBUG_PRINT("PERIOD: %d\n", argv_map.at("sim_period"));
    Init(0, argv_map.at("sim_period"));
    (new Production)->Activate();
    (new Generator)->Activate();
    (new Initialization)->Init(argv_map.at("start_month"));
    Run();
    print_stats();
    return 0;
}