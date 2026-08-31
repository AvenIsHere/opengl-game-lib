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

#include "Scene.h"


#include "GlmMaths.h"

#include <cmath>
#include <fstream>
#include <random>
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "objects/Cuboid.h"

Scene::Scene(const int screen_width, const int screen_height, const json& given_json_data) {
    this->screen_width = screen_width;
    this->screen_height = screen_height;
    this->view_type = FREEROAM;

    load_config(given_json_data);

}

void Scene::render(GLFWwindow* window) const {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (const auto & object : objects) {
        object->draw(view_matrix, proj_matrix, light);
    }

    glfwSwapBuffers(window);
}

void Scene::add_object(const std::shared_ptr<SceneObject>& obj) {
    this->objects.push_back(obj);
}

void Scene::add_objects(const std::vector<std::shared_ptr<SceneObject>>& given_objects) {
    for (const auto& object : given_objects) {
        add_object(object);
    }
}

void Scene::remove_object(const std::shared_ptr<SceneObject> &obj) {
    if (const auto item = std::ranges::find(this->objects, obj); item != this->objects.end()) {
        this->objects.erase(item);
    }
}

void Scene::remove_objects(const std::vector<std::shared_ptr<SceneObject> > &given_objects) {
    for (const auto& obj : given_objects) {
        remove_object(obj);
    }
}

void Scene::add_coaster(const std::shared_ptr<CoasterTrack>& coaster) {
    coasters.emplace_back(coaster);
    add_objects(coaster->get_model_objs());
}

void Scene::remove_coaster(const std::shared_ptr<CoasterTrack> &coaster) {
    if (const auto& item = std::ranges::find(coasters, coaster); item != coasters.end()) {
        remove_objects(coaster->get_model_objs());
        coasters.erase(item);
    }
}

void Scene::add_track_to_coaster(const std::shared_ptr<CoasterTrack> &coaster, const std::string &type) {
    if (const auto& item = std::ranges::find(coasters, coaster); item != coasters.end()) {
        add_objects(coaster->add_track(type));
    }
}

void Scene::pop_track_from_coaster(const std::shared_ptr<CoasterTrack> &coaster) {
    if (const auto& item = std::ranges::find(coasters, coaster); item != coasters.end()) {
        remove_objects(coaster->pop_track());
    }
}

void Scene::load_coaster_from_file(const std::shared_ptr<CoasterTrack> &coaster) {
    if (const auto& item = std::ranges::find(coasters, coaster); item != coasters.end()) {
        remove_objects(coaster->get_model_objs());
        add_objects(coaster->load_from_file());
    }
}

void Scene::set_on_update(const std::function<void(int)> &func) {
    on_update = func;
}

void Scene::move(const Direction direction, const float amount) {
    const float x_dir = std::sin(rotation.x);
    const float z_dir = -std::cos(rotation.x);

    glm::vec3 pos_prev = camera_pos;

    switch (direction) {
        case FORWARDS:
            camera_pos.x += x_dir * amount;
            camera_pos.z += z_dir * amount;
            break;
        case BACKWARDS:
            camera_pos.x -= x_dir * amount;
            camera_pos.z -= z_dir * amount;
            break;
        case LEFT:
            camera_pos.x -= std::cos(rotation.x) * amount;
            camera_pos.z -= std::sin(rotation.x) * amount;
            break;
        case RIGHT:
            camera_pos.x += std::cos(rotation.x) * amount;
            camera_pos.z += std::sin(rotation.x) * amount;
            break;
        case UP:
            camera_pos.y += amount;
            break;
        case DOWN:
            camera_pos.y -= amount;
    }

    glm::vec3 camera_box_start = {camera_pos.x - 0.2, camera_pos.y - 0.2, camera_pos.z - 0.2};
    glm::vec3 camera_box_end = {camera_pos.x + 0.2, camera_pos.y + 0.2, camera_pos.z + 0.2};

    for (const auto& obj : objects) {
        if (obj->name == "sky") continue;
        if (obj->colliding(camera_box_start, camera_box_end)) {
            camera_pos = pos_prev;
        }
    }
}

void Scene::rotate(const Axis axis, const float given_rotation) {
    switch (axis) {
        case X:
            rotation.x += given_rotation;
            break;
        case Y:
            rotation.y += given_rotation;
            break;
    }
}

void Scene::update_view(double time_elapsed) {

    if (view_type == COASTER && !coasters.empty()) {
        const auto pos = coasters[0]->get_cart_location();
        const auto half_coaster_size = coasters[0]->get_cart_size() / 2.0f;
        camera_pos = pos + half_coaster_size + glm::vec3{0.0f, half_coaster_size.y + 2, 0.0f};
    }
    auto view = glm::mat4(1.0f);
    view = glm::rotate(view, rotation.y, glm::vec3(1.0f, 0.0f, 0.0f));
    view = glm::rotate(view, rotation.x, glm::vec3(0.0f, 1.0f, 0.0f));
    view_matrix = glm::translate(view, -camera_pos);
}

void Scene::update(const double time_elapsed, const double prev_time_elapsed) {

    for (const auto& coaster : coasters) {
        coaster->update(time_elapsed, prev_time_elapsed);
    }

    update_view(time_elapsed);
    if (on_update) on_update(time_elapsed);
}

float Scene::get_speed() const {
    return speed;
}

void Scene::screen_resize(const int width, const int height) {
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    proj_matrix = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
    glViewport(0, 0, width, height);
}

void Scene::load_config(const json& given_json_data) {
    this->json_data = given_json_data;

    const float screen_aspect = static_cast<float>(screen_width) / static_cast<float>(screen_height);

    // camera
    camera_pos = glm::vec3(
        json_data["camera-pos"]["x"],
        json_data["camera-pos"]["y"],
        json_data["camera-pos"]["z"]);
    rotation = {
        json_data["camera-rotation"]["x"],
        json_data["camera-rotation"]["y"],
        json_data["camera-rotation"]["z"]
    };
    speed = json_data["camera-speed"];
    view_matrix = GlmMaths::pos_to_translation(camera_pos);
    proj_matrix = glm::perspective(glm::radians(45.0f), screen_aspect, 0.1f, 1000.0f);

    // lighting
    light.pos = glm::vec4(
        json_data["light-pos"]["x"],
        json_data["light-pos"]["y"],
        json_data["light-pos"]["z"],
        json_data["light-pos"]["w"]);
    for (int i = 0; i < 4; i++) {
        light.ambient[i] = json_data["light-ambient"][i];
        light.diffuse[i] = json_data["light-diffuse"][i];
        light.specular[i] = json_data["light-specular"][i];
    }
}

void Scene::set_view(const View view) {
    view_type = view;
}

Scene::View Scene::get_view() const {
    return view_type;
}
