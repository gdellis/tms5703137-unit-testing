# HALCoGen project goes here (not committed)

Generate a HALCoGen project for the **TMS570LS3137ZWT** into this directory so that
`target/halcogen/include/` and `target/halcogen/source/` exist. Only the SCI driver
needs to be enabled. Full instructions: [docs/03-on-target.md](../../docs/03-on-target.md).

Everything in this directory except this file is git-ignored: HALCoGen output is
regenerated, not edited, and the TMS570 project you copy this template into already
has its own.
