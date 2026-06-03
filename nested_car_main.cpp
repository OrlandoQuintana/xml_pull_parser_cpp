// nested_car_main.cpp

#include "XmlEngine.hpp"
#include "CarHandler.hpp"

#include <exception>
#include <iostream>
#include <string>

int main() {
    const std::string xml = R"(
<cars>
    <car>
        <color>red</color>
        <top_speed>185</top_speed>
        <price>72000.50</price>

        <engines>
            <engine horsepower="450">
                <mpg>22</mpg>
                <engine_price>18000.25</engine_price>
            </engine>
            <engine horsepower="520">
                <mpg>18</mpg>
                <engine_price>25000.75</engine_price>
            </engine>
        </engines>

        <interiors>
            <interior>
                <interior_color>black</interior_color>
                <seat_material>leather</seat_material>
                <heated_seats>true</heated_seats>
            </interior>
            <interior>
                <interior_color>tan</interior_color>
                <seat_material>alcantara</seat_material>
                <heated_seats>false</heated_seats>
            </interior>
        </interiors>

        <exteriors>
            <exterior>
                <paint_id>10001</paint_id>
                <insured>true</insured>
            </exterior>
            <exterior>
                <paint_id>10002</paint_id>
                <insured>false</insured>
            </exterior>
        </exteriors>
    </car>

    <car>
        <color>blue</color>
        <top_speed>155</top_speed>
        <price>41000.00</price>

        <engines>
            <engine horsepower="300">
                <mpg>31</mpg>
                <engine_price>9000.00</engine_price>
            </engine>
        </engines>

        <interiors>
            <interior>
                <interior_color>gray</interior_color>
                <seat_material>cloth</seat_material>
                <heated_seats>true</heated_seats>
            </interior>
        </interiors>
    </car>
</cars>
)";

    try {
        XmlEngine engine;
    
        CarHandler handler{
            [](Car&& car) {
                print_car(car);
                std::cout << '\n';
            }
        };
    
        engine.parse_memory(xml, handler);
    
    } catch (const std::exception& e) {
        std::cerr << "Parse failed: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
}