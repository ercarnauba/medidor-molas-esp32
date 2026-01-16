# Changelog

## Fixes
- Move encoder debounce out of ISR to avoid unsafe `millis()` calls in interrupt context.
- Add 20s timeout to stepper `homeToEndstop()` and only reset position when homing succeeded.

## Docs
- Added `ARCHITECTURE.md` (system diagram and flow)
- Added `CODE_REVIEW.md` (issues found and recommendations)

## How to apply
See `README.md` in this directory.
