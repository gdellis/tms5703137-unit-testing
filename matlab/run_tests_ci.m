function run_tests_ci(testFile)
%RUN_TESTS_CI Headless entry point for CI: run a Simulink Test file, export
% results, fail the MATLAB process on any test failure.
%
%   matlab -batch "run_tests_ci('heater_ctrl_tests.mldatx')"
%
% UNVERIFIED: see docs/04-simulink-test.md. sltest.testmanager.load/run and
% resultsToJUnitFormat are real, documented Simulink Test functions, specifically
% intended for CI use, but the exact call has not been exercised here - this sandbox
% has no MATLAB. Run it once locally before trusting it in .github/workflows/
% simulink-test.yml.
%
% Exit codes: 0 all tests passed, 1 one or more failed, 2 the run itself errored
% (missing file, model failed to build, licence checkout failed, ...).

    arguments
        testFile (1,1) string = "heater_ctrl_tests.mldatx"
    end

    if ~isfile(testFile)
        fprintf(2, "run_tests_ci: no such test file: %s\n", testFile);
        exit(2);
    end

    try
        tf = sltest.testmanager.load(testFile);
        resultObj = sltest.testmanager.run(tf);
    catch err
        fprintf(2, "run_tests_ci: test run failed to execute: %s\n", err.message);
        exit(2);
    end

    % VERIFY: exact accessor for pass/fail on your release - getTestFileResults(),
    % .Status, and .getOutcome() have all been valid at different points; check
    % Simulink Test's "Results and Artifacts" API doc page for your version.
    passed = resultObj.getPassFailStatus();

    junitPath = "CIResults-JUnit.xml";
    try
        sltest.testmanager.resultsToJUnitFormat(resultObj, junitPath);
        fprintf("Wrote %s\n", junitPath);
    catch err
        fprintf(2, "run_tests_ci: JUnit export failed (non-fatal): %s\n", err.message);
    end

    reportPath = "CIResults-report.pdf";
    try
        sltest.testmanager.report(resultObj, reportPath, ...
            "Title", "heater_ctrl Simulink Test results");
        fprintf("Wrote %s\n", reportPath);
    catch err
        fprintf(2, "run_tests_ci: report export failed (non-fatal): %s\n", err.message);
    end

    if strcmpi(passed, "Passed")
        fprintf("All tests passed.\n");
        exit(0);
    else
        fprintf(2, "One or more tests failed or were incomplete: %s\n", passed);
        exit(1);
    end
end
