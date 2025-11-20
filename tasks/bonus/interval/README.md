# Interval

Реализуйте структуру [`Interval`](./interval.hpp), позволяющую производить
операции с плавающей точкой и оценивать погрешность в вычислениях.

`Interval` представляет результат вычислений в виде двух чисел – `lower` и `upper`.
`lower` – нижняя оценка на результат вычислений, `upper` – верхняя.
Нижняя оценка получается в результате вычислений с округлением результата вниз,
верхняя оценка получается округлением результатов вычислений вверх.

Для контроля за округлением результатов вычислений используйте библиотечные
функции [`std::fegetround` и `std::fesetround`](https://en.cppreference.com/w/cpp/numeric/fenv/feround.html).

После завершения вычислений `Interval` должен вернуть режим округления нецелочисленных
операций в исходное состояние.

Корректной обработки `NaN` не требуется, также гарантируется, что при делении делитель
отличен от 0.

## Проблемы с компилятором

В некоторых местах вашей реализации вы скорее всего будете вычислять значение одних
и тех же выражений, но с разным режимом округления. Эти вычисления могут дедуплицироваться
из-за [common subexpression elimination](https://en.wikipedia.org/wiki/Common_subexpression_elimination),
для того, чтобы это избежать, ваш код будет собираться с флагом [`-frounding-math`](https://clang.llvm.org/docs/UsersManual.html#cmdoption-f-no-rounding-math).

Но для gcc этого будет [недостаточно](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=34678) и
в сборке с оптимизациями арифметические выражения могут переиспользоваться.
Используйте `volatile`/inline assembly для того, чтобы это избежать.

## Полезные ссылки

- [Boost Interval](https://www.boost.org/doc/libs/latest/libs/numeric/interval/doc/interval.htm)
- [Изменение режима округления в boost interval](https://github.com/boostorg/interval/blob/develop/include/boost/numeric/interval/detail/x86gcc_rounding_control.hpp#L27)
- [Реализации функций fegetround и fesetround в musl](https://elixir.bootlin.com/musl/v1.2.5/source/src/fenv/x86_64/fenv.s#L35)
