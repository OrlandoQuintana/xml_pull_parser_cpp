# XML Struct Parser

A reusable XML to struct parsing framework built on top of libxml2's streaming `xmlTextReader` API.

The goal of the project is to separate XML tokenization from schema-specific parsing logic. The core engine is responsible for reading XML, maintaining parser state, and building element paths. Individual handlers are responsible for interpreting those paths and populating strongly typed C++ structs.

The framework is designed to support both simple flat XML schemas and deeply nested heterogeneous schemas without modifying the core parsing engine.

## Architecture

The project is composed of three primary layers:

### XmlEngine

`XmlEngine` is the reusable streaming parser.

Responsibilities:

- Initialize and manage libxml2 resources
- Read XML from memory or files
- Process XML events
- Maintain the current XML path
- Dispatch parser events to handlers

The engine has no knowledge of the target schema or output structures.

Example event flow:

```text
XML
-> Start Element
-> Text
-> End Element
```

For each event, the engine invokes handler callbacks:

```cpp
handler.on_start_element(...)
handler.on_text(...)
handler.on_end_element(...)
```

### Parse

`Parse.hpp` contains reusable parsing utilities for converting XML text into native C++ types.

Supported conversions include:

```cpp
xmlparse::parse_string(...)
xmlparse::parse_int(...)
xmlparse::parse_u64(...)
xmlparse::parse_float(...)
xmlparse::parse_double(...)
xmlparse::parse_bool(...)
```

These functions return `std::optional<T>` where appropriate and centralize all native type conversion logic.

### Handlers

Handlers contain all schema-specific behavior.

Responsibilities:

- Define output structs
- Define path classifications
- Manage parser state
- Create nested child objects
- Populate fields
- Validate records

The engine remains unchanged regardless of the XML schema being processed.

## Parsing Model

The parser follows a path-based state machine model.

As XML elements are entered and exited, the engine maintains the current path:

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

```cpp
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

## Flat Schema Example

A flat XML document may contain records such as:

```xml
<collection>
    <record>
        <id>A-1001</id>
        <name>Alpha</name>
        <score>98.5</score>
    </record>
</collection>
```

The corresponding handler:

- Creates a new record on `<record>`
- Populates fields as text nodes are encountered
- Finalizes the record on `</record>`

## Nested Schema Example

A nested XML document may contain structures such as:

```xml
<cars>
    <car>
        <engines>
            <engine>
                <horsepower>450</horsepower>
            </engine>
        </engines>
    </car>
</cars>
```

The handler:

- Creates a `Car` object when entering `<car>`
- Creates an `Engine` object when entering `<engine>`
- Populates the current engine while inside that context
- Finalizes the car when leaving `</car>`

The core engine does not change between flat and nested schemas.

## Design Goals

- Streaming XML parsing
- Low memory usage
- Strongly typed output structures
- Separation of parser and schema logic
- Reusable parsing engine
- Support for deeply nested XML documents
- Support for heterogeneous XML schemas
- Easy creation of new schema handlers

## Current Limitations

The framework currently requires a handler implementation for each schema.

Path classification is implemented manually through schema-specific code.

Validation rules are implemented within individual handlers.

XML attributes are not currently mapped into structs.

Namespace-aware path matching has not yet been implemented.

## Future Work: XML Namespaces

The current implementation builds paths using:

```cpp
xmlTextReaderConstLocalName()
```

For an element such as:

```xml
<car:engine xmlns:car="urn:cars">
```

the path currently contains:

```text
engine
```

The namespace prefix and namespace URI are not currently stored in the path representation.

If namespace-aware matching becomes necessary, the path representation can be updated from:

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

Handlers can then classify paths using both the local name and namespace URI when required.

Example:

```cpp
path[2].local == "engine" &&
path[2].uri == "urn:cars"
```

This change only affects the path representation and path classification logic. The overall architecture remains unchanged.