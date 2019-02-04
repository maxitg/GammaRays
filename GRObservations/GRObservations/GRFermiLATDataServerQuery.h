//
//  GRFermiLATDataServerQuery.h
//  GRObservations
//
//  Created by Maxim Piskunov on 23.04.2013.
//  Copyright (c) 2013 Maxim Piskunov. All rights reserved.
//

#ifndef __Gamma_Rays__GRFermiLATDataServerQuery__
#define __Gamma_Rays__GRFermiLATDataServerQuery__

#include <iostream>
#include <vector>

#include "GRFermiLATPhoton.h"
#include "GRLocation.h"
#include "GRPsf.h"
#include "GRExposureMap.h"

using namespace std;

enum GRFermiLATDataServerQueryError {
    GRFermiLATDataServerQueryErrorOk,
    GRFermiLATDataServerQueryErrorNotDownloaded,
    GRFermiLATDataServerQueryErrorNotProcessed,
    GRFermiLATDataServerQueryErrorNotRead,
    // 4
    GRFermiLATDataServerQueryErrorMkdir,
    GRFermiLATDataServerQueryErrorFileOpen,
    GRFermiLATDataServerQueryErrorSymlink,
    // 7
    GRFermiLATDataServerQueryErrorCurlInit,
    GRFermiLATDataServerQueryErrorCurlPerform,
    // 9
    GRFermiLATDataServerQueryErrorFermiDataServerTooEarly,
    GRFermiLATDataServerQueryErrorFermiDataServerUnknown,
    // 11
    GRFermiLATDataServerQueryErrorFermiDataServerQuiryStateUnknown,
    // 12
    GRFermiLATDataServerQueryErrorFermiDataServerEmptyResults,
    GRFermiLATDataServerQueryErrorFermiDataServerUnknownFile,
    // 14
    GRFermiLATDataServerQueryErrorNoEventListFile,
    GRFermiLATDataServerQueryErrorNoFilteredFile,
    GRFermiLATDataServerQueryErrorNoSpacecraftFile,
    GRFermiLATDataServerQueryErrorNoLtCubeFile,
    // 18
    GRFermiLATDataServerQueryErrorGtselectFailed,
    GRFermiLATDataServerQueryErrorGtmktimeFailed,
    GRFermiLATDataServerQueryErrorGtltcubeFailed,
    GRFermiLATDataServerQueryErrorGtexpcube2Failed,
    GRFermiLATDataServerQueryErrorGtpsfFailed
};

class GRFermiLATDataServerQuery {
public:
    double startTime;
    double endTime;
    GRLocation location;

    bool isDownloaded;
    bool isProcessed;
    bool isRead;
    GRFermiLATDataServerQueryError error;
    string errorDescription;
    
    string hash;
    
    vector <GRFermiLATPhoton> events;
    vector <vector <GRPsf> > psfs; // [eventClass][conversionType]
    vector <vector <GRExposureMap> > exposureMaps;
    
private:
    void calculateHash();
    void writeStatus();
    
    static size_t saveToString(char *ptr, size_t size, size_t nmemb, string *string);
    static size_t saveToFile(char *ptr, size_t size, size_t nmemv, FILE *file);
    
    string irfName(GRFermiEventClass eventClass, GRFermiConversionType conversionType, bool includeConversionType);
    string httpToHttps(string http);
    
    int gtselect();
    int gtmktime();
    int gtltcube();
    int gtexpcube(GRFermiEventClass eventClass, GRFermiConversionType conversionType);
    int gtpsf(GRFermiEventClass eventClass, GRFermiConversionType conversionType);
    
    void readPhotons();
    void readPsfs();
    void readExposureMaps();
    
public:    
    void init();
    void download();
    void process();
    void read();
};

#endif /* defined(__Gamma_Rays__GRFermiLATDataServerQuery__) */
