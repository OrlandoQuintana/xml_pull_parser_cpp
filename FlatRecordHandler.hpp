#pragma once

#include <charconv>
#include <cctype>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

struct FlatRecord {
    std::string id;
    std::string name;

    std::optional<int> age;
    std::optional<int> count;
    std::optional<float> score;
    std::optional<float> temperature;
    std::optional<bool> active;
    std::optional<bool> valid;
    std::optional<std::string> category;
    std::optional<std::string> source;
    std::optional<std::string> description;
    std::optional<std::string> timestamp;
};

struct RequiredId {};
struct RequiredName {};
struct Age {};
struct Count {};
struct Score {};
struct Temperature {};
struct Active {};
struct Valid {};
struct Category {};
struct Source {};
struct Description {};
struct Timestamp {};
struct UnknownPath {};

using PathTag = std::variant<
    RequiredId,
    RequiredName,
    Age,
    Count,
    Score,
    Temperature,
    Active,
    Valid,
    Category,
    Source,
    Description,
    Timestamp,
    UnknownPath
>;

inline std::string_view trim_view(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }

    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }

    return s;
}

inline std::string trim_copy(std::string_view s) {
    const auto trimmed = trim_view(s);
    return std::string{trimmed};
}

inline bool iequals_ascii(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;

    for (std::size_t i = 0; i < a.size(); ++i) {
        const auto ca = static_cast<unsigned char>(a[i]);
        const auto cb = static_cast<unsigned char>(b[i]);

        if (std::tolower(ca) != std::tolower(cb)) {
            return false;
        }
    }

    return true;
}

inline std::optional<int> parse_int(std::string_view s) {
    s = trim_view(s);

    int value{};
    const char* begin = s.data();
    const char* end = s.data() + s.size();

    auto [ptr, ec] = std::from_chars(begin, end, value);

    if (ec == std::errc{} && ptr == end) {
        return value;
    }

    return std::nullopt;
}

inline std::optional<float> parse_float(std::string_view s) {
    s = trim_view(s);

#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
    float value{};
    const char* begin = s.data();
    const char* end = s.data() + s.size();

    auto [ptr, ec] = std::from_chars(begin, end, value);

    if (ec == std::errc{} && ptr == end) {
        return value;
    }

    return std::nullopt;
#else
    try {
        return std::stof(std::string{s});
    } catch (...) {
        return std::nullopt;
    }
#endif
}

inline std::optional<bool> parse_bool(std::string_view s) {
    s = trim_view(s);

    if (s == "1" || iequals_ascii(s, "true")) return true;
    if (s == "0" || iequals_ascii(s, "false")) return false;

    return std::nullopt;
}

inline PathTag classify_path(const std::vector<std::string>& path) {
    // Expected shape after root:
    // collection.record.field
    //
    // path[0] = collection
    // path[1] = record
    // path[2] = field

    if (path.size() != 3) {
        return UnknownPath{};
    }

    if (path[0] != "collection") {
        return UnknownPath{};
    }

    if (path[1] != "record") {
        return UnknownPath{};
    }

    const std::string_view field = path[2];

    if (field == "id") return RequiredId{};
    if (field == "name") return RequiredName{};
    if (field == "age") return Age{};
    if (field == "count") return Count{};
    if (field == "score") return Score{};
    if (field == "temperature") return Temperature{};
    if (field == "active") return Active{};
    if (field == "valid") return Valid{};
    if (field == "category") return Category{};
    if (field == "source") return Source{};
    if (field == "description") return Description{};
    if (field == "timestamp") return Timestamp{};

    return UnknownPath{};
}

class FlatRecordHandler {
public:
    void on_start_element(std::string_view name, const std::vector<std::string>& path) {
        if (path.size() == 2 && path[0] == "collection" && name == "record") {
            inside_record_ = true;
            current_ = FlatRecord{};
        }
    }

    void on_text(std::string_view text, const std::vector<std::string>& path) {
        if (!inside_record_) return;

        const auto trimmed = trim_view(text);
        if (trimmed.empty()) return;

        insert_text(current_, path, trimmed);
    }

    void on_end_element(std::string_view name, const std::vector<std::string>& path) {
        if (path.size() == 2 && path[0] == "collection" && name == "record") {
            if (is_valid(current_)) {
                records_.push_back(std::move(current_));
            } else {
                ++invalid_records_;
            }

            current_ = FlatRecord{};
            inside_record_ = false;
        }
    }

    const std::vector<FlatRecord>& records() const {
        return records_;
    }

    std::size_t invalid_records() const {
        return invalid_records_;
    }

private:
    static bool is_valid(const FlatRecord& record) {
        return !record.id.empty() && !record.name.empty();
    }

    static void insert_text(
        FlatRecord& record,
        const std::vector<std::string>& path,
        std::string_view text
    ) {
        PathTag tag = classify_path(path);

        std::visit(
            [&](auto concrete_tag) {
                insert_text(record, concrete_tag, text);
            },
            tag
        );
    }

    static void insert_text(FlatRecord& record, RequiredId, std::string_view text) {
        record.id = std::string{text};
    }

    static void insert_text(FlatRecord& record, RequiredName, std::string_view text) {
        record.name = std::string{text};
    }

    static void insert_text(FlatRecord& record, Age, std::string_view text) {
        record.age = parse_int(text);
    }

    static void insert_text(FlatRecord& record, Count, std::string_view text) {
        record.count = parse_int(text);
    }

    static void insert_text(FlatRecord& record, Score, std::string_view text) {
        record.score = parse_float(text);
    }

    static void insert_text(FlatRecord& record, Temperature, std::string_view text) {
        record.temperature = parse_float(text);
    }

    static void insert_text(FlatRecord& record, Active, std::string_view text) {
        record.active = parse_bool(text);
    }

    static void insert_text(FlatRecord& record, Valid, std::string_view text) {
        record.valid = parse_bool(text);
    }

    static void insert_text(FlatRecord& record, Category, std::string_view text) {
        record.category = std::string{text};
    }

    static void insert_text(FlatRecord& record, Source, std::string_view text) {
        record.source = std::string{text};
    }

    static void insert_text(FlatRecord& record, Description, std::string_view text) {
        record.description = std::string{text};
    }

    static void insert_text(FlatRecord& record, Timestamp, std::string_view text) {
        record.timestamp = std::string{text};
    }

    static void insert_text(FlatRecord&, UnknownPath, std::string_view) {
        // Unknown path. Ignore, count, or log depending on policy.
    }

    std::vector<FlatRecord> records_;
    FlatRecord current_;
    bool inside_record_ = false;
    std::size_t invalid_records_ = 0;
};

inline void print_record(const FlatRecord& record) {
    std::cout << "FlatRecord\n";
    std::cout << "  id: " << record.id << '\n';
    std::cout << "  name: " << record.name << '\n';

    if (record.age) std::cout << "  age: " << *record.age << '\n';
    if (record.count) std::cout << "  count: " << *record.count << '\n';
    if (record.score) std::cout << "  score: " << *record.score << '\n';
    if (record.temperature) std::cout << "  temperature: " << *record.temperature << '\n';
    if (record.active) std::cout << "  active: " << std::boolalpha << *record.active << '\n';
    if (record.valid) std::cout << "  valid: " << std::boolalpha << *record.valid << '\n';
    if (record.category) std::cout << "  category: " << *record.category << '\n';
    if (record.source) std::cout << "  source: " << *record.source << '\n';
    if (record.description) std::cout << "  description: " << *record.description << '\n';
    if (record.timestamp) std::cout << "  timestamp: " << *record.timestamp << '\n';
}