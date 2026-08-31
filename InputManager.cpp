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

#include <cctype>
#include <utility>

#include "InputManager.h"

std::unordered_map<int, bool> InputManager::hold_keys;
std::map<int, std::function<void(float)>> InputManager::hold_functions;
std::map<int, std::function<void()>> InputManager::tap_functions;

void InputManager::handle_input(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (std::isupper(key)) key = std::tolower(key);
    bool set = action == GLFW_PRESS || GLFW_REPEAT;
    hold_keys[key + 256] = set;
    if (tap_functions.count(key + 256)) tap_functions[key + 256]();
}

void InputManager::update(const float delta_time) {
    for (const auto&[key, func] : hold_functions) {
        if (hold_keys[key]) {
            func(delta_time);
        }
    }

}

void InputManager::add_hold_mapping(int key, std::function<void(float)> func, bool special) {
    if (special) key += 256;
    hold_functions[key] = std::move(func);
}

void InputManager::add_hold_mappings(const std::vector<HoldMapping>& mappings) {
    for (const auto& [key, func, special] : mappings) {
        add_hold_mapping(key, func, special);
    }
}

void InputManager::add_tap_mapping(int key, std::function<void()> func, bool special) {
    if (special) key += 256;
    tap_functions[key] = std::move(func);
}

void InputManager::add_tap_mappings(const std::vector<InputMapping> &mappings) {
    for (const auto& [key, func, special] : mappings) {
        add_tap_mapping(key, func, special);
    }
}
