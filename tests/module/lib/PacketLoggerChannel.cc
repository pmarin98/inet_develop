//
// Copyright (C) 2013 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include <fstream>

#include "inet/common/INETDefs.h"

#include "inet/linklayer/ethernet/common/EthernetMacHeader_m.h"
#include "inet/linklayer/common/MacAddress.h"

namespace inet {

class INET_API PacketLoggerChannel : public cDatarateChannel
{
  protected:
    long int counter;
    std::ofstream logfile;

  public:
    explicit PacketLoggerChannel(const char *name = NULL) : cDatarateChannel(name) { counter = 0; }

#if OMNETPP_BUILDNUM < 1504    // OMNETPP_VERSION is 6.0 pre9
    virtual void processMessage(cMessage *msg, simtime_t t, result_t& result) override;
#else
    virtual cChannel::Result processMessage(cMessage *msg, const SendOptions& options, simtime_t t) override;
#endif

  protected:
    virtual void initialize() override;
    void finish() override;
};


Register_Class(PacketLoggerChannel);

void PacketLoggerChannel::initialize()
{
    EV << "PacketLogger initialize()\n";
    cDatarateChannel::initialize();
    const char *logfilename = par("logfile");
    if (*logfilename)
    {
        logfile.open(logfilename, std::ios::out | std::ios::trunc);
        if (!logfile.is_open())
            throw cRuntimeError("logfile '%s' open failed", logfilename);
    }
    counter = 0;
}

#if OMNETPP_BUILDNUM < 1504    // OMNETPP_VERSION is 6.0 pre9
void PacketLoggerChannel::processMessage(cMessage *msg, simtime_t t, result_t& result)
#else
cChannel::Result PacketLoggerChannel::processMessage(cMessage *msg, const SendOptions& options, simtime_t t)
#endif
{
    EV << "PacketLogger processMessage()\n";
#if OMNETPP_BUILDNUM < 1504    // OMNETPP_VERSION is 6.0 pre9
    cDatarateChannel::processMessage(msg, t, result);
#else
    cChannel::Result result = cDatarateChannel::processMessage(msg, options, t);
#endif

    const char *status = "";
#if OMNETPP_BUILDNUM < 1504    // OMNETPP_VERSION is 6.0 pre9
    counter++;
#else
    if (options.transmissionId_ == -1) {
        counter++;
    }
    else if (!options.isUpdate) {
        status = ":start";
        counter++;
    }
    else {
        status = (options.remainingDuration == SIMTIME_ZERO) ? ":end" : ":update";
    }

#endif
    if (logfile.is_open())
    {
        logfile << '#' << counter << ':' << t.raw() << ": '" << msg->getName() << status << "' (" << msg->getClassName() << ") sent:" << msg->getSendingTime().raw();
        cPacket* pk = dynamic_cast<cPacket *>(msg);
        if (pk)
            logfile << " (" << pk->getByteLength() << " byte)";
        logfile << " discard:" << result.discard << ", delay:" << result.delay.raw() << ", duration:" << result.duration.raw();
        logfile << endl;
    }
#if OMNETPP_BUILDNUM >= 1504    // OMNETPP_VERSION is 6.0 pre9
    return result;
#endif
}

void PacketLoggerChannel::finish()
{
    EV << "PacketLogger finish()\n";
    logfile.close();
}

} // namespace inet

