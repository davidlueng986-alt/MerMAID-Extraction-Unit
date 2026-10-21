# MerMAID Extraction Unit

**Open-source hardware and firmware** for a portable soil/water mercury (Hg) detection system, developed as part of an iGEM project.

This repository is actively maintained on GitHub. Contributions, issues, and pull requests are welcome.

## Overview

MerMAID (Mercury Modular Automated Integrated Device) Extraction Unit is a modular, low-cost system designed for field-deployable mercury detection in environmental samples (soil and water). It uses a whole-cell biosensor approach with optical readout, combined with sample conditioning (L-Cysteine / UV-AOP).

The hardware is organised into three main modules:

- **WM** — Water Module (filtration / extraction)
- **SM** — Soil Module (weighing, dosing, stirring)
- **UV** — UV treatment module (H₂O₂ / irradiation)

## Firmware

The firmware lives in the [`firmware/`](firmware/) directory and is a PlatformIO project targeting ESP32.

- Environments: `wm`, `sm`, `uv` (plus `*_fi` variants for fault-injection testing)
- Water module (WM) is the most complete
- Soil and UV modules are currently skeletons with state machines defined

See [`firmware/README.md`](firmware/README.md) for build instructions, state machines, and current status.

## Repository Structure

```
.
├── firmware/          # ESP32 PlatformIO firmware
├── tools/             # Code generation and test helpers
├── bom/               # Bill of Materials (from previous repo)
├── cad/               # CAD / Blender models
├── docs/              # Design documentation
├── electrical/        # Schematics / nets
└── README.md
```

## Open Source & iGEM

This project is released as open source for the iGEM community. The design, firmware, and documentation are freely available for educational, research, and non-commercial use. Please cite the original iGEM team if you build upon this work.

Maintained by the team on GitHub: [MerMAID-Extraction-Unit](https://github.com/davidlueng986-alt/MerMAID-Extraction-Unit)

## Status

Early prototype stage. Firmware for the water module is functional; soil and UV modules require further implementation. Hardware documentation and BOM are available from the original MercurSense repository.

---

*Originally developed as MercurSense hardware. Renamed and continued as MerMAID Extraction Unit for iGEM.*

## License

This project is licensed under the [Creative Commons Attribution 4.0 International License](https://creativecommons.org/licenses/by/4.0/) (CC BY 4.0).

You are free to share and adapt the material for any purpose, provided you give appropriate credit to the MerMAID Extraction Unit / iGEM contributors.
