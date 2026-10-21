# State machine diagrams

State names and codes match `wm_machine.cpp`, `sm_machine.cpp` and `uv_machine.cpp`. WM is fully implemented; SM and UV are skeletons (see the TODO comments in each file).

## WM (implemented)

```mermaid
stateDiagram-v2
  [*] --> IDLE: boot (all outputs off)
  IDLE --> RUN: short press SW1 / START
  RUN --> IDLE: t_run elapsed (2 beeps, S line OK)
  RUN --> ERROR: short press or ABORT (E-ABORT)
  RUN --> ERROR: longer than t_run+30 s (E-WM1)
  IDLE --> MANUAL: long press SW1 >= 1 s / PUMP command
  MANUAL --> IDLE: release / time up
  ERROR --> IDLE: short press SW1 acknowledges
```

## SM (skeleton)

```mermaid
stateDiagram-v2
  [*] --> S0
  S0 --> S1: self-test passed
  S1 --> S2: cup mass 5-15 g stable for 3 s
  S2 --> S3: soil 2.0-3.0 g (else E1)
  S3 --> S4: dosing done (overdose or timeout E3)
  S4 --> S5: t_ext elapsed (E2)
  S5 --> S6: settling 5 min
  S6 --> S7: SW1 (later PULLED) (E6 on timeout)
  S7 --> S8: pulled mass (below 10 g E7)
  S8 --> S1: SW1
  S0 --> ERROR: long press SW1 >= 2 s / ABORT
  ERROR --> S0: short press acknowledges
```

## UV (skeleton)

```mermaid
stateDiagram-v2
  [*] --> U0
  U0 --> U1: self-test (E-UV1 door)
  U1 --> U2: quartz tube in place (E-UV4)
  U2 --> U3: filtrate 8-12 g (E-UV5)
  U3 --> U4: SW1
  U4 --> U5: H2O2 1/9 (E-UV6)
  U5 --> U6: t_uv elapsed (E-UV2, E-UV3)
  U6 --> U7: enzyme + buffer, stir 5 min, settle 1 min
  U5 --> ERROR: door open / SW2 (DIM low first, then relay off)
```
