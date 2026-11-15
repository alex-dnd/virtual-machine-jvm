#include "egg/parser.h"
#include "reader.h"

namespace egg {

// Сохранить переданные токены и имя в tokens_ и source_name_ через перемещение.
// Только владение данными: не запускать разбор или проверку EOF.
// Добавить список инициализации и <utility> при реализации.
// Parser::Parser(std::vector<Token> tokens, std::string source_name)
// {
//     TODO: реализовать.
// }

// Создать новый detail::Reader(tokens_, source_name_) и вернуть run().
// Каждый вызов имеет независимое состояние; сам Parser не изменяется.
// ParseResult Parser::parse() const
// {
//     TODO: реализовать.
// }

} // namespace egg
