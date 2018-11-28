#ifndef MILK_APP_H
#define MILK_APP_H

#include <cstring>
#include <ctime>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <map>
#include <simlib.h>


#ifdef DEBUG
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif

const unsigned MINUTE = 60;
const unsigned HOUR = 3600;
const unsigned DAY = 86400;
const unsigned WEEK = 604800;

#define RECEPTION_LINES                     6
#define CLARIFICATION_LINES                 4
#define PASTEURIZATION_LINES                2
#define PASTEURIZED_BOTTLE_LINES            3
#define ULTRA_PASTEURIZED_BOTTLE_LINES      4
#define RECEPTION_SILOS_CAPACITY            640
#define PROCESSING_SILOS_CAPACITY           1420
#define HOMOGENIZATION_SILOS_CAPACITY       1000
#define PASTEURIZATION_SILOS_CAPACITY       18
#define PUMPING_FROM_TANKER_PERIOD          1.5
#define FAST_CLARIFICATION_PERIOD           2.4
#define SLOW_CLARIFICATION_PERIOD           3.0
#define HOMOGENIZATION_PERIOD               3.0
#define PASTEURIZATION_PERIOD               2
#define PASTEURIZED_PACKAGING_PERIOD        4
#define ULTRA_PASTEURIZED_PACKAGING_PERIOD  5
#define SMALL_BOTTLE_PACKAGING_PERIOD       12
#define ULTRA_PASTEURIZATION_PERIOD         .15
#define PROBABILITY_OF_INFECTED_MILK        .005


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

/** --help string. */
static const char *help_str =
        "Program for simulating developing processing of milk in dairy factory.\n"
                "Usage: ./milk-app [OPTIONS] ...\n"
                "  -h                     : show this help and exit\n"
                "  -m, --month  <MONTH>   : set starting month of the simulation - default January\n"
                "  -p, --period <PERIOD>  : period of the simulation in count of some units - default 2 months\n"
                "                         : Nh (N hours) | N Nd (N days) | Nw (N weeks) | Nm (N months) | Ny (N years)\n";

#endif // MILK_APP_H
