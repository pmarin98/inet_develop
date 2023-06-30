Using Different Traffic Shapers for Different Traffic Classes
=============================================================

Goals
-----

In INET, we can use different traffic shapers for different traffic classes in a
TSN switch. In this showcase, we demonstrate using a credit-based shaper (CBS)
and an asynchronous traffic shaper (ATS).

.. note:: Multiple traffic shapers are combined in the same traffic stream in the :doc:`/showcases/tsn/trafficshaping/cbsandtas/doc/index` showcase.

**TODO** van egy masik showcase/lehet hogy erdekelne egy masik showcase -> you might be interested in looking at another showcase which ....

| INET version: ``4.4``
| Source files location: `inet/showcases/tsn/trafficshaping/mixingshapers <https://github.com/inet-framework/inet/tree/master/showcases/tsn/trafficshaping/mixingshapers>`__

The Model
---------

In this showcase, similarly to the
:doc:`/showcases/tsn/trafficshaping/creditbasedshaper/doc/index` and
:doc:`/showcases/tsn/trafficshaping/asynchronousshaper/doc/index` showcases, we
insert a credit-based shaper and an asynchronous traffic shaper into a
:ned:`Ieee8021qTimeAwareShaper`. We use two traffic streams; one of them is
shaped with the CBS, the other with the ATS. Time-aware shaping isn't enabled.


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

Configuration
~~~~~~~~~~~~~

In this section, we summarize the configuration, going into the details only
when it is relevant for this scenario. More detailed description for the
configuration of CBS and ATS are provided in their respective showcases.

The Network
+++++++++++

There are three network nodes in the network. The client and the server are
:ned:`TsnDevice` modules, and the switch is a :ned:`TsnSwitch` module. The links
between them are 100 Mbps :ned:`EthernetLink` channels.

.. figure:: media/Network.png
   :align: center

Traffic
+++++++

Similarly to the other traffic shaping showcases, we want to observe how the
shapers change the traffic. When generating packets, we make sure that traffic
is only changed significantly in the shapers (i.e. other parts of the network
have no significant shaping effects).

We create two sinusoidally changing traffic streams (called ``best effort`` and
``video``) in the ``client``, with an average data rate of 40 and 20 Mbps. The
two streams are terminated in two packet sinks in the ``server``:

.. literalinclude:: ../omnetpp.ini
   :start-at: client applications
   :end-before: outgoing streams
   :language: ini

Stream Identification and Encoding
++++++++++++++++++++++++++++++++++

We classify packets into two traffic categories (best effort and video) in the
same way as in the Credit-Based Shaping and Asynchronous Traffic shaping
showcases. Here is a summary of the stream identification and encoding
configuration:

- We enable IEEE 802.1 stream identification and stream encoding in the client. 
- We configure the stream identifier module in the bridging layer to assign outgoing packets to named streams by UDP destination port. 
- The stream encoder sets the PCP number according to the assigned stream name.
- After transmission, the switch decodes the streams by the PCP number.

**TODO** remove note and closable thing

.. note::

   .. raw:: html

      <details>
      <summary>The relevant configuration (click to open/close)</summary>

   .. literalinclude:: ../omnetpp.ini
      :start-at: outgoing streams
      :end-before: ingress per-stream filtering
      :language: ini

   .. raw:: html

      </details>

Traffic Shaping
+++++++++++++++

The asynchronous traffic shaper has components in the bridging layer. I.e., The
ATS requires the transmission eligibility time for each packet to be calculated
by the ingress per-stream filtering.

We enable ingress filtering in the switch, this adds a stream filtering layer to
the bridging layer. By default, the ingress filter is a
:ned:`SimpleIeee8021qFilter`. We add the :ned:`EligibilityTimeMeter` and
:ned:`EligibilityTimeFilter` modules here.

.. literalinclude:: ../omnetpp.ini
   :start-at: ingress per-stream filtering
   :end-before: egress traffic shaping
   :language: ini

We only use one traffic class here, because only the video stream is shaped by
the ATS, and only the ATS has components in the ingess filter. The classifier
will send the video stream through the meter and the filter module, while the
best effort stream will just go through the :ned:`SimpleIeee8021qFilter` module
via the non-filtered direct path:

.. figure:: media/filter.png
   :align: center

Also, we need to configure the committed information rate and committed burst
size parameters of the ATS here, at the meter modules. These modules meter the
data rate of the traffic passing through, calculate the eligibility time and put
an eligibility time tag to all packets. We configure the ATS to limit the video
stream to ~20 Mbps, while allowing some bursts.

The traffic shaping takes place in the outgoing network interface of the switch
where both streams pass through. The credit-based shaper only has components
here. We configure the CBS to limit the data rate of the best effort stream to
~40 Mbps.

Here is the egress traffic shaping configuration:

.. literalinclude:: ../omnetpp.ini
   :start-at: egress traffic shaping
   :language: ini

Packets that are held up by the shapers are stored in the MAC layer subqueues of
the corresponding traffic class.

Results
-------

Let's observe the traffic shaping in the switch. The following chart displays
the incoming and outgoing data rate in the base time-aware shaper module for the
two traffic streams:

.. figure:: media/shaper_both.png
   :align: center

**TODO** committed information rate zorder-10

The best effort traffic is shaped by the CBS. The shaper limits the data rate to
the nominal value of 40Mbps, without bursts. The video stream is shaped by the
ATS, and in addition to limiting the data rate to the nominal value, some bursts
are allowed when the traffic increases above the nominal rate.

As mentioned before, we configured the traffic so that the traffic shaping only
happens in the shapers, but there is no significant traffic shaping effects in
other parts of the network.

Sources: :download:`omnetpp.ini <../omnetpp.ini>`

Discussion
----------

Use `this <https://github.com/inet-framework/inet/discussions/801>`__ page in the GitHub issue tracker for commenting on this showcase.

