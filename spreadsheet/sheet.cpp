#include "sheet.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

Sheet::~Sheet() = default;

void Sheet::CheckSheetSize(Position pos) {
    if (pos.row + 1 > sheet_size_.rows) {
        sheet_size_.rows = pos.row + 1;
    }
    if (pos.col + 1 > sheet_size_.cols) {
        sheet_size_.cols = pos.col + 1;
    }
}

void Sheet::InvalidateDependentCells(Position pos) {
    const Cell *cell_ptr = GetCell(pos);
    if (cell_ptr) {
        for (Position cell_pos : cell_ptr->GetDependentCells()) {
            const Cell *ptr_cell = GetCell(cell_pos);
            if (ptr_cell == nullptr) {
                SetCell(cell_pos, "");
                ptr_cell = GetCell(cell_pos);
            }
            ptr_cell->SetValidateFlag(false);
        }
    }
}

void Sheet::SetReferencedAndDependentCells(Position pos) {
    const Cell *cell_ptr = GetCell(pos);
    if (cell_ptr) {
        for (Position cell_pos : cell_ptr->GetReferencedCells()) {
            cell_ptr->SetCellTo(cell_pos);
            const Cell *ptr_cell = GetCell(cell_pos);
            if (ptr_cell == nullptr) {
                SetCell(cell_pos, "");
                ptr_cell = GetCell(cell_pos);
            }
            ptr_cell->SetCellFrom(pos);
        }
    }
}

void Sheet::SetCell(Position pos, std::string text) {
    //проверяем, что позиция ячейки валидна
    if (!pos.IsValid()) {
        using namespace std::literals;
        throw InvalidPositionException("Position is not valid"s);
    }

    auto cell_ptr = GetCell(pos);
    //проверяем, что ячейка не содержит тот же текст
    if (cell_ptr) {
        if (cell_ptr->GetText() == text) {
            return void();
        }
    }

    //создаем временную ячейку (в процессе проверяем корректность формулы и отсутствие цикличных ссылок)
    std::unique_ptr<Cell> tmp_cell_ptr = nullptr;
    try {
        tmp_cell_ptr = std::make_unique<Cell>(*this);
        tmp_cell_ptr->Set(pos, std::move(text));

    } catch (const FormulaException& e) {
        throw FormulaException(e.what());
    } catch (const CircularDependencyException& e) {
        throw CircularDependencyException(e.what());
    }

    //проверяем, что размера таблицы достаточно (при необходимости добавляем строки / столбцы)
    CheckSheetSize(pos);

    if (IsSheetIncludesPos(pos)) {
        //проверяем, инициализирована ли ячейка, если да - переносим в новую ячейку список ячеек "сверху"
        std::set<Position> dependent_cells = sheet_[pos]->GetDependentCells();
        tmp_cell_ptr->SetDependentCells(std::move(dependent_cells));
        //вставляем временную ячейку в таблицу
        std::swap(sheet_[pos], tmp_cell_ptr);
    } else {
        //вставляем временную ячейку в таблицу
        sheet_[pos] = std::move(tmp_cell_ptr);
    }

    //добавляем информацию о связанных ячейках
    SetReferencedAndDependentCells(pos);

    //инвалидируем зависимые ячейки "сверху"
    InvalidateDependentCells(pos);
}

const Cell* Sheet::GetCell(Position pos) const {
    if (pos.IsValid()) {
        if (IsSheetIncludesPos(pos)) {
            if (sheet_.at(pos)->IsEmptyCell()) {
                return nullptr;
            }
            return sheet_.at(pos).get();
        } else {
            return nullptr;
        }
    } else {
        using namespace std::literals;
        throw InvalidPositionException("Position is not valid"s);
    }
}

Cell* Sheet::GetCell(Position pos) {
    if (pos.IsValid()) {
        if (IsSheetIncludesPos(pos)) {
            if (sheet_[pos]->IsEmptyCell()) {
                return nullptr;
            }
            return sheet_[pos].get();
        } else {
            return nullptr;
        }
    } else {
        using namespace std::literals;
        throw InvalidPositionException("Position is not valid"s);
    }
}

void Sheet::ClearCell(Position pos) {
    if (pos.IsValid()) {
        if (IsSheetIncludesPos(pos)) {
            sheet_.at(pos)->Clear();
        }

        int row_max = -1;
        int col_max = -1;
        if (pos.row + 1 == sheet_size_.rows || pos.col + 1 == sheet_size_.cols) {
            for (const auto& [position, cell] : sheet_) {
                if (!cell->IsEmptyCell()) {
                    row_max = std::max(row_max, position.row);
                    col_max = std::max(col_max, position.col);
                }
            }
            sheet_size_.rows = row_max + 1;
            sheet_size_.cols = col_max + 1;
        }
    } else {
        using namespace std::literals;
        throw InvalidPositionException("Position is not valid"s);        
    }
}

Size Sheet::GetPrintableSize() const {
    return sheet_size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size print_size = GetPrintableSize();
    for (int i = 0; i < print_size.rows; ++i) {
        for (int j = 0; j < print_size.cols; ++j) {
            Position pos{i, j};
            if (IsSheetIncludesPos(pos) && !sheet_.at(pos)->IsEmptyCell()) {
                output << sheet_.at(pos)->GetValue();
            }
            if (j + 1 != print_size.cols) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size print_size = GetPrintableSize();
    for (int i = 0; i < print_size.rows; ++i) {
        for (int j = 0; j < print_size.cols; ++j) {
            Position pos{i, j};
            if (IsSheetIncludesPos(pos) && !sheet_.at(pos)->IsEmptyCell()) {
                output << sheet_.at(pos)->GetText();
            }
            if (j + 1 != print_size.cols) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

bool Sheet::IsSheetIncludesPos(Position pos) const {
    auto it = sheet_.find(pos);
    return it != sheet_.end();
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}