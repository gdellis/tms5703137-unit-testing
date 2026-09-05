/*
 * File: heater_ctrl.c
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

/* Block states (default storage) */
DW_heater_ctrl_T heater_ctrl_DW;

/* External inputs (root inport signals with default storage) */
ExtU_heater_ctrl_T heater_ctrl_U;

/* External outputs (root outports fed by signals with default storage) */
ExtY_heater_ctrl_T heater_ctrl_Y;

/* Real-time model */
static RT_MODEL_heater_ctrl_T heater_ctrl_M_;
RT_MODEL_heater_ctrl_T *const heater_ctrl_M = &heater_ctrl_M_;

/* Model step function */
void heater_ctrl_step(void)
{
  boolean_T rtb_heater_cmd;

  /* Outputs for Enabled SubSystem: '<Root>/Controller' incorporates:
   *  EnablePort: '<S1>/Enable'
   *  Inport: '<Root>/enable'
   */
  if (heater_ctrl_U.enable) {
    /* Switch: '<S1>/OverTemp Debounce' incorporates:
     *  Inport: '<Root>/temp_degC'
     *  RelationalOperator: '<S1>/OverTemp Compare'
     *  Sum: '<S1>/Count Up'
     *  UnitDelay: '<S1>/Count Delay'
     */
    if (heater_ctrl_U.temp_degC > heater_ctrl_P.OverTemp_degC) {
      if (heater_ctrl_DW.OverTemp_count < heater_ctrl_P.FaultDebounce_steps) {
        heater_ctrl_DW.OverTemp_count++;
      }
    } else {
      heater_ctrl_DW.OverTemp_count = 0U;
    }

    /* Logic: '<S1>/Fault Latch' incorporates:
     *  RelationalOperator: '<S1>/Debounce Reached'
     */
    if (heater_ctrl_DW.OverTemp_count >= heater_ctrl_P.FaultDebounce_steps) {
      heater_ctrl_DW.Fault_latched = true;
    }

    /* Relay: '<S1>/Hysteresis' incorporates:
     *  Inport: '<Root>/temp_degC'
     */
    if (heater_ctrl_U.temp_degC >= heater_ctrl_P.Setpoint_degC +
        heater_ctrl_P.Hysteresis_degC * 0.5F) {
      heater_ctrl_DW.Relay_Mode = false;
    } else if (heater_ctrl_U.temp_degC <= heater_ctrl_P.Setpoint_degC -
               heater_ctrl_P.Hysteresis_degC * 0.5F) {
      heater_ctrl_DW.Relay_Mode = true;
    } else {
      /* inside the band: hold previous mode */
    }

    /* Logic: '<S1>/Heater Allowed' */
    rtb_heater_cmd = (boolean_T)(heater_ctrl_DW.Relay_Mode &&
      (!heater_ctrl_DW.Fault_latched));
  } else {
    /* Disable for Enabled SubSystem: '<Root>/Controller' (states reset) */
    heater_ctrl_DW.OverTemp_count = 0U;
    heater_ctrl_DW.Relay_Mode = false;
    heater_ctrl_DW.Fault_latched = false;
    rtb_heater_cmd = false;
  }

  /* End of Outputs for SubSystem: '<Root>/Controller' */

  /* Outport: '<Root>/heater_cmd' */
  heater_ctrl_Y.heater_cmd = rtb_heater_cmd;

  /* Outport: '<Root>/fault' */
  heater_ctrl_Y.fault = heater_ctrl_DW.Fault_latched;
}

/* Model initialize function */
void heater_ctrl_initialize(void)
{
  /* Registration code */

  /* initialize error status */
  rtmSetErrorStatus(heater_ctrl_M, (NULL));

  /* (Block states, inputs and outputs are zero-initialised by the C start-up code:
   *  ERT option "Remove internal data zero initialization" is on, which is the
   *  default. Host tests must therefore clear heater_ctrl_DW/U/Y themselves.) */
}

/* Model terminate function */
void heater_ctrl_terminate(void)
{
  /* (no terminate code required) */
}
