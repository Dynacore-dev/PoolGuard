# PoolDosingController (KiCad hardware)

KiCad 10 PCB design for the ESP32 board that this repository's firmware runs on. It regulates power for the ESP32 and a DS18S20 temperature sensor, and switches four 12V output channels for pumps/relays.

## Overview

| | |
|---|---|
| MCU | ESP32-DevKitC (`U3`) |
| Board size | 108.9 x 78.0 mm, 2 layers |
| Power in | 12V via `J2` (2-pin screw terminal) |
| Regulated rails | +12V (unregulated, direct), +5V (`U1`, LM7805S linear regulator), +3.3V (from the ESP32-DevKitC's own onboard regulator) |
| Temperature sensor | DS18S20 (`U2`, onboard) + external sensor header `J1` (3-pin, +3.3V/DATA/GND) |
| Switched outputs | 4 channels via `J3` (8-pin screw terminal), each a low-side IRFZ44N MOSFET driven through an MMBT3904 transistor stage, with an SS16 flyback diode across the load |
| DRC status | 0 errors (see [Design status](#design-status)) |

## Output channels

`J3` provides 4 independent 12V low-side switched outputs (`D2`-`D5` / `Q1`-`Q12` / `R2`-`R13`, one MOSFET + driver transistor + flyback diode per channel):

1. Peristaltic dosing pump 1 (0.3-1A)
2. Peristaltic dosing pump 2 (0.3-1A)
3. Finder relay, drives the main pool filter pump (relay coil current only, ~30-70mA)
4. Reserve / spare channel

Each channel's copper (`D2-A`...`D5-A` nets) is on the `Pumpenausgaenge`→`PumpOutputs` netclass (1.5mm min. trace width, sized for the peristaltic pump's worst-case 1A draw with margin).

## Connectors

| Ref | Type | Function |
|---|---|---|
| `J1` | 3-pin, 2.54mm (Xinya XY308) | External DS18S20 sensor: +3.3V / DATA / GND |
| `J2` | 2-pin, 5.0mm (Phoenix) | +12V power input |
| `J3` | 8-pin, 5.0mm (Phoenix) | 4x switched 12V output channels (2 pins each) |

## Netclasses

| Name | Nets | Trace width | Via (dia/drill) |
|---|---|---|---|
| `PumpOutputs` | `Net-(D2-A)` … `Net-(D5-A)` | 1.5mm | 1.0 / 0.5mm |
| `Supply_12V` | `+12V`, `GND` | 1.0mm | 1.0 / 0.5mm |
| `Supply_Logic` | `+5V`, `+3.3V` | 0.6mm | 0.6 / 0.3mm |
| `Default` | everything else (control/signal lines) | 0.2mm | 0.6 / 0.3mm |

`+12V` and `GND` are additionally backed by copper pours (zones) on `B.Cu`/`F.Cu`, so the netclass trace width only governs short stub connections into the pour, not the bulk current path.

## Bill of materials

Full BOM: [`PoolDosingController-BOM.csv`](PoolDosingController-BOM.csv). Summary:

| Ref | Qty | Value | MPN |
|---|---|---|---|
| C1 | 1 | 0.33uF | |
| C2, C3 | 2 | 0.1uF | |
| C4, C5, C6 | 3 | 10uF | |
| D1 | 1 | 1N4001 | |
| D2, D3, D4, D5 | 4 | SS16 | |
| J1 | 1 | 3-pin 2.54mm terminal block | |
| J2 | 1 | 2-pin 5.0mm terminal block | |
| J3 | 1 | 8-pin 5.0mm terminal block | |
| Q1-Q8 | 8 | MMBT3904 | |
| Q9-Q12 | 4 | IRFZ44N | |
| R1 | 1 | 2.2K | |
| R2, R3, R4, R5 | 4 | 220 | |
| R6-R13 | 8 | 10K | |
| U1 | 1 | LM7805S | L7805ABD2T-TR |
| U2 | 1 | DS18S20 | DS18S20+ |
| U3 | 1 | ESP32-DevKitC | ESP32-DEVKITC-32E |

C2's footprint (`PCM_Capacitor_SMD_AKL:C_0805_2012Metric`) has a documented, not-yet-installed alternate part on its `LCSC_Alt_0402` field: JLCPCB C1525 (Samsung CL05B104KO5NNNC, 0402, 100nF 16V X7R, Basic). Its footprint (`CAPC2012X140N`) is included under `library/footprints/` if you want to switch to it.

## Design status

Last full design review: **0 DRC errors**. Remaining warnings (non-blocking):
- 199x `connection_width` — thermal spoke connections to the copper pours, expected/cosmetic.
- 32x `text_height` — a few silkscreen labels below the strict 0.8mm rule.
- 4x `lib_footprint_mismatch` — the embedded footprint copies for `J1`/`J2`/`J3`/`C1` have drifted slightly from the current version of their (stock KiCad) libraries; purely a library-vs-board diff, not a design error.
- 3x info: `+12V`/`+5V`/`+3.3V` have no test point (informational only).

## Opening this project

This folder is fully self-contained: `library/` holds local copies of every non-stock symbol library, footprint library, and 3D model this design uses (`sym-lib-table`/`fp-lib-table` point at them via `${KIPRJMOD}`), so `git clone` + open in KiCad 10 works without installing any third-party library packages first. The only gap is a missing 3D model for `J1`'s exact connector (cosmetic 3D-preview only, a similarly-sized substitute is used); see the netclass/library setup notes in the project's commit history for details.
