//
//  GRFermiLATDataServerQuery.cpp
//  GRObservations
//
//  Created by Maxim Piskunov on 23.04.2013.
//  Copyright (c) 2013 Maxim Piskunov. All rights reserved.
//

#include <sstream>
#include <fstream>
using namespace std;

#include <sys/stat.h>
#include <sys/errno.h>
#include <unistd.h>

#ifdef __APPLE__
#include <CommonCrypto/CommonDigest.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#include <Security/SecTransform.h>
#include <Security/SecEncodeTransform.h>
#else
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#endif

#include <curl/curl.h>

#include <fitsio.h>

#include "GRFermiLATDataServerQuery.h"

void GRFermiLATDataServerQuery::calculateHash() {
    hash.clear();
    
    double parameters[4];
    parameters[0] = startTime;
    parameters[1] = endTime;
    parameters[2] = location.ra;
    parameters[3] = location.dec;
    int parametersSize = 4*sizeof(double);
    
    int digestLength;
    
#ifdef __APPLE__
    
    digestLength = CC_SHA256_DIGEST_LENGTH;
    CC_SHA256_CTX context;
    unsigned char md[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256_Init(&context);
    CC_SHA256_Update(&context, (unsigned char*)parameters, parametersSize);
    CC_SHA256_Final(md, &context);
    
#else
    
    digestLength = SHA256_DIGEST_LENGTH;
    SHA256_CTX context;
    unsigned char md[SHA256_DIGEST_LENGTH];
    SHA256_Init(&context);
    SHA256_Update(&context, (unsigned char*)parameters, parametersSize);
    SHA256_Final(md, &context);
    
#endif
    
    hash.reserve(digestLength*2);
    for (int i = 0; i < digestLength; i++) {
        unsigned char currentChar = md[i];
        int symbols[2] = {currentChar/16, currentChar%16};
        for (int i = 0; i < 2; i++) {
            if (symbols[i] < 10) hash.push_back('0' + symbols[i]);
            else hash.push_back('a' + symbols[i] - 10);
        }
    }
    
    return;
}

void GRFermiLATDataServerQuery::init() {
    error = GRFermiLATDataServerQueryErrorNotDownloaded;
    calculateHash();
    cout << this->hash << endl;
    
    ifstream status((hash + "/status").c_str());
    int statusCode = 0;
    if (status.is_open()) {
        status >> statusCode;
        status.close();
    }
    
    if (statusCode == 0) error = GRFermiLATDataServerQueryErrorNotDownloaded;
    else if (statusCode == 1) error = GRFermiLATDataServerQueryErrorNotProcessed;
    else if (statusCode == 2) error = GRFermiLATDataServerQueryErrorNotRead;
    else error = GRFermiLATDataServerQueryErrorNotDownloaded;
}

void GRFermiLATDataServerQuery::writeStatus() {
    ofstream status((hash + "/status").c_str());
    
    if (error == GRFermiLATDataServerQueryErrorNotProcessed) status << 1;
    else if (error == GRFermiLATDataServerQueryErrorNotRead) status << 2;
    else status << 0;
    
    status.close();
}

size_t GRFermiLATDataServerQuery::saveToString(char *ptr, size_t size, size_t nmemb, string *string) {
    string->append(ptr, size * nmemb);
    return size * nmemb;
}

size_t GRFermiLATDataServerQuery::saveToFile(char *ptr, size_t size, size_t nmemb, FILE *file) {
    size_t written;
    written = fwrite(ptr, size, nmemb, file);
    return written;
}

void GRFermiLATDataServerQuery::download() {            
    if (mkdir(hash.c_str(), S_IRWXU ^ S_IRWXG ^ S_IRWXO) == -1) {
        if (errno != EEXIST) {
            error = GRFermiLATDataServerQueryErrorMkdir;
            return;
        }
    }
        
    ostringstream coordfield;
    coordfield << fixed << location.ra << ", " << location.dec;
    ostringstream timefield;
    timefield << fixed << startTime << ", " << endTime;
    
    CURL *curl;
    CURLcode res;
    struct curl_httppost *formpost=NULL;
    struct curl_httppost *lastptr=NULL;
    curl_global_init(CURL_GLOBAL_ALL);
    
    curl_formadd(&formpost,
                    &lastptr,
                    CURLFORM_COPYNAME, "destination",
                    CURLFORM_COPYCONTENTS, "query",
                    CURLFORM_END);
    
    curl_formadd(&formpost,
                    &lastptr,
                    CURLFORM_COPYNAME, "coordfield",
                    CURLFORM_COPYCONTENTS, coordfield.str().c_str(),
                    CURLFORM_END);
    
    curl_formadd(&formpost,
                    &lastptr,
                    CURLFORM_COPYNAME, "coordsystem",
                    CURLFORM_COPYCONTENTS, "J2000",
                    CURLFORM_END);
    
    curl_formadd(&formpost,
                    &lastptr,
                    CURLFORM_COPYNAME, "shapefield",
                    CURLFORM_COPYCONTENTS, "60",
                    CURLFORM_END);
    
    curl_formadd(&formpost,
                    &lastptr,
                    CURLFORM_COPYNAME, "timefield",
                    CURLFORM_COPYCONTENTS, timefield.str().c_str(),
                    CURLFORM_END);
    
    curl_formadd(&formpost,
                    &lastptr,
                    CURLFORM_COPYNAME, "timetype",
                    CURLFORM_COPYCONTENTS, "MET",
                    CURLFORM_END);
        
    curl_formadd(&formpost,
                    &lastptr,
                    CURLFORM_COPYNAME, "energyfield",
                    CURLFORM_COPYCONTENTS, "30, 300000",
                    CURLFORM_END);
    
    curl_formadd(&formpost,
                    &lastptr,
                    CURLFORM_COPYNAME, "photonOrExtendedOrNone",
                    CURLFORM_COPYCONTENTS, "Extended",
                    CURLFORM_END);
    
    curl_formadd(&formpost,
                    &lastptr,
                    CURLFORM_COPYNAME, "spacecraft",
                    CURLFORM_COPYCONTENTS, "on",
                    CURLFORM_END);
    
    curl_formadd(&formpost,
                    &lastptr,
                    CURLFORM_COPYNAME, "submit",
                    CURLFORM_COPYCONTENTS, "send",
                    CURLFORM_END);
    
    curl = curl_easy_init();
    
    string responce;
    
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://fermi.gsfc.nasa.gov/cgi-bin/ssc/LAT/LATDataQuery.cgi");
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responce);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &this->saveToString);
        
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            error = GRFermiLATDataServerQueryErrorCurlPerform;
        }
        
        curl_easy_cleanup(curl);
        curl_formfree(formpost);
    } else {
        error = GRFermiLATDataServerQueryErrorCurlInit;
    }
    
    if (error == GRFermiLATDataServerQueryErrorCurlPerform || error == GRFermiLATDataServerQueryErrorCurlInit) return;
    
    string resultsURLLeft = "The results of your query may be found at <a href=\"";
    string resultsURLRight = "\">";
    size_t resultsURLIndex = responce.find(resultsURLLeft);
    
    if (resultsURLIndex == string::npos) {
        string errorMessageLeft = "Unable to handle query";
        string errorMessageRight = "</b>";
        size_t errorMessageIndex = responce.find(errorMessageLeft);
        size_t errorMessageSize = responce.find(errorMessageRight, errorMessageIndex) - errorMessageIndex;
        string errorMessage = responce.substr(errorMessageIndex, errorMessageSize);
        
        if (responce.find("occurs before data start") != string::npos) {
            error = GRFermiLATDataServerQueryErrorFermiDataServerTooEarly;
        } else {
            error = GRFermiLATDataServerQueryErrorFermiDataServerUnknown;
            errorDescription = errorMessage;
        }
        
        return;
    }
    
    resultsURLIndex += resultsURLLeft.size();
    size_t resultsURLSize = responce.find(resultsURLRight, resultsURLIndex) - resultsURLIndex;
    string resultsURL = responce.substr(resultsURLIndex, resultsURLSize);
    
    bool resultsReady = false;
    vector <string> resultURLs;
    
    while (!resultsReady) {
        responce.clear();
        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, resultsURL.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responce);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &this->saveToString);
            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                error = GRFermiLATDataServerQueryErrorCurlPerform;
            }
            
            curl_easy_cleanup(curl);
        } else {
            error = GRFermiLATDataServerQueryErrorCurlInit;
        }
        if (error == GRFermiLATDataServerQueryErrorCurlPerform || error == GRFermiLATDataServerQueryErrorCurlInit) return;
        
        if (responce.find("Query complete") != string::npos) {
            resultsReady = true;
        }
        else if (responce.rfind("Query pending") != string::npos) {
            sleep(1);
        }
        else if (responce.rfind("Query in progress") != string::npos) {
            sleep(1);
        } else {
            error = GRFermiLATDataServerQueryErrorFermiDataServerUnknown;
            errorDescription = resultsURL;
            return;
        }
    }
    
    size_t currentPosition = 0;
    resultURLs.clear();
    while ((currentPosition = responce.find(".fits\">", ++currentPosition)) != string::npos) {
        size_t linkIndex = responce.rfind("href=\"", currentPosition);
        resultURLs.push_back(responce.substr(linkIndex+6, (currentPosition+5) - (linkIndex+6)));
    }
    
    if (!resultURLs.size()) {
        error = GRFermiLATDataServerQueryErrorFermiDataServerEmptyResults;
        return;
    }
    
    vector <string> filenames(resultURLs.size());
    
    for (int i = 0; i < resultURLs.size(); i++) {
        filenames[i] = resultURLs[i].substr(resultURLs[i].rfind("/")+1);
        FILE *fits = fopen((hash + "/" + filenames[i]).c_str(), "wb");
        if (fits == NULL) {
            error = GRFermiLATDataServerQueryErrorFileOpen;
            return;
        }
        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, resultURLs[i].c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fits);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &this->saveToFile);
            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                error = GRFermiLATDataServerQueryErrorCurlPerform;
            }
            curl_easy_cleanup(curl);
        } else {
            error = GRFermiLATDataServerQueryErrorCurlInit;
        }
        fclose(fits);
        if (error == GRFermiLATDataServerQueryErrorCurlPerform || error == GRFermiLATDataServerQueryErrorCurlInit) return;
    }
    
    ofstream eventList((hash + "/eventList.txt").c_str());
    if (eventList.fail()) {
        error = GRFermiLATDataServerQueryErrorFileOpen;
        return;
    }
    for (int i = 0; i < filenames.size(); i++) {
        if (filenames[i].find("_EV") != string::npos) eventList << hash + "/" + filenames[i] << endl;
        else if (filenames[i].find("_SC") != string::npos) {
            if (symlink(filenames[i].c_str(), (hash + "/spacecraft.fits").c_str()) == -1) {
                if (errno == EEXIST) {
                    unlink((hash + "/spacecraft.fits").c_str());
                    symlink(filenames[i].c_str(), (hash + "/spacecraft.fits").c_str());
                }
                if (errno != EEXIST && errno != 0) {
                    error = GRFermiLATDataServerQueryErrorSymlink;
                    errorDescription = "Failed to symlink from " + filenames[i] + " to " + hash + "/spacecraft.fits : ";
                    eventList.close();
                    return;
                }
            }
        }
        else {
            error = GRFermiLATDataServerQueryErrorFermiDataServerUnknownFile;
            eventList.close();
            return;
        }
    }
    eventList.close();
    
    error = GRFermiLATDataServerQueryErrorNotProcessed;
    isDownloaded = true;
    writeStatus();
    
    return;
}

string GRFermiLATDataServerQuery::irfName(GRFermiEventClass eventClass, GRFermiConversionType conversionType) {
    string irfName = "P7REP_";
    
    if (eventClass == GRFermiEventClassUltraclean) irfName += "ULTRACLEAN";
    else if (eventClass == GRFermiEventClassClean) irfName += "CLEAN";
    else if (eventClass == GRFermiEventClassSource) irfName += "SOURCE";
    else irfName += "TRANSIENT";
    
    irfName += (string)"_V15::";
    
    if (conversionType == GRFermiConversionTypeBack) irfName += "BACK";
    else irfName += "FRONT";
    return irfName;
}

int GRFermiLATDataServerQuery::gtselect() {
    ostringstream cmd;
    cmd << fixed << "gtselect" << " ";
    cmd << "infile=@" << hash << "/eventList.txt" << " ";
    cmd << "outfile=" << hash << "/filtered.fits" << " ";
    cmd << "ra=" << "INDEF" << " ";
    cmd << "dec=" << "INDEF" << " ";
    cmd << "rad=" << 180 << " ";
    cmd << "tmin=" << "INDEF" << " ";
    cmd << "tmax=" << "INDEF" << " ";
    cmd << "emin=" << 100. << " ";
    cmd << "emax=" << 300000. << " ";
    cmd << "zmax=" << 100. << " ";
    cmd << "evclass=" << 0 << " ";
    cmd << "convtype=" << -1 << " ";
    cmd << "evtable=" << "EVENTS" << " ";
    cmd << "chatter=" << 0 << " ";
    return system(cmd.str().c_str());
}

int GRFermiLATDataServerQuery::gtmktime() {
    ostringstream cmd;
    cmd << fixed << "gtmktime" << " ";
    cmd << "scfile=" << hash << "/spacecraft.fits" << " ";
    cmd << "filter=" << "\"DATA_QUAL>0 && LAT_CONFIG==1 && ABS(ROCK_ANGLE)<52\"" << " ";
    cmd << "roicut=" << "no" << " ";
    cmd << "evfile=" << hash << "/filtered.fits" << " ";
    cmd << "outfile=" << hash << "/timed.fits" << " ";
    cmd << "chatter=" << 0 << " ";
    return system(cmd.str().c_str());
}

int GRFermiLATDataServerQuery::gtltcube() {
    ostringstream cmd;
    cmd << fixed << "gtltcube" << " ";
    cmd << "evfile=" << hash << "/timed.fits" << " ";
    cmd << "evtable=" << "EVENTS" << " ";
    cmd << "scfile=" << hash << "/spacecraft.fits" << " ";
    cmd << "sctable=" << "SC_DATA" << " ";
    cmd << "outfile=" << hash << "/ltcube.fits" << " ";
    cmd << "dcostheta=" << 0.025 << " ";
    cmd << "binsz=" << 1 << " ";
    cmd << "chatter=" << 0 << " ";
    return system(cmd.str().c_str());
}

int GRFermiLATDataServerQuery::gtexpcube(GRFermiEventClass eventClass, GRFermiConversionType conversionType) {
    string irf = irfName(eventClass, conversionType);
    string expcubeFilename = "expcube_" + irf + ".fits";
    
    ostringstream cmd;
    cmd << fixed << "gtexpcube2" << " ";
    cmd << "infile=" << hash << "/ltcube.fits" << " ";
    cmd << "cmap=" << "none" << " ";
    cmd << "outfile=" << hash << "/" << expcubeFilename << " ";
    cmd << "irfs=" << irf << " ";
    cmd << "nxpix=" << 360 << " ";
    cmd << "nypix=" << 180 << " ";
    cmd << "binsz=" << 1 << " ";
    cmd << "coordsys=" << "CEL" << " ";
    cmd << "xref=" << 0. << " ";
    cmd << "yref=" << 0. << " ";
    cmd << "axisrot=" << 0 << " ";
    cmd << "proj=" << "CAR" << " ";
    cmd << "ebinalg=" << "LOG" << " ";
    cmd << "emin=" << 100. << " ";
    cmd << "emax=" << 1000000. << " ";
    cmd << "enumbins=" << 40 << " ";
    cmd << "ebinfile=" << "NONE" << " ";
    cmd << "bincalc=" << "EDGE" << " ";
    cmd << "ignorephi=" << "no" << " ";
    cmd << "thmax=" << 180 << " ";
    cmd << "thmin=" << 0 << " ";
    cmd << "table=" << "Exposure" << " ";
    cmd << "chatter=" << 0 << " ";
    cmd << "clobber=" << "yes" << " ";
    cmd << "debug=" << "no" << " ";
    cmd << "mode=" << "ql" << " ";
    return system(cmd.str().c_str());
}

int GRFermiLATDataServerQuery::gtpsf(GRFermiEventClass eventClass, GRFermiConversionType conversionType) {
    string irf = irfName(eventClass, conversionType);
    string psfFilename = "psf_" + irf + ".fits";
    
    ostringstream cmd;
    cmd << fixed << "gtpsf" << " ";
    cmd << "expcube=" << hash << "/ltcube.fits" << " ";
    cmd << "outfile=" << hash << "/" << psfFilename << " ";
    cmd << "outtable=" << "PSF" << " ";
    cmd << "irfs=" << irf << " ";
    cmd << "ra=" << location.ra << " ";
    cmd << "dec=" << location.dec << " ";
    cmd << "emin=" << 100. << " ";
    cmd << "emax=" << 1000000. << " ";
    cmd << "nenergies=" << 41 << " ";
    cmd << "thetamax=" << 30 << " ";
    cmd << "ntheta=" << 300 << " ";
    cmd << "chatter=" << 0 << " ";
    return system(cmd.str().c_str());
}

void GRFermiLATDataServerQuery::process() {
    if (gtselect() != 0) {
        error = GRFermiLATDataServerQueryErrorGtselectFailed;
        errorDescription = "gtselect failed.";
        return;
    }
    
    if (gtmktime() != 0) {
        error = GRFermiLATDataServerQueryErrorGtmktimeFailed;
        errorDescription = "gtmktime failed. It means most likely that no photons left after filtering.";
        return;
    }
    
    if (gtltcube() != 0) {
        error = GRFermiLATDataServerQueryErrorGtltcubeFailed;
        return;
    }
    
    for (int i = 0; i < GRFermiEventClassesCount; i++) {
        for (int j = 0; j < GRFermiConversionTypesCount; j++) {
            if (gtexpcube(GRFermiEventClasses[i], GRFermiConversionTypes[j]) != 0) {
                error = GRFermiLATDataServerQueryErrorGtexpcube2Failed;
            }
            if (gtpsf(GRFermiEventClasses[i], GRFermiConversionTypes[j]) != 0) {
                error = GRFermiLATDataServerQueryErrorGtpsfFailed;
            }
        }
    }
    
    if (error == GRFermiLATDataServerQueryErrorGtexpcube2Failed || error == GRFermiLATDataServerQueryErrorGtpsfFailed) return;
    
    error = GRFermiLATDataServerQueryErrorNotRead;
    isProcessed = true;
    writeStatus();
}

void GRFermiLATDataServerQuery::readPhotons() {
    string filename = hash + "/timed.fits";
    fitsfile *timedFile;
    int status = 0;
    long nrows;
    
    fits_open_table(&timedFile, filename.c_str(), READONLY, &status);
    fits_movabs_hdu(timedFile, 2, NULL, &status);
    fits_get_num_rows(timedFile, &nrows, &status);
    events.reserve(nrows);
    
    float *readEnergies;
    float *readRas;
    float *readDecs;
    double *readTimes;
    int *readEventClasses;
    int *readConversionTypes;
    readEnergies = new float[nrows];
    readRas = new float[nrows];
    readDecs = new float[nrows];
    readTimes = new double[nrows];
    readEventClasses = new int[nrows];
    readConversionTypes = new int[nrows];
    
    fits_read_col(timedFile, TFLOAT, 1, 1, 1, nrows, 0, readEnergies, 0, &status);
    
    fits_read_col(timedFile, TFLOAT, 2, 1, 1, nrows, 0, readRas, 0, &status);
    
    fits_read_col(timedFile, TFLOAT, 3, 1, 1, nrows, 0, readDecs, 0, &status);
    
    fits_read_col(timedFile, TDOUBLE, 10, 1, 1, nrows, 0, readTimes, 0, &status);
    
    fits_read_col(timedFile, TINT, 15, 1, 1, nrows, 0, readEventClasses, 0, &status);
    
    fits_read_col(timedFile, TINT, 16, 1, 1, nrows, 0, readConversionTypes, 0, &status);
    
    fits_close_file(timedFile, &status);
    
    if (status) fits_report_error(stderr, status);
    
    for (int i = 0; i < nrows; i++) {
        GRLocation location = GRLocation(GRCoordinateSystemJ2000, readRas[i], readDecs[i]);
        
        GRFermiEventClass eventClass;
        if ((readEventClasses[i]/16)%2) eventClass = GRFermiEventClassUltraclean;
        else if ((readEventClasses[i]/8)%2) eventClass = GRFermiEventClassClean;
        else if ((readEventClasses[i]/4)%2) eventClass = GRFermiEventClassSource;
        else eventClass = GRFermiEventClassTransient;
        
        GRFermiConversionType conversionType;
        if (readEventClasses[i] == 0) conversionType = GRFermiConversionTypeFront;
        else conversionType = GRFermiConversionTypeBack;
        
        events.push_back(GRFermiLATPhoton(readTimes[i], location, readEnergies[i], conversionType, eventClass));
    }
    
    delete readEnergies;
    delete readRas;
    delete readDecs;
    delete readTimes;
    delete readEventClasses;
    delete readConversionTypes;
}

void GRFermiLATDataServerQuery::readPsfs() {
    psfs.resize(GRFermiEventClassesCount);
    for (int i = 0; i < GRFermiEventClassesCount; i++) {
        psfs[i].resize(GRFermiConversionTypesCount);
        for (int j = 0; j < GRFermiConversionTypesCount; j++) {
            
            string fileName = hash + "/psf_" + irfName(GRFermiEventClasses[i], GRFermiConversionTypes[j]) + ".fits";
            fitsfile *psfFile;
            int status = 0;
            long nrows;
            
            fits_open_table(&psfFile, fileName.c_str(), READONLY, &status);
            
            fits_movabs_hdu(psfFile, 3, NULL, &status);
            fits_get_num_rows(psfFile, &nrows, &status);
            float readAngles[nrows];
            fits_read_col(psfFile, TFLOAT, 1, 1, 1, nrows, 0, readAngles, 0, &status);
            psfs[i][j].angles.resize(nrows);
            for (int k = 0; k < nrows; k++) psfs[i][j].angles[k] = readAngles[k];
            
            fits_movabs_hdu(psfFile, 2, NULL, &status);
            fits_get_num_rows(psfFile, &nrows, &status);
            float readEnergies[nrows];
            fits_read_col(psfFile, TFLOAT, 1, 1, 1, nrows, 0, readEnergies, 0, &status);
            psfs[i][j].energies.resize(nrows);
            for (int k = 0; k < nrows; k++) psfs[i][j].energies[k] = readEnergies[k];
            
            psfs[i][j].probabilityDensity.resize(psfs[i][j].energies.size());
            for (int k = 0; k < psfs[i][j].energies.size(); k++) {
                float readProbabilityDensities[nrows];
                fits_read_col(psfFile, TFLOAT, 3, k+1, 1, psfs[i][j].angles.size(), 0, readProbabilityDensities, 0, &status);
                psfs[i][j].probabilityDensity[k].resize(psfs[i][j].angles.size());
                for (int l = 0; l < psfs[i][j].angles.size(); l++) psfs[i][j].probabilityDensity[k][l] = readProbabilityDensities[l];
            }
            
            fits_close_file(psfFile, &status);
            
            if (status) fits_report_error(stderr, status);
            
        }
    }
}

void GRFermiLATDataServerQuery::readExposureMaps() {
    exposureMaps.resize(GRFermiEventClassesCount);
    for (int i = 0; i < GRFermiEventClassesCount; i++) {
        exposureMaps[i].resize(GRFermiConversionTypesCount);
        for (int j = 0; j < GRFermiConversionTypesCount; j++) {
            
            string fileName = hash + "/expcube_" + irfName(GRFermiEventClasses[i], GRFermiConversionTypes[j]) + ".fits";
            fitsfile *expcubeFile;
            int status = 0;
            long nrows;
            
            fits_open_table(&expcubeFile, fileName.c_str(), READONLY, &status);
            
            fits_movabs_hdu(expcubeFile, 2, NULL, &status);
            fits_get_num_rows(expcubeFile, &nrows, &status);
            float readEnergies[nrows];
            fits_read_col(expcubeFile, TFLOAT, 1, 1, 1, nrows, 0, readEnergies, 0, &status);
            exposureMaps[i][j].energies.resize(nrows);
            exposureMaps[i][j].exposures.resize(nrows);
            for (int k = 0; k < nrows; k++) exposureMaps[i][j].energies[k] = readEnergies[k];
            
            fits_movabs_hdu(expcubeFile, 1, NULL, &status);
            float *readExposures;
            long fpixel[3] = {1, 1, 1};
            readExposures = new float[360*180*nrows];
            
            fits_read_pix(expcubeFile, TFLOAT, fpixel, 360*180*nrows, 0, readExposures, 0, &status);
                        
            for (int k = 0; k < nrows; k++) {
                exposureMaps[i][j].exposures[k].resize(360);
                for (int ra = 0; ra < 360; ra++) {
                    int xpixel = 180 - ra;
                    if (xpixel < 0) xpixel += 360;
                    exposureMaps[i][j].exposures[k][ra].resize(180);
                    for (int dec = 0; dec < 180; dec++) {
                        exposureMaps[i][j].exposures[k][ra][dec] = readExposures[xpixel + dec*360 + k*180*360];
                    }
                }
            }
            delete readExposures;
            
            fits_close_file(expcubeFile, &status);
        
            if (status) fits_report_error(stderr, status);
                
        }
    }
}

void GRFermiLATDataServerQuery::read() {
    readPhotons();
    readPsfs();
    readExposureMaps();
    error = GRFermiLATDataServerQueryErrorOk;
    isRead = true;
}