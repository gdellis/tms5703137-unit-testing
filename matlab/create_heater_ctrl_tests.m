function create_heater_ctrl_tests(outputFile)
%CREATE_HEATER_CTRL_TESTS Scaffold a Simulink Test file for the heater_ctrl model.
%
%   create_heater_ctrl_tests('heater_ctrl_tests.mldatx')
%
% UNVERIFIED: written without access to MATLAB (see docs/04-simulink-test.md).
% Every sltest.testmanager call below is real, documented API, but exact property
% names/values (marked VERIFY) are known to shift between MATLAB releases and could
% not be checked here. Run this once interactively, inspect the result in the Test
% Manager GUI, and fix anything the release disagrees with before trusting it in CI.
%
% Assumes a model 'heater_ctrl' is on the MATLAB path with the interface and
% parameters documented in docs/04-simulink-test.md section 3:
%   inports  temp_degC (single), enable (boolean)
%   outports heater_cmd (boolean), fault (boolean)
%   params   Setpoint_degC=40, Hysteresis_degC=2, OverTemp_degC=60,
%            FaultDebounce_steps=3
% This repo ships no such model - src/gen/heater_ctrl_ert_rtw/ is hand-written C
% standing in for what one would generate. Build the model first.
%
% Covers four of the test_heater_ctrl.c scenarios as baseline test cases (record the
% model's own output as truth). Promote each to an equivalence test against SIL/PIL
% once the model builds - see docs/04-simulink-test.md section 4.

    arguments
        outputFile (1,1) string = "heater_ctrl_tests.mldatx"
    end

    modelName = "heater_ctrl";
    if ~bdIsLoaded(modelName)
        load_system(modelName);
    end

    tf = sltest.testmanager.TestFile(outputFile);
    suite = tf.createTestSuite("Hysteresis and fault behavior");

    % Sample time must match the model's; every scenario below is expressed in
    % samples, exactly as test_heater_ctrl.c's step_n() counts them.
    Ts = 0.1; %#ok<NASGU> % VERIFY: match the model's configured sample time

    addBaselineCase(suite, modelName, "Heater turns on at lower threshold", ...
        rampInput(30.0, 39.0, 10, Ts), true, ...
        "temp_degC held at the enable-side edge (39.0 degC): heater_cmd must be true. " + ...
        "Mirrors test_heater_turns_on_at_lower_threshold in test_heater_ctrl.c.");

    addBaselineCase(suite, modelName, "Heater turns off at upper threshold", ...
        rampInput(30.0, 41.0, 20, Ts), false, ...
        "Ramp through the hysteresis band to 41.0 degC: heater_cmd must be false. " + ...
        "Mirrors test_heater_turns_off_at_upper_threshold.");

    addBaselineCase(suite, modelName, "Fault only after debounce", ...
        constantInput(61.0, 3, Ts), true, ...
        "temp_degC held over OverTemp_degC for exactly FaultDebounce_steps samples: " + ...
        "fault must be true only on the last one. Mirrors " + ...
        "test_overtemp_faults_only_after_debounce - check the intermediate samples " + ...
        "in the Test Manager signal viewer, not just the final value.");

    addBaselineCase(suite, modelName, "Overtemp counter saturates without wrapping", ...
        constantInput(61.0, 300, Ts), true, ...
        "300 samples over threshold against a uint8 debounce counter: fault must " + ...
        "still be true, not cleared by a wrapped counter. Mirrors " + ...
        "test_overtemp_counter_saturates_without_wrapping - the coverage report " + ...
        "found this gap in the C in the first pass; the same gap is worth checking " + ...
        "at the model level.");

    tf.save();
    fprintf("Wrote %s with %d test case(s).\n", outputFile, numel(suite.getTestCases()));
    fprintf("Open in Test Manager, run once in Normal mode to record the baseline, ");
    fprintf("then switch each case's System Under Test to SIL and re-run as equivalence.\n");
end

function sig = rampInput(fromDegC, toDegC, nSamples, Ts)
    t = (0:nSamples-1)' * Ts;
    sig = timeseries(single(linspace(fromDegC, toDegC, nSamples))', t);
end

function sig = constantInput(degC, nSamples, Ts)
    t = (0:nSamples-1)' * Ts;
    sig = timeseries(single(repmat(degC, nSamples, 1)), t);
end

function addBaselineCase(suite, modelName, name, tempSignal, expectFinalTrue, description)
% VERIFY: createTestCase's first argument (test case type) and TestCase property
% names are the calls most likely to need adjusting for your release - this follows
% the documented "Baseline Testing" scripting workflow but the exact strings were
% not checked against a running MATLAB.
    tc = suite.createTestCase("baseline", name);
    tc.setProperty("Model", modelName);
    tc.Description = description;

    enableSignal = timeseries(true(tempSignal.Length, 1), tempSignal.Time);

    inputData = Simulink.SimulationData.Dataset;
    inputData = inputData.addElement(tempSignal, "temp_degC");
    inputData = inputData.addElement(enableSignal, "enable");
    tc.setProperty("InputData", inputData); % VERIFY property name for your release

    % A baseline case records outputs on first run; expectFinalTrue documents the
    % intent for a human reviewing the scaffold, it is not asserted here. After the
    % first Normal-mode run, add an explicit Test Assessment (Simulink Test ->
    % Logical and Temporal Assessments) checking the final value of heater_cmd or
    % fault, so a future SIL/PIL regression is caught even if equivalence checking
    % is loosened.
    if expectFinalTrue
        tc.Description = tc.Description + " Expect the final sample to read true.";
    end
end
