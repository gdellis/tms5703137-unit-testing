/*
 * File: heater_ctrl_data.c
 *
 * HAND-WRITTEN STAND-IN mimicking Simulink Embedded Coder output for model
 * 'heater_ctrl'. See heater_ctrl.h. Do not edit generated files.
 *
 * Target selection: ert.tlc
 * Embedded hardware selection: ARM Compatible->ARM Cortex-R
 * Code generation objectives: Unspecified
 * Validation result: Not run
 */
#include "heater_ctrl.h"

/* Block parameters (default storage) */
P_heater_ctrl_T heater_ctrl_P = {
  /* Variable: Setpoint_degC
   * Referenced by: '<S1>/Hysteresis'
   */
  40.0F,

  /* Variable: Hysteresis_degC
   * Referenced by: '<S1>/Hysteresis'
   */
  2.0F,

  /* Variable: OverTemp_degC
   * Referenced by: '<S1>/OverTemp Compare'
   */
  60.0F,

  /* Variable: FaultDebounce_steps
   * Referenced by: '<S1>/OverTemp Debounce'
   */
  3U
};
