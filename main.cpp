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

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <GLFW/glfw3.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <functional>
#include <vector>

#include "CoasterTrack.h"
#include "InputManager.h"
#include "Scene.h"
#include "objects/Cuboid.h"
#include "objects/ModelObject.h"

namespace Application {
    constexpr int SCREEN_WIDTH = 1000;
    constexpr int SCREEN_HEIGHT = 800;
    double time_elapsed = 0;
    double prev_time_elapsed = 0;
    std::unique_ptr<Scene> scene = nullptr;


    GLFWwindow* window;

    json get_json(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) {
            std::cerr << "Could not open file: " << path << std::endl;
            return json::object();
        }
        try {
            return json::parse(f);
        } catch (const json::parse_error& e) {
            std::cerr << "Parse error in " << path << ": " << e.what() << std::endl;
            return json::object();
        }
    }

    void update() {
        time_elapsed = glfwGetTime() * 1000.0;
        const auto delta_time = static_cast<float>(time_elapsed - prev_time_elapsed);
        scene->update(time_elapsed, prev_time_elapsed);
        InputManager::update(delta_time);
        prev_time_elapsed = time_elapsed;
    }

    void run() {
        while (!glfwWindowShouldClose(window)) {
            scene->render(window);
            glfwPollEvents();
            update();
        }
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void screen_resize(GLFWwindow* given_window, const int width, const int height) {
        scene->screen_resize(width, height);
    }

    void load_shaders_json(const std::string &shaders_folder) {
        for (const auto& file : std::filesystem::directory_iterator(shaders_folder)) {
            if (file.path().extension() != ".json") continue;
            json shader_json = get_json(file.path().string());
            SceneObject::add_shader(shader_json["name"], shader_json["vert_path"], shader_json["frag_path"]);
        }
    }

    void load_materials_json(const std::string &materials_folder) {
        for (const auto& file : std::filesystem::directory_iterator(materials_folder)) {
            if (file.path().extension() != ".json") continue;
            json material_json = get_json(file.path().string());
            SceneObject::add_material(material_json["name"], {material_json["material_ambient"], material_json["material_diffuse"], material_json["material_specular"], material_json["material_shininess"]});
        }
    }

    void load_models_json(const std::string &models_folder) {
        for (const auto& file : std::filesystem::directory_iterator(models_folder)) {
            if (file.path().extension() != ".json") continue;
            json model = get_json(file.path().string());
            ModelObject::add_model(model["name"], {model["model_path"], model["shader_name"], model["material_name"]});
        }
    }

    void load_tracks_json(const std::string &tracks_file) {
        json tracks_json = get_json(tracks_file);
        if (!tracks_json.is_object() || !tracks_json.contains("tracks")) return;
        for (const auto& track : tracks_json["tracks"]) {
            std::vector<glm::vec3> points;
            for (const auto& point : track["points"]) {
                points.emplace_back(point[0], point[1], point[2]);
            }
            CoasterTrack::new_track({track["name"], CoasterTrack::TrackData{track["piece_model"], glm::radians(static_cast<float>(track["rotation"])), {track["start_point"][0], track["start_point"][1], track["start_point"][2]}, {track["end_point"][0], track["end_point"][1], track["end_point"][2]}, points}});
        }
    }

    void load_objects_json(const std::string &objects_file) {
        json objects_json = get_json(objects_file);
        if (!objects_json.is_object() || !objects_json.contains("objects")) return;
        for (const auto& object : objects_json["objects"]) {
            if (object["obj_type"] == "Cuboid") {
                scene->add_object(std::make_shared<Cuboid>(glm::vec3{object["position"][0], object["position"][1], object["position"][2]}, glm::vec3{object["dimensions"][0], object["dimensions"][1], object["dimensions"][2]}, object["shader_name"], object["material_name"], object["name"]));
                continue;
            }
            if (object["obj_type"] == "Model") {
                scene-> add_object(std::make_shared<ModelObject>(object["model_name"], glm::vec3{object["position"][0], object["position"][1], object["position"][2]}, glm::vec3{object["dimensions"][0], object["dimensions"][1], object["dimensions"][2]}, object["name"]));
            }
        }
    }

    void load_from_json(const std::string& shaders_folder, const std::string& materials_folder, const std::string& models_folder, const std::string &tracks_folder, const std::string &objects_json) {
        load_shaders_json(shaders_folder);
        load_materials_json(materials_folder);
        load_models_json(models_folder);
        load_tracks_json(tracks_folder);
        load_objects_json(objects_json);
    }

    std::shared_ptr<CoasterTrack> load_default_coaster() {
        return std::make_shared<CoasterTrack>("config/default_coaster.json", glm::vec3{-20, 1, -60}, get_json("config/default_coaster.json")["cart_model"]);
    }

    void init(int screen_width, int screen_height, const std::string& scene_config) {

        if (!glfwInit()) throw std::runtime_error("GLFW did not initialise.");

        window = glfwCreateWindow(screen_width, screen_height, "Test", nullptr, nullptr);
        if (!window) {glfwTerminate(); throw std::runtime_error("GLFW failed to create window.");}

        glfwMakeContextCurrent(window);

        glewExperimental = GL_TRUE;
        if (const GLenum err = glewInit(); GLEW_OK != err) {
            std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
        }

        glfwSetKeyCallback(window, InputManager::handle_input);

        glfwSetFramebufferSizeCallback(window, screen_resize);

        scene = std::make_unique<Scene>(screen_width, screen_height, get_json(scene_config));

        int width_given, height_given;
        glfwGetFramebufferSize(window, &width_given, &height_given);
        screen_resize(window, width_given, height_given);
    }
}

int main() {
    Application::init(Application::SCREEN_WIDTH, Application::SCREEN_HEIGHT, "config/scene_config.json");
    Application::load_from_json("config/shaders", "config/materials", "config/models", "config/tracks.json", "config/objects.json");

    auto coaster = Application::load_default_coaster();
    Application::scene->add_coaster(coaster);

    InputManager::add_hold_mappings({
        // movement
        {GLFW_KEY_W, [](const float dt){Application::scene->move(Scene::FORWARDS, Application::scene->get_speed() * dt / 16.0f);}},
        {GLFW_KEY_S, [](const float dt){Application::scene->move(Scene::BACKWARDS, Application::scene->get_speed() * dt / 16.0f);}},
        {GLFW_KEY_A, [](const float dt){Application::scene->move(Scene::LEFT, Application::scene->get_speed() * dt / 16.0f);}},
        {GLFW_KEY_D, [](const float dt){Application::scene->move(Scene::RIGHT, Application::scene->get_speed() * dt / 16.0f);}},
        {GLFW_KEY_SPACE, [](const float dt){Application::scene->move(Scene::UP, Application::scene->get_speed() * dt / 16.0f);}},
        {GLFW_KEY_LEFT_SHIFT, [](const float dt){Application::scene->move(Scene::DOWN, Application::scene->get_speed() * dt / 16.0f);}},

        // rotation
        {GLFW_KEY_LEFT, [](const float dt){Application::scene->rotate(Scene::X, -0.02f * dt / 16.0f);}},
        {GLFW_KEY_RIGHT, [](const float dt){Application::scene->rotate(Scene::X, 0.02f * dt / 16.0f);}},
        {GLFW_KEY_UP, [](const float dt){Application::scene->rotate(Scene::Y, -0.013f * dt / 16.0f);}},
        {GLFW_KEY_DOWN, [](const float dt){Application::scene->rotate(Scene::Y, 0.013f * dt / 16.0f);}},

        // exit on esc
        {GLFW_KEY_ESCAPE, [](float){glfwSetWindowShouldClose(Application::window, true);}},
    });

    InputManager::add_tap_mappings({
        // edit track
        {GLFW_KEY_1, [coaster]{Application::scene->add_track_to_coaster(coaster, "straight");}},
        {GLFW_KEY_2, [coaster]{Application::scene->add_track_to_coaster(coaster, "left");}},
        {GLFW_KEY_3, [coaster]{Application::scene->add_track_to_coaster(coaster, "right");}},
        {GLFW_KEY_4, [coaster]{Application::scene->add_track_to_coaster(coaster, "loop");}},
        {GLFW_KEY_5, [coaster]{Application::scene->add_track_to_coaster(coaster, "step_up");}},
        {GLFW_KEY_6, [coaster]{Application::scene->add_track_to_coaster(coaster, "step_down");}},
        {GLFW_KEY_BACKSPACE, [coaster]{Application::scene->pop_track_from_coaster(coaster);}},

        // save or load track
        {GLFW_KEY_P, [coaster]{coaster->save_to_file();}},
        {GLFW_KEY_L, [coaster]{Application::scene->load_coaster_from_file(coaster);}},

        // toggle view
        {GLFW_KEY_V, [] {
                if (Application::scene->get_view() == Scene::COASTER) Application::scene->set_view(Scene::FREEROAM);
                else Application::scene ->set_view(Scene::COASTER);
        }},
        {GLFW_KEY_G, [coaster]{ coaster->start_cart(0.001f);}},
        {GLFW_KEY_EQUAL, [coaster]{coaster->change_cart_speed(0.0003);}},
        {GLFW_KEY_MINUS, [coaster]{coaster->change_cart_speed(-0.0003);}},
        {GLFW_KEY_H, [coaster]{ coaster->stop_cart();}},
    });

    Application::run();

    return 0;
}
