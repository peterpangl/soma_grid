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
 * @file DHTTestApp.h
 * @author Ingmar Baumgart
 */

#ifndef __DHTTESTAPP_H_
#define __DHTTESTAPP_H_

#include <omnetpp.h>

#include <GlobalNodeList.h>
#include <GlobalStatistics.h>
#include <UnderlayConfigurator.h>
#include <TransportAddress.h>
#include <OverlayKey.h>
#include <InitStages.h>
#include <BinaryValue.h>
#include <BaseApp.h>
#include <set>
#include <sstream>
#include <vector>
#include <boost/regex.hpp>

class GlobalDhtTestMap;

#define LevelOne 1

struct childNodeInfo
{
    OverlayKey nodeKey;
    bool sentReq;  // true/false if we have/haven't sent a request
    OverlayKey dueNode; // the "father" node for which is being searched the Trust
    bool isItTrusted;
    bool responseRcved;
    int level;

};

struct TrustNode
{
    OverlayKey nodeKey;
    bool isItTrusted;
    int trustAtLevel;
    int numOfChecks;
    simtime_t timestmpSend;
    simtime_t timestmpRcv;
    simtime_t rtt;
    std::vector<childNodeInfo> pendingChildNodes;
    TrustNode() : numOfChecks(0) {}
};

struct TrustNodeLvlOne
{
    bool isItTrusted;
    int foundTrustAtLevel;
    int numOfChecks;
    simtime_t timestmpSend;
    simtime_t timestmpRcv;
    simtime_t rtt;
    TrustNodeLvlOne() : numOfChecks(0) {}
};

/**
 * A simple test application for the DHT layer
 *
 * A simple test application that does random put and get calls
 * on the DHT layer
 *
 * @author Ingmar Baumgart
 */
class DHTTestApp : public BaseApp
{
private:
    /**
     * A container used by the DHTTestApp to
     * store context information for statistics
     *
     * @author Ingmar Baumgart
     */
    class DHTStatsContext : public cPolymorphic
    {
    public:
        bool measurementPhase;
        simtime_t requestTime;
        OverlayKey key;
        BinaryValue value;

        DHTStatsContext(bool measurementPhase,
                        simtime_t requestTime,
                        const OverlayKey& key,
                        const BinaryValue& value = BinaryValue::UNSPECIFIED_VALUE) :
            measurementPhase(measurementPhase), requestTime(requestTime),
            key(key), value(value) {};
    };

    void initializeApp(int stage);

    /**
     * Get a random key of the hashmap
     */
    OverlayKey getRandomKey();

    /**
     * generate a random human readable binary value
     */
    BinaryValue generateRandomValue();

    //-- SOMA related function
    /**
     * Check a structure if the argument k has already been used
     */
//    bool keyAlreadyUsed(const OverlayKey& k);
    //--

    bool handleRpcCall(BaseCallMessage* msg);

    void finishApp();

    void handleGetResponsibleResponse(DHTgetResponsibleResponse* msg);//,
//                                       DHTStatsContext* context);


    void handlePutCall(BaseCallMessage* msg);
    void handleDHTKeyPutCall(DHTKeyPutCall* msg);
    void handleDHTreturnSignedCert(DHTreturnSignedCertCall* msg);
    /**
     * processes get responses
     *
     * method to handle get responses
     * should be overwritten in derived application if needed
     * @param msg get response message
     * @param context context object used for collecting statistics
     */
    virtual void handleGetResponse(DHTgetCAPIResponse* msg,
                                   DHTStatsContext* context);

    /**
     * processes put responses
     *
     * method to handle put responses
     * should be overwritten in derived application if needed
     * @param msg put response message
     * @param context context object used for collecting statistics
     */
    virtual void handlePutResponse(DHTputCAPIResponse* msg,
                                   DHTStatsContext* context);

    /**
     * processes self-messages
     *
     * method to handle self-messages
     * should be overwritten in derived application if needed
     * @param msg self-message
     */
    virtual void handleTimerEvent(cMessage* msg);

    /**
     * handleTraceMessage gets called of handleMessage(cMessage* msg)
     * if a message arrives at trace_in. The command included in this
     * message should be parsed and handled.
     *
     * @param msg the command message to handle
     */
    virtual void handleTraceMessage(cMessage* msg);

    virtual void handleNodeLeaveNotification();

    // see RpcListener.h
    void handleRpcResponse(BaseResponseMessage* msg, const RpcState& state,
                           simtime_t rtt);

    void doTheCertRequest(const OverlayKey& key);

    UnderlayConfigurator* underlayConfigurator; /**< pointer to UnderlayConfigurator in this node */

    GlobalNodeList* globalNodeList; /**< pointer to GlobalNodeList in this node*/

    GlobalStatistics* globalStatistics; /**< pointer to GlobalStatistics module in this node*/
    GlobalDhtTestMap* globalDhtTestMap; /**< pointer to the GlobalDhtTestMap module */

    char *publickey, *cert, *signTemplate;
    OverlayKey somaKey;

    // parameters
    bool debugOutput; /**< debug output yes/no?*/
    double mean; //!< mean time interval between sending test messages
    double deviation; //!< deviation of time interval
    int ttl; /**< ttl for stored DHT records */
    bool p2pnsTraffic; //!< model p2pns application traffic */
    bool activeNetwInitPhase; //!< is app active in network init phase?

    // statistics
    int numSent; /**< number of sent packets*/
    int numGetSent; /**< number of get sent*/
    int numGetError; /**< number of false get responses*/
    int numGetSuccess; /**< number of false get responses*/
    int numPutSent; /**< number of put sent*/
    int numPutError; /**< number of error in put responses*/
    int numPutSuccess; /**< number of success in put responses*/

    int numSomaKeys; /**< number of SOMA keys signed*/
    int numSomaTrustReqs; /**< number of requests for signed certifications from other nodes in order to build the trust chain */
    int globalTrustLevel;
    int sentNumChecks; /**<number of total sent checks*/
    int respNumChecks; /**<number of total response checks*/
    int numRequests;  /**<number of total request checks*/
    bool gotDhtStoredKeys;
    bool extensiveSearch; /**flag, if false getRandomKey will be used to retrieve a key and make a requests
                            * if true, retrieve all the stored keys in the DHT and make request one by one
                            * set it manual to T if **.tier2*.dhtTestApp.somaTrustReqs is very close to the number of nodes
                            **/
    int numOfNodes;

    double certVerficationDelay;
    simtime_t successDelay;

    std::map<OverlayKey, TrustNodeLvlOne> accessedNodes;
    std::vector<childNodeInfo> pendingChildNodes; /**< this vector is associated with every level1 node for which we have started the process of finding trust. Will keep the "child" nodes for the node of level1 */
    std::vector<TrustNode> pendingReqs;  /**< this vector will keep the requests that are made till we find trust regarding the globalTrustLevel */
    std::vector<OverlayKey> DHTStoredKeys;
    std::vector<OverlayKey>::iterator itDHTStoredKeys;


    void dumpAccessedNodes();
    bool haveSignedOtherNodeCert(std::string, std::string reqstdNodeSCert);
    bool existsInPendingReqsLvl1(const OverlayKey& key);
    std::list<OverlayKey> convertIPsToKeys(std::string s);
    void insertChildNodes(std::vector<TrustNode>::iterator it, std::list<OverlayKey> keys, int level);
    OverlayKey getMyKey();

    simtime_t soma_init_timer, soma_total_time;
    simtime_t soma_keyputtime, soma_fkeysigntime;
    //simtime_t soma_findNodeTimer;
    cOutVector timeVector;

    cMessage *dhttestput_timer, *dhttestget_timer, *dhttestmod_timer, *somakeyput_msg, *somafkeysign_msg;
    bool nodeIsLeavingSoon; //!< true if the node is going to be killed shortly

    static const int DHTTESTAPP_VALUE_LEN = 20;
    static const int GET_REQ_INTERVAL = 1;

    // our timer
    cMessage *timerMsg;
    int i;
    bool debug;


public:
    DHTTestApp();

    /**
     * virtual destructor
     */
    virtual ~DHTTestApp();

};

#endif
