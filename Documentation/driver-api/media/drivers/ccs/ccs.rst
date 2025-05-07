.. SPDX-License-Identifier: GPL-2.0-only OR BSD-3-Clause

.. include:: <isonum.txt>

.. _media-ccs-driver:

MIPI CCS camera sensor driver
=============================

The MIPI CCS camera sensor driver is a generic driver for `MIPI CCS
<https://www.mipi.org/specifications/camera-command-set>`_ compliant
camera sensors.

Also see :ref:`the CCS driver UAPI documentation <media-ccs-uapi>`.

CCS static data
---------------

Sensor identification
---------------------

Normally CCS sensor and module identification information can be all obtained
from the sensor's identification registers. In case these registers aren't
programmed to the hardware, CCS compliant sensors may also be identified based
on identification information in DT firmware. In this case the information from
DT will override the sensor and module identification information present in the
sensor's registers. The the following properties are used for the purpose:

- ``mipi,sensor-vendor-id``
- ``mipi,sensor-device-id``
- ``mipi,sensor-revision``
- ``mipi,module-vendor-id``
- ``mipi,module-device-id``
- ``mipi,module-revision``

Please refer to mipi-ccs DT bindings for more information.

Loading CCS static data based on identification registers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The MIPI CCS driver supports CCS static data for all compliant devices,
including not just those compliant with CCS 1.1 but also CCS 1.0 and SMIA(++).
For CCS the file names are formed as

	``ccs/ccs-sensor-vvvv-mmmm-rrrr.fw`` (sensor) and
	``ccs/ccs-module-vvvv-mmmm-rrrr.fw`` (module).

For SMIA++ compliant devices the corresponding file names are

	``ccs/smiapp-sensor-vv-mmmm-rr.fw`` (sensor) and
	``ccs/smiapp-module-vv-mmmm-rrrr.fw`` (module).

For SMIA (non-++) compliant devices the static data file name is

	``ccs/smia-sensor-vv-mmmm-rr.fw`` (sensor).

``vvvv`` or ``vv`` denotes MIPI and SMIA manufacturer IDs respectively, ``mmmm``
model ID and ``rrrr`` or ``rr`` revision number. These are always two or four
characters long, depending on the string denoting them above.

Loading CCS static data without identification information
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CCS static data may be loaded based on the sensor's compatible string (sensor)
and the module identification information documented in the mipi-ccs DT
bindings. The file names in these cases are, respectively

	``ccs/compatible/sensor-S{,-revision}.fw`` (sensor),
	``ccs/canonical/module-M{,-revision}.fw`` (module) and
	``ccs/non-canonical/module-nnnn[,-revision}.fw`` (module).

Above, ``S`` is the sensor's DT compatible string that differs from the CCS or
SMIA compatible strings for standard-compliant devices. ``M`` is the canonical
module identification string and four characters long ``nnnn`` is the module
identifier for non-canonically named modules. See
Documentation/devicetree/bindings/media/camera-module.yaml for module
identification.

If the revision of the device is non-zero, then the revision number is appended
to the file name before the dot, in lower case hexadecimal form (without 0x
prefix). Should no file with the revision exist, a name without the revision is
loaded as a fallback.

CCS tools
~~~~~~~~~

`CCS tools <https://github.com/MIPI-Alliance/ccs-tools/>`_ is a set of
tools for working with CCS static data files. CCS tools includes a
definition of the human-readable CCS static data YAML format and includes a
program to convert it to a binary.

Register definition generator
-----------------------------

The ccs-regs.asc file contains MIPI CCS register definitions that are used
to produce C source code files for definitions that can be better used by
programs written in C language. As there are many dependencies between the
produced files, please do not modify them manually as it's error-prone and
in vain, but instead change the script producing them.

Usage
~~~~~

Conventionally the script is called this way to update the CCS driver
definitions:

.. code-block:: none

	$ Documentation/driver-api/media/drivers/ccs/mk-ccs-regs -k \
		-e drivers/media/i2c/ccs/ccs-regs.h \
		-L drivers/media/i2c/ccs/ccs-limits.h \
		-l drivers/media/i2c/ccs/ccs-limits.c \
		-c Documentation/driver-api/media/drivers/ccs/ccs-regs.asc

CCS PLL calculator
==================

The CCS PLL calculator is used to compute the PLL configuration, given sensor's
capabilities as well as board configuration and user specified configuration. As
the configuration space that encompasses all these configurations is vast, the
PLL calculator isn't entirely trivial. Yet it is relatively simple to use for a
driver.

The PLL model implemented by the PLL calculator corresponds to MIPI CCS 1.1.

.. kernel-doc:: drivers/media/i2c/ccs-pll.h

**Copyright** |copy| 2020, 2025 Intel Corporation
