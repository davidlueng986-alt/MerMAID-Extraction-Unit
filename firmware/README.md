# Firmware

PlatformIO project (`platformio.ini`, platform version pinned). Environments: `wm`, `sm`, `uv`, and `wm_fi`, `sm_fi`, `uv_fi` for error-detection tests only (see below).

| Directory | Content |
|---|---|
| `common/` | Log lines (H/D/E/S), command parser, fault injection, ESP32 helpers; `errors.yaml` -> `error_codes.h`; `pins_*.h` generated from `electrical/nets.yaml` |
| `wm/` | Water module: **fully implemented** WM-R01..R07 (R08, UART commands, not done) |
| `sm/`, `uv/` | **Skeletons**: all outputs off at boot (C-S01), state list, abort handling; behaviour inside the states is `TODO(SM-Rxx)` / `TODO(UV-Rxx)` |
| `test/` | Desktop unit tests (`tools/run_native_tests.sh`) |

## Generated files (rerun after changing the source)

```sh
python3 tools/gen_errors.py   # errors.yaml -> error_codes.h + docs/manual/error_codes.md
python3 tools/gen_pins.py     # electrical/nets.yaml -> pins_*.h
```

## Verification

```sh
tools/run_native_tests.sh     # desktop tests (normal and fault-injection builds) + log2csv test
cd firmware && pio run        # compiles wm, sm, uv
```

## Fault injection

Compiled in only in the `*_fi` environments (`-DENABLE_FAULT_INJECTION=1`); in normal environments an `FI ...` command only answers `ERR,FI_DISABLED` and changes nothing. Only `FI PUMP_TIMEOUT` is wired up so far (WM: disables the completion check so the C-S03 timeout stops the pump and reports E-WM1). `SCALE_DISCONNECT`, `SCALE_UNSTABLE`, `ENCODER_STALL`, `DOOR_OPEN` and `TEMP_HIGH` are named but wait for the SM/UV logic.

## Status

- **Not done**: SM and UV state logic; drivers for HX711, encoder, DS18B20, OLED, MLX90614; calibration flows beyond NVS; UART2 commands.
- **Not run on hardware**: WM passes compilation and desktop tests only; the power-up acceptance checks (B01-B09, B-W01, B-W02 in `11_assembly_and_firmware_requirements.md` section 8) are still to be ticked off one by one.
- **New error code**: `E-WM1` (pump timeout, C-S03) is added by the firmware and marked "to be written into the plan". On a WM timeout the S line result is `ERR` (the spec lists only OK and ABORT); decide whether to change the spec.
- `tools/log2csv.py` splits a serial log into `_D.csv`, `_E.csv`, `_S.csv`, `_H.txt`.
- Tag the repo before every formal test; `tools/fw_version.py` writes the commit into the H line as `FW_COMMIT` (`-dirty` if the working tree has uncommitted changes).
