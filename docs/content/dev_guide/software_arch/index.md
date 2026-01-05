<a id="sw-overview"></a>

# Software Architecture

A primer on the O-RAN gNB architecture has already been outlined in the Knowledge Base, this can be found [here](../../../../knowledge_base/source/oran_gnb/source/index.md#oran-gnb-overview).

This documentation aims to outline how this architecture is implemented srsRAN Project codebase. The function and implementation of each component will be discussed in subsequent sections.

![image](dev_guide/source/software_arch/source/.imgs/code_map.jpg)

The code implements all of the components and interfaces in the above diagram. All of these elements have been implemented in software and are fully performant,
customizable and compliant with the O-RAN standard. Users can also integrate 3rd-party RICs, RUs, and gNB components with srsRAN Project components.

---

# Software Architecture:

* [CU-CP](CU_cp/source/index.md)
* [CU-UP](CU_up/source/index.md)
* [DU-high](DU_high/source/index.md)
* [DU-low](DU_low/source/index.md)
