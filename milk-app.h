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

#define INSPECTION_PERIOD                       (5.0 * MINUTE)
#define PROBABILITY_INFECTED_MILK               0.005

#define RECEPTION_LINES                         7
#define RECEPTION_MILK_PUMPING_SPEED            (1.5 * MINUTE)
#define RECEPTION_CREAM_PUMPING_SPEED           (4.0 * MINUTE)
#define RECEPTION_MILK_SILOS_CAPACITY           450
#define RECEPTION_DERIVATIVES_SILOS_CAPACITY    150
#define RECEPTION_CREAM_SILOS_CAPACITY          80

#define CLARIFICATION_LINES                     4
#define CLARIFICATION_PROCESSING_SPEED_FAST     (2.0 * MINUTE)
#define CLARIFICATION_PROCESSING_SPEED_SLOW     (2.4 * MINUTE)
#define CLARIFICATION_MILK_SILOS_CAPACITY       1700
#define CLARIFICATION_CREAM_SILOS_CAPACITY      30

#define PASTEURIZATION_BOTTLE_MACHINES          3
#define PASTEURIZATION_BOTTLE_MACHINES_SPEED    (3.34 * MINUTE)
#define HOMOGENIZATION_PERIOD                   (1.5 * MINUTE)
#define HOMOGENIZATION_PUMPING_SPEED            (2.0 * MINUTE)
#define PASTEURIZATION_PERIOD                   (0.8 * MINUTE)

#define WHOLEMILK_MACHINES_FAST          2
#define WHOLEMILK_MACHINES_SPEED_FAST    (5 * MINUTE)
#define WHOLEMILK_MACHINES_SLOW          1
#define WHOLEMILK_MACHINES_SPEED_SLOW    (10 * MINUTE)

#define LIGHTMILK_MACHINES          4
#define LIGHTMILK_MACHINES_SPEED    (10 * MINUTE)

#define LACTOFREETMILK_MACHINES         4
#define LACTOFREEMILK_MACHINES_SPEED    (10 * MINUTE)

#define STRAWBERRY_FLAVORED_MILK_MACHINES           2
#define STRAWBERRY_FLAVORED_MILK_MACHINES_SPEED     (5 * MINUTE)

#define CHOLESTEROL_FREE_MILK_MACHINES          2
#define CHOLESTEROL_FREE_MILK_MACHINES_SPEED    (10 * MINUTE)


enum Months {
    January, February, March, April, May, June, July, August, September, October, November, December, MONTH_COUNT
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
        {January,   std::make_tuple(31, 18.4805, 22.7500, 3.0000)},
        {February,  std::make_tuple(28, 18.4600, 22.2500, 2.6800)},
        {March,     std::make_tuple(31, 18.4804, 22.2500, 2.5000)},
        {April,     std::make_tuple(30, 19.3900, 22.5710, 2.5000)},
        {May,       std::make_tuple(31, 18.7000, 22.1140, 2.5000)},
        {June,      std::make_tuple(30, 18.1500, 22.6630, 2.7000)},
        {July,      std::make_tuple(31, 18.4900, 22.3120, 2.3000)},
        {August,    std::make_tuple(31, 18.4900, 22.9000, 2.2500)},
        {September, std::make_tuple(30, 18.4000, 22.7500, 2.8000)},
        {October,   std::make_tuple(31, 18.2000, 22.3500, 2.7990)},
        {November,  std::make_tuple(30, 18.5200, 22.4190, 2.9250)},
        {December,  std::make_tuple(31, 18.4760, 22.7500, 2.4000)},
};

///< Whole Milk 1L - UP
///< Light Milk 1L - UP
///< Strawberry flavored milk 1L - UP
///< UP milk with fruits 250ml (strawberry, mango) - UP

/** --help string. */
static const char *help_str =
"Program for simulating developing processing of milk in dairy factory.\n"
"Usage: ./milk-app [OPTIONS] ...\n"
"  -h                     : show this help and exit\n"
"  -m, --month  <MONTH>   : set starting month of the simulation - default January\n"
"  -p, --period <PERIOD>  : period of the simulation in count of some units - default 2 months\n"
"                         : Nh (N hours) | N Nd (N days) | Nw (N weeks) | Nm (N months) | Ny (N years)\n";

#endif // MILK_APP_H
