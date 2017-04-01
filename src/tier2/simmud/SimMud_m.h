//
// Generated file, do not edit! Created by opp_msgc 4.2 from tier2/simmud/SimMud.msg.
//

#ifndef _SIMMUD_M_H_
#define _SIMMUD_M_H_

#include <omnetpp.h>

// opp_msgc version check
#define MSGC_VERSION 0x0402
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of opp_msgc: 'make clean' should help.
#endif

// cplusplus {{
#include <NodeHandle.h>
#include <CommonMessages_m.h>

#define SIMMUD_MOVE_L(msg) ( NODEHANDLE_L + 2*sizeof(double) + 8 )
// }}



/**
 * Class generated from <tt>tier2/simmud/SimMud.msg</tt> by opp_msgc.
 * <pre>
 * packet SimMudMoveMessage
 * {
 *         NodeHandle src;
 *         double posX;
 *         double posY;
 *         simtime_t timestamp;
 *         bool leaveRegion = false;
 * }
 * </pre>
 */
class SimMudMoveMessage : public ::cPacket
{
  protected:
    NodeHandle src_var;
    double posX_var;
    double posY_var;
    simtime_t timestamp_var;
    bool leaveRegion_var;

  private:
    void copy(const SimMudMoveMessage& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const SimMudMoveMessage&);

  public:
    SimMudMoveMessage(const char *name=NULL, int kind=0);
    SimMudMoveMessage(const SimMudMoveMessage& other);
    virtual ~SimMudMoveMessage();
    SimMudMoveMessage& operator=(const SimMudMoveMessage& other);
    virtual SimMudMoveMessage *dup() const {return new SimMudMoveMessage(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual NodeHandle& getSrc();
    virtual const NodeHandle& getSrc() const {return const_cast<SimMudMoveMessage*>(this)->getSrc();}
    virtual void setSrc(const NodeHandle& src);
    virtual double getPosX() const;
    virtual void setPosX(double posX);
    virtual double getPosY() const;
    virtual void setPosY(double posY);
    virtual simtime_t getTimestamp() const;
    virtual void setTimestamp(simtime_t timestamp);
    virtual bool getLeaveRegion() const;
    virtual void setLeaveRegion(bool leaveRegion);
};

inline void doPacking(cCommBuffer *b, SimMudMoveMessage& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, SimMudMoveMessage& obj) {obj.parsimUnpack(b);}


#endif // _SIMMUD_M_H_
