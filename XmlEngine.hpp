#pragma once

#include <libxml/xmlreader.h>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

class XmlEngine {
public:
    template <typename Handler>
    void parse_file(const char* filename, Handler& handler) {
        xmlTextReaderPtr reader = xmlReaderForFile(filename, nullptr, XML_PARSE_NONET);
        if (!reader) {
            throw std::runtime_error("Failed to create xmlTextReader");
        }

        parse_reader(reader, handler);
        xmlFreeTextReader(reader);
    }

    template <typename Handler>
    void parse_memory(std::string_view xml, Handler& handler) {
        xmlTextReaderPtr reader = xmlReaderForMemory(
            xml.data(),
            static_cast<int>(xml.size()),
            nullptr,
            nullptr,
            XML_PARSE_NONET
        );

        if (!reader) {
            throw std::runtime_error("Failed to create xmlTextReader");
        }

        parse_reader(reader, handler);
        xmlFreeTextReader(reader);
    }

private:
    template <typename Handler>
    void parse_reader(xmlTextReaderPtr reader, Handler& handler) {
        std::vector<std::string> path;

        while (true) {
            const int rc = xmlTextReaderRead(reader);
            if (rc == 0) break;
            if (rc < 0) {
                throw std::runtime_error("XML parse error");
            }

            const int node_type = xmlTextReaderNodeType(reader);
            const xmlChar* raw_name = xmlTextReaderConstName(reader);

            if (!raw_name) continue;

            const std::string_view name{
                reinterpret_cast<const char*>(raw_name)
            };

            switch (node_type) {
                case XML_READER_TYPE_ELEMENT: {
                    path.emplace_back(name);
                    handler.on_start_element(name, path);

                    if (xmlTextReaderIsEmptyElement(reader)) {
                        handler.on_end_element(name, path);
                        path.pop_back();
                    }

                    break;
                }

                case XML_READER_TYPE_TEXT:
                case XML_READER_TYPE_CDATA: {
                    const xmlChar* raw_value = xmlTextReaderConstValue(reader);
                    if (!raw_value) break;

                    const std::string_view text{
                        reinterpret_cast<const char*>(raw_value)
                    };

                    handler.on_text(text, path);
                    break;
                }

                case XML_READER_TYPE_END_ELEMENT: {
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
};