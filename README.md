# BobMan (Pacman Capstone in C++)

```text
██████╗  ██████╗ ██████╗ ███╗   ███╗ █████╗ ███╗   ██╗
██╔══██╗██╔═══██╗██╔══██╗████╗ ████║██╔══██╗████╗  ██║
██████╔╝██║   ██║██████╔╝██╔████╔██║███████║██╔██╗ ██║
██╔══██╗██║   ██║██╔══██╗██║╚██╔╝██║██╔══██║██║╚██╗██║
██████╔╝╚██████╔╝██████╔╝██║ ╚═╝ ██║██║  ██║██║ ╚████║
╚═════╝  ╚═════╝ ╚═════╝ ╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝

      (\_/)
   __(o.o)   BobMan frisst Goodies statt Möhren.
  /__ >🍒
```

BobMan ist ein Pacman-inspiriertes 2D-Spiel, umgesetzt mit **C++17**, **SDL2** und mehreren SDL-Erweiterungen.  
Das Projekt wurde so strukturiert, dass Spiel-Logik, Rendering, Eingaben, Audio und Kartenverwaltung klar getrennt sind.

---

## 1) Funktionsumfang

- **ASCII-basierte Map-Konfiguration** über `data/map.txt`.
- **Dynamische Objekte**: Pacman, Monster, Goodies.
- **Parallele Bewegung per Threads**: Pacman und jedes Monster in eigener Simulation.
- **Kollisionserkennung**:
  - Pacman + Goodie => Punkte + Goodie wird deaktiviert.
  - Pacman + Monster => Spiel verloren.
- **Win-Condition**: Alle Goodies gesammelt.
- **Optionale Audioausgabe** (compile-time steuerbar via `AUDIO` in `src/definitions.h`).

---

## 2) Architektur (High-Level)

Das Projekt folgt einer einfachen, aber sauberen Schichtenaufteilung:

1. **Konfiguration + Basistypen**
   - `src/definitions.h`: Konstanten, Makros, Asset-Pfade, Symbolik der Map.
   - `src/globaltypes.h/.cpp`: Koordinatentypen, Richtungen, Sleep-Helfer.

2. **Domänenmodell / Spielobjekte**
   - `src/mapelement.h/.cpp`: Basisklasse für bewegliche/aktive Elemente.
   - `src/pacman.*`, `src/monster.*`, `src/goodie.*`: konkrete Spielobjekte.

3. **System-Services**
   - `src/events.*`: SDL-Event-Verarbeitung (Tasten + Quit).
   - `src/audio.*`: Laden/Abspielen von Soundeffekten.

4. **Orchestrierung**
   - `src/map.*`: Laden/Validieren/Abfragen der Karte.
   - `src/game.*`: Regeln, Score, Sieg-/Niederlage-Logik.
   - `src/renderer.*`: Zeichenpipeline für Karte, Sprites, Text.

5. **Einstiegspunkt**
   - `src/main.cpp`: Initialisierung, Hauptloop, zyklisches Update/Render.

---

## 3) Implementierungsdetails

### 3.1 Karte und Spielwelt

- Die Datei `data/map.txt` verwendet Zeichen für den Aufbau:
  - `x` = Wand
  - `.` = Pfad
  - `G` = Goodie
  - `P` = Pacman-Start
  - `M` = Monster-Start
- `Map::LoadMap(...)` liest die Datei zeilenweise ein, konvertiert Zeichen nach `ElementType` und speichert Koordinatenlisten für Entitäten.

### 3.2 Bewegung und Threading

- Pacman und Monster laufen in **eigenen Threads** (`simulate(...)`).
- Bewegungsanimation erfolgt per `px_delta` in kleinen Inkrementen, damit Bewegung flüssiger wirkt.
- Ein gemeinsamer Mutex in `MapElement` schützt kritische Bereiche (einfaches Synchronisationsmuster).

### 3.3 Rendering

- SDL initialisiert ein Fenster passend zur Displaygröße.
- Elementgröße wird aus Auflösung und Map-Dimensionen berechnet.
- Renderer zeichnet:
  1. Map-Hintergrund/Wände,
  2. Goodies,
  3. Monster,
  4. Pacman,
  5. HUD-/Status-Text.

### 3.4 Audio (optional)

- Wenn `AUDIO` aktiv ist, lädt `Audio` die WAV-Dateien aus `data/`.
- Beim Einsammeln, Sieg oder Game Over wird der jeweilige Effekt abgespielt.
- Ohne `AUDIO` bleibt der Build kompatibel mit Umgebungen ohne Mixer-Unterstützung.

---

## 4) Abhängigkeiten (präzise)

### 4.1 Build-Werkzeuge

- `g++` oder `clang++` mit C++17-Unterstützung
- `cmake` (empfohlen >= 3.11)
- `make`

### 4.2 Laufzeit-/Dev-Bibliotheken

- SDL2
- SDL2_image
- SDL2_ttf
- SDL2_mixer (für Audio-Feature)

### 4.3 Ubuntu/Debian Installation

```bash
sudo apt update
sudo apt install -y build-essential cmake \
  libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev
```

### 4.4 macOS (Homebrew)

```bash
brew install cmake sdl2 sdl2_image sdl2_ttf sdl2_mixer
```

---

## 5) Build-Anleitung für Newbies (Schritt für Schritt)

> Die folgenden Schritte sind absichtlich sehr detailliert gehalten.

### Schritt 0: Projekt holen und ins Projekt wechseln

```bash
git clone <DEIN_REPO_URL>
cd CPP_Capstone_Pacman
```

### Schritt 1: Prüfen, ob Tools installiert sind

```bash
cmake --version
make --version
g++ --version
```

Wenn ein Befehl „not found“ meldet: erst Abhängigkeiten aus Abschnitt 4 installieren.

### Schritt 2: Build-Ordner anlegen

```bash
mkdir -p build
cd build
```

### Schritt 3: CMake konfigurieren

```bash
cmake ..
```

Was passiert hier?
- CMake sucht Compiler und SDL-Bibliotheken.
- Es erzeugt die Build-Dateien (Makefiles).

### Schritt 4: Kompilieren

```bash
make -j$(nproc)
```

Falls `nproc` nicht existiert (z. B. macOS), nutze einfach:

```bash
make
```

### Schritt 5: Spiel starten

```bash
./pacman
```

### Schritt 6: Steuerung

- Pfeiltasten: Bewegung
- ESC: Beenden
- Fenster schließen: Beenden

---

## 6) Audio aktivieren/deaktivieren

In `src/definitions.h`:

- **Audio aktivieren**: `#define AUDIO` einkommentieren.
- **Audio deaktivieren**: `#define AUDIO` auskommentiert lassen.

Danach immer neu bauen:

```bash
cd build
cmake ..
make
```

---

## 7) Häufige Fehler und Lösungen

1. **SDL nicht gefunden**
   - Prüfe, ob `libsdl2-...-dev` installiert sind.
   - Danach `cmake ..` erneut ausführen.

2. **Kein Sound**
   - Prüfen, ob `AUDIO` definiert ist.
   - `libsdl2-mixer-dev` installiert?
   - Existieren WAV-Dateien in `data/`?

3. **Schwarzes Fenster / sofortiger Exit**
   - Prüfe Assets und Pfade in `src/definitions.h`.
   - Stelle sicher, dass du aus `build/` mit `./pacman` startest.

---

## 8) Verzeichnisstruktur

```text
.
├── CMakeLists.txt
├── cmake/                  # Find-Module für SDL-Komponenten
├── data/                   # Karten-, Bild-, Font- und Sound-Ressourcen
├── src/
│   ├── main.cpp            # App-Start + Hauptloop
│   ├── game.*              # Spielregeln + Thread-Orchestrierung
│   ├── renderer.*          # SDL-Rendering
│   ├── map.*               # Kartendaten + Navigationsoptionen
│   ├── pacman.*            # Spielerlogik
│   ├── monster.*           # Gegnerlogik
│   ├── goodie.*            # Sammelobjektlogik
│   ├── events.*            # Tastatur-/Quit-Eingaben
│   ├── audio.*             # Soundeffekte
│   ├── mapelement.*        # Gemeinsame Basisklasse
│   ├── globaltypes.*       # Koordinaten, Richtungen, Sleep-Helfer
│   └── definitions.h       # zentrale Compile-Time-Konfiguration
└── README.md
```

---

## 9) Kurzfazit

BobMan zeigt ein kompaktes, gut verständliches C++-Game-Projekt mit:
- klarer Klassenstruktur,
- SDL-basierter Ausgabe,
- threadbasierter Bewegung,
- und leicht anpassbarer Kartenlogik.

Ideal als Lernprojekt für OOP, Nebenläufigkeit und Build-Systeme mit CMake.
