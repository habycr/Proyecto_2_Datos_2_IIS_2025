#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <iostream>

int main() {
    mongocxx::instance inst{};
    mongocxx::client client{mongocxx::uri{"mongodb://localhost:27017"}};

    auto db = client["codecoach"];
    auto collection = db["problems"];

    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;
    using bsoncxx::builder::basic::make_array;

    auto doc = make_document(
        kvp("title", "Suma de dos números"),
        kvp("difficulty", "Fácil"),
        kvp("tags", make_array("math", "intro")),
        kvp("test_cases", make_array(
            make_document(
                kvp("input", "2 3"),
                kvp("output", "5")
            )
        ))
    );

    auto result = collection.insert_one(doc.view());
    std::cout << "Documento insertado con ID: "
              << result->inserted_id().get_oid().value.to_string() << std::endl;

    auto cursor = collection.find({});
    std::cout << "\n📦 Documentos en 'problems':\n";
    for (auto&& item : cursor) {
        std::cout << bsoncxx::to_json(item) << std::endl;
    }

    return 0;
}
