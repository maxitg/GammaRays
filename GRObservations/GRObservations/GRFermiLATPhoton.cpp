//
//  GRFermiLATPhoton.cpp
//  GRObservations
//
//  Created by Maxim Piskunov on 24.03.2013.
//  Copyright (c) 2013 Maxim Piskunov. All rights reserved.
//

#include <math.h>

#include <sstream>

#include "GRFermiLATPhoton.h"

string GRFermiLATPhoton::energyDescription() {
    if (energy == 0.) return "0";
    
    ostringstream result;
    
    int magnitude = (int)round(log10(energy));
    
    if (magnitude < -3 || magnitude >= 12) result << energy * 1e6 << " eV";
    else if (magnitude < 0) result << energy * 1e3 << " keV";
    else if (magnitude < 3) result << energy * 1e0 << " MeV";
    else if (magnitude < 6) result << energy * 1e-3 << " GeV";
    else if (magnitude < 9) result << energy * 1e-6 << " TeV";
    else result << energy * 1e-9 << " PeV";
    
    return result.str();
}

string GRFermiLATPhoton::conversionTypeName() {
    if (conversionType == GRFermiConversionTypeFront) return "front";
    else return "back";
}

string GRFermiLATPhoton::eventClassName() {
    if (eventClass == GRFermiEventClassUltraclean) return "ultraclean";
    else if (eventClass == GRFermiEventClassClean) return "clean";
    else if (eventClass == GRFermiEventClassSource) return "source";
    else return "transient";
}

bool GRFermiLATPhoton::operator<(GRFermiLATPhoton right) const {
    return time < right.time;
}

string GRFermiLATPhoton::description() {
    ostringstream result;
    result << "Photon (" << energyDescription() << ", " << eventClassName() << ", " << conversionTypeName() << ") at MET " << (int)time << " from " << location.description();
    return result.str();
}