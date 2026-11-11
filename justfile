# Создание build-каталога
configure:
    cmake -S . -B build

# Создание build с включёнными санитайзерами
configure-with-sanitizers:
    cmake -S . -B build -DEGG_SANITIZE=ON

# Создание build с включёнными тестами
configure-with-tests:
    cmake -S . -B build -DBUILD_TESTING=ON

# Сборка проекта
build: configure
    cmake --build build --parallel

# Сборка проекта с санитайзерами
build-sanitizers: configure-with-sanitizers
    cmake --build build --parallel

# Сборка проекта c тестами
build-tests: configure-with-tests
    cmake --build build --parallel

# Запуск тестов
test: build
    ctest --test-dir build --output-on-failure

# Удаление build-каталога
clean:
    rm -rf build
