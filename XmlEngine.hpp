// XmlEngine.hpp

#pragma once

#include <libxml/parser.h>
#include <libxml/xmlreader.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <limits>

class XmlEngine {
public:
    XmlEngine() {
        LibXmlGlobalState::init();
    }

    template <typename Handler>
    void parse_file(const char* filename, Handler& handler) const {
        ReaderPtr reader{
            xmlReaderForFile(filename, nullptr, parser_options())
        };

        if (!reader) {
            throw std::runtime_error("Failed to create xmlTextReader for file");
        }

        parse_reader(reader.get(), handler);
    }

    template <typename Handler>
    void parse_memory(std::string_view xml, Handler& handler) const {
        if (xml.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw std::runtime_error("XML memory buffer is too large for xmlReaderForMemory");
        }

        ReaderPtr reader{
            xmlReaderForMemory(
                xml.data(),
                static_cast<int>(xml.size()),
                nullptr,
                nullptr,
                parser_options()
            )
        };

        if (!reader) {
            throw std::runtime_error("Failed to create xmlTextReader for memory buffer");
        }

        parse_reader(reader.get(), handler);
    }

private:
    struct ReaderDeleter {
        void operator()(xmlTextReaderPtr reader) const noexcept {
            if (reader) {
                xmlFreeTextReader(reader);
            }
        }
    };

    using ReaderPtr = std::unique_ptr<xmlTextReader, ReaderDeleter>;

    struct LibXmlGlobalState {
        static void init() {
            static const bool initialized = [] {
                xmlInitParser();
                return true;
            }();

            (void)initialized;
        }
    };

    static int parser_options() {
        return XML_PARSE_NONET
             | XML_PARSE_NOERROR
             | XML_PARSE_NOWARNING
             | XML_PARSE_COMPACT;
    }

    template <typename Handler>
    void parse_reader(xmlTextReaderPtr reader, Handler& handler) const {
        std::vector<std::string> path;
        path.reserve(32);

        while (true) {
            const int rc = xmlTextReaderRead(reader);

            if (rc == 0) {
                break;
            }

            if (rc < 0) {
                throw std::runtime_error("XML parse error");
            }

            const int node_type = xmlTextReaderNodeType(reader);

            switch (node_type) {
                case XML_READER_TYPE_ELEMENT: {
                    const auto name = current_name(reader);
                    path.emplace_back(name);

                    handler.on_start_element(name, path);

                    if (xmlTextReaderIsEmptyElement(reader)) {
                        handler.on_end_element(name, path);
                        path.pop_back();
                    }

                    break;
                }

                case XML_READER_TYPE_TEXT:
                case XML_READER_TYPE_CDATA:
                case XML_READER_TYPE_SIGNIFICANT_WHITESPACE: {
                    const auto text = current_value(reader);
                    if (!text.empty()) {
                        handler.on_text(text, path);
                    }

                    break;
                }

                case XML_READER_TYPE_END_ELEMENT: {
                    const auto name = current_name(reader);

                    handler.on_end_element(name, path);

                    if (!path.empty()) {
                        path.pop_back();
                    }

                    break;
                }

                default:
                    break;
            }
        }
    }

    static std::string_view current_name(xmlTextReaderPtr reader) {
        const xmlChar* raw_name = xmlTextReaderConstLocalName(reader);

        if (!raw_name) {
            return {};
        }

        return {
            reinterpret_cast<const char*>(raw_name)
        };
    }

    static std::string_view current_value(xmlTextReaderPtr reader) {
        const xmlChar* raw_value = xmlTextReaderConstValue(reader);

        if (!raw_value) {
            return {};
        }

        return {
            reinterpret_cast<const char*>(raw_value)
        };
    }
};