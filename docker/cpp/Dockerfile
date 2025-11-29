# Partimos de la imagen oficial con g++
FROM gcc:13

# Instalamos 'time' para poder medir memoria con /usr/bin/time -v
RUN apt-get update \
    && apt-get install -y time \
    && rm -rf /var/lib/apt/lists/*

# Crear un usuario sin privilegios para ejecutar los programas del estudiante
RUN useradd -m runner

# Cambiar al usuario no-root
USER runner

# Directorio de trabajo por defecto dentro del contenedor
WORKDIR /workspace

# No definimos ENTRYPOINT ni CMD fija para tener flexibilidad
# Pasaremos el comando a ejecutar desde "docker run ..."
