# variant

Интерфейс и все свойства и гарантии соответствуют [std::variant](https://en.cppreference.com/w/cpp/utility/variant) (но не реализована специализация для `std::hash`).
По аналогии с optional, variant сохраняет тривиальность для special members (деструктора, конструкторов и операторов присваивания).
`std::visit` реализован через таблицу виртуальных функций. 

## Conversion
Такой код должен работает ожидаемым образом:
```
variant<string, bool> x = "abc";             // holds string
```
Но проблема в том, что указатель `char const*` приводится как к `bool`, так и к `string `. Для решения этой проблемы было выбрано решение из [P0608R3](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0608r3.html), Это иногда ведёт себя [неожиданным образом](https://cplusplus.github.io/LWG/issue3228), но пример выше благодаря такому работает.
