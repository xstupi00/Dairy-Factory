#ifndef MILK_APP_H
#define MILK_APP_H

#include <map>

#ifdef DEBUG
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif

const unsigned MINUTE = 60;
const unsigned HOUR = 3600;
const unsigned DAY = 86400;
const unsigned WEEK = 604800;

enum Months {
    January, February, March, April, May, June, July, August, September, October, November, December, MONTH_COUNT
};

std::map<std::string, Months> map_months = {
     {"January", January},
     {"February", February},
     {"March", March},
     {"April", April},
     {"May", May},
     {"June", June},
     {"July", July},
     {"August", August},
     {"September", September},
     {"October", October},
     {"November", November},
     {"December", December},
};


const std::map<int, std::tuple<int, double, int, int>> monthly_details = {
        {January,   std::make_tuple(31, 18.4805, 22750, 3000)},
        {February,  std::make_tuple(28, 18.4600, 22250, 2680)},
        {March,     std::make_tuple(31, 18.4804, 22250, 2500)},
        {April,     std::make_tuple(30, 19.3900, 22571, 2500)},
        {May,       std::make_tuple(31, 18.7000, 22114, 2500)},
        {June,      std::make_tuple(30, 18.1500, 22663, 2700)},
        {July,      std::make_tuple(31, 18.4900, 22312, 2300)},
        {August,    std::make_tuple(31, 18.4900, 22900, 2250)},
        {September, std::make_tuple(30, 18.4000, 22750, 2800)},
        {October,   std::make_tuple(31, 18.2000, 22350, 2799)},
        {November,  std::make_tuple(30, 18.5200, 22419, 2925)},
        {December,  std::make_tuple(31, 18.4760, 22750, 2400)},
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
