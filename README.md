# XML Struct Parser

A reusable XML to struct parsing framework built on top of libxml2's streaming `xmlTextReader` API.

The goal of the project is to separate XML tokenization from schema-specific parsing logic. The core engine is responsible for reading XML, maintaining parser state, building element paths, exposing attributes, and dispatching XML events. Individual handlers are responsible for interpreting those events and populating strongly typed C++ structs.

The framework is designed to support both simple flat XML schemas and deeply nested heterogeneous schemas without modifying the core parsing engine.

## Architecture

The project is composed of three primary layers.

### XmlEngine

`XmlEngine` is the reusable streaming parser.

Responsibilities:

- Initialize and manage libxml2 resources
- Read XML from memory or files
- Process XML events
- Maintain the current XML path
- Expose XML attributes
- Dispatch parser events to handlers

The engine has no knowledge of the target schema or output structures.

Example event flow:

```text
XML
-> Start Element
-> Attribute
-> Text
-> End Element
```

For each event, the engine invokes handler callbacks:

```cpp
handler.on_start_element(...)
handler.on_attribute(...)
handler.on_text(...)
handler.on_end_element(...)
```

### Parse

`Parse.hpp` contains reusable parsing utilities for converting XML text into native C++ types.

Supported conversions include:

```cpp
xmlparse::parse_string(...)
xmlparse::parse_bool(...)
xmlparse::parse_int(...)
xmlparse::parse_i64(...)
xmlparse::parse_u64(...)
xmlparse::parse_float(...)
xmlparse::parse_double(...)
```

These functions return `std::optional<T>` where appropriate and centralize all native type conversion logic.

### Handlers

Handlers contain all schema-specific behavior.

Responsibilities:

- Define output structs
- Define path classifications
- Define attribute classifications
- Manage parser state
- Create nested child objects
- Populate fields
- Validate records

The engine remains unchanged regardless of the XML schema being processed.

## Parsing Model

The parser follows a path-based state machine model.

As XML elements are entered and exited, the engine maintains the current path.

```xml
<cars>
    <car>
        <engine>
            <horsepower>450</horsepower>
        </engine>
    </car>
</cars>
```

Produces:

```text
cars
cars.car
cars.car.engine
cars.car.engine.horsepower
```

Handlers classify paths and route values to the appropriate destination fields.

Example:

```text
cars.car.engine.horsepower
```

becomes:

```cpp
EngineHorsepower{}
```

which is dispatched through variant visitation:

```cpp
std::visit(...)
```

and ultimately populates:

```cpp
current_.engines.back().horsepower
```

## Attribute Support

Attributes are exposed separately from element text.

Example:

```xml
<engine horsepower="450">
```

Produces:

```text
Path:
cars.car.engines.engine

Attribute:
horsepower = 450
```

The engine invokes:

```cpp
handler.on_attribute(
    "horsepower",
    "450",
    path
);
```

Handlers may classify and populate attributes using the same approach used for element text.

## Flat Schema Example

A flat XML document may contain records such as:

```xml
<collection>
    <record source="feed-a">
        <id>A-1001</id>
        <name>Alpha</name>
        <score>98.5</score>
    </record>
</collection>
```

The corresponding handler:

- Creates a new record on `<record>`
- Processes attributes
- Populates fields as text nodes are encountered
- Finalizes the record on `</record>`

## Nested Schema Example

A nested XML document may contain structures such as:

```xml
<cars>
    <car>
        <engines>
            <engine horsepower="450">
                <mpg>22</mpg>
            </engine>
        </engines>
    </car>
</cars>
```

The handler:

- Creates a `Car` object when entering `<car>`
- Creates an `Engine` object when entering `<engine>`
- Processes engine attributes
- Populates the current engine while inside that context
- Finalizes the car when leaving `</car>`

The core engine does not change between flat and nested schemas.

## Design Goals

- Streaming XML parsing
- Low memory usage
- Strongly typed output structures
- Separation of parser and schema logic
- Reusable parsing engine
- Support for XML attributes
- Support for deeply nested XML documents
- Support for heterogeneous XML schemas
- Easy creation of new schema handlers

## Current Limitations

The framework currently requires a handler implementation for each schema.

Path classification is implemented manually through schema-specific code.

Validation rules are implemented within individual handlers.

Namespace-aware path matching has not yet been implemented.

## Future Work: XML Namespaces

The current implementation uses:

```cpp
xmlTextReaderConstLocalName()
```

For XML such as:

```xml
<car:engine xmlns:car="urn:cars">
```

the parser currently sees:

```text
engine
```

This means the parser will often work correctly with namespaced XML as long as element names remain unique. However, namespace information is not currently preserved, so elements with the same local name but different namespaces cannot be distinguished.

To support namespaces, the path representation can be changed from:

```cpp
std::vector<std::string>
```

to:

```cpp
struct XmlName {
    std::string local;
    std::string prefix;
    std::string uri;
};

using XmlPath = std::vector<XmlName>;
```

The engine can populate these fields using:

```cpp
xmlTextReaderConstLocalName(reader);
xmlTextReaderConstPrefix(reader);
xmlTextReaderConstNamespaceUri(reader);
```

Handlers can then classify paths using both local names and namespace URIs when required.

Example:

```cpp
path[3].local == "engine" &&
path[3].uri == "urn:cars"
```

The same approach should be applied to attributes so that namespaced attributes can also be distinguished correctly.

This change only affects path and attribute classification. The overall architecture, event model, parser state machine, and handler design remain unchanged.