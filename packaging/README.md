# EvelentWalker packaging

Builds every EvelentWalker tool as a **single self-contained `.exe`** (all
dependency DLLs embedded with [Costura.Fody](https://github.com/Fody/Costura)),
and produces a Windows installer with Start Menu shortcuts and an uninstaller.

## Tools shipped

| Executable                              | Project                        |
|-----------------------------------------|--------------------------------|
| `EvelentWalker.exe`                     | EvelentWalker                  |
| `EvelentWalker RPF Explorer.exe`        | EvelentWalker.RPFExplorer      |
| `EvelentWalker Vehicle Viewer.exe`      | EvelentWalker.Vehicles         |
| `EvelentWalker Ped Viewer.exe`          | EvelentWalker.Peds             |
| `EvelentWalker Gen9 Converter.exe`      | EvelentWalker.Gen9Converter    |
| `EvelentWalker Mod Manager.exe`         | EvelentWalker.ModManager       |
| `EvelentWalker Error Report Tool.exe`   | EvelentWalker.ErrorReport      |

Each `.exe` has **no loose dependency DLLs** next to it. Shared runtime data
(`Shaders/`, `icons/`, `strings.txt`, `ShadersGen9Conversion.xml`) is deployed
once alongside the tools.

## Build the binaries only

```powershell
powershell -ExecutionPolicy Bypass -File packaging\build-release.ps1
```

Output: `packaging\dist\` — the packed exes + shared data.

## Build the installer

```powershell
powershell -ExecutionPolicy Bypass -File packaging\build-installer.ps1
```

This rebuilds the binaries, then compiles `installer\EvelentWalker.iss` with the
Inno Setup compiler (ISCC). If Inno Setup is missing, install it with:

```powershell
winget install JRSoftware.InnoSetup
```

or pass `-AutoInstallInnoSetup` to the script. To compile the installer against
already-built binaries, add `-SkipBuild`.

Output: `packaging\installer\output\EvelentWalker-Setup-<version>.exe`

The installer:
- installs all tools into `Program Files\EvelentWalker`,
- creates a Start Menu group with a shortcut for each tool,
- offers an optional desktop shortcut for the main app,
- registers an uninstaller / Add-or-Remove-Programs entry.

## How packing works

Each executable project references `Costura.Fody` and contains a
`FodyWeavers.xml` with `<Costura/>`. At build time Fody embeds every referenced
managed assembly into the `.exe` as a compressed resource and removes the loose
copies from the output folder.
