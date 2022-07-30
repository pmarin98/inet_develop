Peeking Under the Hood
======================

Goals
-----

This showcase demonstrates that the scheduling and traffic shaping modules can
work outside the context of a network node. Doing so may facilitate assembling
and validating specific complex scheduling and traffic shaping behaviors which
can be difficult to replicate in a complete network.

| INET version: ``4.4``
| Source files location: `inet/showcases/tsn/trafficshaping/underthehood <https://github.com/inet-framework/inet/tree/master/showcases/tsn/trafficshaping/underthehood>`__

The Model
---------

In this showcase, we examine traffic shaping by directly connecting queueing
components that make up a traffic source, an asynchronous shaper, and a traffic
sink. There is no 'real' network here, but the operation of the traffic shaper
is the same.

.. note:: The :doc:`/showcases/tsn/trafficshaping/asynchronousshaper/doc/index` showcase demonstrates asynchronous traffic shaping in a complete network, and examines the asynchronous traffic shaper (ATS) in more detail. However, we give a quick summary of the required modules in the next section.

ATS Overview
++++++++++++

The Asynchronous Traffic Shaper (ATS) can smooth traffic by limiting it to a
nominal data rate, while also allowing some burstiness. The ATS mechanism is
implemented by the following four modules in INET:

- :ned:`EligibilityTimeMeter`: calculates transmission eligibility time
- :ned:`EligibilityTimeFilter`: filters expired packets
- :ned:`EligibilityTimeQueue`: stores packets ordered by transmission eligibility time
- :ned:`EligibilityTimeGate`: opens at the transmission eligibility time for the next packet

When using asynchronous shapers in a complete network, these modules are located
in different parts of a network node (such as an Ethernet switch): the meter and
filter modules are in the ingress filter of the bridging layer, the queue and
gate are in an interface.

The ATS parameters are set in the :ned:`EligibilityTimeMeter` module: 

- :par:`committedInformationRate`: the data rate the traffic is limited to
- :par:`committedBurstSize`: the allowed burst size

Furthermore, the :par:`maxResidenceTime` parameter can be set in the
:ned:`EligibilityTimeFilter` module. The filter makes sure packets don't wait in
the queue for more than the max residence time by dropping packets that would
wait longer.

The Network
+++++++++++

The traffic sources, shaper components and traffic sinks are connected as
follows. We also added annotations to display where the components would be
located in a complete network:

.. figure:: media/Network4.png
   :align: center

The network contains three independent packet sources. Each packet source is
connected to a :ned:`EligibilityTimeMeter`, so the data rate of each packet
stream is metered individually. The meters are connected to a single
:ned:`EligibilityTimeFilter`, which drops expired packets.

The filter is connected to a :ned:`EligibilityTimeQueue`, which sorts packets by
transmission eligibility time. The queue is connected to an
:ned:`EligibilityTimeGate`, which opens based on transmission eligibility time.

The gate is connected to a server that periodically pulls packets from the queue
when the gate is open, and sends them to three packet sinks via a classifier
that separates the three streams.

.. note:: There is just one traffic shaper, but the three packet streams are still shaped individually according to the transmission eligibility-time calculated by the three meters.

Traffic
+++++++

The packet sources generate traffic with three different sinusoidal production
rates:

.. literalinclude:: ../omnetpp.ini
   :start-at: numSources
   :end-before: meter
   :language: ini

Traffic Shaping
+++++++++++++++

To configure the traffic shaping, we set the committed information rate and
burst size parameters in the meter modules. The outgoing traffic will be
compliant with these values. We also set a max residence time in the meter; the
filter then drops packets whose transmission eligibility time is further than
the residence time, so packets won't wait in the queue longer than the max
residence time.

The active server module pulls packets from the traffic shaper when the gate
allows it. We set a constant processing time for each packet in the server, so
the server tries to pull packets every 0.1ms:

.. literalinclude:: ../omnetpp.ini
   :start-at: meter
   :end-at: server
   :language: ini

Classifying
+++++++++++

We want to separate the streams to three sinks, so traffic flows from a source
to its sink counterpart (i.e. ``source[0]`` -> ``sink[0]`` and so on). The
classifier is a :ned:`ContentBasedClassifier`. In the classifier, we classify
packets by name; the name contains the source module:

.. literalinclude:: ../omnetpp.ini
   :start-at: classifier
   :end-at: classifier
   :language: ini

.. note:: In a complete network, managing streams would involve several features, such as stream identification, encoding and decoding. Packets would be assigned to streams using VLAN IDs or PCP numbers.

Results
-------

Let's examine the data rate of the corresponding source-sink pairs. We can
observe the source traffic, and the traffic arriving in the sinks after shaping:

.. figure:: media/datarate.png
   :align: center

The data rate of the sinusoidal source traffic sometimes goes above the 16Mbps
per-stream shaper limit. The shaper allows some burst, then limits the traffic
to the configured limit.

The next chart shows the traffic before and after shaping for each stream
individually, and the packet drop rate in the filter for all three streams
combined:

.. figure:: media/drop.png
   :align: center

Even though there is only one combined shaper in the network (i.e., one filter,
queue, and gate), streams are limited individually to 16Mbps because they are
separately metered. We can observe that the packet drop rate was higher when the
sinusoidal source traffic was going above the per-stream shaper limit.

Note that as traffic increases above the shaper limit, first some packets are
stored in the queue to be sent later. If the traffic stays above the shaper
limit, some packets would have to wait in the queue for longer than the max
residence time, so the filter drops these excess packets. The drop rate roughly
follows the incoming traffic rate.

The next chart shows how the queueing time changes:

.. figure:: media/QueueingTime.png
   :align: center

The maximum of the queueing time is 10ms, which is due to the max residence time
set in the filter. Packets that would wait longer in the queue are dropped by
the filter before getting there.

The next chart shows the queue length:

.. figure:: media/QueueLength.png
   :align: center

20 packets can accumulate in the queue from each stream: `average production
interval / max residence time = 10ms / 0.5ms = 20`. Since the traffic intensity
is different for each stream, the 20 packets per stream limit is reached at
different times for each stream. We can observe this on the chart. The maximum
of the queue length is 60, when all streams reach their maximum packet count in
the queue.


Sources: :download:`omnetpp.ini <../omnetpp.ini>`, :download:`PeekingUnderTheHoodShowcase.ned <../PeekingUnderTheHoodShowcase.ned>`

Discussion
----------

Use `this <https://github.com/inet-framework/inet/discussions/803>`__ page in the GitHub issue tracker for commenting on this showcase.

