# PloutonLogViewer

This directory contains the source code for Plouton memory log viewer Windows application.

## Features
- Reads physical memory via RWEverything driver (RwDrv.sys) and parses the SMM in-memory circular log.
- Can auto-fetch the log buffer physical address from UEFI variable `PloutonLogAddress`, or accept manual input.
- Bilingual UI (English/Chinese), renders logs as `[Timestamp][Level] Message`.

## Usage
1) Start RWEverything (or ensure RwDrv.sys is loaded) and run PloutonLogViewer as Administrator.
2) Click “Get Address” to read the UEFI variable, or enter the physical address manually (`0x...`).
3) Click Start to poll logs; Pause to stop; Clear to wipe the UI (does not affect the SMM buffer).
4) Admin rights are required. The app attempts to enable `SeSystemEnvironmentPrivilege` automatically.

## Build
- Windows + Visual Studio / `dotnet build`, project: `PloutonLogViewer.csproj`.
- Log buffer default is 8MB; keep viewer `LogBufferSize` aligned with firmware `MEMORY_LOG_BUFFER_SIZE`.

## Layout
- `App.xaml*` / `MainWindow.*`: WPF UI and logic.
- `PhysicalMemoryReader.cs`: IOCTL wrapper for RWEverything physical memory reads.
- `Resources/`: localization strings.
- `.gitignore`: ignores bin/obj/Debug/Release artifacts.
