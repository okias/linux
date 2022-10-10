/* SPDX-License-Identifier: GPL-2.0 */
/*
 * qcom,spmi-haptics.h - Device tree bindings for Qualcomm SPMI haptics
 *
 * Provide macro names used in DTS for wave shapes and play modes so
 * the DTS includes resolve during dtc compilation.
 */

#ifndef __DT_BINDINGS_INPUT_QCOM_SPMI_HAPTICS_H
#define __DT_BINDINGS_INPUT_QCOM_SPMI_HAPTICS_H

/* Wave shapes */
#define HAP_WAVE_SINE      0
#define HAP_WAVE_SQUARE    1
#define HAP_WAVE_TRIANGLE  2
#define HAP_WAVE_SAWTOOTH  3

/* Play modes */
#define HAP_PLAY_DIRECT    0
#define HAP_PLAY_BUFFER    1
#define HAP_PLAY_AUDIO     2
#define HAP_PLAY_PWM       3

#endif /* __DT_BINDINGS_INPUT_QCOM_SPMI_HAPTICS_H */
