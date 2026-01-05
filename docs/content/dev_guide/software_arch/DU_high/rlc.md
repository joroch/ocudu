<a id="rlc-overview"></a>

# Radio Link Control (RLC)

RLC layer connects the MAC and the F1AP/PDCP, as shown in [Fig. 5](#rlcstack), and provides that transfer services to these layers.

<a id="rlcstack"></a>
![image](dev_guide/source/software_arch/source/DU_high/source/.imgs/rlc_stack.png)

It does this, by providing three different modes:

> * **Transparent Mode (TM)**

> > TM is used only in control signaling (SRB0 only) and provides data transfer services without modifying the SDUs/PDUs at all.
> * **Unacknowledged Mode (UM)**

> > UM can only be used in data traffic (DRBs only) and provides data transfer services with segmentation/reassembly. It is usually used in delay-sensitive and loss-tolerant traffic, as it does not provide re-transmissions.
> * **Acknowledged Mode (AM)**

> > [AM](rlc_am.md#rlc-am) can be used in data and control traffic (mandatory for SRBs, optional for DRBs) and provides data transfer services with segmentation and ARQ procedures. This mode is usually used for traffic that is more loss-sensitive, but more delay-tolerant.

---

* [RLC Acknowledged Mode (AM)](rlc_am.md)

<!-- Add to TOCTREE when ready
rlc_tm.rst
rlc_um.rst -->
