.. SPDX-License-Identifier: GFDL-1.1-no-invariants-or-later
.. _v4l2-meta-fmt-bcm2835-isp-stats:

+*****************************************
V4L2_META_FMT_BCM2835_ISP_STATS  ('BSTA')
*****************************************

BCM2835 ISP Statistics

Description
===========

The BCM2835 ISP hardware calculate image statistics for an input Bayer frame.
These statistics are obtained from the "bcm2835-isp0-capture3" device node
using the :c:type:`v4l2_meta_format` interface. They are formatted as described
by the :c:type:`bcm2835_isp_stats` structure below.

.. code-block:: c

       #define DEFAULT_AWB_REGIONS_X 16
       #define DEFAULT_AWB_REGIONS_Y 12

       #define NUM_HISTOGRAMS 2
       #define NUM_HISTOGRAM_BINS 128
       #define AWB_REGIONS (DEFAULT_AWB_REGIONS_X * DEFAULT_AWB_REGIONS_Y)
       #define FLOATING_REGIONS 16
       #define AGC_REGIONS 16
       #define FOCUS_REGIONS 12

.. kernel-doc:: include/uapi/linux/bcm2835-isp.h
   :functions: bcm2835_isp_stats_hist bcm2835_isp_stats_region
                    bcm2835_isp_stats_focus bcm2835_isp_stats
