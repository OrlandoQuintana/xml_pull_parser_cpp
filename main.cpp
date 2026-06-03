#include "XmlEngine.hpp"
#include "FlatRecordHandler.hpp"

#include <iostream>
#include <string>

int main() {
    const std::string xml = R"(
<collection>
    <record>
        <id>A-1001</id>
        <name>Alpha</name>
        <age>27</age>
        <count>12</count>
        <score>98.5</score>
        <temperature>72.25</temperature>
        <active>true</active>
        <valid>1</valid>
        <category>engineering</category>
        <source>feed-a</source>
        <description>First flat demo record</description>
        <timestamp>2026-06-02T12:00:00Z</timestamp>
    </record>

    <record>
        <id>B-2002</id>
        <name>Beta</name>
        <score>88.75</score>
        <active>false</active>
        <category>science</category>
        <source>feed-b</source>
    </record>

    <record>
        <id>C-3003</id>
        <name>Gamma</name>
        <count>44</count>
        <temperature>66.8</temperature>
        <valid>true</valid>
        <description>Sparse optional fields</description>
    </record>

    <record>
        <id>D-4004</id>
        <score>12.5</score>
        <active>true</active>
    </record>
</collection>
)";

    try {
        XmlEngine engine;
        FlatRecordHandler handler;

        engine.parse_memory(xml, handler);

        std::cout << "Valid records: " << handler.records().size() << '\n';
        std::cout << "Invalid records: " << handler.invalid_records() << "\n\n";

        for (const auto& record : handler.records()) {
            print_record(record);
            std::cout << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << "Parse failed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}