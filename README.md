# L2mRankingReaderCV

Windows utilities built with OpenCV and Tesseract OCR.

## Components

- `RankingReader` — reads ranking table data from the screen and writes it to a file.
- `ExpertiseRoller` — recognizes nine expertise properties and checks them against configured combinations.
- `LifeStoneRoller` — recognizes three life stone properties and checks them against configured combinations.
- `GetImage` — captures a window image for debugging. Run it as `GetImage.exe <character-name>`.
- `MemAnalysisTest` — experimental process-memory diagnostics.

Shared window capture, OCR, and input helpers are located in `Tools`.

## Requirements

- Windows;
- CMake 3.20 or newer;
- a compiler with C++23 support;
- OpenCV;
- Tesseract OCR with `eng.traineddata`;
- Boost.

The current build setup targets MSYS2/MinGW64.

## Build

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

If CMake cannot find MinGW, add `C:\msys64\mingw64\bin` to `PATH`.

Executables are written to `build/bin`. After each executable is linked, CMake scans its runtime dependencies and copies the required MinGW, OpenCV, and Tesseract DLLs into the same directory. Configuration files and image assets are copied there during configuration as well, so `build/bin` can be used as the application directory.

## Configuration

### ExpertiseRoller

Configuration file: `settings_ExpertiseRoller.json`.

```json
{
  "charName": "CharacterName",
  "expertisParametrs": [
    {
      "UpLeft":   { "color": 2, "propertyName": "weapon damage boost", "val": 4 },
      "UpMid":    { "color": 3, "propertyName": "",                    "val": 0 },
      "UpRight":  { "color": 3, "propertyName": "",                    "val": 0 },
      "MidLeft":  { "color": 3, "propertyName": "",                    "val": 0 },
      "MidMid":   { "color": 1, "propertyName": "accuracy",            "val": 2 },
      "MidRight": { "color": 3, "propertyName": "",                    "val": 0 },
      "DwnLeft":  { "color": 3, "propertyName": "",                    "val": 0 },
      "DwnMid":   { "color": 3, "propertyName": "",                    "val": 0 },
      "DwnRight": { "color": 3, "propertyName": "",                    "val": 0 }
    }
  ]
}
```

Fields:

- `charName` is the character name used to locate the game window.
- `expertisParametrs` is an array of accepted result sets. Each object describes the 3x3 grid from top-left to bottom-right.
- `color` is the required cell color: `0` = purple, `1` = green, `2` = blue, `3` = any color.
- `propertyName` is the required OCR property name. Use lowercase text. An empty string accepts any property.
- `val` is the minimum accepted property value.

All non-wildcard cells inside one result set must match. When several objects are added to `expertisParametrs`, matching any one of them stops rolling.

For example, the configuration above accepts a blue `weapon damage boost +4` or better in the upper-left cell and a green `accuracy +2` or better in the center. All other cells are ignored.

To accept another combination, add another object to the array:

```json
"expertisParametrs": [
  { "UpLeft": { "color": 2, "propertyName": "accuracy", "val": 3 } },
  { "MidMid":  { "color": 0, "propertyName": "weapon damage boost", "val": 5 } }
]
```

Missing grid positions are treated as wildcards, although listing all nine positions makes the configuration easier to read and edit.

`diamond.jpg` must also be available in the working directory.

### LifeStoneRoller

Configuration file: `settings_lifeStoneRoller.json`.

```json
{
  "charName": "CharacterName",
  "LifeStoneParametrs": [
    [
      { "propertyName": "defense", "propVal": 2 },
      { "propertyName": "",        "propVal": 0 },
      { "propertyName": "",        "propVal": 0 }
    ],
    [
      { "propertyName": "",        "propVal": 0 },
      { "propertyName": "defense", "propVal": 2 },
      { "propertyName": "",        "propVal": 0 }
    ]
  ]
}
```

Fields:

- `charName` is the character name used to locate the game window.
- `LifeStoneParametrs` is an array of accepted result sets.
- Each result set contains three entries in the same order as the three properties displayed in the game: top, middle, bottom.
- `propertyName` is the required OCR property name. Use lowercase text. An empty string accepts any property in that position.
- `propVal` is the minimum accepted numeric value.

All non-wildcard entries inside one result set must match. Matching any result set in `LifeStoneParametrs` stops rolling. The example accepts `defense +2` or better in either the first or second position.

Each result set should contain exactly three entries. Use an empty `propertyName` with `propVal: 0` for positions that do not matter.

The legacy `propeName` field is also supported.

## Running

The tools locate a window with the following title:

```text
Lineage2M l <charName>
```

Run an executable from a directory containing its configuration, required image assets, and Tesseract data.

The current recognition regions are hard-coded for a 1600x900 window and HUD scale 100 unless stated otherwise in the corresponding source file.
