//
// Copyright (C) 2004 OpenSim Ltd.
// Copyright (C) 2009-2010 Thomas Reschka
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#ifndef __INET_TCPBASEALG_H
#define __INET_TCPBASEALG_H

#include "inet/transportlayer/tcp/TcpAlgorithm.h"

namespace inet {
namespace tcp {

/**
 * State variables for TcpBaseAlg.
 */
class INET_API TcpBaseAlgStateVariables : public TcpStateVariables
{
  public:
    TcpBaseAlgStateVariables();
    virtual std::string str() const override;
    virtual std::string detailedInfo() const override;

    /// retransmit count
    //@{
    int rexmit_count; ///< number of retransmissions (=1 after first rexmit)
    simtime_t rexmit_timeout; ///< current retransmission timeout (aka RTO)
    //@}

    /// persist factor
    //@{
    uint persist_factor; ///< factor needed for simplified PERSIST timer calculation
    simtime_t persist_timeout; ///< current persist timeout
    //@}

    /// congestion window
    //@{
    uint32_t snd_cwnd; ///< congestion window
    //@}

    /// round-trip time measurements
    //@{
    uint32_t rtseq; ///< starting sequence number of timed data
    simtime_t rtseq_sendtime; ///< time when rtseq was sent (0 if RTT measurement is not running)
    //@}

    /// round-trip time estimation (Jacobson's algorithm)
    //@{
    simtime_t srtt; ///< smoothed round-trip time
    simtime_t rttvar; ///< variance of round-trip time
    //@}

    /// number of RTOs
    //@{
    uint32_t numRtos; ///< total number of RTOs
    //@}

    /// RFC 3782 variables
    //@{
    uint32_t recover; ///< recover (RFC 3782)
    bool firstPartialACK; ///< first partial acknowledgement (RFC 3782)
    //@}
};

/**
 * Includes basic TCP algorithms: adaptive retransmission, PERSIST timer,
 * keep-alive, delayed acks -- EXCLUDING congestion control. Congestion
 * control is implemented in subclasses such as TCPTahoeAlg or TCPRenoAlg.
 *
 * Implements:
 *   - delayed ACK algorithm (RFC 1122)
 *   - Jacobson's and Karn's algorithms for adaptive retransmission
 *   - Nagle's algorithm (RFC 896) to prevent silly window syndrome
 *   - Increased Initial Window (RFC 3390)
 *   - PERSIST timer
 *
 * To be done:
 *   - KEEP-ALIVE timer
 *
 * Note: currently the timers and time calculations are done in double
 * and NOT in Unix (200ms or 500ms) ticks. It's possible to write another
 * TcpAlgorithm which uses ticks (or rather, factor out timer handling to
 * separate methods, and redefine only those).
 *
 * Congestion window is set to SMSS when the connection is established,
 * and not touched after that. Subclasses may redefine any of the virtual
 * functions here to add their congestion control code.
 */
class INET_API TcpBaseAlg : public TcpAlgorithm
{
  protected:
    TcpBaseAlgStateVariables *& state; // alias to TcpAlgorithm's 'state'

    cMessage *rexmitTimer;
    cMessage *persistTimer;
    cMessage *delayedAckTimer;
    cMessage *keepAliveTimer;

    static simsignal_t cwndSignal; // will record changes to snd_cwnd
    static simsignal_t ssthreshSignal; // will record changes to ssthresh
    static simsignal_t rttSignal; // will record measured RTT
    static simsignal_t srttSignal; // will record smoothed RTT
    static simsignal_t rttvarSignal; // will record RTT variance (rttvar)
    static simsignal_t rtoSignal; // will record retransmission timeout
    static simsignal_t numRtosSignal; // will record total number of RTOs

  protected:
    /** @name Process REXMIT, PERSIST, DELAYED-ACK and KEEP-ALIVE timers */
    //@{
    virtual void processRexmitTimer(TcpEventCode& event);
    virtual void processPersistTimer(TcpEventCode& event);
    virtual void processDelayedAckTimer(TcpEventCode& event);
    virtual void processKeepAliveTimer(TcpEventCode& event);
    //@}

    /**
     * Start REXMIT timer and initialize retransmission variables
     */
    virtual void startRexmitTimer();

    /**
     * Update state vars with new measured RTT value. Passing two simtime_t's
     * will allow rttMeasurementComplete() to do calculations in double or
     * in 200ms/500ms ticks, as needed)
     */
    virtual void rttMeasurementComplete(simtime_t tSent, simtime_t tAcked);

    /**
     * Converting uint32_t echoedTS to simtime_t and calling rttMeasurementComplete()
     * to update state vars with new measured RTT value.
     */
    virtual void rttMeasurementCompleteUsingTS(uint32_t echoedTS) override;

    /**
     * Send data, observing Nagle's algorithm and congestion window
     */
    virtual bool sendData(bool sendCommandInvoked);

    /** Utility function */
    cMessage *cancelEvent(cMessage *msg) { return conn->cancelEvent(msg); }

  public:
    /**
     * Ctor.
     */
    TcpBaseAlg();

    /**
     * Virtual dtor.
     */
    virtual ~TcpBaseAlg();

    /**
     * Create timers, etc.
     */
    virtual void initialize() override;

    virtual void established(bool active) override;

    virtual void connectionClosed() override;

    /**
     * Process REXMIT, PERSIST, DELAYED-ACK and KEEP-ALIVE timers.
     */
    virtual void processTimer(cMessage *timer, TcpEventCode& event) override;

    virtual void sendCommandInvoked() override;

    virtual void receivedOutOfOrderSegment() override;

    virtual void receiveSeqChanged() override;

    virtual void receivedDataAck(uint32_t firstSeqAcked) override;

    virtual void receivedDuplicateAck() override;

    virtual void receivedAckForDataNotYetSent(uint32_t seq) override;

    virtual void ackSent() override;

    virtual void dataSent(uint32_t fromseq) override;

    virtual void segmentRetransmitted(uint32_t fromseq, uint32_t toseq) override;

    virtual void restartRexmitTimer() override;

    virtual bool shouldMarkAck() override;

    virtual void processEcnInEstablished() override;
};

} // namespace tcp
} // namespace inet

#endif

