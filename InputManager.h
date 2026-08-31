// OpenGL Rollercoaster Simulation
// Copyright (C) 2026 Aven Furness
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef GRAPHICS_COURSEWORK_INPUTMANAGER_H
#define GRAPHICS_COURSEWORK_INPUTMANAGER_H
#include <functional>
#include <map>
#include <GLFW/glfw3.h>

struct InputMapping {
    int key;
    std::function<void()> function;
    bool special = false;
};

struct HoldMapping {
    int key;
    std::function<void(float)> function;
    bool special = false;
};

class InputManager {

    static std::unordered_map<int, bool> hold_keys;
    static std::map<int, std::function<void(float)>> hold_functions;

    static std::map<int, std::function<void()>> tap_functions;

public:

    //handle ASCII input
    static void handle_input(GLFWwindow* window, int key, int scancode, int action, int mods);

    static void update(float delta_time);

    static void add_hold_mapping(int key, std::function<void(float)> func, bool special=false);
    static void add_hold_mappings(const std::vector<HoldMapping>& mappings);

    static void add_tap_mapping(int key, std::function<void()> func, bool special=false);
    static void add_tap_mappings(const std::vector<InputMapping>& mappings);
};


#endif //GRAPHICS_COURSEWORK_INPUTMANAGER_H