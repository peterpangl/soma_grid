# soma_grid
-------------Boost------------
You will have to install the Boost library in order to run the code since the last commits.
In the DHTTestApp code, boost functions are used.
Boost library used for the regular expressions related functions that provides. In order to add it and use it along with the omnetpp,
below are the steps that need to follow:
-download boost 1.65.1
-tar xzvf boost_1_65_1.tar.gz
-cd boost_1_65_1/

-sudo apt-get update
-sudo apt-get install autotools-dev libicu-dev libbz2-dev libboost-all-dev

-./bootstrap.sh --prefix=/usr/

-./b2
-sudo ./b2 install

-Edit the make configuration of omnet:
in OMNeT++ go to your project properties, then OMNeT++ | Makemake | check the directory with your sources | Options | Custom | Makefrag, and add the path to the *.a libraries, for example: 
LIBS += -L/usr/lib
OMNETPP_LIBS += -lgmp -lboost_regex -lboost_regex-mt
