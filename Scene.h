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

#ifndef GRAPHICS_COURSEWORK_SCENE_H
#define GRAPHICS_COURSEWORK_SCENE_H
#include <memory>
#include <vector>
#include <nlohmann/json.hpp>

#include "CoasterTrack.h"
#include "Light.h"
using json = nlohmann::json;

#include <GLFW/glfw3.h>

#include "objects/SceneObject.h"


class Scene {
public:
    enum Direction {
        FORWARDS,
        BACKWARDS,
        LEFT,
        RIGHT,
        UP,
        DOWN,
    };

    enum Axis {
        X,
        Y,
    };

    enum View {
        FREEROAM,
        COASTER,
    };

    Scene(int screen_width, int screen_height, const json& given_json_data);

    void update(double time_elapsed, double prev_time_elapsed);
    void update_view(double time_elapsed);
    void render(GLFWwindow* window) const;

    void add_object(const std::shared_ptr<SceneObject>& obj);
    void add_objects(const std::vector<std::shared_ptr<SceneObject>>& given_objects);

    void remove_object(const std::shared_ptr<SceneObject>& obj);
    void remove_objects(const std::vector<std::shared_ptr<SceneObject>>& given_objects);

    void add_coaster(const std::shared_ptr<CoasterTrack>& coaster);
    void remove_coaster(const std::shared_ptr<CoasterTrack> &coaster);

    void add_track_to_coaster(const std::shared_ptr<CoasterTrack>& coaster, const std::string &type);
    void pop_track_from_coaster(const std::shared_ptr<CoasterTrack>& coaster);
    void load_coaster_from_file(const std::shared_ptr<CoasterTrack>& coaster);

    void set_on_update(const std::function<void(int)> &func);

    void move(Direction direction, float amount);
    void rotate(Axis axis, float given_rotation);
    void set_view(View view);
    View get_view() const;

    void screen_resize(int width, int height);

    [[nodiscard]] float get_speed() const;

private:
    std::vector<std::shared_ptr<CoasterTrack>> coasters;
    std::vector<std::shared_ptr<SceneObject>> objects;

    std::function<void(double)> on_update;

    json json_data;
    void load_config(const json& given_json_data);

    View view_type;
    glm::vec3 camera_pos{};
    glm::vec3 rotation{};
    float speed{};

    glm::mat4 view_matrix{};
    glm::mat4 proj_matrix{};

    Light light{};

    int screen_width;
    int screen_height;
};


#endif //GRAPHICS_COURSEWORK_SCENE_H