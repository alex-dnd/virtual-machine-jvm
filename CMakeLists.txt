# ============================================================================
# egg_lexer — библиотека лексера и её тесты
# ============================================================================

cmake_minimum_required(VERSION 3.20)
project(egg_lexer LANGUAGES CXX)

# ----------------------------------------------------------------------------
# Базовые настройки языка
# ----------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ----------------------------------------------------------------------------
# Опции сборки
#
# EGG_WERROR — предупреждения превращаются в ошибки.
#   Нужно, чтобы CI не пропускал небрежный код.
#
# EGG_SANITIZE — включает AddressSanitizer и UndefinedBehaviorSanitizer.
#   Нужно для поиска выхода за границы массива и неопределённого поведения.
# ----------------------------------------------------------------------------

option(EGG_WERROR "Treat compiler warnings as errors" ON)
option(EGG_SANITIZE "Build with AddressSanitizer and UBSan" OFF)

# ----------------------------------------------------------------------------
# Библиотека egg_lexer
# ----------------------------------------------------------------------------

add_library(egg_lexer
  src/lexer/lexer.cpp
  src/lexer/scanner.cpp
  src/lexer/trivia.cpp
  src/lexer/tokens.cpp
  src/lexer/literals.cpp
)

# Заголовки из include/ доступны и самой библиотеке, и тем, кто её использует.
target_include_directories(egg_lexer PUBLIC include)

# ----------------------------------------------------------------------------
# Предупреждения компилятора
#
# Флаги разные для MSVC и GCC/Clang, поэтому они задаются отдельно.
# ----------------------------------------------------------------------------

if(MSVC)
  target_compile_options(egg_lexer PRIVATE
    /W4      # подробный уровень предупреждений
    /utf-8   # корректная работа с исходниками в UTF-8
  )

  if(EGG_WERROR)
    target_compile_options(egg_lexer PRIVATE /WX)
  endif()
else()
  target_compile_options(egg_lexer PRIVATE
    -Wall       # основные предупреждения
    -Wextra     # дополнительные предупреждения
    -Wpedantic  # предупреждения о нестандартных расширениях языка
  )

  if(EGG_WERROR)
    target_compile_options(egg_lexer PRIVATE -Werror)
  endif()
endif()

# ----------------------------------------------------------------------------
# Санитайзеры
#
# Включаются только по запросу: -DEGG_SANITIZE=ON.
# Флаги добавляются и при компиляции, и при линковке.
# ----------------------------------------------------------------------------

if(EGG_SANITIZE AND NOT MSVC)
  set(EGG_SANITIZER_FLAGS
    -fsanitize=address,undefined
    -fno-omit-frame-pointer
  )

  target_compile_options(egg_lexer PRIVATE ${EGG_SANITIZER_FLAGS})
  target_link_options(egg_lexer PUBLIC ${EGG_SANITIZER_FLAGS})
endif()

# ----------------------------------------------------------------------------
# Тесты. Тесты собираются только если включена поддержка CTest
# ----------------------------------------------------------------------------

include(CTest)

if(BUILD_TESTING)
  # Catch2 подключается автоматически через FetchContent.
  # GIT_SHALLOW уменьшает объём загрузки: клонируется только нужный коммит.
  include(FetchContent)

  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.16.1
    GIT_SHALLOW    TRUE
  )

  FetchContent_MakeAvailable(Catch2)

  # Исполняемый файл со всеми модульными тестами.
  add_executable(lexer_tests
    tests/test_trivia.cpp    # Нина
    tests/test_words.cpp     # Дима Петрик
    tests/test_literals.cpp  # Леша
    tests/test_spec.cpp      # Дима Цой
  )

  # egg_lexer — тестируемая библиотека.
  # Catch2::Catch2WithMain предоставляет main() для тестового исполняемого файла.
  target_link_libraries(lexer_tests PRIVATE
    egg_lexer
    Catch2::Catch2WithMain
  )

  if(MSVC)
    target_compile_options(lexer_tests PRIVATE /utf-8)
  endif()

  # Регистрирует каждый TEST_CASE из Catch2 как отдельный тест в CTest.
  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  include(Catch)
  catch_discover_tests(lexer_tests)
endif()
