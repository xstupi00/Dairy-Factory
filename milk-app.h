/**************************************************************
 * Project:     DNS Export
 * File:		milk-app.h
 * Author:		Šimon Stupinský, Tomáš Zubrik
 * University: 	Brno University of Technology
 * Faculty: 	Faculty of Information Technology
 * Course:	    Modelling ang Simulation
 * Date:		27.11.2018
 * Last change:	09.12.2018
 *
 *
 **************************************************************/

/**
 * @file    milk-app.h
 * @brief
 */


#ifndef MILK_APP_H
#define MILK_APP_H

#include <algorithm>
#include <cstring>
#include <ctime>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <map>
#include <random>
#include <simlib.h>
#include <vector>


#ifdef DEBUG
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif

const unsigned MINUTE = 60;
const unsigned HOUR = 3600;
const unsigned DAY = 86400;
const unsigned WEEK = 604800;


#define CLARIFICATION_MILK_SILOS_CAPACITY       1700
#define RECEPTION_MILK_SILOS_CAPACITY           450
#define RECEPTION_CREAM_SILOS_CAPACITY          230
#define CREAM_BOTTLE_TANKS_CAPACITY             20
#define STANDARDIZATORS_TANKS_CAPACITY          1200

#define RECEPTION_LINES                         7
#define CLARIFICATION_LINES                     4
#define PASTEURIZATION_BOTTLE_MACHINES          2
#define WHOLE_MILK_MACHINES_FAST                2
#define WHOLE_MILK_MACHINES_SLOW                1
#define LIGHT_MILK_MACHINES                     4
#define LACTO_FREE_MILK_MACHINES                4
#define STRAWBERRY_FLAVORED_MILK_MACHINES       1
#define CHOLESTEROL_FREE_MILK_MACHINES          2
#define STRAWBERRY_FRUIT_MILK_MACHINES_SLOW     3
#define STRAWBERRY_FRUIT_MILK_MACHINES_FAST     1
#define MANGO_FRUIT_MILK_MACHINES               1
#define STANDARDIZATORS                         3
#define CREAM_PASTEURIZERS                      3
#define CREAM_BOTTLE_MACHINES                   3

#define INSPECTION_PERIOD                       (12.00 * MINUTE)
#define RECEPTION_MILK_PUMPING_SPEED            (1.50 * MINUTE)
#define RECEPTION_CREAM_PUMPING_SPEED           (4.00 * MINUTE)
#define CLARIFICATION_PROCESSING_SPEED_FAST     (2.00 * MINUTE)
#define CLARIFICATION_PROCESSING_SPEED_SLOW     (2.40 * MINUTE)
#define PASTEURIZATION_BOTTLE_MACHINES_SPEED    (3.34 * MINUTE)
#define HOMOGENIZATION_PERIOD                   (1.50 * MINUTE)
#define HOMOGENIZATION_PUMPING_SPEED            (2.00 * MINUTE)
#define PASTEURIZATION_PERIOD                   (0.41 * MINUTE)
#define STANDARDIZATORS_FAST_SPEED              (4.30 * MINUTE)
#define STANDARDIZATORS_SLOW_SPEED              (6.67 * MINUTE)
#define CREAM_PASTEURIZATORS_FAST_SPEED         (17.20 * MINUTE)

#define CREAM_BOTTLE_MACHINES_SPEED                 (6 * MINUTE)
#define CREAM_PASTEURIZATORS_SLOW_SPEED             (30 * MINUTE)
#define WHOLE_MILK_MACHINES_SPEED_FAST              (5 * MINUTE)
#define WHOLE_MILK_MACHINES_SPEED_SLOW              (10 * MINUTE)
#define LIGHT_MILK_MACHINES_SPEED                   (10 * MINUTE)
#define LACTO_FREE_MILK_MACHINES_SPEED              (10 * MINUTE)
#define STRAWBERRY_FLAVORED_MILK_MACHINES_SPEED     (5 * MINUTE)
#define CHOLESTEROL_FREE_MILK_MACHINES_SPEED        (10 * MINUTE)
#define STRAWBERRY_FRUIT_MILK_MACHINES_SLOW_SPEED   (40 * MINUTE)
#define STRAWBERRY_FRUIT_MILK_MACHINES_FAST_SPEED   (32 * MINUTE)
#define MANGO_FRUIT_MILK_MACHINES_SPEED             (12 * MINUTE)

#define PROBABILITY_INFECTED_MILK               0.005


enum Months {
    January, February, March, April, May, June, July, August, September, October, November, December, MONTH_COUNT
};

enum ProcessingMachines {
    Pasteurized,
    WholeMilk,
    LightMilk,
    LactoFree,
    StrawberryFlavored,
    CholesterolFree,
    StrawberryFruits,
    MangoFruits,
    Cream,
    ProcessingMachineCount,
    ClarificationMachine,
    StandardizationMachine,
    CreamPasteurizationMachine
};

std::map<unsigned, ProcessingMachines> machines_mapping = {
        {0, Pasteurized},
        {1, WholeMilk},
        {2, LightMilk},
        {3, LactoFree},
        {4, StrawberryFlavored},
        {5, CholesterolFree},
        {6, StrawberryFruits},
        {7, MangoFruits},
        {8, Cream},
        {10, ClarificationMachine},
        {11, StandardizationMachine},
        {12, CreamPasteurizationMachine},
};

const std::map<ProcessingMachines, std::pair<double, double>> ProductionDetails = {
        {Pasteurized,           std::make_pair(18.1872, 21.7134)},
        {WholeMilk,             std::make_pair(15.1320, 18.0690)},
        {LightMilk,             std::make_pair(14.1056, 16.1452)},
        {LactoFree,             std::make_pair(12.1056, 14.1452)},
        {StrawberryFlavored,    std::make_pair(10.1056, 12.1452)},
        {CholesterolFree,       std::make_pair(8.0704, 9.6368)},
        {StrawberryFruits,      std::make_pair(5.0440, 6.0230)},
        {MangoFruits,           std::make_pair(3.5308, 4.2161)},
        {Cream,                 std::make_pair(17.3850, 17.9580)},
};

std::map<std::string, Months> map_months = {
        {"January",   January},
        {"February",  February},
        {"March",     March},
        {"April",     April},
        {"May",       May},
        {"June",      June},
        {"July",      July},
        {"August",    August},
        {"September", September},
        {"October",   October},
        {"November",  November},
        {"December",  December},
};


const std::map<int, std::tuple<int, double, double, double>> monthly_details = {
        {January,   std::make_tuple(31, 12.9805, 22.7500, 3.0000)},
        {February,  std::make_tuple(28, 12.9600, 22.2500, 2.6800)},
        {March,     std::make_tuple(31, 12.9804, 22.2500, 2.5000)},
        {April,     std::make_tuple(30, 12.8900, 22.5710, 2.5000)},
        {May,       std::make_tuple(31, 13.5000, 22.1140, 2.5000)},
        {June,      std::make_tuple(30, 12.6500, 22.6630, 2.7000)},
        {July,      std::make_tuple(31, 13.0000, 22.3120, 2.3000)},
        {August,    std::make_tuple(31, 12.9900, 22.9000, 2.2500)},
        {September, std::make_tuple(30, 12.9000, 22.7500, 2.8000)},
        {October,   std::make_tuple(31, 12.7000, 22.3500, 2.7990)},
        {November,  std::make_tuple(30, 13.0200, 22.4190, 2.9250)},
        {December,  std::make_tuple(31, 12.9760, 22.7500, 2.4000)},
};

/** --help string. */
static const char *help_str =
        "Program for simulating developing processing of milk in dairy factory.\n"
                "Usage: ./milk-app [OPTIONS] ...\n"
                "  -h                     : show this help and exit\n"
                "  -m, --month  <MONTH>   : set starting month of the simulation - default January\n"
                "  -p, --period <PERIOD>  : period of the simulation in count of some units - default 2 months\n"
                "                         : Nh (N hours) | N Nd (N days) | Nw (N weeks) | Nm (N months) | Ny (N years)\n";


#endif // MILK_APP_H
