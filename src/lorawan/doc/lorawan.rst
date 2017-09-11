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

Briefly describe the software design of the model and how it fits into
the existing ns-3 architecture.

The module closely follows the design of the lr-wpan module in |ns3|. It
implements the LoRa PHY as part of the SpectrumPhy concept in |ns3|. The LoRa
PHY is modeled by means of BER curves and a chunk-based PER evaluation based
on the SINR of LoRa receptions. These BER curves were constructed by means of
baseband simulations in Matlab (see paper for more details).

Scope and Limitations
=====================

What can the model do?  What can it not do?  Please use this section to
describe the scope and limitations of the model.

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

References
==========

Add academic citations here, such as if you published a paper on this
model, or if readers should read a particular specification or other work.

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
