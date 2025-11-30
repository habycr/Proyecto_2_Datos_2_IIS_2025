#pragma once
#include <string>
#include <map>

using namespace std;

// Clase sencilla para cargar variables desde un archivo .env estilo KEY=VALUE.
class EnvLoader {
private:
    map<string, string> envVars;  // Mapa en memoria de variable -> valor

public:
    // Constructor: intenta cargar el archivo .env (o el que se especifique).
    EnvLoader(const string& filename = ".env");

    // Indica si se cargó al menos una variable de entorno.
    bool load();

    // Obtiene el valor de una variable. Si no existe, devuelve el valor por defecto.
    string get(const string& key, const string& defaultValue = "");
};
