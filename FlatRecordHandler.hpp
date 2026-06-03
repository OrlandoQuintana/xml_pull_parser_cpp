#pragma once

#include <charconv>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

struct FlatRecord {
    std::string id;    // required
    std::string name;  // required

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

inline std::string trim_copy(std::string_view s) {
    const auto begin = s.find_first_not_of(" \n\r\t");
    if (begin == std::string_view::npos) return {};

    const auto end = s.find_last_not_of(" \n\r\t");
    return std::string{s.substr(begin, end - begin + 1)};
}

inline std::optional<int> parse_int(std::string_view s) {
    const auto trimmed = trim_copy(s);

    int value{};
    const char* begin = trimmed.data();
    const char* end = trimmed.data() + trimmed.size();

    auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec == std::errc{} && ptr == end) return value;

    return std::nullopt;
}

inline std::optional<float> parse_float(std::string_view s) {
    try {
        return std::stof(trim_copy(s));
    } catch (...) {
        return std::nullopt;
    }
}

inline std::optional<bool> parse_bool(std::string_view s) {
    const auto trimmed = trim_copy(s);

    if (trimmed == "true" || trimmed == "TRUE" || trimmed == "1") return true;
    if (trimmed == "false" || trimmed == "FALSE" || trimmed == "0") return false;

    return std::nullopt;
}

inline std::string join_path_without_root(
    const std::vector<std::string>& path,
    std::string_view root
) {
    std::string joined;

    std::size_t start = 0;
    if (!path.empty() && path.front() == root) {
        start = 1;
    }

    for (std::size_t i = start; i < path.size(); ++i) {
        if (!joined.empty()) joined += ".";
        joined += path[i];
    }

    return joined;
}

inline PathTag classify_path(std::string_view path) {
    if (path == "record.id") return RequiredId{};
    if (path == "record.name") return RequiredName{};
    if (path == "record.age") return Age{};
    if (path == "record.count") return Count{};
    if (path == "record.score") return Score{};
    if (path == "record.temperature") return Temperature{};
    if (path == "record.active") return Active{};
    if (path == "record.valid") return Valid{};
    if (path == "record.category") return Category{};
    if (path == "record.source") return Source{};
    if (path == "record.description") return Description{};
    if (path == "record.timestamp") return Timestamp{};

    return UnknownPath{};
}

class FlatRecordHandler {
public:
    void on_start_element(std::string_view name, const std::vector<std::string>&) {
        if (name == "record") {
            inside_record_ = true;
            current_ = FlatRecord{};
        }
    }

    void on_text(std::string_view text, const std::vector<std::string>& path) {
        if (!inside_record_) return;

        const auto joined = join_path_without_root(path, "collection");
        insert_text(current_, joined, text);
    }

    void on_end_element(std::string_view name, const std::vector<std::string>&) {
        if (name == "record") {
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

    static void insert_text(FlatRecord& record, std::string_view path, std::string_view text) {
        PathTag tag = classify_path(path);

        std::visit(
            [&](auto concrete_tag) {
                insert_text(record, concrete_tag, text);
            },
            tag
        );
    }

    static void insert_text(FlatRecord& record, RequiredId, std::string_view text) {
        record.id = trim_copy(text);
    }

    static void insert_text(FlatRecord& record, RequiredName, std::string_view text) {
        record.name = trim_copy(text);
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
        record.category = trim_copy(text);
    }

    static void insert_text(FlatRecord& record, Source, std::string_view text) {
        record.source = trim_copy(text);
    }

    static void insert_text(FlatRecord& record, Description, std::string_view text) {
        record.description = trim_copy(text);
    }

    static void insert_text(FlatRecord& record, Timestamp, std::string_view text) {
        record.timestamp = trim_copy(text);
    }

    static void insert_text(FlatRecord&, UnknownPath, std::string_view) {
        // Ignore unknown path.
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
    if (record.active) std::cout << "  active: " << *record.active << '\n';
    if (record.valid) std::cout << "  valid: " << *record.valid << '\n';
    if (record.category) std::cout << "  category: " << *record.category << '\n';
    if (record.source) std::cout << "  source: " << *record.source << '\n';
    if (record.description) std::cout << "  description: " << *record.description << '\n';
    if (record.timestamp) std::cout << "  timestamp: " << *record.timestamp << '\n';
}