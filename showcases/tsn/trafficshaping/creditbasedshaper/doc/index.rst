Credit-Based Shaping
====================

Goals
-----

.. **V orig** Credit-based shaping is a method of smoothing out traffic and reducing bursting
.. in the outgoing interfaces of network nodes. It works by adding gaps (idle
.. periods) between successive packets of an incoming packet burst. These gaps can
.. be used to transmit traffic belonging to a higher priority traffic class, for
.. example, which can help to reduce the delay of higher priority traffic.
.. Credit-based shaping is often used to improve the performance of time-sensitive
.. applications by reducing delay and jitter.

.. In this example, we will demonstrate how to use the credit-based traffic shaper
.. to smooth out traffic and reduce bursting in network nodes.

Credit-based shaping (CBS), as defined in the IEEE 802.1Qav standard, is a
traffic shaping mechanism that regulates the transmission rate of Ethernet
frames to smooth out traffic and reduce bursts.

In this showcase, we demonstrate the configuration and operation of credit-based shaping in INET
with an example simulation.



.. **TODO** add IEEE 802.1Qav

.. **TODO** dont need to explain cbs here; adding gaps -> not good like this;

.. **TODO** some interesting stuff to show? -> shaping in general increases delay even for high priority frames. but can overall decrease delay (as it decreases delay for lower priority frames)

| INET version: ``4.4``
| Source files location: `inet/showcases/tsn/trafficshaping/creditbasedshaper <https://github.com/inet-framework/inet/tree/master/showcases/tsn/trafficshaping/creditbasedshaper>`__

Credit-Based Shaping Overview
-----------------------------

   The shaper maintains an amount of credit that changes depending on whether the
   interface is currently transmitting or idle. Frame transmission is only allowed
   when the credit count is non-negative. When frames are transmitted, credit
   decreases with the channel data rate (`send slope`) until the  transmission is
   complete. When there is no transmission, credit increases with a rate called
   `idle slope`. The next frame can begin transmitting when the credit is zero or positive.
   The idle slope controls the average outgoing data rate.

In CBS, each outgoing queue is associated with a credit counter. The credit
counter accumulates credits when the queue is idle, and consumes credits when
frames are transmitted. The rate at which credits are accumulated and consumed
is configured using parameters such as the `idle slope` and `send slope`.

When the queue contains a packet to be transmitted, the credit counter is checked. If the
credit counter is non-negative, the frame is transmitted immediately. If the
credit counter is negative, the frame is held back until the credit counter
becomes non-negative.

By controlling the transmission of frames based on credits, CBS introduces gaps
between frames, smoothing the traffic and providing opportunities for
higher-priority traffic to be transmitted. This helps in reducing latency and
jitter.

.. **V1** In INET, the credit-based shaper is implemented by the
.. :ned:`Ieee8021qCreditBasedShaper` simple module. This is a packet gate module (i.e. it only passes packets when it is open),
.. that can be combined with a packet queue to implement the credit based shaper
.. algorithm. A convenient way to insert a credit-based shaper into a network interface is to add it as a submodule to a :ned:`Ieee8021qTimeAwareShaper`.
.. The time aware shaper module has a configurable number of traffic classes, already contains queues for each, 
.. and can be added to Ethernet interfaces by setting the TODO parameter in the network node to true. 
.. The ``transmissionSelectionAlgorithm`` submodule of the time-aware shaper can be overriden to be a credit-based shaper.
.. For example, here is a time-aware shaper module with credit-based shaper submodules, and two traffic classes:

.. **V2** In INET, the credit-based shaper is implemented using the
.. Ieee8021qCreditBasedShaper simple module. This module acts as a packet gate,
.. allowing packets to pass through only when it is open. It can be combined with a
.. packet queue to implement the credit-based shaper algorithm. To conveniently
.. incorporate a credit-based shaper into a network interface, it can be added as a
.. submodule to an Ieee8021qTimeAwareShaper. The Ieee8021qTimeAwareShaper module
.. supports a configurable number of traffic classes, pre-existing queues for each class, and
.. can be enabled for Ethernet interfaces by setting the appropriate parameter in
.. the network node to true. To utilize a credit-based shaper, the
.. transmissionSelectionAlgorithm submodule of the time-aware shaper can be
.. overridden accordingly. As an example, here is a time-aware shaper module that
.. incorporates credit-based shaper submodules and supports two traffic classes.

CBS Implementation in INET
--------------------------

In INET, the credit-based shaper is implemented by the
:ned:`Ieee8021qCreditBasedShaper` simple module. This module acts as a packet gate,
allowing packets to pass through only when it is open. It can be combined with a
packet queue to implement the credit-based shaper algorithm. To conveniently
incorporate a credit-based shaper into a network interface, it can be added as a
submodule to an :ned:`Ieee8021qTimeAwareShaper`. The :ned:`Ieee8021qTimeAwareShaper` module
supports a configurable number of traffic classes, pre-existing queues for each
class, and can be enabled for Ethernet interfaces by setting the
:par:`enableEgressTrafficShaping` parameter in the network node to ``true``. To utilize a
credit-based shaper, the ``transmissionSelectionAlgorithm`` submodule of the
time-aware shaper can be overridden accordingly. As an example, here is a
time-aware shaper module that incorporates credit-based shaper submodules and
supports two traffic classes:

.. The credit-based shaper
.. module takes the place of the optional ``transmissionSelectionAlgorithm``
.. submodule of the time-aware shaper. The submodules of a time-aware shaper module
.. is shown below:

.. - the credit based shaper is a packet gate module (i.e. it only passes packets when it is open)
.. that can be combined wit ha packet queue to implement the CBS algorithm. A convenient way to insert
.. a CBS into a network node is as a submodule of a time aware shaper.
.. - the time aware shaper has a configurable number of traffic classes, already contains queues for each, 
.. and can be added to Ethernet interfaces by setting the TODO parameter in the network node to true.
.. - then, the transmission selection algorithm submodule of the time aware shaper can be a credit based shaper

.. figure:: media/timeawareshaper_.png
   :align: center

.. **TODO** annotate the CBS on the image

.. Packets arriving in the time-aware shaper module are classified to the different
.. traffic categories based on their PCP number. The priority is decided by the
.. ``transmissionSelection`` submodule. This is a :ned:`PriorityScheduler`
.. configured to work in reverse order, i.e., priority increases with traffic class
.. index (on the image above, ``video`` has priority over ``best effort``).

.. **V2**

Packets entering the time-aware shaper module are classified into different
traffic categories based on their PCP number using the
:ned:`PcpTrafficClassClassifier`. The priority assignment is determined by the
`transmissionSelection` submodule, which utilizes a :ned:`PriorityScheduler` configured
to operate in reverse order, i.e. priority increases with the
traffic class index. For example, in the provided image, video traffic takes
precedence over best effort traffic.

.. .. note:: By default, the time-aware shaper doesn't do any time-aware shaping, as its gates are always open. Thus it is possible to use the combined time-aware
..    shaper/credit-based shaper module as only a credit-based shaper this way, or add optional time-aware shaping as well (by specifying gate schedules).

.. **TODO** not a note, and not here.

   The :ned:`Ieee8021qCreditBasedShaper` module's :par:`idleSlope` parameter
   specifies the outgoing data rate of the shaper in `bps`. By default, the gate is
   open when the number of credits is zero or more. When the number of credits is
   positive, the shaper builds a burst reserve. However, by default, if there are
   no packets in the queue, the credits are set to zero.

The :par:`idleSlope` parameter of the :par:`Ieee8021qCreditBasedShaper` module determines the
outgoing data rate of the shaper, measured in bits per second. The specified
value determines the data rate to which the traffic will be limited, `as measured
in the Ethernet channel` (thus including protocol overhead). The shaper allows
packets to pass through when the number of credits is zero or more. When the
number of credits is positive, the shaper accumulates a burst reserve. As
defined in the standard, if there are no packets in the queue, the credits are
set to zero.

.. The idleSlope parameter of the Ieee8021qCreditBasedShaper module determines the
.. outgoing data rate of the shaper, measured in bits per second, within the
.. channel. This parameter defines the data rate at which the shaper limits traffic.

.. The idleSlope parameter of the Ieee8021qCreditBasedShaper module determines the
.. outgoing data rate of the shaper, measured in bits per second within the
.. channel. This parameter defines the data rate at which the shaper limits the
.. traffic.

.. when specifying the idleslop, note that this value will be measured 

.. the value to which traffic will be limited is measured on the channel

.. The specified value determines the data rate to which the traffic will be limited to,
.. as measured in the Ethernet channel (thus including protocol orverheads).

.. .. note:: The value specified with the :par:`idleSlope` represents the shaper's outgoing data rate
..           as measured in the channel. However, the actual data
..           rate within the shaper may be slightly lower due to protocol overhead. -> so is this needed?

Other parameters include the following:

- :par:`sendSlope`: The consumption rate of credits during transmission (default: idleSlope - channel bitrate)
- :par:`transmitCreditLimit`: credit limit above which the gate is open (default: 0)
- :par:`minCredit` and :par:`maxCredit`: a minimum and maximum limit for credits (default: no limit)

.. **TODO** other parameters as well; min and max credits

   .. note:: The :par:`idleSlope` parameter specifies the `channel data rate`. However, this is not the same as the data rate in the shaper due to protocol overhead.
            This is relevant when observing traffic in the shaper, as it isn't limited to the value of :par:`idleSlope`, but slightly less.

   .. note:: The value specified with the :par:`idleSlope` represents the shaper's outgoing data rate
            as measured in the channel. However, the actual data
            rate within the shaper may be slightly lower due to protocol overhead. -> i guess this not here

The Configuration
-----------------

The Network
+++++++++++

The network contains three network nodes. The client and the server (:ned:`TsnDevice`)
are connected through the switch (:ned:`TsnSwitch`), with 100Mbps EthernetLink
channels:

.. figure:: media/Network.png
   :align: center

Traffic
+++++++

In this simulation, we configure the client to generate two streams of
fluctuating traffic, which are assigned to two traffic categories. We insert
credit-based shapers for each category into the switch's outgoing interface
(``eth1``) to smooth traffic.

   Similarly to the ``Time-Aware Shaping`` showcase, we want to observe only the
   effect of the credit-based shaper on the traffic. Thus our goal is for the
   traffic to only get altered in the traffic shaper, and minimize unintended
   traffic shaping effects in other parts of the network.
   We configure two traffic source applications in the client to create two
   independent data streams that change sinusoidally, with ~37.7 and ~16.7 Mbps mean values,
   respectively, but the links in the network on average are not saturated. Later
   on, we configure the traffic shaper to limit the data rate to ~42 and ~21 Mbps
   for the data streams, thus the incoming traffic is on average less than the
   outgoing limit. Here is the traffic configuration:

Analogous to the Time-Aware Shaping showcase, our objective is to isolate and
observe the impact of the credit-based shaper on network traffic. To this end,
we aim for the traffic to be only modified significantly by the credit-based
shaper, avoiding any unintended traffic shaping effects elsewhere in the
network. To achieve this, we set up two traffic source applications in the
client, creating two separate data streams whose throughput varies sinusoidally
with maximum values of ~47 Mbps and ~34 Mbps, respectively. Given these
values, the network links are not operating at their full capacity,
thereby eliminating any significant traffic shaping effects resulting from link
saturation. Subsequently, we configure the traffic shaper to cap the data rates
of these streams at ~42 Mbps and ~21 Mbps, respectively. As a result, the
average incoming data rate is lower than the outgoing limit. Below are the
details of the traffic configuration:

.. literalinclude:: ../omnetpp.ini
   :start-at: client applications
   :end-before: outgoing streams
   :language: ini

Traffic Shaping
+++++++++++++++

   In the client, we want to classify packets from the two packet sources into two
   traffic classes: best effort and video. To do that, we enable IEEE 802.1 stream
   identification and stream encoding by setting the :par:`hasOutgoingStreams`
   parameter to ``true``. We configure the stream identifier module in the bridging
   layer to assign the outgoing packets to named streams by their UDP destination
   port. Then, the stream encoder sets the PCP number on the packets according to
   the assigned stream name (using the IEEE 802.1Q header's PCP field):

Within the client, our goal is to classify packets originating from the two
packet sources into two traffic classes: `best effort` and
`video`. To achieve this, we activate IEEE 802.1 stream
identification and stream encoding functionalities by setting the
:par:`hasOutgoingStreams` parameter in the switch to ``true``. We proceed by configuring the stream
identifier module within the bridging layer; this module is responsible for
associating outgoing packets with named streams based on their UDP destination
ports. Following this, the stream encoder sets the Priority Code Point (PCP) number on the packets according to
the assigned stream name (using the IEEE 802.1Q header's PCP field):

.. Following this, the stream encoder sets the Priority Code Point (PCP)
.. values within the packets according to the designated stream names,
.. utilizing the PCP field in the IEEE 802.1Q header:

.. literalinclude:: ../omnetpp.ini
   :start-at: outgoing streams
   :end-before: egress traffic shaping
   :language: ini

.. X

..    Traffic shaping takes place in the outgoing network interface of the switch
..    (``eth1``). The traffic shaper in the switch classifies packets by their PCP
..    number, and limits the data rate of the best effort stream to ~42 Mbps and the
..    data rate of the video stream to ~21 Mbps. The excess traffic is stored in the
..    MAC layer subqueues of the corresponding traffic class. **TODO** not here; -> results section

.. .. note:: We'll use time-aware shaper modules in the interface's MAC layer to add the credit-based shapers to (as described earlier, this is a convenient way
..    to add credit-based shapers to interfaces, as the time-aware shaper provides the necessarly modules, such as queues and classifiers; also, we don't
..    configure gate schedules, so there is no time-aware shaping).

**orig**

   We enable egress traffic shaping in the interface (this adds the time-aware
   shaper module). Then, we specify two traffic classes in the time-aware shaper,
   and set the transmission selection algorithm submodule's type to
   :ned:`Ieee8021qCreditBasedShaper` (this adds two credit-based shaper modules,
   one per traffic category). We configure the idle slope parameters of two
   credit-based shapers to ~42 and ~21 Mbps:

We enable egress traffic shaping in the switch, which
adds the time-aware shaper module to interfaces. In the time-aware shaper, we define two
traffic classes, and configure the transmission selection algorithm
submodules by setting their type to :ned:`Ieee8021qCreditBasedShaper`. This action
adds two credit-based shaper modules, one for each traffic class. We
then set the idle slope parameters of the two credit-based shapers to
~42 Mbps and ~21 Mbps, respectively:

.. literalinclude:: ../omnetpp.ini
   :start-at: egress traffic shaping
   :language: ini

Results
-------

Let's take a look at how the traffic changes in and between the various network nodes. First, we
compare the data rate of the client application traffic with the shaper incoming
traffic for the two traffic categories:

.. figure:: media/client_shaper.png
   :align: center

**orig**

   The traffic is very similar. The shaper incoming traffic has a bit higher data
   rate due to protocol overhead, which was not yet present in the applications.
   Also, the two packet streams are mixed in the client's network interface, which
   can delay packets. So even if the protocol overhead would be accounted for, the
   traffic would not be the same.

The client application and shaper incoming traffic is quite similar, but not identical. The shaper's incoming traffic
has a slightly higher data rate because of additional protocol overhead that
wasn't present in the application. Also, the two streams of packets are
combined in the client's network interface, which can cause some packets to be
delayed. Therefore, even if we adjusted for the extra protocol overhead, the traffic
wouldn't match exactly.

Now let's examine how the traffic changes in the shaper by comparing the data
rate of the incoming and outgoing traffic:

.. figure:: media/shaper_both.png
   :align: center

**orig**

   The data rate of the incoming traffic is on average less than the shaper's
   limit, but it periodically goes over the limit. In this case, the shaper limits
   the data rate to the configured limit. The shaper stores packets, and eventually
   sends them, so the outgoing traffic is smoothed. When the incoming traffic is
   below the shaper limit, there is no need for traffic shaping, and the outgoing
   traffic is the same as the incoming.

On average, the data rate of the incoming traffic is below the shaper's limit, but there are intermittent periods when it exceeds the limit.
When this happens, the shaper
caps the data rate at the set limit, by storing packets temporarily
and then sending them out eventually, which smooths out the
outgoing traffic. When the data rate of the incoming traffic is below the
shaper's limit, traffic shaping isn't needed, and the outgoing traffic mirrors
the incoming traffic.

.. .. note:: The data rate specified in the ini file as the idle slope parameter is the channel data rate, but this is somewhat different from the outgoing data rate in the shaper due to protocol overhead (PHY + IFG).  In this chart, we measure data rate in the shaper, thus we displayed the data rate limits as calculated for the shaper. 

.. note:: The data rate specified in the ini file as the idle slope parameter
          corresponds to the channel data rate. However, the outgoing data rate inside the
          shaper differs slightly due to protocol overhead, including factors like the PHY
          (Physical Layer) and IFG (Interframe Gap). In this chart, we measure the data
          rate within the shaper, so we have displayed the data rate limits as calculated
          specifically for the shaper.

The next chart compares the shaper outgoing and server application traffic:

.. figure:: media/shaper_server.png
   :align: center

**orig**

   Similarly to the first chart, the traffic is similar, with the shaper traffic
   being higher due to protocol overhead. The traffic only changes significantly in
   the shaper, other parts of the network have no traffic shaping effect, as
   expected.

Much like the first chart, the shaper outgoing and server application traffic profiles are similar, but the shaper's
traffic is slightly higher due to protocol overhead. 

.. The traffic undergoes
.. significant changes primarily within the shaper, while other parts of the
.. network remain unaffected by traffic shaping, which aligns with our
.. expectations.

Having examined the traffic across the entire network, we can conclude that the
traffic undergoes significant changes mainly within the shaper, while other
parts of the network remain unaffected by traffic shaping, consistent with our
expectations.

.. Having examined the traffic across the entire network, we can conclude that the
.. traffic undergoes significant changes primarily within the shaper, while other
.. parts of the network do not introduce unintended traffic shaping effects,
.. aligning with our expectations.

   The following sequence chart displays frame transmissions in the network. The
   best effort traffic category is colored blue, the video red:

The sequence chart below illustrates the transmission of frames within the
network. In this chart, the `best effort` traffic category is represented in
blue, while the `video category` is depicted in red.

.. figure:: media/seqchart2.png
   :align: center

**orig**

   The traffic is more bursty when it arrives in the switch. The traffic shaper in
   the switch distributes packets evenly, and interleaves video packets with best
   effort ones.

The traffic is bursty when it arrives in the switch.
The traffic shaper within the switch evenly distributes the packets and
interleaves video packets with those of the best effort category.

   The next diagram shows the queue lengths of the traffic classes in the outgoing
   network interface of the switch. The queue lengths don't increase over time
   because the data rate of the shaper incoming traffic is, on average, less than
   the data rate of the allowed outgoing traffic. (The incoming data rate
   fluctuates around ~37.7 and 16.7 Mbps, and the shaper limits the data rate to
   ~42 and ~21 Mbps, respectively.) Excess packets are stored in the queues, and
   not dropped.

The next diagram illustrates the queue lengths of the traffic classes in
the switch's outgoing network interface. The queue lengths don't increase over
time, as the average data rate of the incoming traffic to the shaper is lower
than the permitted data rate for outgoing traffic.

.. figure:: media/TrafficShaperQueueLengths.png
   :align: center

**orig**

   The next chart shows the number of credits as it fluctuates rapidly.
   The number can grow above zero if the corresponding queue is not empty.

The following chart displays the rapid fluctuations in the number of credits.
The number can exceed zero when the corresponding queue is not empty.

.. **TODO** it doesn't accumulate burst reserve if there are no packets -> is this needed?

.. figure:: media/TrafficShaperNumberOfCredits.png
   :align: center

.. The next charts shows the same chart zoomed in:

The next chart provides a closer view of the chart shown above, with a zoomed-in perspective:

.. figure:: media/TrafficShaperNumberOfCredits_zoomed.png
   :align: center

**orig**

   The next chart shows the gate states for the two credit-based shapers, and the
   transmitting state of the outgoing interface in the switch. Note that the gates
   can be open for periods longer than the duration of one packet transmission if
   the number of credits is zero or more. The transmitter is not transmitting all
   the time, as the maximum outgoing data rate of the switch (~63Mbps) is less than
   the channel capacity (100Mbps).

The following chart illustrates the gate states for the two credit-based
shapers, as well as the transmitting state of the outgoing interface in the
switch. It is worth noting that the gates can remain open for longer durations
than a single packet transmission if the number of credits is zero or higher.
Additionally, the transmitter is not in a constant transmitting state since the
maximum outgoing data rate of the switch (~63Mbps) is lower than the channel
capacity (100Mbps).

.. figure:: media/TransmittingStateAndGateStates.png
   :align: center

**orig**

   The next diagram shows the relationships between the number of credits, the gate
   state of the credit-based transmission selection algorithm, and the transmitting
   state of the outgoing network interface for both traffic classes. The diagram
   shows only the first 2ms of the simulation to make the details visible:

The following diagram depicts the relationships among the number of credits, the
gate state of the credit-based transmission selection algorithm, and the
transmitting state of the outgoing network interface for both traffic classes.
To ensure visibility of the details, the diagram focuses on the first 2ms of the simulation:

.. figure:: media/TrafficShaping.png
   :align: center

**orig**

   Note that the queue length is zero most of the time because the queue length
   doesn't increase to 1 if an incoming packet can be transmitted immediately.
   Also, in the transmitter, sometimes two packets (each from a different category)
   are being trasmitted back-to-back, with just an Interframe Gap period in between
   them - e.g. the first two transmissions. This doesn't cause per-traffic-class
   bursting because the two packets are of different traffic classes.

Note that the queue length remains at zero for most of the time since an
incoming packet can be transmitted immediately without causing the queue length
to increase to 1. Additionally, in the transmitter, there are instances where
two packets (each from a different traffic class) are transmitted consecutively,
with only an Interframe Gap period between them. For example, this can be
observed in the first two transmissions. It's important to highlight that such
back-to-back transmission of packets from different traffic classes does not
result in bursting within individual traffic classes.

**orig**

   We can observe the operation of the credit-based shaper in this diagram. Take,
   for example, the number of credits for the video traffic category. First the
   number of credits is 0. Then, a packet arrives at the queue and starts
   trasmitting, and the number of credits decreases. When the transmission is
   finished, the number of credits begins to increase. When the number of credits
   reaches 0, another transmission starts, and the number of credits starts
   decreasing again.

In this diagram, we can observe the operation of the credit-based shaper.
Let's consider the number of credits for the video traffic category as an
example. Initially, the number of credits is at 0. Then, when a packet arrives
at the queue and begins transmitting, the number of credits decreases. After the
transmission completes, the number of credits begins to increase again. As the
number of credits reaches 0 once more, another transmission starts, causing the
number of credits to decrease once again.

Sources: :download:`omnetpp.ini <../omnetpp.ini>`

Discussion
----------

Use `this <https://github.com/inet-framework/inet/discussions/800>`__ page in the GitHub issue tracker for commenting on this showcase.

