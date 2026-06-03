// CarHandler.hpp

#pragma once

#include "Parse.hpp"

#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <functional>
#include <utility>

struct Exterior {
    std::optional<unsigned long long> paint_id;
    std::optional<bool> insured;
};

struct Interior {
    std::optional<std::string> interior_color;
    std::optional<std::string> seat_material;
    std::optional<bool> heated_seats;
};

struct Engine {
    std::optional<unsigned long long> horsepower;
    std::optional<unsigned long long> mpg;
    std::optional<double> engine_price;
};

struct Car {
    std::optional<std::string> color;
    std::optional<unsigned long long> top_speed;
    std::optional<double> price;

    std::vector<Engine> engines;
    std::vector<Interior> interiors;
    std::vector<Exterior> exteriors;
};

struct CarColor {};
struct CarTopSpeed {};
struct CarPrice {};
struct EngineHorsepower {};
struct EngineMpg {};
struct EnginePrice {};
struct InteriorColor {};
struct SeatMaterial {};
struct HeatedSeats {};
struct PaintId {};
struct Insured {};
struct UnknownPath {};

using PathTag = std::variant<
    CarColor,
    CarTopSpeed,
    CarPrice,
    EngineHorsepower,
    EngineMpg,
    EnginePrice,
    InteriorColor,
    SeatMaterial,
    HeatedSeats,
    PaintId,
    Insured,
    UnknownPath
>;

inline PathTag classify_path(const std::vector<std::string>& path) {
    if (path.size() == 3 && path[0] == "cars" && path[1] == "car") {
        const std::string_view field = path[2];

        if (field == "color") return CarColor{};
        if (field == "top_speed") return CarTopSpeed{};
        if (field == "price") return CarPrice{};

        return UnknownPath{};
    }

    if (path.size() == 5 &&
        path[0] == "cars" &&
        path[1] == "car" &&
        path[2] == "engines" &&
        path[3] == "engine") {

        const std::string_view field = path[4];

        if (field == "horsepower") return EngineHorsepower{};
        if (field == "mpg") return EngineMpg{};
        if (field == "engine_price") return EnginePrice{};

        return UnknownPath{};
    }

    if (path.size() == 5 &&
        path[0] == "cars" &&
        path[1] == "car" &&
        path[2] == "interiors" &&
        path[3] == "interior") {

        const std::string_view field = path[4];

        if (field == "interior_color") return InteriorColor{};
        if (field == "seat_material") return SeatMaterial{};
        if (field == "heated_seats") return HeatedSeats{};

        return UnknownPath{};
    }

    if (path.size() == 5 &&
        path[0] == "cars" &&
        path[1] == "car" &&
        path[2] == "exteriors" &&
        path[3] == "exterior") {

        const std::string_view field = path[4];

        if (field == "paint_id") return PaintId{};
        if (field == "insured") return Insured{};

        return UnknownPath{};
    }

    return UnknownPath{};
}

inline PathTag classify_car_attribute(std::string_view name) {
    if (name == "color") return CarColor{};
    if (name == "top_speed") return CarTopSpeed{};
    if (name == "price") return CarPrice{};

    return UnknownPath{};
}

inline PathTag classify_engine_attribute(std::string_view name) {
    if (name == "horsepower") return EngineHorsepower{};
    if (name == "mpg") return EngineMpg{};
    if (name == "engine_price") return EnginePrice{};

    return UnknownPath{};
}

inline PathTag classify_interior_attribute(std::string_view name) {
    if (name == "interior_color") return InteriorColor{};
    if (name == "seat_material") return SeatMaterial{};
    if (name == "heated_seats") return HeatedSeats{};

    return UnknownPath{};
}

inline PathTag classify_exterior_attribute(std::string_view name) {
    if (name == "paint_id") return PaintId{};
    if (name == "insured") return Insured{};

    return UnknownPath{};
}

class CarHandler {
public:
    using CarSink = std::function<void(Car&&)>;

    explicit CarHandler(CarSink on_car)
        : on_car_(std::move(on_car)) {}

    void on_start_element(std::string_view name, const std::vector<std::string>& path) {
        if (path.size() == 2 && path[0] == "cars" && name == "car") {
            inside_car_ = true;
            current_ = Car{};
            return;
        }

        if (!inside_car_) return;

        if (path.size() == 4 &&
            path[0] == "cars" &&
            path[1] == "car" &&
            path[2] == "engines" &&
            name == "engine") {
            current_.engines.emplace_back();
            return;
        }

        if (path.size() == 4 &&
            path[0] == "cars" &&
            path[1] == "car" &&
            path[2] == "interiors" &&
            name == "interior") {
            current_.interiors.emplace_back();
            return;
        }

        if (path.size() == 4 &&
            path[0] == "cars" &&
            path[1] == "car" &&
            path[2] == "exteriors" &&
            name == "exterior") {
            current_.exteriors.emplace_back();
            return;
        }
    }

    void on_attribute(
        std::string_view name,
        std::string_view value,
        const std::vector<std::string>& path
    ) {
        if (!inside_car_) return;

        const auto trimmed = xmlparse::trim_view(value);
        if (trimmed.empty()) return;

        insert_attribute(current_, path, name, trimmed);
    }

    void on_text(std::string_view text, const std::vector<std::string>& path) {
        if (!inside_car_) return;

        const auto trimmed = xmlparse::trim_view(text);
        if (trimmed.empty()) return;

        insert_text(current_, path, trimmed);
    }

    void on_end_element(std::string_view name, const std::vector<std::string>& path) {
        if (path.size() == 2 && path[0] == "cars" && name == "car") {
            on_car_(std::move(current_));
            current_ = Car{};
            inside_car_ = false;
        }
    }

private:
    static bool is_car_path(const std::vector<std::string>& path) {
        return path.size() == 2 &&
               path[0] == "cars" &&
               path[1] == "car";
    }

    static bool is_engine_path(const std::vector<std::string>& path) {
        return path.size() == 4 &&
               path[0] == "cars" &&
               path[1] == "car" &&
               path[2] == "engines" &&
               path[3] == "engine";
    }

    static bool is_interior_path(const std::vector<std::string>& path) {
        return path.size() == 4 &&
               path[0] == "cars" &&
               path[1] == "car" &&
               path[2] == "interiors" &&
               path[3] == "interior";
    }

    static bool is_exterior_path(const std::vector<std::string>& path) {
        return path.size() == 4 &&
               path[0] == "cars" &&
               path[1] == "car" &&
               path[2] == "exteriors" &&
               path[3] == "exterior";
    }

    static void insert_text(
        Car& car,
        const std::vector<std::string>& path,
        std::string_view text
    ) {
        const PathTag tag = classify_path(path);
        insert_by_tag(car, tag, text);
    }

    static void insert_attribute(
        Car& car,
        const std::vector<std::string>& path,
        std::string_view name,
        std::string_view value
    ) {
        PathTag tag = UnknownPath{};

        if (is_car_path(path)) {
            tag = classify_car_attribute(name);
        } else if (is_engine_path(path)) {
            tag = classify_engine_attribute(name);
        } else if (is_interior_path(path)) {
            tag = classify_interior_attribute(name);
        } else if (is_exterior_path(path)) {
            tag = classify_exterior_attribute(name);
        }

        insert_by_tag(car, tag, value);
    }

    static void insert_by_tag(
        Car& car,
        const PathTag& tag,
        std::string_view text
    ) {
        std::visit(
            [&](auto concrete_tag) {
                insert_value(car, concrete_tag, text);
            },
            tag
        );
    }

    static void insert_value(Car& car, CarColor, std::string_view text) {
        car.color = xmlparse::parse_string(text);
    }

    static void insert_value(Car& car, CarTopSpeed, std::string_view text) {
        car.top_speed = xmlparse::parse_u64(text);
    }

    static void insert_value(Car& car, CarPrice, std::string_view text) {
        car.price = xmlparse::parse_double(text);
    }

    static void insert_value(Car& car, EngineHorsepower, std::string_view text) {
        if (!car.engines.empty()) {
            car.engines.back().horsepower = xmlparse::parse_u64(text);
        }
    }

    static void insert_value(Car& car, EngineMpg, std::string_view text) {
        if (!car.engines.empty()) {
            car.engines.back().mpg = xmlparse::parse_u64(text);
        }
    }

    static void insert_value(Car& car, EnginePrice, std::string_view text) {
        if (!car.engines.empty()) {
            car.engines.back().engine_price = xmlparse::parse_double(text);
        }
    }

    static void insert_value(Car& car, InteriorColor, std::string_view text) {
        if (!car.interiors.empty()) {
            car.interiors.back().interior_color = xmlparse::parse_string(text);
        }
    }

    static void insert_value(Car& car, SeatMaterial, std::string_view text) {
        if (!car.interiors.empty()) {
            car.interiors.back().seat_material = xmlparse::parse_string(text);
        }
    }

    static void insert_value(Car& car, HeatedSeats, std::string_view text) {
        if (!car.interiors.empty()) {
            car.interiors.back().heated_seats = xmlparse::parse_bool(text);
        }
    }

    static void insert_value(Car& car, PaintId, std::string_view text) {
        if (!car.exteriors.empty()) {
            car.exteriors.back().paint_id = xmlparse::parse_u64(text);
        }
    }

    static void insert_value(Car& car, Insured, std::string_view text) {
        if (!car.exteriors.empty()) {
            car.exteriors.back().insured = xmlparse::parse_bool(text);
        }
    }

    static void insert_value(Car&, UnknownPath, std::string_view) {
        // Unknown path or attribute. Ignore, count, or log depending on policy.
    }

    std::function<void(Car&&)> on_car_;
    Car current_;
    bool inside_car_ = false;
};

inline void print_car(const Car& car) {
    std::cout << "Car\n";

    if (car.color) std::cout << "  color: " << *car.color << '\n';
    if (car.top_speed) std::cout << "  top_speed: " << *car.top_speed << '\n';
    if (car.price) std::cout << "  price: " << *car.price << '\n';

    for (const auto& engine : car.engines) {
        std::cout << "  Engine\n";
        if (engine.horsepower) std::cout << "    horsepower: " << *engine.horsepower << '\n';
        if (engine.mpg) std::cout << "    mpg: " << *engine.mpg << '\n';
        if (engine.engine_price) std::cout << "    engine_price: " << *engine.engine_price << '\n';
    }

    for (const auto& interior : car.interiors) {
        std::cout << "  Interior\n";
        if (interior.interior_color) std::cout << "    interior_color: " << *interior.interior_color << '\n';
        if (interior.seat_material) std::cout << "    seat_material: " << *interior.seat_material << '\n';
        if (interior.heated_seats) {
            std::cout << "    heated_seats: " << std::boolalpha << *interior.heated_seats << '\n';
        }
    }

    for (const auto& exterior : car.exteriors) {
        std::cout << "  Exterior\n";
        if (exterior.paint_id) std::cout << "    paint_id: " << *exterior.paint_id << '\n';
        if (exterior.insured) {
            std::cout << "    insured: " << std::boolalpha << *exterior.insured << '\n';
        }
    }
}