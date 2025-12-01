#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <iostream>

int main() {
    // -------------------------------------------------------------------------
    // Inicialización obligatoria del driver de MongoDB.
    // Esta instancia debe mantenerse viva durante toda la ejecución del programa
    // porque administra estados internos del driver (conexiones, pools, etc.).
    // -------------------------------------------------------------------------
    mongocxx::instance inst{};

    // -------------------------------------------------------------------------
    // Conexión al servidor MongoDB usando URI estándar.
    // En este caso se utiliza localhost:27017, la configuración típica en local.
    // El objeto 'client' permite acceder a bases de datos y colecciones.
    // -------------------------------------------------------------------------
    mongocxx::client client{mongocxx::uri{"mongodb://localhost:27017"}};

    // Selecciona la base de datos "codecoach" y la colección "problems".
    auto db = client["codecoach"];
    auto collection = db["problems"];

    // Alias para simplificar escritura de documentos BSON.
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;
    using bsoncxx::builder::basic::make_array;

    // -------------------------------------------------------------------------
    // Construcción de un documento BSON para insertar un problema de ejemplo.
    // Este documento incluye:
    //   - title        → título del problema
    //   - difficulty   → nivel de dificultad
    //   - tags         → arreglo de etiquetas
    //   - test_cases   → arreglo de casos de prueba, cada uno con input/output.
    //
    // Nota: este archivo sirve típicamente como "script de prueba" para
    // verificar que MongoDB y el driver de C++ están funcionando correctamente.
    // -------------------------------------------------------------------------
    auto doc = make_document(
        kvp("title", "Suma de tres   números"),
        kvp("difficulty", "Fácil"),
        kvp("tags", make_array("math", "intro")),
        kvp("test_cases", make_array(
            make_document(
                kvp("input", "2 3"),
                kvp("output", "5")
            )
        ))
    );

    // -------------------------------------------------------------------------
    // Inserción del documento en la colección.
    // insert_one() devuelve un optional con metadata del resultado.
    // Aquí se imprime el ObjectId asignado automáticamente por MongoDB.
    // -------------------------------------------------------------------------
    auto result = collection.insert_one(doc.view());
    std::cout << "Documento insertado con ID: "
              << result->inserted_id().get_oid().value.to_string() << std::endl;

    // -------------------------------------------------------------------------
    // Consulta: imprimir todos los documentos actuales en la colección.
    // collection.find({}) devuelve un cursor que puede recorrer todos los items.
    // bsoncxx::to_json() convierte documentos BSON a JSON legible.
    // -------------------------------------------------------------------------
    auto cursor = collection.find({});
    std::cout << "\n📦 Documentos en 'problems':\n";
    for (auto&& item : cursor) {
        std::cout << bsoncxx::to_json(item) << std::endl;
    }

    return 0;
}
