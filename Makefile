# **************************************************************
# * Project:        Simulation of Milk Developing Process in Dairy Company
# * File:		    Makefile
# * Author:		    Šimon Stupinský, Tomáš Zubrík
# * University:     Brno University of Technology
# * Faculty: 	    Faculty of Information Technology
# * Course:	        Modelling ang Simulation
# * Date:		    27.11.2018
# * Last change:    09.12.2018
# *
# * Subscribe:	Makefile
# *
# **************************************************************/

# Usage:
#   $ make              # Compile project
#   $ make debug        # Compile project with debug purpose
#   $ make clean        # Remove object files and deplist
#   $ make clean-all    # Remove object files, deplist and binaries

APP = milk-app

CXX = g++
RM = rm -f
CPPFLAGS = -g -std=c++17 -Wall -Wextra -Wpedantic
LDLIBS = -lm -lsimlib -lstdc++
SRCS = $(wildcard *.cpp)
OBJS = $(subst .cpp,.o,$(SRCS))

all: $(APP)

debug: CPPFLAGS += -D DEBUG -g
debug: $(APP)

cd: all debug clean clean-all run

run: $(APP)
	./milk-app -m January -p 1y

$(APP): $(OBJS)
	$(CXX) -o $(APP) $(OBJS) $(LDLIBS)

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend 2>/dev/null
	$(CXX) $(CPPFLAGS) -MM $^ >> ./.depend 2>/dev/null

clean:
	$(RM) $(OBJS)

clean-all:
	$(RM) $(OBJS) $(APP)

include .depend
