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

#include "CoasterTrack.h"

#include <cmath>
#include <utility>
#include <glm/ext/matrix_transform.hpp>
#include <fstream>
#include <chrono>
#include <magic_enum/magic_enum.hpp>
#include <nfd.h>
#include <regex>
#include <nlohmann/json.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/spline.hpp>

#include "GlmMaths.h"

using json = nlohmann::json;

#include "nfd.hpp"

std::unordered_map<std::string, CoasterTrack::TrackData> CoasterTrack::tracks;

CoasterTrack::CoasterTrack(const std::vector<std::string> &given_track, const glm::vec3 displacement, const std::string& cart_model) {
    position = displacement;
    coaster_model = cart_model;
    add_tracks(given_track);

}

CoasterTrack::CoasterTrack(const std::string &json_path, glm::vec3 displacement, const std::string& cart_model) {
    position = displacement;
    coaster_model = cart_model;
    add_tracks(parse_json(json_path));

}

CoasterTrack::CoasterTrack(const glm::vec3 displacement, const std::string &cart_model) {
    track = {};
    initial_start_point = std::nullopt;
    position = displacement;
    coaster_model = cart_model;
}

void CoasterTrack::new_track(std::pair<std::string, TrackData> given_track) {
    tracks.emplace(given_track);
}

void CoasterTrack::new_tracks(const std::unordered_map<std::string, TrackData>& given_tracks) {
    for (const auto& track : given_tracks) {
        new_track(track);
    }
}

std::vector<std::shared_ptr<SceneObject>> CoasterTrack::add_track(const std::string& given_track) {
    std::vector<std::shared_ptr<SceneObject>> return_objects;
    track.emplace_back(given_track);
    const auto& data = tracks.at(given_track);
    const glm::vec3 obj_origin = position - rotation_vec(data.start_point);

    const auto object = std::make_shared<ModelObject>(data.piece_model, obj_origin, glm::vec3(1,1,1), given_track);
    model_objects.emplace_back(object);
    return_objects.emplace_back(object);

    object->rotate(rotation.x, {1,0,0});
    object->rotate(rotation.y, {0,1,0});
    object->rotate(rotation.z, {0,0,1});

    if (!initial_start_point) {
        initial_start_point = position;
        segment_positions.emplace_back(position);
        segment_rotations.emplace_back(0.0f);

        cart = std::make_shared<Cart>(position, coaster_model, "cart");
        return_objects.emplace_back(cart);
    } else {
        const auto& prev_data = tracks.at(track[track.size() - 2]);
        float prev_y_rot = segment_rotations.back();
        glm::vec3 prev_pos = segment_positions.back();

        glm::mat4 prev_rotation_mat(1.0f);
        prev_rotation_mat = glm::rotate(prev_rotation_mat, prev_y_rot, glm::vec3(0, 1, 0));

        glm::vec3 next_pos = prev_pos + glm::vec3(prev_rotation_mat * glm::vec4(prev_data.end_point - prev_data.start_point, 0.0f));
        float next_y_rot = prev_y_rot + prev_data.rotation;

        segment_positions.emplace_back(next_pos);
        segment_rotations.emplace_back(next_y_rot);
    }

    handle_movement(given_track, false);
    return return_objects;
}

std::vector<std::shared_ptr<SceneObject>> CoasterTrack::add_tracks(const std::vector<std::string>& given_tracks) {
    std::vector<std::shared_ptr<SceneObject>> return_objects;
    for (const auto& given_track : given_tracks) {
        auto new_objs = add_track(given_track);
        return_objects.insert(return_objects.end(), new_objs.begin(), new_objs.end());
    }
    return return_objects;
}

std::vector<std::shared_ptr<SceneObject>> CoasterTrack::pop_track() {
    std::vector<std::shared_ptr<SceneObject>> return_objects;
    if (!track.empty()) {
        const auto track_type = track.back();
        track.pop_back();
        handle_movement(track_type, true);
        return_objects.push_back(model_objects.back());
        model_objects.pop_back();
        segment_positions.pop_back();
        segment_rotations.pop_back();

        if (track.empty()) {
            if (cart) return_objects.push_back(cart);
            cart = nullptr;
            initial_start_point = std::nullopt;
        }
    }
    return return_objects;
}

glm::vec3 CoasterTrack::rotation_vec(const glm::vec3 direction) const {
    glm::mat4 rotate(1.0f);
    rotate = glm::rotate(rotate, rotation.y, glm::vec3(0,1,0));
    rotate = glm::rotate(rotate, rotation.x, glm::vec3(1,0,0));
    rotate = glm::rotate(rotate, rotation.z, glm::vec3(0,0,1));
    return {rotate * glm::vec4(direction, 0.0f)};
}

void CoasterTrack::handle_movement(const std::string& type, const bool undo) {
    const auto& data = tracks.at(type);
    if (!undo) {
        position += rotation_vec(data.end_point - data.start_point);
        rotation.y += data.rotation;
        return;
    }
    rotation.y -= data.rotation;
    position -= rotation_vec(data.end_point - data.start_point);
}

void CoasterTrack::save_to_file() const {
    const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const auto time_now = std::localtime(&time);
    char buf[64];
    std::strftime(buf, sizeof(buf), "coaster_%Y-%m-%d_%H-%M-%S.json", time_now);
    std::ofstream save_file(buf);

    json json_tracks = json::array();
    for (const auto& given_track : track) {
        json_tracks.push_back(given_track);
    }

    json result = json::object({
        {"tracks", json_tracks}
    });
    save_file << result;
}

std::vector<std::shared_ptr<SceneObject>> CoasterTrack::load_from_file() {
    clear_tracks();

    NFD_Init();
    NFD::UniquePath file_path;
    constexpr nfdu8filteritem_t filters[1] = {{"JSON file", "json"}};
    const nfdresult_t result = NFD::OpenDialog(file_path, filters, 1);
    if (result == NFD_CANCEL) {
        NFD_Quit();
        return {};
    }
    if (result != NFD_OKAY) throw std::runtime_error("Error opening file");
    NFD_Quit();

    const auto tracks_to_add = parse_json(file_path.get());
    return add_tracks(tracks_to_add);
}

std::vector<std::string> CoasterTrack::parse_json(const std::string& file_path) {

    std::ifstream coaster_file(file_path);
    auto json = json::parse(coaster_file);
    auto tracks_to_add = json["tracks"];

    return tracks_to_add;
}

std::vector<std::shared_ptr<SceneObject> > CoasterTrack::clear_tracks() {
    std::vector<std::shared_ptr<SceneObject>> return_tracks;
    auto popped = pop_track();
    while (!popped.empty()) {
        return_tracks.insert(return_tracks.end(), popped.begin(), popped.end());
        popped = pop_track();
    }
    initial_start_point = std::nullopt;
    segment_positions.clear();
    segment_rotations.clear();
    return return_tracks;
}

void CoasterTrack::start_cart(const double speed) {
    is_moving = true;
    cart_speed = speed;
}

void CoasterTrack::set_cart_speed(double speed) {
    if (speed < 0) speed = 0;
    cart_speed = speed;
}

void CoasterTrack::change_cart_speed(const double change) {
    set_cart_speed(cart_speed + change);
}

void CoasterTrack::stop_cart() {
    is_moving = false;
}

std::vector<std::shared_ptr<SceneObject> > CoasterTrack::get_model_objs() const {
    auto objs = model_objects;
    if (cart) objs.push_back(cart);
    return objs;
}

glm::vec3 CoasterTrack::get_cart_location() const {
    return GlmMaths::mat_info(cart->model_matrix).translation;
}

glm::vec3 CoasterTrack::get_cart_size() const {
    return GlmMaths::mat_info(cart->model_matrix).scale;
}

void CoasterTrack::update(double time_elapsed, double prev_time_elapsed) {
    if (!is_moving || track.empty() || !cart) return;

    if (current_segment >= track.size()) {
        current_segment = 0;
        segment_progress = 0.0f;
    }

    const auto delta_time = time_elapsed - prev_time_elapsed;
    if (delta_time <= 0.0f) return;
    segment_progress += cart_speed * delta_time;

    while (segment_progress >= 1.0f) {
        segment_progress -= 1.0f;
        current_segment = (current_segment + 1) % static_cast<int>(track.size());
    }

    glm::vec3 segment_position = segment_positions[current_segment];
    float segment_rotation = segment_rotations[current_segment];

    const auto& data = tracks.at(track[current_segment]);

    std::vector<glm::vec3> control_points;
    control_points.reserve(data.points.size() + 2);
    control_points.emplace_back(data.start_point);
    control_points.insert(control_points.end(), data.points.begin(), data.points.end());
    control_points.emplace_back(data.end_point);

    if (control_points.size() < 2) return;

    const double scaled_progress = segment_progress * static_cast<double>(control_points.size() - 1);
    const int point_index = std::min(
        static_cast<int>(scaled_progress),
        static_cast<int>(control_points.size()) - 2
    );
    const double local_progress = scaled_progress - static_cast<double>(point_index);

    const glm::vec3 p0 = control_points[std::max(point_index - 1, 0)];
    const glm::vec3 p1 = control_points[point_index];
    const glm::vec3 p2 = control_points[point_index + 1];
    const glm::vec3 p3 = control_points[std::min(point_index + 2, static_cast<int>(control_points.size()) - 1)];

    const glm::vec3 local_cart_position = glm::catmullRom(p0, p1, p2, p3, static_cast<float>(local_progress));

    const double tangent_sample_progress = std::min(local_progress + 0.01, 1.0);
    const glm::vec3 local_next_position = glm::catmullRom(p0, p1, p2, p3, static_cast<float>(tangent_sample_progress));
    const glm::vec3 local_tangent = glm::normalize(local_next_position - local_cart_position);

    glm::mat4 segment_rotation_matrix(1.0f);
    segment_rotation_matrix = glm::rotate(segment_rotation_matrix, segment_rotation, glm::vec3(0, 1, 0));

    const auto world_offset = glm::vec3(segment_rotation_matrix * glm::vec4(local_cart_position - data.start_point, 0.0f));
    const glm::vec3 world_cart_position = segment_position + world_offset;

    const glm::vec3 world_tangent = glm::normalize(glm::vec3(segment_rotation_matrix * glm::vec4(local_tangent, 0.0f)));
    float cart_y_rotation = std::atan2(world_tangent.x, world_tangent.z);

    constexpr float cart_y_offset = glm::radians(-90.0f);
    cart_y_rotation += cart_y_offset;

    cart->model_matrix = glm::translate(glm::mat4(1.0f), world_cart_position);
    cart->rotation = glm::vec3(0.0f, cart_y_rotation, 0.0f);
}