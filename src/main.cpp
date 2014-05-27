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
    
    int debugInboxSize = netModule->getAllInboxes().size();
    std::cout << "Inbox Size is : " << debugInboxSize << " messages." << std::endl;
    // Uncomment this section to test deleting messages
    /*
    if(debugInboxSize > 0){
        std::string debugInboxLastMessageID = netModule->getAllInboxes().at(debugInboxSize-1).getMessageID();
        std::cout << "Trashing oldest message from Inbox, with ID: " << debugInboxLastMessageID << std::endl;
        netModule->deleteMessage(debugInboxLastMessageID);
    }
    */
    // Uncomment this section to test marking messages as read
    /*
    if(debugInboxSize > 0){
        std::string debugInboxLastMessageID = netModule->getAllInboxes().at(debugInboxSize-1).getMessageID();
        std::cout << "Marking last message in inbox as unread, with ID: " << debugInboxLastMessageID << std::endl;
        netModule->markRead(debugInboxLastMessageID, false);
    }
    */
     
    int debugAddressInboxSize = netModule->getInbox("BM-2cXRVEbV9q3p1v6EDBH5jhm21h2MWLNaTa").size();
    std::cout << "Inbox Size for \"BM-2cXRVEbV9q3p1v6EDBH5jhm21h2MWLNaTa\" is : " << debugAddressInboxSize << " messages." << std::endl;
    if(debugAddressInboxSize > 0){
        std::cout << "Subject of first message for \"BM-2cXRVEbV9q3p1v6EDBH5jhm21h2MWLNaTa\": " << netModule->getInbox("BM-2cXRVEbV9q3p1v6EDBH5jhm21h2MWLNaTa").at(0).getSubject() << std::endl;
    }
    
       
    int debugUnreadInboxSize = netModule->getAllUnreadMail().size();
    std::cout << "Number of unread messages in the inbox: " << debugUnreadInboxSize << std::endl;
    if(debugUnreadInboxSize > 0){
        std::cout << "First unread message: " << std::endl << std::endl << netModule->getAllUnreadMail().at(0).getMessage() << std::endl;
    }
    
    
    std::cout << std::endl;
    std::cout << std::endl;

    // This is a bit of a more complex example, how to send mail:
    std::cout << "Sending Message from first useable address to Echo Server" << std::endl;
    std::vector<std::string> useableAddresses = netModule->getLocalAddresses();
    if(useableAddresses.size() > 0){
        std::string firstAddress = useableAddresses.at(0);
        std::cout << "First Useable Address: " << firstAddress << std::endl;
        
        // BM-orkCbppXWSqPpAxnz6jnfTZ2djb5pJKDb - BitMessage Echo Server, useful for Debugging
        NetworkMail outgoingMessage(firstAddress, "BM-orkCbppXWSqPpAxnz6jnfTZ2djb5pJKDb", "VP-AuditService Test", "Testing Echo Server");
        netModule->sendMail(outgoingMessage);
    }
    else{
        std::cout << "No Useable Addresses" << std::endl;
    }
    
    
    
    
    
    
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

