![logo](.imgs/logo.png)

# OCUDU 5G Enterprise Plugin

Check the full docs here: https://docs.srs.io/en/latest

This plugin will be automatically included in your OCUDU compilation. A line like the following is expected in the build output:

```bash
-- Adding plugin: plugins/enterprise
```

You can disable it by adding `-DENABLE_PLUGINS=False` to the cmake command line.

Please read following sections to know which flags are required for each feature.

## BBDEV library build

Among others, OCUDU 5G Enterprise provides support for BBDEV-based hardware-accelerators, such as the ACC100. BBDEV is part of DPDK and use of version 23.11 is recommended with hardware-accelerated OCUDU DUs. To build the libraries implementing the requied functions to use a BBDEV-based accelerator, hence, use DPDK needs to be enabled by adding `-DENABLE_DPDK=True` to the cmake command line. The following lines are expected in the build output:

```bash
-- BBDEV hardware-acceleration enabled for PDSCH.
-- BBDEV hardware-acceleration enabled for PUSCH.
```
