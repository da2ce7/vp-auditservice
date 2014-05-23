#pragma once
//
//  MainConfigParser.h
//  vp-auditservice
//


#include <string>
#include <memory>

#include "Network.h"

class MainConfigParser {
    
public:
    MainConfigParser(int _argc, char* _argv[]) : argc(_argc), argv(_argv){};
    
    bool parse();
    
    
    // Object Passing Functions
    NetworkModule* passNetworkModule();
    
private:
    
    // Variables
    
    int argc;
    char** argv;
    
    std::string configpath;
    
    // Network
    //std::string m_netmodule;
    int remote_bitmessageport;
    std::string remote_bitmessagehost;
    std::string remote_bitmessageuser;
    std::string remote_bitmessagepass;
    
    
    // Smart Pointer Objects
    std::shared_ptr<NetworkModule> m_netmodule;


    // CallBack Functions
    void setNetworkModule(std::string const module);
    
};


