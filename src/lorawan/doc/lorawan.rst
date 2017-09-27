LoRaWAN module
----------------------------

Example Module Documentation
----------------------------

.. include:: replace.txt
.. highlight:: cpp

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

This is a suggested outline for adding new module documentation to |ns3|.
See ``src/click/doc/click.rst`` for an example.

The introductory paragraph is for describing what this code is trying to
model.

For consistency (italicized formatting), please use |ns3| to refer to
ns-3 in the documentation (and likewise, |ns2| for ns-2).  These macros
are defined in the file ``replace.txt``.

Model Description
*****************

The source code for the new module lives in the directory ``src/lorawan``.

The module models LoRaWAN networks with class A end-devices. Currently, class
B and C are not supported. A simple network server has been implemented in
order to acknowledge messages and model downstream traffic. The module stops
at the LoRaWAN MAC layer. LoRaWAN specification 1.0.2 was followed.

Design
======

.. Briefly describe the software design of the model and how it fits into
   the existing ns-3 architecture.

The module closely follows the design of the lr-wpan module in |ns3|. It
implements the LoRa PHY as part of the SpectrumPhy concept in |ns3|. The LoRa
PHY is modeled by means of BER curves and a chunk-based PER evaluation based
on the SINR of LoRa receptions. These BER curves were constructed by means of
baseband simulations in Matlab (see paper for more details).

                                                          +-----------------------------+
                                                          |     LoRaWANNetworkServer    |
                                                          +-----------------------------+
                                                                        | |
  +-----------------------------+                         +-----------------------------+
  | LoRaWANEndDeviceApplication |                         |  LoRaWANGatewayApplication  |
  +-----------------------------+                         +-----------------------------+
  +-----------------------------+                         +-----------------------------+
  |        LoRaWANNetDevice     |                         |        LoRaWANNetDevice     |
  +-----------------------------+                         +-----------------------------+
  +-----------------------------+                         +-----------------------------+
  |          LoRaWANMac         |                         |   ... |  LoRaWANMac  | ...  |
  +-----------------------------+                         +-----------------------------+
  +-----------------------------+     +-------------+     +-----------------------------+
  |          LoRaWANPhy         | <-> | SpectrumPhy | <-> |   ... |  LoRaWANPhy  | ...  |
  +-----------------------------+     +-------------+     +-----------------------------+

As in most |ns3| modules, traffic is exchanged between applications running on
nodes. For lorawan, there are applications for end devices and gateways.

The LoRaWANEndDeviceApplication periodically generates upstream traffic
according to a configurable period. Data can be sent as unconfirmed or
confirmed MAC messages. The packets created in the application are the payload
of MAC messages. There are various ways to pass meta data from the application
to the lower layers: the lorawan frame header, LoRaWANPhyParamsTag and
LoRaWANMsgTypeTag. See the LoRaWANEndDeviceApplication::SendPacket method
for more details.

The LoRaWANGatewayApplication passes received packets to the
LoRaWANNetworkServer. LoRaWANNetworkServer is a singleton class of which a
single instance is shared between all gateway applications. The network server
performs deduplication and generates Acks if necessary. It also selects
gateways for sending downstream messages. LoRaWANNetworkServer is also
responsible for tracking and scheduling downstream retransmissions.

LoRaWANMac contains a packet buffer where MAC messages are stored (m_txQueue).
When the MAC object is in the IDLE state and the node has radio time
available, the MAC object will initiate a transmission. The first step
involves scheduling a state change to the MAC_TX state. As part of this state
change the MAC object configures the Phy with the correct parameters (SF,
power, channel) for transmission and sends a request to the PHY layer to
switch its transceiver on (LoRaWANPhy::SetTRXStateRequest (LORAWAN_PHY_TX_ON));
The Phy will call LoRaWANMac::SetTRXStateConfirm (LoRaWANPhyEnumeration
status) to inform the MAC object of the status of the MAC's request. If
everything went ok, then the Phy will report LORAWAN_PHY_TX_ON as the status
to the MAC layer. After receiving this response, the MAC layer will call
PdDataRequest on the Phy layer. Here the first packet in the queue is used.
If the Phy layer is in the LORAWAN_PHY_TX_ON state, then it will start
transmitting by calling StartTx on the SpectrumChannel object. The Phy layer
changes its state to LORAWAN_PHY_BUSY_TX and schedules a call to EndTx when
the transmission ends. EndTx will report the status of transmission to the MAC
layer via LoRaWANMac::PdDataConfirm. This prompts the MAC layer to call its
m_dataConfirmCallback in case of an unconfirmed message and to remove a packet
from the queue if it has reached zero remaining transmissions.
m_dataConfirmCallback reports the succesful transmission of UNC messages to
upper layers. For CON messages m_dataConfirmCallback will report the successful
transmission of message only when the MAC object has received an
Acknowledgement. Finally, PdDataConfirm will also configure the state of the
MAC and Phy layer after the transmission has ended.

For receiving packets, the Phy layer has to be in the LORAWAN_PHY_RX_ON state
and should be configured for the same channel and data rate as the
transmission (see LoRaWANPhy::StartRx). In case of a different channel, StartRX
does not update the interference. In case of a different spreading factor,
LoRaWANPhy does update the interference but does not continue to try and receive
the packet (seen as noise, keep Phy free for another reception at the correct
SF). Next, StartRx checks whether the SINR of the reception is not below the
cut-off of the error model. If it is higher than the cut-off, then the Phy
starts the reception of the packet by changing its state to the BUSY_RX. Now,
every time the interference level changes (i.e.  a new transmission starts or
an existing transmission ends during the ongoing reception), the Phy object will
determine the probability that a chunk at continuous SINR was received successfully
up until the change in the interference level. This all happens in
LoRaWANPhy::CheckInterference.  The same happens one more time at the end of
the reception. If an ongoing reception is destroyed due to interference, then
m_currentRxPacket.second.destroyed will be set to true which allows the Phy to
drop the packet in EndRx and call the corresponding trace sources. In case a
packet was successfully received, then the Phy object will call
m_pdDataIndicationCallback which calls LoRaWANMac::PdDataIndication. The MAC
layer checks for acknowledgments and removes packets from the queue if
necessary. The MAC layer will also inform higher layers of the packet
reception via m_dataIndicationCallback. Normally this calls
LoRaWANNetDevice::DataIndication which will in turn call its m_receiveCallback.
This will call the PacketSocket's ForwardUp method, which will eventually call
the RecvCallback of the PacketSocket which is set to
LoRaWANEndDeviceApplication::HandleRead for end devices and
LoRaWANGatewayApplication::HandleRead for gateways.

Scope and Limitations
=====================

.. What can the model do?  What can it not do?  Please use this section to
   describe the scope and limitations of the model.

The error model has not experimentally validated. One can ask whether modelling
interference as noise is valid for LoRa. My personal opinion is that the SINR
approach likely underestimates the impact of interference and that therefore
the simulation of interference-limited scenarios have a tendency to be too
positive.  Note that this is solely based on personal opinion/feeling.
Verification would be necessary here.

Depending on the traffic load, networks with hundreds or thousands of end
devices and one or more gateways can be simulated. These end devices may send
either confirmed or unconfirmed messages. The network server supports
downstream acknowledgements and downstream data, both confirmed and
unconfirmed.

One limitation is that high event rate simulations take a long time to run.
Such simulations either consist of a high number of end devices sending at a
small to moderate rate OR smaller networks with higher rates (uncommon in
LoRaWAN). The simulations in the paper with 10 000 end devices sending every
600 seconds took a long time to complete (i.e. 2 days on our virtual wall
infrastructure). Note that these were single channel network simulations.

Currently not modelled:
- Class B, C end devices.
- Frequency hopping between subsequent transmissions.


References
==========

.. Add academic citations here, such as if you published a paper on this
   model, or if readers should read a particular specification or other work.

Paper submitted to IEEE Internet of Things journal, pending revision.

Usage
*****

This section is principally concerned with the usage of your model, using
the public API.  Focus first on most common usage patterns, then go
into more advanced topics.

Building New Module
===================

Include this subsection only if there are special build instructions or
platform limitations.

Helpers
=======

What helper API will users typically use?  Describe it here.

Attributes
==========

What classes hold attributes, and what are the key ones worth mentioning?

Output
======

What kind of data does the model generate?  What are the key trace
sources?   What kind of logging output can be enabled?

Advanced Usage
==============

Go into further details (such as using the API outside of the helpers)
in additional sections, as needed.

Examples
========

What examples using this new code are available?  Describe them here.

Troubleshooting
===============

Add any tips for avoiding pitfalls, etc.

Validation
**********

Describe how the model has been tested/validated.  What tests run in the
test suite?  How much API and code is covered by the tests?  Again, 
references to outside published work may help here.
