Using Different Traffic Shapers for Different Traffic Classes
=============================================================

Goals
-----

In INET, it is possible to use different traffic shapers for different traffic classes in a
TSN switch. In this showcase, we demonstrate using a credit-based shaper (CBS)
and an asynchronous traffic shaper (ATS).

.. note:: You might be interested in looking at another showcase, in which multiple traffic shapers are combined in the same traffic stream: :doc:`/showcases/tsn/trafficshaping/cbsandtas/doc/index`.

.. **TODO** van egy masik showcase/lehet hogy erdekelne egy masik showcase -> you might be interested in looking at another showcase which ....

| INET version: ``4.4``
| Source files location: `inet/showcases/tsn/trafficshaping/mixingshapers <https://github.com/inet-framework/inet/tree/master/showcases/tsn/trafficshaping/mixingshapers>`__

The Model
---------

.. In this showcase, similarly to the
.. :doc:`/showcases/tsn/trafficshaping/creditbasedshaper/doc/index` and
.. :doc:`/showcases/tsn/trafficshaping/asynchronousshaper/doc/index` showcases, we
.. insert a credit-based shaper and an asynchronous traffic shaper into a
.. :ned:`Ieee8021qTimeAwareShaper`. We use two traffic streams; one of them is
.. shaped with the CBS, the other with the ATS. Time-aware shaping isn't enabled.

.. In this demonstration, similarly to the
.. :doc:/showcases/tsn/trafficshaping/creditbasedshaper/doc/index and
.. :doc:/showcases/tsn/trafficshaping/asynchronousshaper/doc/index showcases, Our focus is on
.. the :ned:Ieee8021qTimeAwareShaper module, into which we incorporate a Credit
.. Based Shaper (CBS) and an Asynchronous Traffic Shaper (ATS). This module is
.. designed to manage two traffic classes. The CBS is inserted into one traffic
.. class, and the ATS is assigned to the other. However, in this instance,
.. time-aware shaping is not enabled. This is the common methodology across these
.. showcases.

In this demonstration, similarly to the CBS and ATS showcases, we employ a Time
Aware Shaper module with two traffic classes. However, one class is shaped using
a Credit-Based Shaper (CBS), and the other class is shaped by an Asynchronous
Traffic Shaper (ATS). Time-aware shaping is not enabled.

.. One class incorporates a Credit-Based Shaper, while the other
.. utilizes an Asynchronous Traffic Shaper.


.. Overview
.. ~~~~~~~~

.. The following is a quick overview of the traffic shaper architecture in INET.
.. For more details, check the NED documentation of traffic shaper modules and the other shaper showcases.

.. - The :ned:`Ieee8021qTimeAwareShaper` module is added to interfaces of TSN network nodes when the :par:`hasEgressTrafficShaping` parameter is set to ``true``.
..   This module has a configurable number of traffic classes (:par:`numTrafficClasses`), and a ``queue`` and ``transmissionSelectionAlgorithm`` submodule for each
..   traffic class. A convenient way to add CBS or ATS capability to an interface is to override the type of these submodules
..   with :ned:`Ieee8021qCreditBasedShaper` or :ned:`Ieee8021qAsynchronousShaper`.
.. - :ned:`Ieee8021qCreditBasedShaper` and :ned:`Ieee8021qAsynchronousShaper` are packet gate modules, and take the place of the ``transmissionSelectionAlgorithm`` module in the TAS.
.. - ATS also needs a :ned:`EligibilityTimeQueue` as the queue in the TAS, and an :ned:`EligibilityTimeMeter` and :ned:`EligibilityTimeFilter` in the bridging layer
..   of the TSN network node.

.. .. The type of these submodules can be overriden with :ned:`Ieee8021qCreditBasedShaper` or :ned:`Ieee8021qAsynchronousShaper` to add CBS or ATS
..   to the network node/interface. 

.. We present the example network and summarize the configuration in the following
.. sections, going into the details only when it is relevant for this scenario.
.. More detailed description for the configuration of CBS and ATS are provided in
.. their respective showcases.

In the sections that follow, we introduce the example network and provide a
summary of the configuration, delving into specifics only when they are
pertinent to this scenario. Comprehensive descriptions regarding the
configuration of the Credit-Based Shaper (CBS) and the Asynchronous Traffic
Shaper (ATS) can be found in their individual showcases.

The Network
+++++++++++

For this demonstration, we'll use a simple network with three network nodes: a client, a switch and a server. The client and the server are
:ned:`TsnDevice` modules, and the switch is a :ned:`TsnSwitch` module. They are connected by 100 Mbps Ethernet links.

.. figure:: media/Network.png
   :align: center

Traffic
+++++++

.. Similarly to the other traffic shaping showcases, we want to observe how the
.. shapers change the traffic. When generating packets, we make sure that traffic
.. is only changed significantly in the shapers (i.e. other parts of the network
.. have no significant shaping effects).

.. We create two sinusoidally changing traffic streams (called ``best effort`` and
.. ``video``) in the ``client``, with an average data rate of 40 and 20 Mbps. The
.. two streams are terminated in two packet sinks in the ``server``:

.. In alignment with our other traffic shaping showcases, our goal is to examine
.. the changes the shapers impose on the traffic. When generating packets, we
.. ensure that any significant alterations to traffic occur primarily within the
.. shapers, meaning other network segments do not have substantial shaping effects.

As in the other traffic shaping showcases, we aim to examine how the
shapers modify traffic patterns. To ensure that any significant alterations are
due to the shapers, we configure packet generation such that other
network components do not introduce substantial shaping effects.

We generate two sinusoidally fluctuating traffic streams, termed `best
effort` and `video`, within the client. These streams have average data rates
of 40 Mbps and 20 Mbps, respectively. Both streams are terminated at two
separate packet sinks in the server.

.. literalinclude:: ../omnetpp.ini
   :start-at: client applications
   :end-before: outgoing streams
   :language: ini

Stream Identification and Encoding
++++++++++++++++++++++++++++++++++

.. We classify packets into two traffic categories (best effort and video) in the
.. same way as in the Credit-Based Shaping and Asynchronous Traffic shaping
.. showcases. Here is a summary of the stream identification and encoding
.. configuration:

.. Just like in the Credit-Based Shaping and Asynchronous Traffic Shaping
.. showcases, we categorize packets into two traffic categories: best effort and video.
.. Below is a summary of the procedure for stream identification and encoding, followed by the
.. configuration:

.. - We enable IEEE 802.1 stream identification and stream encoding in the client. 
.. - We configure the stream identifier module in the bridging layer to assign outgoing packets to named streams by UDP destination port. 
.. - The stream encoder sets the PCP number according to the assigned stream name.
.. - After transmission, the switch decodes the streams by the PCP number.

We employ the same method as in the Credit-Based Shaping and Asynchronous
Traffic Shaping showcases to classify packets into two traffic categories: best
effort and video. A summary of our stream identification and encoding
configuration is as follows:

- In the client, we activate IEEE 802.1 stream identification and stream encoding.
- The stream identifier module in the bridging layer is set up to allocate outgoing packets to named streams based on the UDP destination port.
- The stream encoder sets the PCP (Priority Code Point) number according to the designated stream name.
- Following transmission, the switch decodes the streams using the PCP number.

This is achieved by the following:

.. literalinclude:: ../omnetpp.ini
   :start-at: outgoing streams
   :end-before: ingress per-stream filtering
   :language: ini

Traffic Shaping
+++++++++++++++

**TODO** two sections? filtering and trafficshaping? or just describe that we need to set up filtering because the ATS has parts in the filtering layer.

In the this section, we take a look at what needs to be configured for ATS and CBS.
ATS has its meter and filter components in the briding layer, which need to be enabled.

- In the this section, we take a look at what needs to be configured for ATS and CBS.
- ats has its meter and filter components in the bridging layer, we need to add these modules
- the ats parameters are configured in the meter module, so we configure the committed information rate and committed burst size

.. In this section, we enable and configure traffic shaping.
.. The asynchronous traffic shaper has components in the bridging layer. I.e., The
.. ATS requires the transmission eligibility time for each packet to be calculated
.. by the ingress per-stream filtering.

In this section, we focus on enabling and configuring traffic shaping. It's
important to note that the ATS also has components in the bridging layer.
Specifically, ATS requires the per-stream filtering to calculate the
transmission eligibility time for each packet.

.. We enable ingress filtering in the switch, this adds a stream filtering layer to
.. the bridging layer. By default, the ingress filter is a
.. :ned:`SimpleIeee8021qFilter`. We add the :ned:`EligibilityTimeMeter` and
.. :ned:`EligibilityTimeFilter` modules here.

We enable ingress filtering in the switch, which adds a stream filtering layer
to the bridging layer. By default, we use the :ned:`SimpleIeee8021qFilter`` as the
ingress filter. We add the :ned:`EligibilityTimeMeter`` and
:ned:`EligibilityTimeFilter`` to the this filter module.

.. literalinclude:: ../omnetpp.ini
   :start-at: ingress per-stream filtering
   :end-before: egress traffic shaping
   :language: ini

.. We only use one traffic class here, because only the video stream is shaped by
.. the ATS, and only the ATS has components in the ingess filter. The classifier
.. will send the video stream through the meter and the filter module, while the
.. best effort stream will just go through the :ned:`SimpleIeee8021qFilter` module
.. via the non-filtered direct path:

In this scenario, we utilize a single traffic class since only the video stream
undergoes shaping through the ATS, which has components in the ingress filter.
The classifier directs the video stream through the meter and filter modules,
while the best effort stream follows the non-filtered direct path in the
:ned:`SimpleIeee8021qFilter`` module, as illustrated on the following image:

.. figure:: media/filter.png
   :align: center

**TODO** restructure this

.. *******************

.. Furthermore, the ATS parameters are configured at the meter modules in the filter module.
.. We configure the committed information rate and committed burst size.
.. The meter modules measure the
.. data rate of the traffic passing through, calculate the transmission eligibility time and put
.. an eligibility time tag to all packets. We configure the ATS to limit the video
.. stream to ~20 Mbps, while allowing some bursts.

.. The meter module measures the data rate of the traffic passing through,
.. calculate the transmission eligibility time and put an eligibility time tag to
.. all packets. The eligibility time is calculated based on the configured
.. committed information rate and committed burst size parameters. The eligibility
.. time will then be used in the interface to delay packets as necessary. We
.. configure the ATS to limit the video stream to ~20 Mbps, while allowing some
.. bursts.

The meter module measures the traffic data rate, calculates the transmission
eligibility time, and assigns an eligibility time tag to each packet. This
eligibility time is then used at the interface level to appropriately delay
packets as needed. In this configuration, we limit the video stream to around 20
Mbps while allowing for occasional bursts.

.. Also, we need to configure the committed information rate and committed burst
.. size parameters of the ATS here, at the meter modules. These modules meter the
.. data rate of the traffic passing through, calculate the eligibility time and put
.. an eligibility time tag to all packets. We configure the ATS to limit the video
.. stream to ~20 Mbps, while allowing some bursts.

The traffic shaping takes place in the outgoing network interface of the switch.
The credit-based shaper only has components here. We configure the CBS to limit
the data rate of the best effort stream to ~40 Mbps.

Here is the egress traffic shaping configuration:

.. literalinclude:: ../omnetpp.ini
   :start-at: egress traffic shaping
   :language: ini

.. *******************

Packets that are held up by the shapers are stored in the MAC layer subqueues of
the corresponding traffic class.

Results
-------

.. Let's observe the traffic shaping in the switch. The following chart displays
.. the incoming and outgoing data rate in the base time-aware shaper module for the
.. two traffic streams:

Let's examine the traffic shaping process within the switch. The chart below
illustrates the incoming and outgoing data rates within the base time-aware
shaper module for the two traffic streams:

.. figure:: media/shaper_both.png
   :align: center

.. The best effort traffic is shaped by the CBS. The shaper limits the data rate to
.. the nominal value of 40Mbps, without bursts. The video stream is shaped by the
.. ATS, and in addition to limiting the data rate to the nominal value, some bursts
.. are allowed when the traffic increases above the nominal rate.

The CBS shapes the best effort traffic by restricting the data rate to a fixed
value of 40 Mbps without any bursts. On the other hand, the ATS shapes the video
stream by not only limiting the data rate to the desired nominal value but also
allowing for bursts when the traffic exceeds the nominal rate.

As mentioned before, we configured the traffic so that the traffic shaping only
happens in the shapers, but there is no significant traffic shaping effects in
other parts of the network.

As expected, the traffic shaping effects are concentrated within the designated
shaper modules, but not in other parts of the network.

Sources: :download:`omnetpp.ini <../omnetpp.ini>`

Discussion
----------

Use `this <https://github.com/inet-framework/inet/discussions/801>`__ page in the GitHub issue tracker for commenting on this showcase.

