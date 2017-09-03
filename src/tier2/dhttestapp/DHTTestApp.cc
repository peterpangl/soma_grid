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

    numSomaKeys = 0;
    sentReqsFlag = 0;
    soma_init_timer  = simTime();
    soma_total_time  = 0;
    soma_keyputtime = -1;
    soma_fkeysigntime = -1;
    //soma_findNodeTimer = par("sendTCPeriod");

    timeVector.setName("SomaJoinTime");

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

    EV << nodeIp << " -> s omakey : " << somaKey << endl;

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

    std::map<OverlayKey, Trust>::iterator it;

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


// Retrieve the signed certificate value
void DHTTestApp::handleDHTreturnSignedCert(DHTreturnSignedCertCall* msg)
{
    EV << "DHTTestApp::handleDHTreturnSignedCert myIP:" << thisNode.getIp() << endl;
    EV << "SOMASigned Cert Value: "  << msg->getSignedCert() <<
            "\nother node's key: " << msg->getNodeKey() << endl;

    dumpAccessedNodes();
    string myIp = thisNode.getIp().str();
    string reqstdNodeSCert = msg->getSignedCert();
    OverlayKey reqstdNodeKey = msg->getNodeKey();
    std::map<OverlayKey, Trust>::iterator it;

    it = accessedNodes.find(reqstdNodeKey);
    if(it != accessedNodes.end()) {
        //key found in accessedNodes
        bool haveSignedCert = haveSignedOtherNodeCert(myIp, reqstdNodeSCert);
        if(haveSignedCert){
            // update the Trust for that node
            it->second.isItTrusted = true;
            EV << "Have signed it, trust it" << endl;
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

    globalDhtTestMap->dumpDHTTestMap();

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

    char* cmd = new char[strlen(msg->getName()) + 1];
    strcpy(cmd, msg->getName());

    if (strlen(msg->getName()) < 5) {
        delete[] cmd;
        delete msg;
        return;
    }

    if (strncmp(cmd, "PUT ", 4) == 0) {
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
    } else if (strncmp(cmd, "GET ", 4) == 0) {
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
            soma_total_time = soma_fkeysigntime - soma_init_timer;
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
        EV << "get_timer 1 node:" << thisNode.getIp() << endl;
        // do nothing if the network is still in the initialization phase
        if (((!activeNetwInitPhase) && (underlayConfigurator->isInInitPhase()))
                || underlayConfigurator->isSimulationEndingSoon()
                || nodeIsLeavingSoon) {
            scheduleAt(simTime() + 4,  msg); // reschedule the selfmsg
            return;
        }

        //-- SOMA send the request to other node
        if (sentReqsFlag < numSomaTrustReqs) {

            const OverlayKey& key = globalDhtTestMap->getRandomKey();

            OverlayKey tKey = key;
            Trust tmpTrust;
            tmpTrust.isItTrusted = false;
            int trials = 0;
            while( !accessedNodes.insert(std::make_pair(tKey, tmpTrust)).second ){
                // element already present, choose another random key
                EV << "Key already present: " << tKey << endl;
                const OverlayKey& key = globalDhtTestMap->getRandomKey();
                OverlayKey tKey = key;
                trials++;

                // don't loop forever
                if (trials > 10){
                    scheduleAt(simTime() + 4,  msg); // reschedule the msg
                    EV << "Reached maximum trials for getting a different key " << endl;
                    return;
                }
            }

            EV << "Node: " << thisNode.getIp() << " is going to send request to node with SOMA key : " << key << endl;

            if (key.isUnspecified()) {
                EV << "[DHTTestApp::handleTimerEvent() @ " << thisNode.getIp()
                                   << " (" << thisNode.getKey().toString(16) << ")]\n"
                                   << "    Error: No key available in global DHT test map!"
                                   << endl;
                return;
            }
            sentReqsFlag++;

            DHTgetCAPICall* dhtGetMsg = new DHTgetCAPICall();
            dhtGetMsg->setKey(key);
            RECORD_STATS(numSent++; numGetSent++);

            sendInternalRpcCall(TIER1_COMP, dhtGetMsg,
                    new DHTStatsContext(globalStatistics->isMeasuring(),
                            simTime(), key));

            scheduleAt(simTime() + 4,  msg); // reschedule the msg

        }
        //--
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

        // TrustChain Results
        if (accessedNodes.size() > 0){
            std::map<OverlayKey, Trust>::iterator it;
            EV << "\nNode: " << thisNode.getIp() << " requested certs from " << accessedNodes.size() << " nodes.\nThe node's keys and trust result are: " <<     endl;
            int cnter = 0;
            for(it = accessedNodes.begin(); it != accessedNodes.end(); it++){
                if(it->second.isItTrusted){
                    EV << "key: " << it->first << " - Trust" << endl;
                    cnter++;
                }
                else{
                    EV << "key: " << it->first << " - no Trust" << endl;
                }
            }
            double trustPercentOverRequests = ((double)cnter / accessedNodes.size()) *100;
            EV << "trustPercentOverRequests: " << trustPercentOverRequests << "%" << endl;
        }
    }
}

