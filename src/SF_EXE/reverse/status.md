# Reconstruction status labels

- `identified`: the address and broad purpose are known.
- `analyzed`: control flow and important side effects are understood.
- `partial`: some corresponding portable behavior is implemented, but it is
  not complete enough for a fidelity claim.
- `implemented`: the portable behavior is complete enough for focused tests.
- `verified`: behavior has passed an original-versus-reconstruction test with
  representative retail data or event sequences.

Addresses describe the retail executable only. The portable executable does
not try to preserve internal addresses, compiler-generated class layouts, or
operating-system handles.
