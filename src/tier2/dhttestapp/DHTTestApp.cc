//
// Copyright (C) 2007 Institut fuer Telematik, Universitaet Karlsruhe (TH)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

/**
 * @file DHTTestApp.cc
 * @author Ingmar Baumgart
 */

#include <IPAddressResolver.h>
#include <GlobalNodeListAccess.h>
#include <GlobalStatisticsAccess.h>
#include <UnderlayConfiguratorAccess.h>
#include <RpcMacros.h>
#include "CommonMessages_m.h"

#include <iostream>
#include <fstream>
#include <GlobalDhtTestMap.h>


#include "DHTTestApp.h"

Define_Module(DHTTestApp);

using namespace std;

DHTTestApp::~DHTTestApp()
{
    cancelAndDelete(somakeyput_msg);
    cancelAndDelete(somafkeysign_msg);

    cancelAndDelete(dhttestput_timer);
    cancelAndDelete(dhttestget_timer);
    cancelAndDelete(dhttestmod_timer);
}

DHTTestApp::DHTTestApp()
{
    somakeyput_msg = NULL;
    somafkeysign_msg = NULL;

    dhttestput_timer = NULL;
    dhttestget_timer = NULL;
    dhttestmod_timer = NULL;
}

void DHTTestApp::initializeApp(int stage)
{
    if (stage != MIN_STAGE_APP)
        return;

    // fetch parameters
    debugOutput = par("debugOutput");
    activeNetwInitPhase = par("activeNetwInitPhase");

    mean = par("testInterval");
    p2pnsTraffic = par("p2pnsTraffic");
    deviation = mean / 10;

    //-- SOMA TC(Trust Chain)
    numSomaTrustReqs = par("somaTrustReqs");
    globalTrustLevel = par("globalTrustLevel");

    EV << "Node " << thisNode.getIp() << ", will request signed certs from " << numSomaTrustReqs << " other nodes." << endl;
    //--

    if (p2pnsTraffic) {
        ttl = 3600*24*365;
    } else {
        ttl = par("testTtl");
    }

    globalNodeList = GlobalNodeListAccess().get();
    underlayConfigurator = UnderlayConfiguratorAccess().get();
    globalStatistics = GlobalStatisticsAccess().get();

    globalDhtTestMap = dynamic_cast<GlobalDhtTestMap*>(simulation.getModuleByPath(
            "globalObserver.globalFunctions[0].function"));

    if (globalDhtTestMap == NULL) {
        throw cRuntimeError("DHTTestApp::initializeApp(): "
                                "GlobalDhtTestMap module not found!");
    }

    // statistics
    numSent = 0;
    numGetSent = 0;
    numGetError = 0;
    numGetSuccess = 0;
    numPutSent = 0;
    numPutError = 0;
    numPutSuccess = 0;

    successDelay = 0.0;
    numSomaKeys = 0;
    sentReqsFlag = 0;
    soma_init_timer  = simTime();
    soma_total_time  = 0;
    soma_keyputtime = -1;
    soma_fkeysigntime = -1;
    //soma_findNodeTimer = par("sendTCPeriod");
    certVerficationDelay = 0.005;
    timeVector.setName("SomaJoinTime");
    debug = true;
    pendReqsNum = 0;
    //initRpcs();
    WATCH(numSent);
    WATCH(numGetSent);
    WATCH(numGetError);
    WATCH(numGetSuccess);
    WATCH(numPutSent);
    WATCH(numPutError);
    WATCH(numPutSuccess);
    //WATCH(numSomaKeys);
    //globalStatistics = GlobalStatisticsAccess().get();

    nodeIsLeavingSoon = false;

    // RSA 2048 bit public key and certificate
    publickey = (char *)"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtYyJkl9HNu1ER0jE+Hhm\
F9GftLsI/k474satrgxS1zYb2Osrsl+Fs1G1ZCg6qKyaWYn4FsimxXZ/k8tLgpUM\
JkUJUq0AzcS4e/Oc/f7Bm8Zf1dfAoqwtm0Yqvu7p3R6zweubiLA9iBFVplCcJsOo\
kLreQBM1aW9m+Y5ztZMcT4Y4gVVCcjT7J5gD2ds8XJdTsp3Sr3lEYVai0SF0XxPw\
uhzOi2tQ/8cyN43DInAHQPIeDPFllfGqxyUZLp6l03h28lZYGKYfRAZAyqqnA8RN\
5cpYV4DCMGD3CnqhM5d5tffRQ2LmnoYTgGyZzuUHaToxUT0ZmOMSGakHVkma0r9a\
lQIDAQAB";

    cert = (char *)"MIIDXTCCAkWgAwIBAgIJAKFLjcdVAiS/MA0GCSqGSIb3DQEBCwUAMEUxCzAJBgNV\
BAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYDVQQKDBhJbnRlcm5ldCBX\
aWRnaXRzIFB0eSBMdGQwHhcNMTcwMzIyMTAwMTE5WhcNMTgwMzIyMTAwMTE5WjBF\
MQswCQYDVQQGEwJBVTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50\
ZXJuZXQgV2lkZ2l0cyBQdHkgTHRkMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB\
CgKCAQEAtYyJkl9HNu1ER0jE+HhmF9GftLsI/k474satrgxS1zYb2Osrsl+Fs1G1\
ZCg6qKyaWYn4FsimxXZ/k8tLgpUMJkUJUq0AzcS4e/Oc/f7Bm8Zf1dfAoqwtm0Yq\
vu7p3R6zweubiLA9iBFVplCcJsOokLreQBM1aW9m+Y5ztZMcT4Y4gVVCcjT7J5gD\
2ds8XJdTsp3Sr3lEYVai0SF0XxPwuhzOi2tQ/8cyN43DInAHQPIeDPFllfGqxyUZ\
Lp6l03h28lZYGKYfRAZAyqqnA8RN5cpYV4DCMGD3CnqhM5d5tffRQ2LmnoYTgGyZ\
zuUHaToxUT0ZmOMSGakHVkma0r9alQIDAQABo1AwTjAdBgNVHQ4EFgQUTWbAGO6Q\
hNWGRhn2+jnTiJ9PFNowHwYDVR0jBBgwFoAUTWbAGO6QhNWGRhn2+jnTiJ9PFNow\
DAYDVR0TBAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAUMmh4HGBWKHTYrbs8eeY\
+oCLdv56IP/xqgtUvwYDbfseNHUr2MMcQhXYHHandpOTQBCd1uD9ZEGxJ2anwla1\
ec0WFEkoDjBGy6eQo1pQOKEtj7KZnIAukznYBvhHup9U2PaDZ071NjcQ2atY8pGN\
Kb3a6n5K0BlvkRIDY03blWki+Hg1r2GqxI/t7Zgvr3wl3QB1V29Ath4Sd4OO83Gk\
mAkrECPAXrOa6Eoo5TkMdqCI2Gp9ayMuTffjF+rMZJM3aU/s7q7BuFXqCp91wDE/\
KTgz5xj/eQMqLRnn1ll+q9MjfpzV9ZrC4+d3KQYcT5BraGDfqp7NBBd5fgnlMH/C\
GA==";

    signTemplate = (char *)"|YcmV5HrJNBQ/zVQZNaOtHvG3BxePT4b6wi+ab23hMfdn0SapmCOqWGtIxpm9BvM\
3Ua9rQ2RZEablzga0GosgkclhTB8VJGA3jt7+U+u8nKKwqTeKLLq9SncGU0b9PEM\
i9YNqDyhNlETrkNOYLvTFlraQCaoPa27x5bz26gA7BX7IgXe0Qlbel0fXkZD/cbh\
d30PZ1s5fOWAi6OB/7oseewUqtwpiZnouJ+Tf+W+/sjv1Rw/fsB1xIbtcxqOstRs\
e9GBLPQ76DYU8pNYRM6Romt+GaIJLASFneYlUpBHZYpVTG450Qe6cmApYFP5HNDd\
nlTe05wO4ZmcALSX| ";


    dhttestget_timer = new cMessage("dhttest_get_timer");

    scheduleAt(simTime() + 5,  dhttestget_timer);

    /*dhttestput_timer = new cMessage("dhttest_put_timer");
    dhttestget_timer = new cMessage("dhttest_get_timer");
    dhttestmod_timer = new cMessage("dhttest_mod_timer");

    if (mean > 0) {
        scheduleAt(simTime() + truncnormal(mean, deviation),
                   dhttestput_timer);
        scheduleAt(simTime() + truncnormal(mean + mean / 3,
                                                      deviation),
                                                      dhttestget_timer);
        scheduleAt(simTime() + truncnormal(mean + 2 * mean / 3,
                                                      deviation),
                                                      dhttestmod_timer);


    }*/
}

bool DHTTestApp::handleRpcCall(BaseCallMessage* msg)
{
    // start a switch
    RPC_SWITCH_START(msg);

    // enters the following block if the message is of type ChordStateReadyCall (note the shortened parameter!)
    RPC_ON_CALL(ChordStateReady) {

        handlePutCall(msg);
        break;
    }
    RPC_ON_CALL(DHTKeyPut)
    {
        handleDHTKeyPutCall(_DHTKeyPutCall);//,
        break;
    }
    RPC_ON_CALL(DHTreturnSignedCert)
    {
        handleDHTreturnSignedCert(_DHTreturnSignedCertCall);
        break;
    }
    // end the switch
    RPC_SWITCH_END();

    // return whether we handled the message or not.
    // don't delete unhandled messages!
    return RPC_HANDLED;


}

void DHTTestApp::handlePutCall(BaseCallMessage* msg)
{
    // Send our own key for sign
    string nodeIp = thisNode.getIp().str();
    string pkIp = string(publickey) + string(nodeIp);
    string certIp = string(cert) +
            string(signTemplate) +
            string(nodeIp) + "|";

    OverlayKey somaKey(OverlayKey::sha1(pkIp));

    if (debug){
        std::ofstream outFile;
        outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
        outFile << "\n nodeIp: " << nodeIp << " - somaKey: " << somaKey << std::flush;
    }

    EV << nodeIp << " -> somakey : " << somaKey << endl;

      // Debugging
//    std::ofstream outFile;
//    outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
//
//    outFile << "\nnode: " << thisNode.getIp() << " key sent:" << somaKey << std::flush;

    DHTputCAPICall* dhtPutMsg = new DHTputCAPICall();
    dhtPutMsg->setKey(somaKey);
    dhtPutMsg->setValue(certIp);
    dhtPutMsg->setTtl(ttl);
    dhtPutMsg->setIsModifiable(true);

    RECORD_STATS(numSent++; numPutSent++);

    sendInternalRpcCall(TIER1_COMP, dhtPutMsg,
            new DHTStatsContext(globalStatistics->isMeasuring(),
                                simTime(), somaKey, dhtPutMsg->getValue()));

    // Sign the keys that are assigned to us
    DHTgetResponsibleCall* dhtGetRespMsg = new DHTgetResponsibleCall();
    RECORD_STATS(numSent++; numGetSent++);

    sendInternalRpcCall(TIER1_COMP, dhtGetRespMsg);
}

void DHTTestApp::dumpAccessedNodes()
{
    EV << "dumpAccessedNodes: " << endl;

    std::map<OverlayKey, TrustNodeLvlOne>::iterator it;

    if (accessedNodes.size() > 0) {
        int i = 0;
        for (it = accessedNodes.begin(); it != accessedNodes.end(); it++,i++) {
            EV << "key " << i << ": " << it->first << endl;
            if(it->second.isItTrusted){
                EV << " can be trusted" << endl;
            }
            else{
                EV << " cannot be trusted" << endl;
            }
        }
    }
}


bool DHTTestApp::haveSignedOtherNodeCert(string myIP, string signedCert)
{
    bool haveSigned = false;
    std::string findStr = myIP + "|";

    return haveSigned = signedCert.find(findStr) != std::string::npos;

}


// Convert the Ips that exist in the signedCertificate to Overlay keys (somakeys)
std::list<OverlayKey> DHTTestApp::convertIPsToKeys(std::string s){
    std::list<OverlayKey> keyList;
    std::list<string> IPs;

    boost::smatch m;
    boost::regex e ("(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})");   // matches IP addresses

    while (boost::regex_search (s,m,e)) {
      for (boost::smatch::iterator it = m.begin(); it!=m.end(); ++it) {
          std::cout << *it << std::endl;
          IPs.push_back(*it);
          if(debug) {
              std::ofstream outFile;
              outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
              outFile << "\n IP: " << *it << std::flush;
          }
          ++it;
      }
      s = m.suffix().str();
    }
    if ( m.size() > 0 )
    {
        string pkIp;
        std::list<string>::iterator it;
        for (it = IPs.begin(); it != IPs.end(); it++)
        {
            pkIp = string(publickey) + string(*it);
            OverlayKey key(OverlayKey::sha1(pkIp));
            keyList.push_back(key);

            if(debug) {
                std::ofstream outFile;
                outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                outFile << "\n converted to key: " << key << std::flush;
            }
        }
    }

    return keyList;
}


// the keys to be inserted as child nodes for search in the pendingChildNodes list of level 1 node
void DHTTestApp::insertChildNodes(std::vector<TrustNode>::iterator it, std::list<OverlayKey> keys, int level)
{
    std::list<OverlayKey>::iterator itList;
    std::vector<childNodeInfo>::iterator it2;
    std::vector<TrustNode>::iterator itPR;
    int brkFlag = false;
    if (debug){
        std::ofstream outFile;
        outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
        outFile << "\nin insertChildNodes " << std::flush;
    }
    for (itList = keys.begin(); itList != keys.end(); itList++) {
        bool alreadyExists = false;
        for(itPR = pendingReqs.begin(); itPR != pendingReqs.end(); itPR++){
            if(itPR->nodeKey == *itList and itPR->pendingChildNodes.size() > 0){
                alreadyExists = true;
                break;
            }
            else{
                for(it2 = it->pendingChildNodes.begin(); it2 != it->pendingChildNodes.end(); it2++) {
                    if (it2->nodeKey == *itList) {
                        alreadyExists = true;
                        brkFlag = true;
                        break;
                    }
                }
                if (brkFlag)
                    break;
            }
        }
        if(!alreadyExists) {
            if (debug){
                std::ofstream outFile;
                outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                outFile << "\n " << thisNode.getIp() << " insert in pendingChildNodes the:  " << *itList << std::flush;
            }
            childNodeInfo newNode;
            newNode.dueNode = it->nodeKey;
            newNode.isItTrusted = false;
            newNode.level = level;
            newNode.nodeKey = *itList;
            newNode.responseRcved = false;
            newNode.sentReq = false;
            it->pendingChildNodes.push_back(newNode);
            if (debug){
                std::ofstream outFile;
                outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                outFile << "\n newNode.dueNode " << newNode.dueNode << std::flush;
                outFile << "\n newNode.isItTrusted " << newNode.isItTrusted << std::flush;
                outFile << "\n newNode.level " << newNode.level << std::flush;
                outFile << "\n newNode.nodeKey " << newNode.nodeKey << std::flush;
                outFile << "\n newNode.responseRcved " << newNode.responseRcved << std::flush;
                outFile << "\n newNode.sentReq " << newNode.sentReq << std::flush;

            }
        }
    }
}


// Retrieve the signed certificate value
void DHTTestApp::handleDHTreturnSignedCert(DHTreturnSignedCertCall* msg)
{
    EV << "DHTTestApp::handleDHTreturnSignedCert myIP:" << thisNode.getIp() << endl;
    EV << "SOMASigned Cert Value: "  << msg->getSignedCert() <<
            "\nother node's key: " << msg->getNodeKey() << endl;

    //    dumpAccessedNodes();
    if (globalTrustLevel == LevelOne) {

        string myIp = thisNode.getIp().str();
        string reqstdNodeSCert = msg->getSignedCert();
        OverlayKey reqstdNodeKey = msg->getNodeKey();
        std::map<OverlayKey, TrustNodeLvlOne>::iterator it;

        it = accessedNodes.find(reqstdNodeKey);
        if(it != accessedNodes.end()) {

            //key found in accessedNodes
            bool haveSignedCert = haveSignedOtherNodeCert(myIp, reqstdNodeSCert);
            if(haveSignedCert){
                // update the Trust for that node
                it->second.isItTrusted = true;
                //            EV << "Have signed it, trust it" << endl;

                //-- Measure the time from sending the Request till the reception of the response
                it->second.timestmpRcv = simTime();
                //            EV << "timestmpSend: " << it->second.timestmpSend << endl;
                //            EV << "timestmpRcv: " << it->second.timestmpRcv << endl;

                it->second.rtt = it->second.timestmpRcv - it->second.timestmpSend + certVerficationDelay;
                it->second.foundTrustAtLevel = LevelOne;

                EV << "timestmpRTT: " << it->second.rtt << endl;
                //--
            }
            else{
                it->second.isItTrusted = false;
                EV << "Don't have signed it, don't trust it" << endl;
            }
        }
        else{
            EV << reqstdNodeKey << " did not found in accessedNodes " << endl;
        }
    }
    else {
        /*-- a cert has arrived.
         *-- get the key of the node that responded to the request; OverlayKey reqstdNodeKey = msg->getNodeKey();
         *--  We have 1 vector (pendingReqs) that we store the requests that have been initiated.
         * Upon we find or not Trust of this node, is removed from pendingReqs and is inserted in accessedNodes
         *
         *--With the given key
         *--parse the pendingReqs nodes in depth in order to find the key that has been made a request for; if you don't find it then go to the next pendingReq node of level1
         */
        string myIp = thisNode.getIp().str();
        string pkIp = string(publickey) + string(myIp);
        OverlayKey somaKey(OverlayKey::sha1(pkIp));
        string reqstdNodeSCert = msg->getSignedCert();
        OverlayKey reqstdNodeKey = msg->getNodeKey();

        if (debug) {
            std::ofstream outFile;
            outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
            outFile << "\n node:" << myIp << " Got CERT Response from: " << reqstdNodeKey << " with cert: " << reqstdNodeSCert<<std::flush;
        }

        std::list<OverlayKey> keysSignedReqedNode = convertIPsToKeys(reqstdNodeSCert); //if thisNode exists then we trust
//        if (debug) {
//            std::ofstream outFile;
//            outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
//            std::list<OverlayKey>::iterator it;
//            outFile << "\n IPs to keys: " <<std::flush;
//            for(it = keysSignedReqedNode.begin(); it != keysSignedReqedNode.end(); it++){
//                outFile << "\n " << *it <<std::flush;
//            }
//        }
        std::list<OverlayKey>::iterator it;
        bool isTheNodeTrusted = false;
        if (debug) {
            std::ofstream outFile;
            outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
            outFile << "\nis the node trusted? "<< std::flush;
        }
        // search if somaKey exists in the signatures
        for(it = keysSignedReqedNode.begin(); it != keysSignedReqedNode.end(); it++){
            if(*it == somaKey){
                isTheNodeTrusted = true;
                break;
            }
        }
        if (debug) {
            std::ofstream outFile;
            outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
            outFile << "\nTrusted: " << isTheNodeTrusted << std::flush;
        }

        if( !pendingReqs.empty()) {
            std::vector<TrustNode>::iterator it = pendingReqs.begin();
            // Find the reqed Node (in pendingReqs), if is levelOne or greater level
            bool breakSrch = false;
            for(; it != pendingReqs.end(); it++) {

                if(it->nodeKey != reqstdNodeKey){
                    // Depth search for the node of Level1
                    if (debug) {
                        std::ofstream outFile;
                        outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                        outFile << "\n it->node search: " << it->nodeKey << std::flush;
                    }

                    if(!it->pendingChildNodes.empty()) {
                        // Find the ChildNode
                        std::vector<childNodeInfo>::iterator itCh = it->pendingChildNodes.begin();
                        for (; itCh != it->pendingChildNodes.end(); itCh++){

                            if(debug){
                                std::ofstream outFile;
                                outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                                outFile << "\n itCh->node: " << itCh->nodeKey <<  std::flush;
                            }

                            if (itCh->sentReq && itCh->nodeKey == reqstdNodeKey && !itCh->responseRcved) {
                                // The node with the key found
                                if(debug){
                                    std::ofstream outFile;
                                    outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                                    outFile << "\n Child that sent req for, found: "<< itCh->nodeKey << std::flush;
                                }
                                itCh->responseRcved = true;

                                if (isTheNodeTrusted) {
                                    //  Move the node of level1 to the accessedNodes
                                    TrustNodeLvlOne nodeLvlOne;
                                    nodeLvlOne.foundTrustAtLevel = itCh->level;
                                    nodeLvlOne.isItTrusted = isTheNodeTrusted;
                                    nodeLvlOne.timestmpSend = it->timestmpSend;
                                    nodeLvlOne.timestmpRcv = simTime();
                                    nodeLvlOne.rtt = nodeLvlOne.timestmpRcv - nodeLvlOne.timestmpSend + certVerficationDelay;

                                    if(debug){
                                        std::ofstream outFile;
                                        outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                                        outFile << "\n*  key: " << it->nodeKey <<
                                                "\n  foundTrustAtLevel: " << nodeLvlOne.foundTrustAtLevel <<
                                                "\n  isTheNodeTrusted: " << nodeLvlOne.isItTrusted <<
                                                "\n  rtt: " << nodeLvlOne.rtt << std::flush;
                                        outFile << "\nSuccess found with delay: " << nodeLvlOne.rtt << std::flush;
                                    }
                                    OverlayKey k = it->nodeKey;
                                    accessedNodes.insert(std::make_pair(k, nodeLvlOne));   // store the node in the accessedNodes
                                    successDelay += nodeLvlOne.rtt;

                                    if(debug){
                                        std::ofstream outFile;
                                        outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                                        outFile << "\n  dumpAccessedNodes: " << std::flush;
                                        std::map<OverlayKey, TrustNodeLvlOne>::iterator itAcc;
                                        for(itAcc = accessedNodes.begin(); itAcc != accessedNodes.end(); itAcc++){
                                            outFile << "\n  s " << std::flush;
                                            outFile << "\n  key:  " << itAcc->first << std::flush;
                                            outFile << "\n  e " << std::flush;
                                        }
                                        outFile << "\n  dumpAccessedNodes end " << std::flush;
                                    }
                                    pendingReqs.erase(it);  // remove it from the pendingReqs since we found a trust
                                    if (debug){
                                        std::ofstream outFile;
                                        outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                                        outFile << "\n pendingReqs.erased (it)" << std::flush;
                                    }

                                } //--
                                else {
                                    // Node is not trusted,
                                    // Check the globalTrust level and decide if you will include the childs of child in the trust chain search
                                    if (itCh->level < globalTrustLevel) {
                                        // insert the nodes in the pendingChildNodes
                                        insertChildNodes(it, keysSignedReqedNode, itCh->level+1);
                                    }
                                    else{
                                        // Check if is the last node and trust has not been found and no other requests are going to happen
                                        std::vector<childNodeInfo>::iterator itCh2 = it->pendingChildNodes.begin();
                                        bool lastOne = false;
                                        if(sentReqsFlag >= numSomaTrustReqs){
                                            lastOne = true;

                                            for (; itCh2 != it->pendingChildNodes.end(); itCh2++) {
                                                if (!itCh2->responseRcved) {
                                                    lastOne = false;
                                                }
                                            }
                                        }
                                        if(lastOne) {
                                            if(debug){
                                                std::ofstream outFile;
                                                outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                                                outFile << "\n last node" << std::flush;
                                            }
                                            TrustNodeLvlOne nodeLvlOne;
                                            nodeLvlOne.foundTrustAtLevel = itCh->level;
                                            nodeLvlOne.isItTrusted = isTheNodeTrusted;
                                            nodeLvlOne.timestmpSend = it->timestmpSend;
                                            nodeLvlOne.timestmpRcv = simTime();
                                            nodeLvlOne.rtt = nodeLvlOne.timestmpRcv - nodeLvlOne.timestmpSend + certVerficationDelay;

                                            if(debug){
                                                std::ofstream outFile;
                                                outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                                                outFile << "\n**  key: " << it->nodeKey <<
                                                        "\n  foundTrustAtLevel: " << nodeLvlOne.foundTrustAtLevel <<
                                                        "\n  isTheNodeTrusted: " << nodeLvlOne.isItTrusted <<
                                                        "\n  rtt: " << nodeLvlOne.rtt << std::flush;
                                            }
                                            OverlayKey k = it->nodeKey;
                                            accessedNodes.insert(std::make_pair(k, nodeLvlOne));   // store the node in the accessedNodes
                                            pendingReqs.erase(it);  // remove it from the pendingReqs since we found a negative trust
                                            if(debug){
                                                std::ofstream outFile;
                                                outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                                                outFile << "\n  dumpAccessedNodes: " << std::flush;
                                                std::map<OverlayKey, TrustNodeLvlOne>::iterator itAcc;
                                                for(itAcc = accessedNodes.begin(); itAcc != accessedNodes.end(); itAcc++){
                                                    outFile << "\n  key:  " << itAcc->first << std::flush;
                                                }
                                                outFile << "\n dumpAccessedNodes end" << std::flush;
                                            }
                                        }
                                    }
                                }
                                breakSrch = true;
                                break;
                            }//--
                        }//-- for pendingChildNodes
                        if(breakSrch)
                            break; // break from the loop of pendingReqs
                    }
                }
                else{
                   // found key of node at level1, if is trusted put it in accessedNodes
                    if (debug){
                        std::ofstream outFile;
                        outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                        outFile << "\n " << thisNode.getIp() << " !!found key of node at level1" << std::flush;
                    }
                    if(isTheNodeTrusted){
                        // Node is trusted, move the node of level1 to the accessedNodes

                        TrustNodeLvlOne nodeLvlOne;
                        nodeLvlOne.foundTrustAtLevel = 1;
                        nodeLvlOne.isItTrusted = isTheNodeTrusted;
                        nodeLvlOne.timestmpSend = it->timestmpSend;
                        nodeLvlOne.timestmpRcv = simTime();
                        nodeLvlOne.rtt = nodeLvlOne.timestmpRcv - nodeLvlOne.timestmpSend + certVerficationDelay;
                        if(debug){
                            std::ofstream outFile;
                            outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                            outFile << "\n  key: " << it->nodeKey <<
                                    "\n  foundTrustAtLevel: " << nodeLvlOne.foundTrustAtLevel <<
                                    "\n  isTheNodeTrusted: " << nodeLvlOne.isItTrusted <<
                                    "\n  rtt: " << nodeLvlOne.rtt << std::flush;
                            outFile << "\nSuccess found with delay: " << nodeLvlOne.rtt << std::flush;
                        }
                        accessedNodes.insert(std::make_pair(it->nodeKey, nodeLvlOne));   // store the node in the accessedNodes
                        successDelay += nodeLvlOne.rtt;

                        if(debug){
                            std::ofstream outFile;
                            outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                            outFile << "\n  dumpAccessedNodes: " << std::flush;
                            std::map<OverlayKey, TrustNodeLvlOne>::iterator itAcc;
                            for(itAcc = accessedNodes.begin(); itAcc != accessedNodes.end(); itAcc++){
                                outFile << "\n  key:  " << itAcc->first << std::flush;
                            }
                        }
                        pendingReqs.erase(it);
                        break;
                    }
                    else{
                        // Node is not trusted, if signed by other nodes put these nodes in pendingChildNodes
                        if (debug){
                            std::ofstream outFile;
                            outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                            outFile << "\n !!Node level1 is not trusted, put sign nodes in pendingChildNodes" << std::flush;
                        }
                        insertChildNodes(it, keysSignedReqedNode, LevelOne+1);
                    }
                }
            }// -- for pendingReqs
        }
        else{
            //do nothing
            if (debug){
                std::ofstream outFile;
                outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                outFile << "\n  pendingReqs is empty " << std::flush;
            }
        }
    }
}


void DHTTestApp::handleRpcResponse(BaseResponseMessage* msg,
                                   const RpcState& state, simtime_t rtt)
{
    RPC_SWITCH_START(msg)
    RPC_ON_RESPONSE( DHTputCAPI ) {
        handlePutResponse(_DHTputCAPIResponse,
                          check_and_cast<DHTStatsContext*>(state.getContext()));
        EV << "[DHTTestApp::handleRpcResponse()]\n"
           << "    DHT Put RPC Response received: id=" << state.getId()
           << " msg=" << *_DHTputCAPIResponse << " rtt=" << rtt
           << endl;
        break;
    }
    RPC_ON_RESPONSE(DHTgetCAPI)
    {
        handleGetResponse(_DHTgetCAPIResponse,
                          check_and_cast<DHTStatsContext*>(state.getContext()));
        EV << "[DHTTestApp::handleRpcResponse()]\n"
           << "    DHT Get RPC Response received: id=" << state.getId()
           << " msg=" << *_DHTgetCAPIResponse << " rtt=" << rtt
           << endl;
        break;
    }
    RPC_ON_RESPONSE(DHTgetResponsible)
    {
        handleGetResponsibleResponse(_DHTgetResponsibleResponse);//,
        //                              check_and_cast<DHTStatsContext*>(state.getContext()));
        // EV << "[SOMA-DHTTestApp::handleResponsibleResponse()]\n";
        break;
    }
    RPC_SWITCH_END()
}

void DHTTestApp::handleDHTKeyPutCall(DHTKeyPutCall* msg)
{
    // the key has already been put to the other node, signing delay has been taken into account
    somakeyput_msg = new cMessage("somakey_put_timer");
    soma_keyputtime = simTime();
    scheduleAt(simTime(), somakeyput_msg);
}

void DHTTestApp::handlePutResponse(DHTputCAPIResponse* msg,
                                   DHTStatsContext* context)
{
    EV << "[SOMA-SIGN-DHTTestApp::handle putResponse] node: " << thisNode.getIp() << "\n" ;

    DHTEntry entry = {context->value, simTime() + ttl, simTime()};

    globalDhtTestMap->insertEntry(context->key, entry);
    // --Debug
    //globalDhtTestMap->dumpDHTTestMap();
    //--
    if (context->measurementPhase == false) {
        // don't count response, if the request was not sent
        // in the measurement phase
        delete context;
        return;
    }

    if (msg->getIsSuccess()) {
        RECORD_STATS(numPutSuccess++);
        RECORD_STATS(globalStatistics->addStdDev("DHTTestApp: PUT Latency (s)",
                               SIMTIME_DBL(simTime() - context->requestTime)));
    } else {
        //cout << "DHTTestApp: PUT failed" << endl;
        RECORD_STATS(numPutError++);
    }

    delete context;
}

void DHTTestApp::handleGetResponsibleResponse(DHTgetResponsibleResponse* msg)
{
    int keysResp = msg->getResp();

    simtime_t signingDelay = keysResp *  (0.03973 + 0.03847); // Verify CA signature + sign delay

    numSomaKeys = numSomaKeys + keysResp;

    somafkeysign_msg = new cMessage("somafkeysign_timer");
    scheduleAt(simTime() + signingDelay, somafkeysign_msg);


}

void DHTTestApp::handleGetResponse(DHTgetCAPIResponse* msg,
                                   DHTStatsContext* context)
{
    if (context->measurementPhase == false) {
        // don't count response, if the request was not sent
        // in the measurement phase
        delete context;
        return;
    }

    RECORD_STATS(globalStatistics->addStdDev("DHTTestApp: GET Latency (s)",
                               SIMTIME_DBL(simTime() - context->requestTime)));

    if (!(msg->getIsSuccess())) {
        //cout << "DHTTestApp: success == false" << endl;
        RECORD_STATS(numGetError++);
        delete context;
        return;
    }

    const DHTEntry* entry = globalDhtTestMap->findEntry(context->key);

    if (entry == NULL) {
        //unexpected key
        RECORD_STATS(numGetError++);
        //cout << "DHTTestApp: unexpected key" << endl;
        delete context;
        return;
    }

    if (simTime() > entry->endtime) {
        //this key doesn't exist anymore in the DHT, delete it in our hashtable

        globalDhtTestMap->eraseEntry(context->key);
        delete context;

        if (msg->getResultArraySize() > 0) {
            RECORD_STATS(numGetError++);
            //cout << "DHTTestApp: deleted key still available" << endl;
            return;
        } else {
            RECORD_STATS(numGetSuccess++);
            //cout << "DHTTestApp: success (1)" << endl;
            return;
        }
    } else {
        delete context;
        if ((msg->getResultArraySize() > 0) &&
                (msg->getResult(0).getValue() == entry->value)) {
            RECORD_STATS(numGetSuccess++);
            //cout << "DHTTestApp: success (2)" << endl;
            return;
        } else {
            RECORD_STATS(numGetError++);
#if 0
            if (msg->getResultArraySize()) {
                cout << "DHTTestApp: wrong value: " << msg->getResult(0).getValue() << endl;
            } else {
                cout << "DHTTestApp: no value" << endl;
            }
#endif
            return;
        }
    }

}

void DHTTestApp::handleTraceMessage(cMessage* msg)
{
    //EV << "[SOMA-DHTTestApp::handleTraceMessage()\n";

    char* cmd = new char[std::strlen(msg->getName()) + 1];
    std::strcpy(cmd, msg->getName());

    if (std::strlen(msg->getName()) < 5) {
        delete[] cmd;
        delete msg;
        return;
    }

    if (std::strncmp(cmd, "PUT ", 4) == 0) {
        // Generate key
        char* buf = cmd + 4;

        while (!isspace(buf[0])) {
            if (buf[0] == '\0')
                throw cRuntimeError("Error parsing PUT command");
            buf++;
        }

        buf[0] = '\0';
        BinaryValue b(cmd + 4);
        OverlayKey destKey(OverlayKey::sha1(b));

        // get value
        buf++;

        // build putMsg
        DHTputCAPICall* dhtPutMsg = new DHTputCAPICall();
        dhtPutMsg->setKey(destKey);
        dhtPutMsg->setValue(buf);
        dhtPutMsg->setTtl(ttl);
        dhtPutMsg->setIsModifiable(true);
        RECORD_STATS(numSent++; numPutSent++);
        sendInternalRpcCall(TIER1_COMP, dhtPutMsg,
                new DHTStatsContext(globalStatistics->isMeasuring(),
                                    simTime(), destKey, buf));
    } else if (std::strncmp(cmd, "GET ", 4) == 0) {
        // Get key
        BinaryValue b(cmd + 4);
        OverlayKey key(OverlayKey::sha1(b));

        DHTgetCAPICall* dhtGetMsg = new DHTgetCAPICall();
        dhtGetMsg->setKey(key);
        RECORD_STATS(numSent++; numGetSent++);
        sendInternalRpcCall(TIER1_COMP, dhtGetMsg,
                new DHTStatsContext(globalStatistics->isMeasuring(),
                                    simTime(), key));
    } else {
        throw cRuntimeError("Unknown trace command; "
                                "only GET and PUT are allowed");
    }

    delete[] cmd;
    delete msg;
}


void DHTTestApp::doTheCertRequest(const OverlayKey& key)
{
    sentReqsFlag++;
    DHTgetCAPICall* dhtGetMsg = new DHTgetCAPICall();
    dhtGetMsg->setKey(key);
    RECORD_STATS(numSent++; numGetSent++);
    if(debug) {
        std::ofstream outFile;
        outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
        outFile << "\n send request to : " << key << std::flush;
    }

    sendInternalRpcCall(TIER1_COMP, dhtGetMsg,
            new DHTStatsContext(globalStatistics->isMeasuring(),
                    simTime(), key));
}


bool DHTTestApp::existsInPendingReqsLvl1(const OverlayKey& key)
{

    bool nodeExists = false;

    if (!pendingReqs.empty()) {
        std::vector<TrustNode>::iterator it = pendingReqs.begin();
        for(; it != pendingReqs.end(); it++)
            if(it->nodeKey == key) {
                // Debugging
                if (debug){
                    std::ofstream outFile;
                    outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                    outFile << "\n node with key: " << it->nodeKey << " already exists in existsInPendingReqsLvl1" << std::flush;
                }
                nodeExists = true;
                break;
            }
    }
    return nodeExists;
}


OverlayKey DHTTestApp::getMyKey()
{
    string myIp = thisNode.getIp().str();
    string pkIp = string(publickey) + string(myIp);
    OverlayKey somaKey(OverlayKey::sha1(pkIp));
//    if (debug){
//        std::ofstream outFile;
//        outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
//        outFile << "\ngetMyKey: " << somaKey << std::flush;
//    }
    return somaKey;
}

void DHTTestApp::handleTimerEvent(cMessage* msg)
{
    if (msg->isName("somakey_put_timer")) {

        if (soma_fkeysigntime > -1 && soma_keyputtime >= soma_fkeysigntime)
        {
            soma_total_time = soma_keyputtime - soma_init_timer;
            timeVector.record(soma_total_time);
        }
        else if (soma_fkeysigntime > -1 && soma_keyputtime < soma_fkeysigntime)
        {
//            soma_totaTrustl_time = soma_fkeysigntime - soma_init_timer;
            timeVector.record(soma_total_time);
        }
    }
    else if (msg->isName("somafkeysign_timer")) {
        soma_fkeysigntime = simTime();

        if (soma_keyputtime > -1 && soma_keyputtime >= soma_fkeysigntime)
        {
            soma_total_time = soma_keyputtime - soma_init_timer;
            timeVector.record(soma_total_time);
        }
        else if (soma_keyputtime > -1 && soma_keyputtime < soma_fkeysigntime)
        {
            soma_total_time = soma_fkeysigntime - soma_init_timer;
            timeVector.record(soma_total_time);
        }
    }
//    else if (msg->isName("dhttest_put_timer")) {
//        //EV << "[SOMA-DHTTestApp::handleTimerEvent() : dhttest_put_timer\n";
//        // schedule next timer event
//        scheduleAt(simTime() + truncnormal(mean, deviation), msg);
//
//        // do nothing if the network is still in the initialization phase
//        if (((!activeNetwInitPhase) && (underlayConfigurator->isInInitPhase()))
//                || underlayConfigurator->isSimulationEndingSoon()
//                || nodeIsLeavingSoon)
//            return;
//
//        if (p2pnsTraffic) {
//            if (globalDhtTestMap->p2pnsNameCount < 4*globalNodeList->getNumNodes()) {
//                for (int i = 0; i < 4; i++) {
//                    // create a put test message with random destination key
//                    OverlayKey destKey = OverlayKey::random();
//                    DHTputCAPICall* dhtPutMsg = new DHTputCAPICall();
//                    dhtPutMsg->setKey(destKey);
//                    dhtPutMsg->setValue(generateRandomValue());
//                    dhtPutMsg->setTtl(ttl);
//                    dhtPutMsg->setIsModifiable(true);
//
//                    RECORD_STATS(numSent++; numPutSent++);
//                    sendInternalRpcCall(TIER1_COMP, dhtPutMsg,
//                            new DHTStatsContext(globalStatistics->isMeasuring(),
//                                                simTime(), destKey, dhtPutMsg->getValue()));
//                    globalDhtTestMap->p2pnsNameCount++;
//                }
//            }
//            cancelEvent(msg);
//            return;
//        }
//
//        // create a put test message with random destination key
//        OverlayKey destKey = OverlayKey::random();
//        DHTputCAPICall* dhtPutMsg = new DHTputCAPICall();
//        dhtPutMsg->setKey(destKey);
//        dhtPutMsg->setValue(generateRandomValue());
//        dhtPutMsg->setTtl(ttl);
//        dhtPutMsg->setIsModifiable(true);
//
//        RECORD_STATS(numSent++; numPutSent++);
//        sendInternalRpcCall(TIER1_COMP, dhtPutMsg,
//                new DHTStatsContext(globalStatistics->isMeasuring(),
//                                    simTime(), destKey, dhtPutMsg->getValue()));
//    }
    else if (msg->isName("dhttest_get_timer")) {
        // scheduleAt(simTime() + truncnormal(mean, deviation), msg);

        // do nothing if the network is still in the initialization phase
        if (((!activeNetwInitPhase) && (underlayConfigurator->isInInitPhase()))
                || underlayConfigurator->isSimulationEndingSoon()
                || nodeIsLeavingSoon) {
            scheduleAt(simTime() + GET_REQ_INTERVAL,  msg); // reschedule the selfmsg
            return;
        }
        // Debugging
        std::ofstream outFile;
        outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);

        /* Handle TC of levelOne */
        if (globalTrustLevel == LevelOne) {

            //-- SOMA send the request to other node
            if (sentReqsFlag < numSomaTrustReqs) {

                const OverlayKey& key = globalDhtTestMap->getRandomKey();

                if (key.isUnspecified()){
                    scheduleAt(simTime() + GET_REQ_INTERVAL,  msg); // reschedule the msg
                    return;
                }

                OverlayKey tKey = key;
                TrustNodeLvlOne tmpTrust;
                tmpTrust.isItTrusted = false;
                int trials = 0;

                while (accessedNodes.find(tKey) != accessedNodes.end() or tKey == getMyKey()){
                    // element already present, choose another random key
                    EV << "Key already present: " << tKey << endl;
                    const OverlayKey& key = globalDhtTestMap->getRandomKey();
                    tKey = key;
                    trials++;

                    // keep the trials check or you may loop forever
                    if (trials > 10){
                        scheduleAt(simTime() + GET_REQ_INTERVAL,  msg); // reschedule the msg
                        EV << "Reached maximum trials for getting a different key " << endl;
                        return;
                    }
                }

                accessedNodes.insert(std::make_pair(tKey, tmpTrust));
                if(debug){
                    std::ofstream outFile;
                    outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                    outFile << "\n  dumpAccessedNodes: " << std::flush;
                    std::map<OverlayKey, TrustNodeLvlOne>::iterator itAcc;
                    for(itAcc = accessedNodes.begin(); itAcc != accessedNodes.end(); itAcc++){
                        outFile << "\n  key:  " << itAcc->first << std::flush;
                    }
                }
                std::map<OverlayKey, TrustNodeLvlOne>::iterator it;
                it = accessedNodes.find(tKey);

                if (it != accessedNodes.end()){
                    it->second.timestmpSend = simTime();
                    EV << "timestmpSend: " << it->second.timestmpSend << endl;
                }

//                EV << "Node: " << thisNode.getIp() << " is going to send request to node with SOMA key : " << key << endl;
//                std::ofstream outFile;
//                outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
//
//                outFile << "Node: " << thisNode.getIp() << " is going to send request to node with SOMA key : " << key << std::flush;

                if (key.isUnspecified()) {
                    EV << "[DHTTestApp::handleTimerEvent() @ " << thisNode.getIp()
                                           << " (" << thisNode.getKey().toString(16) << ")]\n"
                                           << "    Error: No key available in global DHT test map!"
                                           << endl;
                    return;
                }

                pendReqsNum++;

                doTheCertRequest(key);
                scheduleAt(simTime() + GET_REQ_INTERVAL,  msg); // reschedule the msg

            }
        }
        //--
        else {
            /*
             * If pendingReqs is not empty and there is node in pendingChildNodes that has not been Requested then proceed with the request of this node.
             * If all nodes of pendingChildNodes have been requested or pendingReqs is empty then get a new key(does not exists in accessedNodes neither in pendingReqs) and proceed with the Request.
             * Note: since the node exists in pendingReqs, has not been decided if is trusted or not
             * */
            //-- SOMA send the request to other node
            outFile << "\nnode: " << thisNode.getIp() << " time_to REQ"  << std::flush;

            if (!pendingReqs.empty()){
                std::vector<TrustNode>::iterator it = pendingReqs.begin();
                outFile << "\n pendingReqs not empty" << std::flush;
                // Parse the child entries of level 1 till you find a ChildNode which has not been requested
                for (; it != pendingReqs.end(); it++) {

                    outFile << "\n it->node search: " << it->nodeKey << std::flush;
                    if(!it->pendingChildNodes.empty()) {

                        // Search for a child node for which we have not made a request
                        std::vector<childNodeInfo>::iterator itCh = it->pendingChildNodes.begin();
                        for (; itCh != it->pendingChildNodes.end(); itCh++){

                            outFile << "\n Child node to find in pendingChildNodes: " << itCh->nodeKey << std::flush;
                            if (!itCh->sentReq) {
                                // Found node that has not been requested
                                outFile << "\n " << itCh->nodeKey << " has not sent request for, search in the keyRing "<< std::flush;

                                // Search if already exist in my KeyRing
                                std::map<OverlayKey, TrustNodeLvlOne>::iterator itAcc;
                                itAcc = accessedNodes.find(itCh->nodeKey);
                                if (itAcc != accessedNodes.end()) {
                                    // Node exists in my KeyRing (trusted or not); don't send the request to the network

                                    outFile << "\n Child node : " << itCh->nodeKey << " found in the keyRing" << std::flush;

                                    TrustNodeLvlOne trust;
                                    trust.foundTrustAtLevel = itCh->level;
                                    trust.isItTrusted = itAcc->second.isItTrusted; // true/false
                                    trust.timestmpSend = it->timestmpSend;
                                    trust.timestmpRcv = simTime();
                                    trust.rtt = simTime() - it->timestmpSend + certVerficationDelay; // no request is made

                                    // Check if it is the last node in the childPendingNodes. if yes, decide the Trust of the level1 node and put it in the accessedNodes
                                    if (itAcc->second.isItTrusted) {
                                        // The level1 node is trusted so i trust this level1 father node (it->nodeKey)
                                        // Put it in the accessedNodes and remove the entry from pendingReqs
                                        outFile << "\n Child node : " << itCh->nodeKey << " is Trusted from the KeyRing" << std::flush;
                                        accessedNodes.insert(std::make_pair(it->nodeKey, trust));   // store the node in the accessedNodes
                                        outFile << "\nSuccess found with delay: " << trust.rtt << std::flush;
                                        successDelay += trust.rtt;

                                        if(debug){
                                            std::ofstream outFile;
                                            outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                                            outFile << "\n  dumpAccessedNodes: " << std::flush;
                                            std::map<OverlayKey, TrustNodeLvlOne>::iterator itAcc;
                                            for(itAcc = accessedNodes.begin(); itAcc != accessedNodes.end(); itAcc++){
                                                outFile << "\n  key:  " << itAcc->first << std::flush;
                                            }
                                        }
                                        pendingReqs.erase(it);  // remove it from the pendingReqs since we found a trust
                                    }
                                    else{
                                        outFile << "\n Child node : " << itCh->nodeKey << " is not Trusted" << std::flush;
                                        // the level1 node is not trusted
                                        itCh->sentReq = true; // virtually
                                        itCh->isItTrusted = false;
                                        itCh->responseRcved = true;
                                        // check if this is the last one node in the pendingChild nodes , all the other nodes have received a Response and this is the last one
                                        std::vector<childNodeInfo>::iterator itCh2 = it->pendingChildNodes.begin();
                                        bool lastOne = false;
                                        if(sentReqsFlag >= numSomaTrustReqs){
                                            lastOne = true;
                                            for (; itCh2 != it->pendingChildNodes.end(); itCh2++) {
                                                if (!itCh2->responseRcved) {
                                                    lastOne = false;
                                                }
                                            }
                                        }
                                        if(lastOne) {
                                            outFile << "\n was the last node, father it->nodeKey: " << it->nodeKey << std::flush;
                                            accessedNodes.insert(std::make_pair(it->nodeKey, trust));   // store the node in the accessedNodes
                                            if(debug){
                                                outFile << "\n  dumpAccessedNodes: " << std::flush;
                                                std::map<OverlayKey, TrustNodeLvlOne>::iterator itAcc;
                                                for(itAcc = accessedNodes.begin(); itAcc != accessedNodes.end(); itAcc++){
                                                    outFile << "\n  key:  " << itAcc->first << std::flush;
                                                }
                                            }
                                            pendingReqs.erase(it);  // remove it from the pendingReqs since we found a trust
                                        }
                                    }
                                    scheduleAt(simTime(),  msg); // reschedule the msg with no retard since we have not make any request
                                }
                                else{
                                    // Node does not exist in the keyRing; proceed with the Request in the network
                                    outFile << "\n " << itCh->nodeKey << " does not exist in the keyRing; proceed with the Request in the network "<< std::flush;
                                    if (itCh->nodeKey != getMyKey()){
                                        itCh->sentReq = true;
                                        itCh->responseRcved = false;
                                        doTheCertRequest(itCh->nodeKey);
                                    }
                                    scheduleAt(simTime() + GET_REQ_INTERVAL,  msg); // reschedule the msg
                                }
                                return; // remove the return if you want to make the requests without scheduled event
                            }
                        }
                    }
                }
            }
            // pendingReqs do not exist or all the nodes in pendingChildNodes have been requested, (itCh->sentReq) is true
            if (sentReqsFlag <= numSomaTrustReqs) {
                const OverlayKey& key = globalDhtTestMap->getRandomKey();

                if (key.isUnspecified()){
                    scheduleAt(simTime() + GET_REQ_INTERVAL,  msg); // reschedule the msg
                    return;
                }

                OverlayKey tKey = key;
                TrustNode tmpTrust;
                tmpTrust.isItTrusted = false;
                int trials = 0;

                while ((accessedNodes.find(tKey) != accessedNodes.end()) or
                        existsInPendingReqsLvl1(tKey) or tKey == getMyKey()){
                    // this key has already been accessed or it's thisNode, choose another random key
                    //   EV << "Key already present: " << tKey << endl;
                    const OverlayKey& key = globalDhtTestMap->getRandomKey();
                    tKey = key;
                    trials++;

                    // keep the trials check or you may loop forever
                    if (trials > 10){
                        scheduleAt(simTime() + GET_REQ_INTERVAL,  msg); // reschedule the msg
                        EV << "Reached maximum trials for getting a different key " << endl;
                        return;
                    }
                }

                // insert the new node in the PendingReqs
                TrustNode tN;
                tN.isItTrusted = false;
                tN.nodeKey = tKey;
                tN.timestmpSend = simTime();
                tN.timestmpRcv = 0;
                tN.rtt = 0;
                tN.trustAtLevel = 0;

                pendingReqs.push_back(tN);
                if (debug){
                    std::ofstream outFile;
                    outFile.open("/home/xubuntu/sim/OverSim/simulations/results/logs.txt", std::ios_base::app);
                    outFile << "\n " << thisNode.getIp() << " **Insert node in pendingReqs: " << tN.nodeKey << std::flush;
                }
                pendReqsNum++;
                // do the Request
                doTheCertRequest(tN.nodeKey);
                scheduleAt(simTime() + GET_REQ_INTERVAL,  msg); // reschedule the msg
            }
        }
    } else if (msg->isName("dhttest_mod_timer")) {
        scheduleAt(simTime() + truncnormal(mean, deviation), msg);

        // do nothing if the network is still in the initialization phase
        if (((!activeNetwInitPhase) && (underlayConfigurator->isInInitPhase()))
                || underlayConfigurator->isSimulationEndingSoon()
                || nodeIsLeavingSoon) {
            return;
        }

        if (p2pnsTraffic) {
            if (globalDhtTestMap->p2pnsNameCount >= 4*globalNodeList->getNumNodes()) {
                const OverlayKey& key = globalDhtTestMap->getRandomKey();

                if (key.isUnspecified())
                    return;

                DHTputCAPICall* dhtPutMsg = new DHTputCAPICall();
                dhtPutMsg->setKey(key);
                dhtPutMsg->setValue(generateRandomValue());
                dhtPutMsg->setTtl(ttl);
                dhtPutMsg->setIsModifiable(true);

                RECORD_STATS(numSent++; numPutSent++);
                sendInternalRpcCall(TIER1_COMP, dhtPutMsg,
                        new DHTStatsContext(globalStatistics->isMeasuring(),
                                            simTime(), key, dhtPutMsg->getValue()));
            }
            cancelEvent(msg);
            return;
        }

        const OverlayKey& key = globalDhtTestMap->getRandomKey();

        if (key.isUnspecified())
            return;
#if 0
        const DHTEntry* entry = globalDhtTestMap->findEntry(key);
        if (entry->insertiontime + 10.0 > simTime()) {
            std::cout << "avoided early get" << std::endl;
            return;
        }
#endif
        //EV << "[SOMA-DHTTestApp::handleTimerEvent() : dhttest_mod_timer\n";

        DHTputCAPICall* dhtPutMsg = new DHTputCAPICall();
        dhtPutMsg->setKey(key);
        dhtPutMsg->setValue(generateRandomValue());
        dhtPutMsg->setTtl(ttl);
        dhtPutMsg->setIsModifiable(true);

        RECORD_STATS(numSent++; numPutSent++);
        sendInternalRpcCall(TIER1_COMP, dhtPutMsg,
                new DHTStatsContext(globalStatistics->isMeasuring(),
                                    simTime(), key, dhtPutMsg->getValue()));
    }
}


BinaryValue DHTTestApp::generateRandomValue()
{
    char value[DHTTESTAPP_VALUE_LEN + 1];

    for (int i = 0; i < DHTTESTAPP_VALUE_LEN; i++) {
        value[i] = intuniform(0, 25) + 'a';
    }

    value[DHTTESTAPP_VALUE_LEN] = '\0';
    return BinaryValue(value);
}

void DHTTestApp::handleNodeLeaveNotification()
{
    nodeIsLeavingSoon = true;
}

void DHTTestApp::finishApp()
{
    simtime_t time = globalStatistics->calcMeasuredLifetime(creationTime);
    //recordScalar("SOMA keys", numSomaKeys);
    if (time >= GlobalStatistics::MIN_MEASURED) {
        // record scalar data
        globalStatistics->addStdDev("DHTTestApp: Sent Total Messages/s",
                                    numSent / time);
        globalStatistics->addStdDev("DHTTestApp: Sent GET Messages/s",
                                    numGetSent / time);
        globalStatistics->addStdDev("DHTTestApp: Failed GET Requests/s",
                                    numGetError / time);
        globalStatistics->addStdDev("DHTTestApp: Successful GET Requests/s",
                                    numGetSuccess / time);

        globalStatistics->addStdDev("DHTTestApp: Sent PUT Messages/s",
                                    numPutSent / time);
        globalStatistics->addStdDev("DHTTestApp: Failed PUT Requests/s",
                                    numPutError / time);
        globalStatistics->addStdDev("DHTTestApp: Successful PUT Requests/s",
                                    numPutSuccess / time);


 //       globalStatistics->addStdDev("DHTTestApp: Total SOMA keys",
  //                                  numSomaKeys);


        if ((numGetSuccess + numGetError) > 0) {
            globalStatistics->addStdDev("DHTTestApp: GET Success Ratio",
                                        (double) numGetSuccess
                                        / (double) (numGetSuccess + numGetError));
        }
        std::ofstream outFile;
        outFile.open("/home/xubuntu/sim/OverSim/simulations/results/experiments/stats.txt", std::ios_base::app);
        outFile << "\n\nNode: " << thisNode.getIp()  << " started Level 1 Requests for " << pendReqsNum << " nodes";
        outFile << "\nTotal Requests: " << sentReqsFlag;
        // TrustChain Results
        if (accessedNodes.size() > 0){

            std::map<OverlayKey, TrustNodeLvlOne>::iterator it;

            //EV << "\nNode: " << thisNode.getIp() << " requested certs from " << accessedNodes.size() << " nodes.\nThe node's keys and trust result are: " <<     endl;
            outFile << "\nThe node's accessed keys and trust result are: " << endl;

            int cnter = 0;
            for(it = accessedNodes.begin(); it != accessedNodes.end(); it++){
                if(it->second.isItTrusted){
                    outFile << "--key: " << it->first << " - Trust" << endl;
                    outFile << "--SimTime Delay for response: " << it->second.rtt << endl;
                    if (globalTrustLevel > LevelOne)
                        outFile << "-Found Trust at Level: " << it->second.foundTrustAtLevel << endl;
                    cnter++;
                }
//                else{
//                    outFile << "--key: " << it->first << " - no Trust" << endl;
//                }
            }

            double trustPercentOverRequests = ((double)cnter / pendReqsNum) *100;
            outFile << "trust Percent Over Level1 Requests: " << trustPercentOverRequests << "%" << endl;

        }
        else {
            outFile << "\n--Found no Trust \ntrust Percent Over Level1 Requests: 0%" << endl;
        }

        outFile.close();

        // write total requests in the network of each node
        outFile.open("/home/xubuntu/sim/OverSim/simulations/results/experiments/totalReqs.txt", std::ios_base::app);
        outFile  <<thisNode.getIp()<< ":" << sentReqsFlag << endl;
        outFile.close();

        // write successful Trust delay of each node
        outFile.open("/home/xubuntu/sim/OverSim/simulations/results/experiments/successDelay.txt", std::ios_base::app);
        outFile  <<thisNode.getIp()<< ":" << successDelay << endl;
        outFile.close();

        // write total Chain Length upon trust for each node
        if (accessedNodes.size() > 0) {

            outFile.open("/home/xubuntu/sim/OverSim/simulations/results/experiments/chainLength.txt", std::ios_base::app);

            std::map<OverlayKey, TrustNodeLvlOne>::iterator it;

            int total_ch_len = 0;
            bool foundTrust = false;
            int success_trust_cnter = 0;
            for (it = accessedNodes.begin(); it != accessedNodes.end(); it++) {
                if (it->second.isItTrusted) {
                    foundTrust = true;
                    total_ch_len = total_ch_len  + it->second.foundTrustAtLevel;
                    success_trust_cnter += 1;
                }
            }
            if (foundTrust){
                outFile  << thisNode.getIp() << ":" << total_ch_len << endl;
                outFile.close();

                // use the result to count the successfull trusts
                outFile.open("/home/xubuntu/sim/OverSim/simulations/results/experiments/successfulTrusts.txt", std::ios_base::app);
                outFile  << thisNode.getIp() << ":" << success_trust_cnter << endl;
                outFile.close();
            }
        }
    }
}

