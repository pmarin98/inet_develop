//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __INET_GATEVISUALIZERBASE_H
#define __INET_GATEVISUALIZERBASE_H

#include "inet/clock/common/ClockTime.h" // TODO
#include "inet/clock/contract/ClockTime.h"
#include "inet/queueing/contract/IPacketGate.h"
#include "inet/visualizer/base/VisualizerBase.h"
#include "inet/visualizer/util/GateFilter.h"
#include "inet/visualizer/util/Placement.h"

namespace inet {

namespace visualizer {

class INET_API GateVisualizerBase : public VisualizerBase
{
  protected:
    class INET_API GateVisitor : public cVisitor {
      public:
        std::vector<queueing::IPacketGate *> gates;

      public:
        virtual bool visit(cObject *object) override;
    };

    class INET_API GateVisualization {
      public:
        queueing::IPacketGate *gate = nullptr;

      public:
        GateVisualization(queueing::IPacketGate *gate);
        virtual ~GateVisualization() {}
    };

  protected:
    /** @name Parameters */
    //@{
    bool displayGates = false;
    GateFilter gateFilter;
    double width;
    double height;
    double spacing;
    Placement placementHint;
    double placementPriority;
    clocktime_t displayDuration;
    double currentTimePosition;
    //@}

    mutable simtime_t lastRefreshTime = -1;
    std::vector<const GateVisualization *> gateVisualizations;

  protected:
    virtual void initialize(int stage) override;
    virtual void handleParameterChange(const char *name) override;
    virtual void refreshDisplay() const override;
    virtual void preDelete(cComponent *root) override;

    virtual GateVisualization *createGateVisualization(queueing::IPacketGate *gate) const = 0;
    virtual void addGateVisualization(const GateVisualization *gateVisualization);
    virtual void addGateVisualizations();
    virtual void removeGateVisualization(const GateVisualization *gateVisualization);
    virtual void refreshGateVisualization(const GateVisualization *gateVisualization) const = 0;
    virtual void removeAllGateVisualizations();
};

} // namespace visualizer

} // namespace inet

#endif

