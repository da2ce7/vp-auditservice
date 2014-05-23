//
//  main.cpp
//  vp-auditservice
//


#include <iostream>
#include <string>
#include "MainConfigParser.h"
#include "Network.h"
#include "BitMessage.h"

#include "base64.h"

int main(int argc, char * argv[])
{

    // Read Config
    MainConfigParser mainConfigParser(argc, argv);
    if(!mainConfigParser.parse()){
        std::cout << "Configuration Parser Error" << std::endl;
    }
    
    // Startup Network
    
    std::cerr << "Starting Network Module" << std::endl;
    
    NetworkModule *netModule = mainConfigParser.passNetworkModule();
    if(netModule->accessible()){
        std::cout << "Transport Layer is Accessible" << std::endl;
    }
    else{
        std::cout << "Transport Layer Failure" << std::endl;
        exit(0);
    }
    
    std::cout << std::endl;
    std::cout << "Running Network Module Tests:" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Number of Modules Loaded: " << netModule->modulesLoaded() << std::endl;
    std::cout << "Number of Modules Alive: " << netModule->modulesAlive() << std::endl;
    std::cout << "Network Accessibiility Test: " << netModule->accessible() << std::endl;
    std::cout << "Number of Modules Alive: " << netModule->modulesAlive() << std::endl;

    std::cout << "Address Accesibility Test: " << netModule->addressAccessible("BM-2cWNiCJahFd26v5ev5jH1GzKMNWiwbimDJ") << std::endl;
    std::cout << "Check Publish Support: " << netModule->publishSupport() << std::endl;
    std::cout << "Asking for Inbox: " << netModule->checkMail() << std::endl;
    std::cout << "Checking for new mail: " << netModule->newMailExists() << std::endl;
    std::cout << "Checking for new mail on \"BM-2cXRVEbV9q3p1v6EDBH5jhm21h2MWLNaTa\": " << netModule->newMailExists("BM-2cXRVEbV9q3p1v6EDBH5jhm21h2MWLNaTa") <<std::endl;
    
    std::cout << std::endl;
    
    while(dynamic_cast<BitMessage*>(netModule)->queueSize() != 0){
        ;
    }
    
    std::cout << std::endl;
    std::cout << "Base64 Test" << std::endl;
    base64 message("Hello");
    std::cout << message.encoded() << std::endl;
    std::string decoded;
    decoded << message;
    std::cout << decoded << std::endl;
    std::string world("World");
    world >> message;
    std::cout << message.encoded() << std::endl;
    decoded << message;
    std::cout << decoded << std::endl << std::endl;
    
    
}

