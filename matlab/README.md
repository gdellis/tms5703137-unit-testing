# Simulink Test scripts (unverified sketch)

Read [docs/04-simulink-test.md](../docs/04-simulink-test.md) first. Both `.m` files
here were written without access to MATLAB and have never been run - they are a
starting skeleton for an optional, separately-licensed testing track, not a verified
tool. Calls flagged `VERIFY` in comments are the ones most likely to need adjusting
for your MATLAB release.

- `create_heater_ctrl_tests.m` - scaffolds a Test Manager file with baseline test
  cases mirroring four scenarios from `test/test_heater_ctrl.c`. Needs a real
  `heater_ctrl.slx` on the path, matching the interface in the doc's section 3; this
  repo does not ship one.
- `run_tests_ci.m` - headless runner for CI: loads a test file, runs it, exports
  JUnit XML and a report, exits non-zero on failure. Used by
  `.github/workflows/simulink-test.yml`, which needs a self-hosted runner with a
  licensed MATLAB/Simulink/Simulink Test install and is not part of the required CI
  checks.
