#include "BitMessage.h"
#include "../parsers/jsoncpp/json/json.h"
#include "base64.h"
#include "VectorHelp.h"

#include <boost/tokenizer.hpp>

#include <string>
#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <thread>
#include <chrono>
#include <functional>

BitMessage::BitMessage(std::string commstring) : NetworkModule(commstring) {

    parseCommstring(commstring);  // This is its own function now, purely for parsing and setting up the config as necessary.
    
    m_xmllib = new XmlRPC(m_host, m_port, true, 10000);
    m_xmllib->setAuth(m_username, m_pass);
    
    // Runs to setup our counter
    checkAlive();
    
    initializeUserData();
    
    // Thread Handler
    bm_queue = new BitMessageQueue(this);
    
    /* TESTING */
    
    startQueue();   // Start Listener Thread
    //std::this_thread::sleep_for(std::chrono::seconds(1));  // Testing this functionality - Pause while listener posts to cout.
    //stopQueue();    // Stop Listener Thread
    
    /* End Testing */
}


BitMessage::~BitMessage(){

    std::cerr << "Cleaning Up BitMessage Class" << std::endl; // Temporary

    // Clean up Objects
    
  //  if(!m_forceKill)  // If we haven't asked for a force kill
  //      while(bm_queue->processing()){
  //          ; // If we're in the middle of processing, don't kill the message queue.
  //      }
    
    delete bm_queue;  // Queue will be stopped automatically upon deletion
    delete m_xmllib;
    
    std::cerr << "Done Cleaning up BitMessage Class" << std::endl; // Temporary
}



/*
 * Virtual Functions
 */


bool BitMessage::accessible(){
    
    return m_serverAvailable;

}


bool BitMessage::createAddress(std::string options=""){
    
    try{
        // Here we will push a request to create a new random address onto the queue
        // And then immediately request the updated list of addresses
        std::function<void()> firstCommand = std::bind(&BitMessage::createRandomAddress, this, base64(""), false, 1, 1);
        bm_queue->addToQueue(firstCommand);
    
        std::function<void()> secondCommand = std::bind(&BitMessage::listAddresses, this);
        bm_queue->addToQueue(secondCommand);
        return true;
    }
    catch(...){
        return false;
    }
}  // Queued


bool BitMessage::createDeterministicAddress(std::string key){
    
    try{
        std::function<void()> firstCommand = std::bind(&BitMessage::createRandomAddress, this, base64(key), false, 1, 1);
        bm_queue->addToQueue(firstCommand);
    
        std::function<void()> secondCommand = std::bind(&BitMessage::listAddresses, this);
        bm_queue->addToQueue(secondCommand);
        
        return true;
    }
    catch(...){
        return false;
    }
    
}  // Queued

bool BitMessage::addressAccessible(std::string address){
    
    std::unique_lock<std::mutex> mlock(m_localIdentitiesMutex);

    for(int x = 0; x < m_localIdentities.size(); x++){
            if(m_localIdentities.at(x).getAddress() == address){
                mlock.unlock();
            return true;
        }
    }
    mlock.unlock();
    
    checkAddresses(); // If the address isn't acccessible, try and fetch the latest
                     // Address book from the API server for another try later.
                     // In most cases, you won't be checking for an address that you
                     // Don't already know about from the addressbook.
    
    return false;
} // Queued

std::vector<std::string> BitMessage::getAddresses(){
    
    std::unique_lock<std::mutex> mlock(m_localAddressBookMutex);

    std::vector<std::string> addresses;
    for(int x = 0; x < m_localAddressBook.size(); x++){
        addresses.push_back(m_localAddressBook.at(x).getAddress());
    }
    
    mlock.unlock();
    
    return addresses;

} // Queued

bool BitMessage::checkAddresses(){
    try{
        std::function<void()> command = std::bind(&BitMessage::listAddresses, this);
        bm_queue->addToQueue(command);
        return true;
    }
    catch(...){
        return false;
    }
} // Queued

bool BitMessage::checkMail(){
    try{
        std::function<void()> command = std::bind(&BitMessage::getAllInboxMessages, this);
        bm_queue->addToQueue(command);
        return true;
    }
    catch(...){
        return false;

    }
} // checks for new mail, returns true if there is new mail in the queue. // Queued

bool BitMessage::newMailExists(std::string address){
    
    if(m_localInbox.size() == 0){
        checkMail();
    }
    
    if(address != ""){
        for(int x=0; x<m_localInbox.size(); x++){

            if(m_localInbox.at(x).getTo() == address && m_localInbox.at(x).getRead() == false)
                return true;
        }
    }
    else{
        for(int x = 0; x < m_localInbox.size(); x++){
            if(m_localInbox.at(x).getRead() == false)
                return true;
        }
    }
    
    return false;

}


std::vector<NetworkMail> BitMessage::getUnreadMail(std::string address){return std::vector<NetworkMail>();} // You don't want to have to do copies of your whole inbox for every download
bool BitMessage::deleteMessage(NetworkMail message){return false;} // Any part of the message should be able to be used to delete it from an inbox
bool BitMessage::markRead(NetworkMail message, bool read){return false;} // By default this marks a given message as read or not, not all API's will support this and should thus return false.


std::vector<NetworkMail> BitMessage::getInbox(std::string address){return std::vector<NetworkMail>();}
std::vector<NetworkMail> BitMessage::getAllInboxes(){return getInbox();}
std::vector<NetworkMail> BitMessage::getAllUnread(){return std::vector<NetworkMail>();}

bool BitMessage::sendMail(NetworkMail message){return false;}


std::vector<std::string> BitMessage::getSubscriptions(){return std::vector<std::string>();}

bool BitMessage::checkContacts(){
    
    try{
        std::function<void()> command = std::bind(&BitMessage::listAddressBookEntries, this); // push a list address request to the queue.
        bm_queue->addToQueue(command);
        return true;
    }
    catch(...){
        return false;
    }
}

/*
 * Message Queue Interaction
 */

bool BitMessage::startQueue(){
    
    if(bm_queue != nullptr){
        if(bm_queue->start())   // Should also have checks to determine if queue was already started.
            return true;
        else{
            std::cerr << "If you are seeing this, something went terribly wrong: bm_queue is a nullptr" << std::endl;
            return false;
        }
    }
    else
        return false;
}

bool BitMessage::stopQueue(){
    
    if(bm_queue != nullptr){
        if(bm_queue->stop())   // Should also have checks to determine if queue was already stopped.
            return true;
        else
            return false;
    }
    else
        return false;
}

bool BitMessage::flushQueue(){
    try{
        bm_queue->clearQueue();
        return true;
    }
    catch(...){
        return false;
    }
}

int BitMessage::queueSize(){
    if(bm_queue != nullptr){
        return bm_queue->queueSize();
    }
    else{
        std::cerr << "Message Queue does not exist!" << std::endl;
        return 0;
    }
}






/*
 * Direct "Low-Level" API Functions
 */

// Inbox Management


void BitMessage::getAllInboxMessages(){

    Parameters params;
    std::vector<BitInboxMessage> inbox;

    XmlResponse result = m_xmllib->run("getAllInboxMessages", params);
    
    if(result.first == false){
        std::cerr << "Error: getAllInboxMessages failed" << std::endl;
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
        }
    }
    
    Json::Value root;
    Json::Reader reader;
    
    bool parsesuccess = reader.parse( ValueString(result.second), root );
    if ( !parsesuccess )
    {
        std::cerr  << "Failed to parse inbox\n" << reader.getFormattedErrorMessages();
    }
    
    const Json::Value inboxMessages = root["inboxMessages"];
    for ( int index = 0; index < inboxMessages.size(); ++index ){  // Iterates over the sequence elements.
        
        // We need to sanitize our string, or else it will get cut off because of the newlines.
        std::string dirtyMessage = inboxMessages[index].get("message", "").asString();
        dirtyMessage.erase(std::remove(dirtyMessage.begin(), dirtyMessage.end(), '\n'), dirtyMessage.end());
        base64 cleanMessage(dirtyMessage, true);
        
        BitInboxMessage message(inboxMessages[index].get("msgid", "").asString(), inboxMessages[index].get("toAddress", "").asString(), inboxMessages[index].get("fromAddress", "").asString(), base64(inboxMessages[index].get("subject", "").asString(), true), cleanMessage, inboxMessages[index].get("encodingType", 0).asInt(), std::atoi(inboxMessages[index].get("receivedTime", 0).asString().c_str()), inboxMessages[index].get("read", false).asBool());
        
        inbox.push_back(message);
        
    }

    std::unique_lock<std::mutex> mlock(m_localInboxMutex); // Lock so that we dont have a race condition.
    // Populate our local inbox.

    for(int x=0; x<inbox.size(); x++){
        NetworkMail l_mail(inbox.at(x).getFromAddress(), inbox.at(x).getToAddress(), inbox.at(x).getSubject().decoded(), inbox.at(x).getMessage().decoded(), inbox.at(x).getRead(), inbox.at(x).getReceivedTime() );
        
        m_localInbox.push_back(l_mail);
    }
    std::reverse(m_localInbox.begin(), m_localInbox.end());  // New messages at the front
    mlock.unlock(); // Release our lock so that others can access the inbox
    
};


BitInboxMessage BitMessage::getInboxMessageByID(std::string msgID, bool setRead){

    Parameters params;
    
    params.push_back(ValueString(msgID));
    params.push_back(ValueBool(setRead));

    XmlResponse result = m_xmllib->run("getInboxMessageByID", params);
    
    if(result.first == false){
        std::cerr << "Error: getInboxMessageByID failed" << std::endl;
        BitInboxMessage message("", "", "", base64(""), base64(""), 0, 0, false);
        return message;
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            BitInboxMessage message("", "", "", base64(""), base64(""), 0, 0, false);
            return message;
        }
    }
    
    
    Json::Value root;
    Json::Reader reader;
    
    bool parsesuccess = reader.parse( ValueString(result.second), root );
    if ( !parsesuccess )
    {
        std::cerr  << "Failed to parse inbox\n" << reader.getFormattedErrorMessages();
        BitInboxMessage message("", "", "", base64(""), base64(""), 0, 0, false);
        return message;
    }
    
    const Json::Value inboxMessage = root["inboxMessage"];
    
    std::string dirtyMessage = inboxMessage[0].get("message", "").asString();
    dirtyMessage.erase(std::remove(dirtyMessage.begin(), dirtyMessage.end(), '\n'), dirtyMessage.end());
    base64 cleanMessage(dirtyMessage, true);
    
    
    BitInboxMessage message(inboxMessage[0].get("msgid", "").asString(), inboxMessage[0].get("toAddress", "").asString(), inboxMessage[0].get("fromAddress", "").asString(), base64(inboxMessage[0].get("subject", "").asString(), true), cleanMessage, inboxMessage[0].get("encodingType", 0).asInt(), std::atoi(inboxMessage[0].get("receivedTime", 0).asString().c_str()), inboxMessage[0].get("read", false).asBool());
     
    return message;

};


BitMessageOutbox BitMessage::getAllSentMessages(){

    Parameters params;
    BitMessageOutbox outbox;
    
    XmlResponse result = m_xmllib->run("getAllSentMessages", params);
    
    if(result.first == false){
        std::cerr << "Error: getAllSentMessages failed" << std::endl;;
        return outbox;
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            BitInboxMessage message("", "", "", base64(""), base64(""), 0, 0, false);
            return outbox;
        }
    }
    
    Json::Value root;
    Json::Reader reader;
    
    const Json::Value sentMessages = root["sentMessages"];
    for ( int index = 0; index < sentMessages.size(); ++index ){  // Iterates over the sequence elements.
        
        // We need to sanitize our string, or else it will get cut off because of the newlines.
        std::string dirtyMessage = sentMessages[index].get("message", "").asString();
        dirtyMessage.erase(std::remove(dirtyMessage.begin(), dirtyMessage.end(), '\n'), dirtyMessage.end());
        base64 cleanMessage(dirtyMessage, true);
        
        BitSentMessage message(sentMessages[index].get("msgid", "").asString(), sentMessages[index].get("toAddress", "").asString(), sentMessages[index].get("fromAddress", "").asString(), base64(sentMessages[index].get("subject", "").asString(), true), cleanMessage, sentMessages[index].get("encodingType", 0).asInt(), std::atoi(sentMessages[index].get("lastActionTime", 0).asString().c_str()), sentMessages[index].get("status", false).asString(), sentMessages[index].get("ackData", false).asString());
        
        outbox.push_back(message);
        
    }

   return outbox;
    
};


BitSentMessage BitMessage::getSentMessageByID(std::string msgID){
    
    Parameters params;
    
    params.push_back(ValueString(msgID));
    
    XmlResponse result = m_xmllib->run("getSentMessageByID", params);
    
    if(result.first == false){
        std::cerr << "Error: getSentMessageByID failed" << std::endl;;
        return BitSentMessage("", "", "", base64(""), base64(""), 0, 0, "", "");
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return BitSentMessage("", "", "", base64(""), base64(""), 0, 0, "", "");
        }
    }
    
    
    Json::Value root;
    Json::Reader reader;
    
    bool parsesuccess = reader.parse( ValueString(result.second), root );
    if ( !parsesuccess )
    {
        std::cerr  << "Failed to parse outbox\n" << reader.getFormattedErrorMessages();
        return BitSentMessage("", "", "", base64(""), base64(""), 0, 0, "", "");
    }
    
    const Json::Value sentMessage = root["sentMessage"];
    
    std::string dirtyMessage = sentMessage[0].get("message", "").asString();
    dirtyMessage.erase(std::remove(dirtyMessage.begin(), dirtyMessage.end(), '\n'), dirtyMessage.end());
    base64 cleanMessage(dirtyMessage, true);
    
    
    BitSentMessage message(sentMessage[0].get("msgid", "").asString(), sentMessage[0].get("toAddress", "").asString(), sentMessage[0].get("fromAddress", "").asString(), base64(sentMessage[0].get("subject", "").asString(), true), cleanMessage, sentMessage[0].get("encodingType", 0).asInt(), sentMessage[0].get("lastActionTime", 0).asInt(), sentMessage[0].get("status", false).asString(), sentMessage[0].get("ackData", false).asString());
    
    return message;
    
};


BitSentMessage BitMessage::getSentMessageByAckData(std::string ackData){

    Parameters params;
    
    params.push_back(ValueString(ackData));
    
    XmlResponse result = m_xmllib->run("getSentMessageByAckData", params);
    
    if(result.first == false){
        std::cerr << "Error: getSentMessageByAckData failed" << std::endl;;
        return BitSentMessage("", "", "", base64(""), base64(""), 0, 0, "", "");
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return BitSentMessage("", "", "", base64(""), base64(""), 0, 0, "", "");
        }
    }
    
    
    Json::Value root;
    Json::Reader reader;
    
    bool parsesuccess = reader.parse( ValueString(result.second), root );
    if ( !parsesuccess )
    {
        std::cerr  << "Failed to parse outbox\n" << reader.getFormattedErrorMessages();
        return BitSentMessage("", "", "", base64(""), base64(""), 0, 0, "", "");
    }
    
    const Json::Value sentMessage = root["sentMessage"];
    
    std::string dirtyMessage = sentMessage[0].get("message", "").asString();
    dirtyMessage.erase(std::remove(dirtyMessage.begin(), dirtyMessage.end(), '\n'), dirtyMessage.end());
    base64 cleanMessage(dirtyMessage, true);
    
    
    BitSentMessage message(sentMessage[0].get("msgid", "").asString(), sentMessage[0].get("toAddress", "").asString(), sentMessage[0].get("fromAddress", "").asString(), base64(sentMessage[0].get("subject", "").asString(), true), cleanMessage, sentMessage[0].get("encodingType", 0).asInt(), sentMessage[0].get("lastActionTime", 0).asInt(), sentMessage[0].get("status", false).asString(), sentMessage[0].get("ackData", false).asString());
    
    return message;
    
};


std::vector<BitSentMessage> BitMessage::getSentMessagesBySender(std::string address){

    
    Parameters params;
    BitMessageOutbox outbox;
    
    params.push_back(ValueString(address));
    
    XmlResponse result = m_xmllib->run("getSentMessagesBySender", params);
    
    if(result.first == false){
        std::cerr << "Error: getAllSentMessages failed" << std::endl;;
        return outbox;
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            BitInboxMessage message("", "", "", base64(""), base64(""), 0, 0, false);
            return outbox;
        }
    }
    
    Json::Value root;
    Json::Reader reader;
    
    const Json::Value sentMessages = root["sentMessages"];
    for ( int index = 0; index < sentMessages.size(); ++index ){  // Iterates over the sequence elements.
        
        // We need to sanitize our string, or else it will get cut off because of the newlines.
        std::string dirtyMessage = sentMessages[index].get("message", "").asString();
        dirtyMessage.erase(std::remove(dirtyMessage.begin(), dirtyMessage.end(), '\n'), dirtyMessage.end());
        base64 cleanMessage(dirtyMessage, true);
        
        BitSentMessage message(sentMessages[index].get("msgid", "").asString(), sentMessages[index].get("toAddress", "").asString(), sentMessages[index].get("fromAddress", "").asString(), base64(sentMessages[index].get("subject", "").asString(), true), cleanMessage, sentMessages[index].get("encodingType", 0).asInt(), std::atoi(sentMessages[index].get("lastActionTime", 0).asString().c_str()), sentMessages[index].get("status", false).asString(), sentMessages[index].get("ackData", false).asString());
        
        outbox.push_back(message);
        
    }
    
    return outbox;

};


bool BitMessage::trashMessage(std::string msgID){

    Parameters params;
    params.push_back(ValueString(msgID));
    
    XmlResponse result = m_xmllib->run("trashMessage", params);
    
    if(result.first == false){
        std::cerr << "Error: trashMessage failed" << std::endl;
        return false;
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return false;
        }
    }
    
    return true;
    
};


bool BitMessage::trashSentMessageByAckData(std::string ackData){

    Parameters params;
    params.push_back(ValueString(ackData));
    
    XmlResponse result = m_xmllib->run("trashSentMessageByAckData", params);
    
    if(result.first == false){
        std::cerr << "Error: trashSentMessageByAckData failed" << std::endl;
        return false;
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return false;
        }
    }
    
    return true;

};



// Message Management


std::string BitMessage::sendMessage(std::string fromAddress, std::string toAddress, base64 subject, base64 message, int encodingType){

    Parameters params;
    params.push_back(ValueString(fromAddress));
    params.push_back(ValueString(toAddress));
    params.push_back(ValueString(subject.encoded()));
    params.push_back(ValueString(message.encoded()));
    params.push_back(ValueInt(encodingType));

    
    XmlResponse result = m_xmllib->run("sendMessage", params);
    
    if(result.first == false){
        std::cerr << "Error: sendMessage failed" << std::endl;
        return "";
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return "";
        }
    }
    
    return std::string(ValueString(result.second));

};


std::string BitMessage::sendBroadcast(std::string fromAddress, base64 subject, base64 message, int encodingType){

    Parameters params;
    params.push_back(ValueString(fromAddress));
    params.push_back(ValueString(subject.encoded()));
    params.push_back(ValueString(message.encoded()));
    params.push_back(ValueInt(encodingType));
    
    
    XmlResponse result = m_xmllib->run("sendBroadcast", params);
    
    if(result.first == false){
        std::cerr << "Error: sendBroadcast failed" << std::endl;
        return "";
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return "";
        }
    }
    
    return std::string(ValueString(result.second));

};



// Subscription Management


BitMessageSubscriptionList BitMessage::listSubscriptions(){

    Parameters params;
    BitMessageSubscriptionList subscriptionList;
    
    XmlResponse result = m_xmllib->run("listSubscriptions", params);
    
    if(result.first == false){
        std::cerr << "Error: listSubscriptions failed" << std::endl;
        return subscriptionList;
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return subscriptionList;
        }
    }
    
    Json::Value root;
    Json::Reader reader;
    
    bool parsesuccess = reader.parse( ValueString(result.second), root );
    if ( !parsesuccess )
    {
        std::cerr  << "Failed to parse subscription list\n" << reader.getFormattedErrorMessages();
        return subscriptionList;
    }
    
    const Json::Value subscriptions = root["subscriptions"];
    for ( int index = 0; index < subscriptions.size(); ++index ){  // Iterates over the sequence elements.
        
        std::string dirtyLabel = subscriptions[index].get("label", "").asString();
        dirtyLabel.erase(std::remove(dirtyLabel.begin(), dirtyLabel.end(), '\n'), dirtyLabel.end());
        std::string cleanRipe(dirtyLabel);
        
        BitMessageSubscription subscription(subscriptions[index].get("address", "").asString(), subscriptions[index].get("enabled", "").asBool(), base64(cleanRipe, true));
        
        subscriptionList.push_back(subscription);
        
    }
    
    return subscriptionList;

};


bool BitMessage::addSubscription(std::string address, base64 label){

    Parameters params;
    params.push_back(ValueString(address));
    params.push_back(ValueString(label.encoded()));

    
    XmlResponse result = m_xmllib->run("addSubscription", params);
    
    if(result.first == false){
        std::cerr << "Error: createChan failed" << std::endl;
        return false;
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return false;
        }
    }
    
    return true;

};


bool BitMessage::deleteSubscription(std::string address){

    Parameters params;
    params.push_back(ValueString(address));
    
    XmlResponse result = m_xmllib->run("deleteSubscription", params);
    
    if(result.first == false){
        std::cerr << "Error: createChan failed" << std::endl;
        return false;
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return false;
        }
    }
    
    return true;

};



// Channel Management


std::string BitMessage::createChan(base64 password){

    Parameters params;
    params.push_back(ValueString(password.encoded()));
    
    XmlResponse result = m_xmllib->run("createChan", params);
    
    if(result.first == false){
        std::cerr << "Error: createChan failed" << std::endl;
        return "";
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return "";
        }
    }
    
    return std::string(ValueString(result.second));

};


bool BitMessage::joinChan(base64 password, std::string address){

    Parameters params;
    params.push_back(ValueString(password.encoded()));
    params.push_back(ValueString(address));
    
    XmlResponse result = m_xmllib->run("joinChan", params);
    
    if(result.first == false){
        std::cerr << "Error: joinChan failed" << std::endl;
        return false;
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return false;
        }
    }
    
    return true;

};


bool BitMessage::leaveChan(std::string address){

    Parameters params;
    params.push_back(ValueString(address));
    
    XmlResponse result = m_xmllib->run("leaveChan", params);
    
    if(result.first == false){
        std::cerr << "Error: leaveChan failed" << std::endl;
        return false;
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return false;
        }
    }
    
    return true;
};




// Address Management


void BitMessage::listAddresses(){
    
    std::vector<xmlrpc_c::value> params;
    BitMessageIdentities responses;

    XmlResponse result = m_xmllib->run("listAddresses2", params);
    
    if(result.first == false){
        std::cerr << "Error: listAddresses2 failed" << std::endl;
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
        }
    }
    
    Json::Value root;
    Json::Reader reader;
    
    bool parsesuccess = reader.parse( ValueString(result.second), root );
    if ( !parsesuccess )
    {
        std::cerr << "Failed to parse configuration\n" << reader.getFormattedErrorMessages();
    }
    
    const Json::Value addresses = root["addresses"];
    for ( int index = 0; index < addresses.size(); ++index ){  // Iterates over the sequence elements.
        BitMessageIdentity entry(base64(addresses[index].get("label", "").asString()), addresses[index].get("address", "").asString(), addresses[index].get("stream", 0).asInt(), addresses[index].get("enabled", false).asBool(), addresses[index].get("chan", false).asBool());
        
        responses.push_back(entry);
        
    }
    
    std::unique_lock<std::mutex> mlock(m_localIdentitiesMutex);
    m_localIdentities = responses;
    mlock.unlock();
    
}


void BitMessage::createRandomAddress(base64 label, bool eighteenByteRipe, int totalDifficulty, int smallMessageDifficulty){

    Parameters params;
    params.push_back(ValueString(label.encoded()));
    params.push_back(ValueBool(eighteenByteRipe));
    params.push_back(ValueInt(totalDifficulty));
    params.push_back(ValueInt(smallMessageDifficulty));

    
    XmlResponse result = m_xmllib->run("createRandomAddress", params);
    
    if(result.first == false){
        std::cerr << "Error: createRandomAddress failed" << std::endl;
        //return "";
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            //return "";
        }
    }
    
    std::unique_lock<std::mutex> mlock(m_newestCreatedAddressMutex);
    newestCreatedAddress = std::string(ValueString(result.second));
    mlock.unlock();
    
};


std::vector<BitMessageAddress> BitMessage::createDeterministicAddresses(base64 password, int numberOfAddresses, int addressVersionNumber, int streamNumber, bool eighteenByteRipe, int totalDifficulty, int smallMessageDifficulty){

    Parameters params;
    std::vector<BitMessageAddress> addressList;
    
    params.push_back(ValueString(password.encoded()));
    params.push_back(ValueInt(numberOfAddresses));
    params.push_back(ValueInt(addressVersionNumber));
    params.push_back(ValueInt(streamNumber));
    params.push_back(ValueBool(eighteenByteRipe));
    params.push_back(ValueInt(totalDifficulty));
    params.push_back(ValueInt(smallMessageDifficulty));

    
    XmlResponse result = m_xmllib->run("createDeterministicAddresses", params);
    
    if(result.first == false){
        std::cerr << "Error: createDeterministicAddresses failed" << std::endl;
        return addressList;
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return addressList;
        }
    }
    
    Json::Value root;
    Json::Reader reader;
    
    bool parsesuccess = reader.parse( ValueString(result.second), root );
    if ( !parsesuccess )
    {
        std::cerr  << "Failed to parse address list\n" << reader.getFormattedErrorMessages();
        return addressList;
    }
    
    const Json::Value addresses = root["addresses"];
    for ( int index = 0; index < addresses.size(); ++index ){  // Iterates over the sequence elements.
        BitMessageAddress generatedAddress = addresses[index].asString();
        
        addressList.push_back(generatedAddress);
        
    }
    
    return addressList;

};


BitMessageAddress BitMessage::getDeterministicAddress(base64 password, int addressVersionNumber, int streamNumber){
    
    Parameters params;
    params.push_back(ValueString(password.encoded()));
    params.push_back(ValueInt(addressVersionNumber));
    params.push_back(ValueInt(streamNumber));
    
    
    XmlResponse result = m_xmllib->run("getDeterministicAddress", params);
    
    if(result.first == false){
        std::cerr << "Error: getDeterministicAddress failed" << std::endl;
        return "";
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return "";
        }
    }
    
    return std::string(ValueString(result.second));

};


void BitMessage::listAddressBookEntries(){

    Parameters params;
    
    BitMessageAddressBook addressBook;
    
    XmlResponse result = m_xmllib->run("listAddressBookEntries", params);
    
    if(result.first == false){
        std::cerr << "Error: listAddressBookEntries failed" << std::endl;
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
        }
    }
    
    Json::Value root;
    Json::Reader reader;
    
    bool parsesuccess = reader.parse( ValueString(result.second), root );
    if ( !parsesuccess )
    {
        std::cerr  << "Failed to parse address list\n" << reader.getFormattedErrorMessages();
    }
    
    const Json::Value addresses = root["addresses"];
    for ( int index = 0; index < addresses.size(); ++index ){  // Iterates over the sequence elements.
        
        std::string dirtyLabel = addresses[index].get("label", "").asString();
        dirtyLabel.erase(std::remove(dirtyLabel.begin(), dirtyLabel.end(), '\n'), dirtyLabel.end());
        std::string cleanRipe(dirtyLabel);
        
        BitMessageAddressBookEntry address(addresses[index].get("address", "").asString(), base64(cleanRipe, true));
        
        addressBook.push_back(address);
        
    }
    
    std::unique_lock<std::mutex> mlock(m_localAddressBookMutex);
    m_localAddressBook = addressBook;
    mlock.unlock();
    
};


bool BitMessage::addAddressBookEntry(std::string address, base64 label){

    Parameters params;
    params.push_back(ValueString(address));
    params.push_back(ValueString(label.encoded()));
    
    XmlResponse result = m_xmllib->run("addAddressBookEntry", params);
    
    if(result.first == false){
        std::cerr << "Error: addAddressBookEntry failed" << std::endl;
        return false;
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return false;
        }
    }
    
    std::cerr << "BitMessage API Response: " << std::string(ValueString(result.second)) << std::endl;
    
    return true;

};


bool BitMessage::deleteAddressBookEntry(std::string address){

    Parameters params;
    params.push_back(ValueString(address));
    
    XmlResponse result = m_xmllib->run("deleteAddressBookEntry", params);
    
    if(result.first == false){
        std::cerr << "Error: deleteAddressBookEntry failed" << std::endl;
        return false;
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return false;
        }
    }
    
    std::cerr << "BitMessage API Response: " << std::string(ValueString(result.second)) << std::endl;
    
    return true;

};


bool BitMessage::deleteAddress(std::string address){

    Parameters params;
    params.push_back(ValueString(address));
    
    XmlResponse result = m_xmllib->run("deleteAddress", params);
    
    if(result.first == false){
        std::cerr << "Error: deleteAddress failed" << std::endl;
        return false;
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return false;
        }
    }
    
    return true;

};


BitDecodedAddress BitMessage::decodeAddress(std::string address){

    Parameters params;
    
    params.push_back(ValueString(address));
    
    XmlResponse result = m_xmllib->run("decodeAddress", params);
    
    if(result.first == false){
        std::cerr << "Error Accessing BitMessage API" << std::endl;
        return BitDecodedAddress("", 0, "", 0);
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return BitDecodedAddress("", 0, "", 0);
        }
    }
    
    
    Json::Value root;
    Json::Reader reader;
    
    bool parsesuccess = reader.parse( ValueString(result.second), root );
    if ( !parsesuccess )
    {
        std::cerr  << "Failed to parse configuration\n" << reader.getFormattedErrorMessages();
        return BitDecodedAddress("", 0, "", 0);
    }
    
    const Json::Value decodedAddress = root;
    
    std::string dirtyRipe = decodedAddress.get("ripe", "").asString();
    dirtyRipe.erase(std::remove(dirtyRipe.begin(), dirtyRipe.end(), '\n'), dirtyRipe.end());
    std::string cleanRipe(dirtyRipe);
    
    return BitDecodedAddress(decodedAddress.get("status", "").asString(), decodedAddress.get("addressVersion", "").asInt(), cleanRipe, decodedAddress.get("streamNumber", "").asInt());

};





// Other API Commands


std::string BitMessage::helloWorld(std::string first, std::string second){
    
    Parameters params;
    params.push_back(ValueString(first));
    params.push_back(ValueString(second));
    
    XmlResponse result = m_xmllib->run("helloWorld", params);
    
    if(result.first == false){
        std::cerr << "Error Accessing BitMessage API" << std::endl;
        return "";
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return "";
        }
    }
    
    return std::string(ValueString(result.second));
    
}


int BitMessage::add(int x, int y){
    
    Parameters params;
    params.push_back(ValueInt(x));
    params.push_back(ValueInt(y));

    XmlResponse result = m_xmllib->run("add", params);
    
    if(result.first == false){
        std::cerr << "Error: add failed" << std::endl;
        return -1;
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return -1;
        }
        std::cerr << "Unexpected Response to API Command: add" << std::endl;
        std::cerr << std::string(ValueString(result.second)) << std::endl;
        return -1;
    }
    else{
        return ValueInt(result.second);
    }
    
};


std::string BitMessage::getStatus(std::string ackData){

    Parameters params;
    
    params.push_back(ValueString(ackData));

    XmlResponse result = m_xmllib->run("getStatus", params);
    
    if(result.first == false){
        std::cerr << "Error Accessing BitMessage API" << std::endl;
        return "";
    }
    else if(result.second.type() == xmlrpc_c::value::TYPE_STRING){
        std::size_t found;
        found=std::string(ValueString(result.second)).find("API Error");
        if(found!=std::string::npos){
            std::cerr << std::string(ValueString(result.second)) << std::endl;
            return "";
        }
    }
    
    return std::string(ValueString(result.second));

};


void BitMessage::setServerAlive(bool alive){
    
    if(alive){
        NetCounter::setAlive();
        m_serverAvailable = true;
    }
    else{
        NetCounter::dead();
        m_serverAvailable = false;
    }
    
}

void BitMessage::checkAlive(){
    
    if(helloWorld("Hello","World") != "Hello-World"){
        setServerAlive(false);
    }
    else{
        setServerAlive(true);
    }
    
}

void BitMessage::parseCommstring(std::string commstring){
    
    std::vector<std::string> parsedList;
    boost::tokenizer<boost::escaped_list_separator<char> > tokens(commstring);
    
    for(boost::tokenizer<boost::escaped_list_separator<char> >::iterator it=tokens.begin(); it!=tokens.end();++it){
        parsedList.push_back(*it);
    }
    
    if(parsedList.size() > 0)
        m_host = parsedList.at(0);
    else
        m_host = "localhost";
    
    if(parsedList.size() > 1)
        m_port = std::atoi(parsedList.at(1).c_str());
    else
        m_port = 8442;
    
    if(parsedList.size() > 2)
        m_username = parsedList.at(2);
    else
        m_username = "defaultuser";
    
    if(parsedList.size() > 3)
        m_pass = parsedList.at(3);
    else
        m_pass = "defaultpass";
    
}

void BitMessage::initializeUserData(){
    
    listAddresses(); // Populates Local Owned Addresses
    listAddressBookEntries();  // Populates address book data, for remote users we have addresses for.
    getAllInboxMessages();
    
}
