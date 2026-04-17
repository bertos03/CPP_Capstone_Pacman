/*
 * PROJECT COMMENT (main.cpp)
 * ---------------------------------------------------------------------------
 * This file is part of "BobMan", an SDL2 game inspired by Pac-Man.
 * The code in this unit has a clear responsibility so newcomers can quickly
 * understand the architecture: data model (map, elements), runtime logic
 * (game, events), rendering (renderer), and optional audio output.
 *
 * Important notes for newcomers:
 * - Header files declare classes, methods, and data types.
 * - CPP files contain the concrete implementation of the logic.
 * - Multiple threads move game entities in parallel, so shared data is read
 *   and written in a controlled way.
 * - Macros in definitions.h control resource paths, colors, and features.
 */

#include <SDL.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "audio.h"
#include "events.h"
#include "game.h"
#include "globaltypes.h"
#include "map.h"
#include "renderer.h"
#include "paths.h"
#include "settings.h"

namespace {

// Selection in the start menu.
enum MenuSelection {
  MENU_START = 0,
  MENU_MAP = 1,
  MENU_EDITOR = 2,
  MENU_CONFIG = 3,
  MENU_END = 4,
};

// Selection in the configuration menu.
enum ConfigSelection {
  CONFIG_DIFFICULTY = 0,
  CONFIG_BACK = 1,
};

enum class MenuScreen {
  Main,
  Config,
  MapSelection,
  EditorSelection,
  EditorSizeSelection
};

// Predefined sizes for new maps in the editor.
enum class NewMapSize { Small = 0, Medium = 1, Large = 2 };

// Parameters used to launch the editor.
struct EditorRequest {
  bool is_new_map{false};
  std::string map_path;
  std::string display_name;
  int rows{0};
  int cols{0};
};

// Return value after leaving the editor.
struct EditorResult {
  bool quit_requested{false};
  bool should_reload_maps{false};
  std::string preferred_map_path;
};

// Result of the layout validation.
struct MapValidationResult {
  bool is_valid{false};
  std::string message;
};

// Complete runtime state of the editor.
struct EditorState {
  MapFileData map_file;
  std::string map_path;
  bool is_new_map{false};
  MapCoord cursor{1, 1};
  bool show_exit_dialog{false};
  bool show_name_dialog{false};
  int exit_dialog_selected{0};
  MapValidationResult validation;
  std::string transient_error;
  std::string name_input;
  std::string name_dialog_message;
};

constexpr int kEditorExitSave = 0;
constexpr int kEditorExitCancel = 1;
constexpr int kEditorExitDiscard = 2;

constexpr int kSmallMapRows = 13;
constexpr int kSmallMapCols = 22;
constexpr int kMediumMapRows = 17;
constexpr int kMediumMapCols = 30;
constexpr int kLargeMapRows = 21;
constexpr int kLargeMapCols = 38;
constexpr Uint32 kEndScreenMinimumMs = 3000;

Difficulty NextDifficulty(Difficulty difficulty) {
  switch (difficulty) {
  case Difficulty::Easy:
    return Difficulty::Medium;
  case Difficulty::Medium:
    return Difficulty::Hard;
  case Difficulty::Hard:
    return Difficulty::Easy;
  default:
    return Difficulty::Medium;
  }
}

std::vector<std::string>
GetMapDisplayNames(const std::vector<MapDefinition> &available_maps) {
  std::vector<std::string> names;
  names.reserve(available_maps.size());
  for (const auto &map : available_maps) {
    names.push_back(map.display_name);
  }
  return names;
}

int FindMapIndexByPath(const std::vector<MapDefinition> &available_maps,
                       const std::string &map_path) {
  for (int i = 0; i < static_cast<int>(available_maps.size()); i++) {
    if (available_maps[i].file_path == map_path) {
      return i;
    }
  }
  return -1;
}

void RefreshSelectedMap(const std::vector<MapDefinition> &available_maps,
                        const std::string &preferred_map_path,
                        int &selected_map_index, int &active_map_index) {
  if (available_maps.empty()) {
    selected_map_index = 0;
    active_map_index = 0;
    return;
  }

  int resolved_index = -1;
  if (!preferred_map_path.empty()) {
    resolved_index = FindMapIndexByPath(available_maps, preferred_map_path);
  }

  if (resolved_index < 0) {
    resolved_index = std::clamp(selected_map_index, 0,
                                static_cast<int>(available_maps.size()) - 1);
  }

  selected_map_index = resolved_index;
  active_map_index = resolved_index;
}

void PersistAppSettings(Difficulty difficulty,
                        const std::string &selected_map_path) {
  const AppSettings settings{difficulty, selected_map_path};
  SaveAppSettings(settings);
}

std::pair<int, int> GetDimensionsForSize(NewMapSize size) {
  switch (size) {
  case NewMapSize::Small:
    return {kSmallMapRows, kSmallMapCols};
  case NewMapSize::Medium:
    return {kMediumMapRows, kMediumMapCols};
  case NewMapSize::Large:
  default:
    return {kLargeMapRows, kLargeMapCols};
  }
}

std::string GetSizeLabel(NewMapSize size) {
  switch (size) {
  case NewMapSize::Small:
    return "Klein";
  case NewMapSize::Medium:
    return "Mittel";
  case NewMapSize::Large:
  default:
    return "Gross";
  }
}

std::string Slugify(const std::string &text) {
  std::string slug;
  bool last_was_separator = false;

  for (unsigned char character : text) {
    if (std::isalnum(character) != 0) {
      slug.push_back(static_cast<char>(std::tolower(character)));
      last_was_separator = false;
    } else if (!last_was_separator) {
      slug.push_back('_');
      last_was_separator = true;
    }
  }

  while (!slug.empty() && slug.front() == '_') {
    slug.erase(slug.begin());
  }
  while (!slug.empty() && slug.back() == '_') {
    slug.pop_back();
  }

  return slug.empty() ? "eigene_karte" : slug;
}

std::string TrimWhitespace(const std::string &text) {
  const auto first = std::find_if_not(text.begin(), text.end(), [](unsigned char ch) {
    return std::isspace(ch) != 0;
  });
  if (first == text.end()) {
    return "";
  }

  const auto last =
      std::find_if_not(text.rbegin(), text.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
      }).base();
  return std::string(first, last);
}

std::string CreateUniqueMapPath(const std::string &display_name) {
  namespace fs = std::filesystem;

  const std::string slug = Slugify(display_name);
  fs::path base_path = fs::path(Paths::GetWritableMapsDirectory()) /
                       (slug + ".txt");
  if (!fs::exists(base_path)) {
    return base_path.string();
  }

  int suffix = 2;
  while (true) {
    fs::path candidate =
        fs::path(Paths::GetWritableMapsDirectory()) /
        (slug + "_" + std::to_string(suffix) + ".txt");
    if (!fs::exists(candidate)) {
      return candidate.string();
    }
    suffix++;
  }
}

std::string ResolveEditableMapPath(const std::string &current_map_path,
                                   const std::string &display_name) {
  namespace fs = std::filesystem;

  if (Paths::IsWritableMapPath(current_map_path)) {
    return current_map_path;
  }

  const fs::path current_filename = fs::path(current_map_path).filename();
  if (!current_filename.empty()) {
    return (fs::path(Paths::GetWritableMapsDirectory()) / current_filename)
        .string();
  }

  return CreateUniqueMapPath(display_name);
}

EditorRequest CreateNewMapRequest(NewMapSize size) {
  const auto [rows, cols] = GetDimensionsForSize(size);
  EditorRequest request;
  request.is_new_map = true;
  request.display_name = "Neue Karte " + GetSizeLabel(size);
  request.rows = rows;
  request.cols = cols;
  return request;
}

MapFileData CreateEmptyMapFile(const std::string &display_name, int rows,
                               int cols) {
  MapFileData map_file;
  map_file.display_name = display_name;
  map_file.layout_rows.assign(rows, std::string(cols, PATH));

  for (int row = 0; row < rows; row++) {
    map_file.layout_rows[row][0] = BRICK;
    map_file.layout_rows[row][cols - 1] = BRICK;
  }
  for (int col = 0; col < cols; col++) {
    map_file.layout_rows[0][col] = BRICK;
    map_file.layout_rows[rows - 1][col] = BRICK;
  }

  map_file.layout_rows[1][1] = PACMAN;
  map_file.layout_rows[rows - 2][cols - 2] = GOODIE;
  return map_file;
}

MapValidationResult ValidateEditorMap(const MapFileData &map_file) {
  const std::vector<std::string> &layout = map_file.layout_rows;
  if (layout.empty()) {
    return {false, "Warnung: Karte enthaelt kein Layout"};
  }

  const int rows = static_cast<int>(layout.size());
  const int cols = static_cast<int>(layout.front().size());
  if (rows < 3 || cols < 3) {
    return {false, "Warnung: Karte ist zu klein"};
  }

  for (const auto &row : layout) {
    if (static_cast<int>(row.size()) != cols) {
      return {false, "Warnung: Zeilenlaengen sind uneinheitlich"};
    }
  }

  if (std::count(layout.front().begin(), layout.front().end(), BRICK) != cols ||
      std::count(layout.back().begin(), layout.back().end(), BRICK) != cols) {
    return {false, "Warnung: Die Aussenmauer muss geschlossen sein"};
  }

  for (const auto &row : layout) {
    if (row.front() != BRICK || row.back() != BRICK) {
      return {false, "Warnung: Die Aussenmauer muss geschlossen sein"};
    }
  }

  int pacman_count = 0;
  int goodie_count = 0;
  MapCoord pacman_coord{0, 0};
  std::array<std::vector<MapCoord>, 5> teleporter_slots;

  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      if (layout[row][col] == PACMAN) {
        pacman_count++;
        pacman_coord = {row, col};
      } else if (layout[row][col] == GOODIE) {
        goodie_count++;
      } else if (Map::IsTeleporterChar(layout[row][col])) {
        teleporter_slots[layout[row][col] - '1'].push_back({row, col});
      }
    }
  }

  if (pacman_count != 1) {
    return {false, "Warnung: Genau eine Spielfigur ist erforderlich"};
  }
  if (goodie_count == 0) {
    return {false, "Warnung: Mindestens ein Goodie ist erforderlich"};
  }
  for (size_t teleporter_index = 0; teleporter_index < teleporter_slots.size();
       teleporter_index++) {
    const size_t count = teleporter_slots[teleporter_index].size();
    if (count != 0 && count != 2) {
      return {false,
              "Warnung: Jede Teleporter-Ziffer 1-5 muss genau 2 Mal vorkommen"};
    }
  }

  std::queue<MapCoord> open_cells;
  std::vector<std::vector<bool>> visited(
      rows, std::vector<bool>(cols, false));
  open_cells.push(pacman_coord);
  visited[pacman_coord.u][pacman_coord.v] = true;

  while (!open_cells.empty()) {
    const MapCoord current = open_cells.front();
    open_cells.pop();

    const std::vector<MapCoord> neighbours{
        {current.u - 1, current.v}, {current.u + 1, current.v},
        {current.u, current.v - 1}, {current.u, current.v + 1}};

    for (const auto &neighbour : neighbours) {
      if (neighbour.u < 0 || neighbour.u >= rows || neighbour.v < 0 ||
          neighbour.v >= cols) {
        continue;
      }
      if (visited[neighbour.u][neighbour.v] ||
          layout[neighbour.u][neighbour.v] == BRICK) {
        continue;
      }
      visited[neighbour.u][neighbour.v] = true;
      open_cells.push(neighbour);
    }

    const char current_tile = layout[current.u][current.v];
    if (Map::IsTeleporterChar(current_tile)) {
      const auto &teleporter_positions = teleporter_slots[current_tile - '1'];
      if (teleporter_positions.size() == 2) {
        const MapCoord target =
            (teleporter_positions[0].u == current.u &&
             teleporter_positions[0].v == current.v)
                ? teleporter_positions[1]
                : teleporter_positions[0];
        if (!visited[target.u][target.v]) {
          visited[target.u][target.v] = true;
          open_cells.push(target);
        }
      }
    }
  }

  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      if (layout[row][col] != BRICK && !visited[row][col]) {
        return {false,
                "Warnung: Nicht alle Felder und Goodies sind erreichbar"};
      }
    }
  }

  return {true, ""};
}

void RevalidateEditor(EditorState &editor_state) {
  editor_state.validation = ValidateEditorMap(editor_state.map_file);
}

bool IsEditorInteriorCoord(const EditorState &editor_state, MapCoord coord) {
  const int row_count =
      static_cast<int>(editor_state.map_file.layout_rows.size());
  const int col_count =
      static_cast<int>(editor_state.map_file.layout_rows.front().size());
  return coord.u >= 1 && coord.v >= 1 && coord.u < row_count - 1 &&
         coord.v < col_count - 1;
}

void MoveEditorCursor(EditorState &editor_state, int row_delta, int col_delta) {
  const int max_row =
      static_cast<int>(editor_state.map_file.layout_rows.size()) - 2;
  const int max_col =
      static_cast<int>(editor_state.map_file.layout_rows.front().size()) - 2;
  editor_state.cursor.u =
      std::clamp(editor_state.cursor.u + row_delta, 1, max_row);
  editor_state.cursor.v =
      std::clamp(editor_state.cursor.v + col_delta, 1, max_col);
}

void PlaceEditorTile(EditorState &editor_state, char tile) {
  if (tile == PACMAN) {
    for (auto &row : editor_state.map_file.layout_rows) {
      std::replace(row.begin(), row.end(), PACMAN, PATH);
    }
  }

  editor_state.map_file.layout_rows[editor_state.cursor.u][editor_state.cursor.v] =
      tile;
  editor_state.transient_error.clear();
  RevalidateEditor(editor_state);
}

void HandleEditorMouseToggle(EditorState &editor_state, MapCoord clicked_coord,
                             Audio *audio) {
  editor_state.cursor = clicked_coord;
  if (!IsEditorInteriorCoord(editor_state, clicked_coord)) {
    audio->PlayEditorBlocked();
    return;
  }

  char &tile =
      editor_state.map_file.layout_rows[clicked_coord.u][clicked_coord.v];
  if (tile == PATH) {
    PlaceEditorTile(editor_state, BRICK);
    audio->PlayCountdownTick();
    return;
  }
  if (tile == BRICK) {
    PlaceEditorTile(editor_state, PATH);
    audio->PlayCountdownTick();
    return;
  }

  audio->PlayEditorBlocked();
}

std::string GetEditorWarningMessage(const EditorState &editor_state) {
  if (!editor_state.transient_error.empty()) {
    return editor_state.transient_error;
  }
  if (!editor_state.validation.is_valid) {
    return editor_state.validation.message;
  }
  return "";
}

EditorResult RunEditorSession(const EditorRequest &editor_request, Audio *audio) {
  namespace fs = std::filesystem;

  EditorState editor_state;
  editor_state.is_new_map = editor_request.is_new_map;
  editor_state.map_path = editor_request.map_path;

  if (editor_request.is_new_map) {
    editor_state.map_file = CreateEmptyMapFile(editor_request.display_name,
                                               editor_request.rows,
                                               editor_request.cols);
  } else if (!Map::LoadMapFile(editor_request.map_path, editor_state.map_file)) {
    std::cerr << "Could not load map for editor: " << editor_request.map_path
              << "\n";
    return {};
  }

  RevalidateEditor(editor_state);
  Renderer renderer(editor_state.map_file.layout_rows.size(),
                    editor_state.map_file.layout_rows.front().size());

  bool quit_requested = false;
  bool finished = false;
  bool should_reload_maps = false;
  std::string preferred_map_path;
  auto begin_name_dialog = [&]() {
    editor_state.show_exit_dialog = false;
    editor_state.show_name_dialog = true;
    editor_state.name_input.clear();
    editor_state.name_dialog_message.clear();
    SDL_StartTextInput();
  };
  auto end_name_dialog = [&]() {
    editor_state.show_name_dialog = false;
    editor_state.name_dialog_message.clear();
    SDL_StopTextInput();
  };

  while (!quit_requested && !finished) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        quit_requested = true;
        break;
      }

      if (!editor_state.show_name_dialog && !editor_state.show_exit_dialog &&
          event.type == SDL_MOUSEBUTTONDOWN &&
          event.button.button == SDL_BUTTON_LEFT) {
        MapCoord clicked_coord{0, 0};
        if (renderer.TryGetLayoutCoordFromScreen(event.button.x, event.button.y,
                                                 clicked_coord)) {
          HandleEditorMouseToggle(editor_state, clicked_coord, audio);
        }
        continue;
      }

      if (editor_state.show_name_dialog) {
        if (event.type == SDL_TEXTINPUT) {
          if (editor_state.name_input.size() < 40) {
            editor_state.name_input += event.text.text;
            editor_state.name_dialog_message.clear();
          }
          continue;
        }

        if (event.type != SDL_KEYDOWN) {
          continue;
        }

        switch (event.key.keysym.sym) {
        case SDLK_BACKSPACE:
          if (!editor_state.name_input.empty()) {
            editor_state.name_input.pop_back();
            editor_state.name_dialog_message.clear();
          }
          break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER: {
          audio->PlayMenuSelect();
          const std::string trimmed_name =
              TrimWhitespace(editor_state.name_input);
          if (trimmed_name.empty()) {
            editor_state.name_dialog_message =
                "Warnung: Bitte einen Kartennamen eingeben";
            break;
          }

          editor_state.map_file.display_name = trimmed_name;
          editor_state.map_path = CreateUniqueMapPath(trimmed_name);
          fs::create_directories(fs::path(editor_state.map_path).parent_path());
          if (Map::SaveMapFile(editor_state.map_path, editor_state.map_file)) {
            finished = true;
            should_reload_maps = true;
            preferred_map_path = editor_state.map_path;
            end_name_dialog();
          } else {
            editor_state.name_dialog_message =
                "Warnung: Speichern fehlgeschlagen";
          }
          break;
        }
        case SDLK_ESCAPE:
          end_name_dialog();
          editor_state.show_exit_dialog = true;
          break;
        default:
          break;
        }
        continue;
      }

      if (event.type != SDL_KEYDOWN) {
        continue;
      }

      if (editor_state.show_exit_dialog) {
        switch (event.key.keysym.sym) {
        case SDLK_UP:
          editor_state.exit_dialog_selected =
              (editor_state.exit_dialog_selected + 2) % 3;
          audio->PlayMenuMove();
          break;
        case SDLK_DOWN:
          editor_state.exit_dialog_selected =
              (editor_state.exit_dialog_selected + 1) % 3;
          audio->PlayMenuMove();
          break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
          audio->PlayMenuSelect();
          if (editor_state.exit_dialog_selected == kEditorExitSave) {
            if (editor_state.is_new_map) {
              begin_name_dialog();
            } else {
              editor_state.map_path = ResolveEditableMapPath(
                  editor_state.map_path, editor_state.map_file.display_name);
              fs::create_directories(
                  fs::path(editor_state.map_path).parent_path());
              if (Map::SaveMapFile(editor_state.map_path,
                                   editor_state.map_file)) {
                finished = true;
                should_reload_maps = true;
                preferred_map_path = editor_state.map_path;
              } else {
                editor_state.transient_error =
                    "Warnung: Speichern fehlgeschlagen";
                editor_state.show_exit_dialog = false;
              }
            }
          } else if (editor_state.exit_dialog_selected == kEditorExitCancel) {
            editor_state.show_exit_dialog = false;
          } else {
            finished = true;
            should_reload_maps = true;
          }
          break;
        case SDLK_ESCAPE:
          editor_state.show_exit_dialog = false;
          break;
        default:
          break;
        }
        continue;
      }

      const MapCoord previous_cursor = editor_state.cursor;
      switch (event.key.keysym.sym) {
      case SDLK_UP:
        MoveEditorCursor(editor_state, -1, 0);
        break;
      case SDLK_DOWN:
        MoveEditorCursor(editor_state, 1, 0);
        break;
      case SDLK_LEFT:
        MoveEditorCursor(editor_state, 0, -1);
        break;
      case SDLK_RIGHT:
        MoveEditorCursor(editor_state, 0, 1);
        break;
      case SDLK_x:
      case SDLK_w:
        PlaceEditorTile(editor_state, BRICK);
        break;
      case SDLK_SPACE:
      case SDLK_BACKSPACE:
      case SDLK_DELETE:
      case SDLK_PERIOD:
        PlaceEditorTile(editor_state, PATH);
        break;
      case SDLK_g:
        PlaceEditorTile(editor_state, GOODIE);
        break;
      case SDLK_p:
        PlaceEditorTile(editor_state, PACMAN);
        break;
      case SDLK_m:
        PlaceEditorTile(editor_state, MONSTER_FEW);
        break;
      case SDLK_n:
        PlaceEditorTile(editor_state, MONSTER_MEDIUM);
        break;
      case SDLK_o:
        PlaceEditorTile(editor_state, MONSTER_MANY);
        break;
      case SDLK_k:
        PlaceEditorTile(editor_state, MONSTER_EXTRA);
        break;
      case SDLK_1:
      case SDLK_2:
      case SDLK_3:
      case SDLK_4:
      case SDLK_5:
        PlaceEditorTile(editor_state,
                        static_cast<char>(event.key.keysym.sym));
        break;
      case SDLK_ESCAPE:
        if (editor_state.validation.is_valid) {
          editor_state.show_exit_dialog = true;
          editor_state.exit_dialog_selected = kEditorExitSave;
          audio->PlayMenuSelect();
        }
        break;
      default:
        break;
      }

      if (previous_cursor.u != editor_state.cursor.u ||
          previous_cursor.v != editor_state.cursor.v) {
        audio->PlayMenuMove();
      }
    }

    renderer.RenderEditor(editor_state.map_file.layout_rows,
                          editor_state.map_file.display_name, editor_state.cursor,
                          GetEditorWarningMessage(editor_state),
                          editor_state.show_exit_dialog,
                          editor_state.exit_dialog_selected,
                          editor_state.show_name_dialog,
                          editor_state.name_input,
                          editor_state.name_dialog_message);
    sleep(40);
  }

  SDL_StopTextInput();
  return {quit_requested, should_reload_maps, preferred_map_path};
}

void processMainMenuEvents(int &selected_item, MenuScreen &menu_screen,
                           bool &start_requested, bool &quit_requested,
                           Audio *audio) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      quit_requested = true;
      continue;
    }

    if (event.type != SDL_KEYDOWN) {
      continue;
    }

    switch (event.key.keysym.sym) {
    case SDLK_UP:
      selected_item = (selected_item + 4) % 5;
      audio->PlayMenuMove();
      break;
    case SDLK_DOWN:
      selected_item = (selected_item + 1) % 5;
      audio->PlayMenuMove();
      break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      audio->PlayMenuSelect();
      if (selected_item == MENU_START) {
        start_requested = true;
      } else if (selected_item == MENU_MAP) {
        menu_screen = MenuScreen::MapSelection;
      } else if (selected_item == MENU_EDITOR) {
        menu_screen = MenuScreen::EditorSelection;
      } else if (selected_item == MENU_CONFIG) {
        menu_screen = MenuScreen::Config;
      } else if (selected_item == MENU_END) {
        quit_requested = true;
      }
      break;
    case SDLK_ESCAPE:
      quit_requested = true;
      break;
    default:
      break;
    }
  }
}

void processConfigMenuEvents(int &selected_item, Difficulty &difficulty,
                             MenuScreen &menu_screen, bool &quit_requested,
                             Audio *audio) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      quit_requested = true;
      continue;
    }

    if (event.type != SDL_KEYDOWN) {
      continue;
    }

    switch (event.key.keysym.sym) {
    case SDLK_UP:
      selected_item = (selected_item + 1) % 2;
      audio->PlayMenuMove();
      break;
    case SDLK_DOWN:
      selected_item = (selected_item + 1) % 2;
      audio->PlayMenuMove();
      break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      audio->PlayMenuSelect();
      if (selected_item == CONFIG_DIFFICULTY) {
        difficulty = NextDifficulty(difficulty);
      } else if (selected_item == CONFIG_BACK) {
        menu_screen = MenuScreen::Main;
      }
      break;
    case SDLK_ESCAPE:
      menu_screen = MenuScreen::Main;
      break;
    default:
      break;
    }
  }
}

void processMapSelectionEvents(
    int &active_map_index, int &selected_map_index, MenuScreen &menu_screen,
    bool &quit_requested, Audio *audio, int map_count) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      quit_requested = true;
      continue;
    }

    if (event.type != SDL_KEYDOWN) {
      continue;
    }

    switch (event.key.keysym.sym) {
    case SDLK_UP:
      if (map_count > 1) {
        active_map_index = (active_map_index + map_count - 1) % map_count;
        audio->PlayMenuMove();
      }
      break;
    case SDLK_DOWN:
      if (map_count > 1) {
        active_map_index = (active_map_index + 1) % map_count;
        audio->PlayMenuMove();
      }
      break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      audio->PlayMenuSelect();
      selected_map_index = active_map_index;
      menu_screen = MenuScreen::Main;
      break;
    case SDLK_ESCAPE:
      if (active_map_index != selected_map_index) {
        active_map_index = selected_map_index;
      }
      menu_screen = MenuScreen::Main;
      break;
    default:
      break;
    }
  }
}

void processEditorSelectionEvents(
    int &selected_item, MenuScreen &menu_screen, bool &quit_requested,
    bool &launch_editor, EditorRequest &editor_request, Audio *audio,
    const std::vector<MapDefinition> &available_maps) {
  const int item_count = static_cast<int>(available_maps.size()) + 1;
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      quit_requested = true;
      continue;
    }

    if (event.type != SDL_KEYDOWN) {
      continue;
    }

    switch (event.key.keysym.sym) {
    case SDLK_UP:
      selected_item = (selected_item + item_count - 1) % item_count;
      audio->PlayMenuMove();
      break;
    case SDLK_DOWN:
      selected_item = (selected_item + 1) % item_count;
      audio->PlayMenuMove();
      break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      audio->PlayMenuSelect();
      if (selected_item == 0) {
        menu_screen = MenuScreen::EditorSizeSelection;
      } else {
        const int map_index = selected_item - 1;
        editor_request = {false, available_maps[map_index].file_path,
                          available_maps[map_index].display_name, 0, 0};
        launch_editor = true;
      }
      break;
    case SDLK_ESCAPE:
      menu_screen = MenuScreen::Main;
      break;
    default:
      break;
    }
  }
}

void processEditorSizeSelectionEvents(
    int &selected_item, MenuScreen &menu_screen, bool &quit_requested,
    bool &launch_editor, EditorRequest &editor_request, Audio *audio) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      quit_requested = true;
      continue;
    }

    if (event.type != SDL_KEYDOWN) {
      continue;
    }

    switch (event.key.keysym.sym) {
    case SDLK_UP:
      selected_item = (selected_item + 2) % 3;
      audio->PlayMenuMove();
      break;
    case SDLK_DOWN:
      selected_item = (selected_item + 1) % 3;
      audio->PlayMenuMove();
      break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      audio->PlayMenuSelect();
      editor_request = CreateNewMapRequest(
          static_cast<NewMapSize>(selected_item));
      launch_editor = true;
      break;
    case SDLK_ESCAPE:
      menu_screen = MenuScreen::EditorSelection;
      break;
    default:
      break;
    }
  }
}

void processOverlayEvents(bool &quit_requested) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      quit_requested = true;
    }

    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
      quit_requested = true;
    }
  }
}

void processEndScreenEvents(bool &return_to_menu, bool &quit_requested,
                            bool allow_dismiss) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      quit_requested = true;
      return;
    }

    if (!allow_dismiss) {
      continue;
    }

    if (event.type == SDL_KEYDOWN) {
      return_to_menu = true;
      return;
    }
  }
}

} // namespace

int main() {
  std::cout << "BobMan starting up ...\n";

  const int countdown_seconds = std::clamp(GAME_START_COUNTDOWN, 3, 9);
  const int menu_music_fade_ms = std::max(0, MENU_MUSIC_FADE_OUT_MS);
  std::vector<MapDefinition> available_maps = Map::DiscoverAvailableMaps();
  if (available_maps.empty()) {
    std::cerr << "No maps found in "
              << Paths::DescribeMapSearchDirectories() << "\n";
    return 1;
  }

  AppSettings app_settings = LoadAppSettings();

  bool quit_application = false;
  bool maps_need_reload = false;
  Difficulty difficulty = app_settings.difficulty;
  std::string preferred_selected_map_path = app_settings.selected_map_path;
  int selected_map_index = 0;
  int active_map_index = 0;
  int main_selected_item = MENU_START;
  int config_selected_item = CONFIG_DIFFICULTY;
  int editor_selected_item = 0;
  int editor_size_selected_item = 0;
  std::shared_ptr<Audio> audio(new Audio());

  RefreshSelectedMap(available_maps, preferred_selected_map_path,
                     selected_map_index, active_map_index);
  preferred_selected_map_path = available_maps[selected_map_index].file_path;
  if (app_settings.selected_map_path != preferred_selected_map_path) {
    PersistAppSettings(difficulty, preferred_selected_map_path);
  }

  while (!quit_application) {
    if (maps_need_reload) {
      available_maps = Map::DiscoverAvailableMaps();
      if (available_maps.empty()) {
        std::cerr << "No maps found in "
                  << Paths::DescribeMapSearchDirectories() << "\n";
        return 1;
      }
      RefreshSelectedMap(available_maps, preferred_selected_map_path,
                         selected_map_index, active_map_index);
      const std::string resolved_selected_map_path =
          available_maps[selected_map_index].file_path;
      if (preferred_selected_map_path != resolved_selected_map_path) {
        preferred_selected_map_path = resolved_selected_map_path;
        PersistAppSettings(difficulty, preferred_selected_map_path);
      }
      editor_selected_item =
          std::clamp(editor_selected_item, 0,
                     static_cast<int>(available_maps.size()));
      maps_need_reload = false;
    }

    std::shared_ptr<Map> map(new Map(available_maps[active_map_index].file_path));
    std::shared_ptr<Events> events(new Events());
    std::shared_ptr<Game> game(
        new Game(map.get(), events.get(), audio.get(), difficulty));
    Renderer renderer(map.get(), game.get());
    auto refreshPreview = [&]() {
      map.reset(new Map(available_maps[active_map_index].file_path));
      events.reset(new Events());
      game.reset(new Game(map.get(), events.get(), audio.get(), difficulty));
      renderer.SetScene(map.get(), game.get());
    };

    bool start_requested = false;
    bool launch_editor = false;
    MenuScreen menu_screen = MenuScreen::Main;
    EditorRequest editor_request;
    audio->StartMenuMusic();

    while (!quit_application && !start_requested && !launch_editor) {
      const std::vector<std::string> map_display_names =
          GetMapDisplayNames(available_maps);
      std::vector<std::string> editor_items;
      editor_items.reserve(map_display_names.size() + 1);
      editor_items.push_back("+ neue Karte");
      editor_items.insert(editor_items.end(), map_display_names.begin(),
                          map_display_names.end());
      editor_selected_item =
          std::clamp(editor_selected_item, 0,
                     static_cast<int>(editor_items.size()) - 1);

      if (menu_screen == MenuScreen::Main) {
        processMainMenuEvents(main_selected_item, menu_screen, start_requested,
                              quit_application, audio.get());
        renderer.RenderStartMenu(main_selected_item,
                                 available_maps[selected_map_index].display_name,
                                 "");
      } else if (menu_screen == MenuScreen::Config) {
        const Difficulty previous_difficulty = difficulty;
        processConfigMenuEvents(config_selected_item, difficulty, menu_screen,
                                quit_application, audio.get());
        if (difficulty != previous_difficulty) {
          PersistAppSettings(difficulty, preferred_selected_map_path);
          refreshPreview();
        }
        renderer.RenderConfigMenu(config_selected_item, difficulty);
      } else if (menu_screen == MenuScreen::MapSelection) {
        const int previous_selected_map_index = selected_map_index;
        const int previous_active_map_index = active_map_index;
        processMapSelectionEvents(active_map_index, selected_map_index,
                                  menu_screen, quit_application, audio.get(),
                                  static_cast<int>(available_maps.size()));
        if (selected_map_index != previous_selected_map_index) {
          preferred_selected_map_path =
              available_maps[selected_map_index].file_path;
          PersistAppSettings(difficulty, preferred_selected_map_path);
        }
        if (active_map_index != previous_active_map_index) {
          refreshPreview();
        }
        renderer.RenderMapSelectionMenu(map_display_names, active_map_index);
      } else if (menu_screen == MenuScreen::EditorSelection) {
        processEditorSelectionEvents(editor_selected_item, menu_screen,
                                     quit_application, launch_editor,
                                     editor_request, audio.get(),
                                     available_maps);
        renderer.RenderEditorSelectionMenu(editor_items, editor_selected_item);
      } else {
        processEditorSizeSelectionEvents(
            editor_size_selected_item, menu_screen, quit_application,
            launch_editor, editor_request, audio.get());
        renderer.RenderEditorSizeSelectionMenu(editor_size_selected_item);
      }

      sleep(40);
    }

    if (quit_application) {
      break;
    }

    if (launch_editor) {
      audio->StopMenuMusic();
      const EditorResult editor_result =
          RunEditorSession(editor_request, audio.get());
      if (editor_result.quit_requested) {
        quit_application = true;
        break;
      }

      maps_need_reload = true;
      if (!editor_result.preferred_map_path.empty()) {
        preferred_selected_map_path = editor_result.preferred_map_path;
        PersistAppSettings(difficulty, preferred_selected_map_path);
      }
      continue;
    }

    preferred_selected_map_path = available_maps[selected_map_index].file_path;

    if (menu_music_fade_ms > 0 && audio->FadeOutMenuMusic(menu_music_fade_ms)) {
      const Uint32 fade_started = SDL_GetTicks();
      while (!quit_application && audio->IsMenuMusicPlaying() &&
             SDL_GetTicks() - fade_started <=
                 static_cast<Uint32>(menu_music_fade_ms + 250)) {
        processOverlayEvents(quit_application);
        renderer.RenderStartMenu(main_selected_item,
                                 available_maps[selected_map_index].display_name,
                                 "");
        sleep(16);
      }
    } else {
      audio->StopMenuMusic();
    }

    if (quit_application) {
      break;
    }

    for (int remaining = countdown_seconds; remaining > 0 && !quit_application;
         remaining--) {
      audio->PlayCountdownTick();
      const Uint32 second_started = SDL_GetTicks();
      while (!quit_application && SDL_GetTicks() - second_started < 1000) {
        processOverlayEvents(quit_application);
        renderer.RenderCountdown(remaining);
        sleep(16);
      }
    }

    if (quit_application) {
      break;
    }

    audio->PlayStartTrumpet();
    game->StartSimulation();

    bool return_to_menu = false;
    bool frozen_end_screen = false;
    Uint32 end_screen_started = 0;
    while ((!events->is_quit() || frozen_end_screen) && !return_to_menu &&
           !quit_application) {
      if (!game->is_lost() && !game->is_won()) {
        events->update();
        if (!events->is_quit()) {
          game->Update();
        }
        if ((game->is_lost() || game->is_won()) && !frozen_end_screen) {
          events->RequestQuit();
          frozen_end_screen = true;
          end_screen_started = SDL_GetTicks();
        }
      } else {
        if ((game->is_lost() || game->is_won()) && !frozen_end_screen) {
          events->RequestQuit();
          frozen_end_screen = true;
          end_screen_started = SDL_GetTicks();
        }
        if (game->is_lost()) {
          game->Update();
        }
        const bool allow_end_screen_dismiss =
            end_screen_started != 0 &&
            SDL_GetTicks() - end_screen_started >= kEndScreenMinimumMs;
        processEndScreenEvents(return_to_menu, quit_application,
                               allow_end_screen_dismiss);
      }

      renderer.Render();
      sleep(40);
    }

    audio->StopEndScreenMusic();
  }

  std::cout << "Quitting app.\n";
}
