# Spreadsheet (Сводная таблица)

Проект сводной таблицы (аналог Excel)

# Использование:

Проект позволяет создавать таблицу аналогично таблице Excel. В ячейки можно вводить информацию в виде текста, числа или формулы. В формулах можно задавать ссылки на другие ячейки. Расчет формул производится со любым унарным и бинарным оператором. 
Результаты можно вывести на консоль. Выводится как текстовое представление ячеек, так и числовое. 
При вводе значения в ячейке проверяется корректность ввода. В случае ошибки - пользователю выводится сообщение. 
При расчете значений ячейки также проверяется корректность данных в ячейках, в случае ошибки вычисления - пользователю выводится сообщение с кодом ошибки.

При расчете формулы используется дерево формулы из библиотеки ANTLR.

Запустить проект - добавить соответствующие команды в файл main.cpp

# Системные требования:
1. С++17
2. GCC(MinGW-w64) 11.2.0
3. Библиотека ANTLR4 https://www.antlr.org/
4. CMake version 3.8+ https://cmake.org

# Планы по доработке:
1. Добавить интерфейс для работы пользователя с таблицей из консоли.
