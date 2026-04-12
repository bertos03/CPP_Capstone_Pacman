# BobMan (Pacman Capstone in C++)

BobMan ist ein Pacman-inspiriertes 2D-Spiel mit **C++17**, **SDL2** und SDL-Erweiterungen.
Der aktuelle Stand enthält neben dem eigentlichen Spiel auch ein Menüsystem, Kartenverwaltung, Schwierigkeitsgrade und einen integrierten Karteneditor.

## Features (aktueller Stand)

- **Startmenü** mit Einträgen für:
  - Spielstart
  - Kartenauswahl
  - Karteneditor
  - Konfiguration
- **Schwierigkeitsgrad über Monster-Anzahl** (`Few`, `Medium`, `Many`).
- **Mehrere Monster-Typen** mit unterschiedlichen Fähigkeiten:
  - wenige Monster: Standardgegner
  - mittlere Monster: Gaswolken
  - viele Monster: Feuerbälle mit Sichtlinienlogik
- **Teleport-Felder** (`1` bis `5`) als paarweise Portale.
- **Kartenbibliothek** aus `data/maps/` mit Kartenname in der ersten Zeile.
- **In-Game-Karteneditor**:
  - vorhandene Karte laden
  - neue Karte in vordefinierter Größe anlegen
  - Karte validieren und speichern
- **Audioeffekte** (optional über `AUDIO` in `src/definitions.h`).

## Projektstruktur

- `src/main.cpp`: Menüfluss, Spielstart, Editor-Workflows.
- `src/game.*`: Spielregeln, Kollisionen, Effekte, Teleport-Handling.
- `src/map.*`: Karten laden/speichern/discovern, Parsing, Teleporter-Paare.
- `src/renderer.*`: Zeichnen von Spiel, Menüs, HUD, Editor, Effekten.
- `src/events.*`: Tastatureingaben und Quit-Handling.
- `src/audio.*`: Laden und Abspielen von Sounds.
- `data/maps/`: Karten-Dateien (`.txt`, `.map`).

## Kartenformat

Eine Karten-Datei enthält:
1. **Zeile 1:** Anzeigename der Karte
2. **Ab Zeile 2:** Layout

Wichtige Zeichen:
- `x` = Wand
- `.` = Pfad
- `G` = Goodie
- `P` = Pacman-Start
- `a` / `b` / `M` = Monster je Schwierigkeitsstufe (siehe `definitions.h`)
- `1` bis `5` = Teleporter (jede Ziffer muss genau 0 oder 2-mal vorkommen)

## Build

### Voraussetzungen

- CMake
- C++17-Compiler (`g++` oder `clang++`)
- SDL2 + SDL2_image + SDL2_ttf
- SDL2_mixer (für Audio)

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install -y build-essential cmake \
  libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev
```

### Build + Start

```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
./pacman
```

## Steuerung

- **Menüs:** Pfeiltasten hoch/runter, `Enter` bestätigen, `Esc` zurück.
- **Spiel:** Pfeiltasten bewegen Pacman, `Esc` beendet.
- **Editor:**
  - Pfeiltasten bewegen Cursor
  - Zeichen setzen/ändern über die vorgesehenen Tasten laut Overlay
  - `Esc` öffnet Speichern/Verwerfen-Abfrage

## Typische Probleme

1. **SDL wird nicht gefunden**
   - Abhängigkeiten installieren und `cmake ..` erneut ausführen.

2. **Kein Ton**
   - `AUDIO` in `src/definitions.h` aktivieren und neu bauen.

3. **Karte lädt nicht**
   - Prüfen, ob erste Zeile ein Name ist und Layoutzeilen danach folgen.
   - Teleporter-Ziffern müssen paarweise vorhanden sein.
