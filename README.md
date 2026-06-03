# xml_pull_parser_cpp

XML Namespace Handling Note

The current XML engine builds paths using only the local element name via xmlTextReaderConstLocalName. This means an element such as:

<car:engine xmlns:car="urn:cars">

is currently represented in the path as:

engine

The namespace prefix and namespace URI are not lost by libxml2, but the current engine is not capturing them.

This is acceptable for early prototyping if the candidate XML schemas do not contain name collisions or if namespace identity is not needed for field classification. However, for production XML sources that use namespaces, the engine should become namespace-aware.

The recommended refactor is to replace the path type:

std::vector<std::string> path;

with a namespace-aware path type:

struct XmlName {
    std::string local;   // e.g. "engine"
    std::string prefix;  // e.g. "car"
    std::string uri;     // e.g. "urn:cars"
};
using XmlPath = std::vector<XmlName>;

XmlEngine should populate each XmlName using libxml2 reader APIs:

xmlTextReaderConstLocalName(reader);
xmlTextReaderConstPrefix(reader);
xmlTextReaderConstNamespaceUri(reader);

Handlers can then choose their matching policy.

For schemas where namespaces do not matter:

path[2].local == "engine"

For schemas where namespaces are semantically important:

path[2].local == "engine" &&
path[2].uri == "urn:cars"

Path matching should generally prefer namespace URI over prefix. Prefixes are aliases and may change between XML documents:

<a:engine xmlns:a="urn:cars">
<b:engine xmlns:b="urn:cars">

Both elements belong to the same namespace because the URI is the same. The prefix is only a document-local shorthand.

This change does not alter the core architecture. The existing model still holds:

libxml2 event stream
-> path builder
-> schema-specific handler
-> path classification
-> variant visitation
-> typed struct mutation

The only change is that the path builder stores richer names. Existing handlers can initially continue matching only on local, while namespace-sensitive handlers can match on local + uri.

Recommended implementation path:

1. Introduce XmlName and XmlPath.
2. Update XmlEngine to push XmlName instead of std::string.
3. Provide small helper functions such as local(path, i) and uri(path, i) to keep handlers readable.
4. Update handlers one at a time.
5. Add tests showing that different prefixes with the same URI classify identically.
6. Add tests showing that the same local name in different namespaces can classify differently.

This keeps namespace support as an incremental refactor rather than a redesign.