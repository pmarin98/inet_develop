Combining Time-Aware and Credit-Based Shaping
=============================================

Goals
-----

Multiple traffic shapers can be used in the same traffic stream. This showcase
demonstrates combining credit-based and time-aware shaping.

.. note:: Multiple traffic shapers are used in different traffic streams in the :doc:`/showcases/tsn/trafficshaping/cbsandats/doc/index` showcase.

| INET version: ``4.4``
| Source files location: `inet/showcases/tsn/trafficshaping/cbsandtas <https://github.com/inet-framework/inet/tree/master/showcases/tsn/trafficshaping/cbsandtas>`__

The Model
---------

Time-aware shapers (TAS) and credit-based shapers (CBS) can be combined by
inserting an :ned:`Ieee8021qTimeAwareShaper` module into an interface, and
setting the queue type to :ned:`Ieee8021qCreditBasedShaper`. The number of
credits in the CBS only changes when corresponding gate of the time-aware shaper
is open.

The configuration is similar to the one in the credit-based shaping showcase,
but time-aware shaping is enabled. The following sections present a summary of
the configuration. Detailed description is only provided when relevant to this
scenario.

.. note:: For more details about CBS and TAS, check out the relevant showcases: :doc:`/showcases/tsn/trafficshaping/creditbasedshaper/doc/index` and :doc:`/showcases/tsn/trafficshaping/asynchronousshaper/doc/index`.

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

.. note::

   .. raw:: html

      <details>
      <summary>The relevant configuration (click to open/close)</summary>

   .. literalinclude:: ../omnetpp.ini
      :start-at: outgoing streams
      :end-before: egress traffic shaping
      :language: ini

   .. raw:: html

      </details>

Traffic Shaping
+++++++++++++++

The traffic shaping takes place in the outgoing network interface of the switch
where both streams pass through. We configure the CBS to limit the data rate of
the best effort stream to ~40 Mbps, and the video stream to ~20 Mbps.

Here is the egress traffic shaping configuration:

.. literalinclude:: ../omnetpp.ini
   :start-at: egress traffic shaping
   :language: ini

Note that the actual committed information rate is 1/3 and 2/3 of the values set
here, as the corresponding gates are open for 1/3 and 2/3 of the time.

Packets that are held up by the shapers are stored in the MAC layer subqueues of
the corresponding traffic class.

Results
-------

The following chart displays the incoming and outgoing data rate in the
credit-based shapers:

.. figure:: media/shaper_both.png
   :align: center
   :width: 100%

Data rate measurement produces a data point after every 100 packet
transmissions, i.e. ~10 ms of continuous transmission. This is the same as the
cycle time of the time-aware shaping (including the periods when the gate is
closed), so ~2.5 open-gate periods for best effort, ~5 for video. Thus, the
fluctuation depends on how many idle periods are counted in a measurement
interval (so the data rate seems to fluctuate between two distinct values).

The following sequence chart displays packet transmissions for both traffic
categories (blue for best effort, orange for video). We can observe the
time-aware shaping gate schedules:

.. figure:: media/seqchart2.png
   :align: center
   :width: 100%


Sources: :download:`omnetpp.ini <../omnetpp.ini>`

Discussion
----------

Use `this <https://github.com/inet-framework/inet/discussions/TODO>`__ page in the GitHub issue tracker for commenting on this showcase.

