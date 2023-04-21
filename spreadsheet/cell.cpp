#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

//---------EmptyImpl--------------------------------

EmptyImpl::EmptyImpl() : zero_val_(0.0) {
}

CellInterface::Value EmptyImpl::GetValue(const SheetInterface& sheet) const {
    return zero_val_;
}

std::string EmptyImpl::GetText() const {
    return {};
}

std::vector<Position> EmptyImpl::GetReferencedCells() const {
    return {};
}

//---------TextImpl---------------------------------
TextImpl::TextImpl(std::string text) : text_(std::move(text)) {
}

CellInterface::Value TextImpl::GetValue(const SheetInterface& sheet) const {
    if (text_[0] == ESCAPE_SIGN) {
        if (text_.length() > 1) {
            return text_.substr(1);
        } else {
            return std::string("");
        }
    }
    return text_;
}

std::string TextImpl::GetText() const {
    return text_;
}

std::vector<Position> TextImpl::GetReferencedCells() const {
    return {};
}

//---------FormulaImpl-------------------------------

FormulaImpl::FormulaImpl(std::string text) 
    : formula_(std::move(ParseFormula(std::move(text)))) {
}

CellInterface::Value FormulaImpl::GetValue(const SheetInterface& sheet) const {
    CellInterface::Value cell_value;
    FormulaInterface::Value formula_value = formula_->Evaluate(sheet);

    std::visit([&cell_value](const auto& x) { cell_value = x; }, formula_value);

    return cell_value;
}

std::string FormulaImpl::GetText() const {
    return '=' + formula_->GetExpression();
}

std::vector<Position> FormulaImpl::GetReferencedCells() const {
    return std::move(formula_->GetReferencedCells());
}

//-------------------Cell--------------------------------------

Cell::Cell(Sheet& sheet) 
    : impl_(std::make_unique<EmptyImpl>())
    , sheet_(sheet) {
}

bool Cell::IsEmptyCell() const {
    return impl_ == nullptr || impl_ == std::make_unique<EmptyImpl>();
}

void Cell::Clear() {
    //инвалидируем кэш (устанавливаем признак валидации false) в зависимых "сверху" ячейках
    for (Position cell_pos : cash_.cells_from_) {
        const Cell* cell_ptr = GetCell(cell_pos);
        if (cell_ptr) {
            cell_ptr->SetValidateFlag(false);
        }
    }

    std::unique_ptr<Impl> empty_ptr = nullptr;
    swap(impl_, empty_ptr);

    cash_.value_ = CellInterface::Value();
    cash_.is_validate_ = false;
    cash_.cells_to_.clear();
}

void Cell::BuildGraph(std::unordered_map<Position, std::set<Position>, CellPositionHasher>& graph, std::set<Position> cells, Position pos, Position new_pos) const {
    for (Position cell_pos : cells) {
        graph[pos].insert(cell_pos);
        const Cell *cell_ptr = GetCell(cell_pos);

        if (cell_ptr && !cell_ptr->cash_.cells_to_.empty() && !(cell_pos == new_pos)) {
            BuildGraph(graph, cell_ptr->cash_.cells_to_, cell_pos, new_pos);
        }
    }
}

void Cell::CheckCycle(Position pos, const std::vector<Position>& cells) const {
    std::unordered_map<Position, std::set<Position>, CellPositionHasher> graph;
    std::unordered_map<Position, int, CellPositionHasher> visited;
    
    if (!cells.empty()) {
        std::set<Position> cells_pos(cells.begin(), cells.end());
        BuildGraph(graph, cells_pos, pos, pos);
    }

    for (const auto& [cell_pos, cells] : graph) {
        if (visited[cell_pos] == 0) {
            try {
                DFS(graph, visited, cell_pos, pos);
            } catch (const std::exception& e) {
                throw CircularDependencyException(e.what());
            }
        }
    }
}

void Cell::SetFormulaCell(Position pos, const std::string& text) {
    std::unique_ptr<Impl> tmp_impl_ptr = nullptr;
    try {
        tmp_impl_ptr = ParseFormulaCell(text);
    } catch (const std::exception& e) {
        throw FormulaException(e.what());
    }

    std::vector<Position> cells(tmp_impl_ptr->GetReferencedCells());
    try {
        CheckCycle(pos, cells);
    } catch (const std::exception& e) {
        throw CircularDependencyException(e.what());
    }

    impl_ = std::move(tmp_impl_ptr);
}

void Cell::SetTextCell(const std::string& text) {
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    } else {
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }
}

void Cell::Set(Position pos, std::string text) {
    if (text.length() > 1 && text[0] == FORMULA_SIGN) {
        try {
            text.erase(0, 1);
            SetFormulaCell(pos, text);
        } catch (const FormulaException& e) {
            throw FormulaException(e.what());
        } catch (const CircularDependencyException& e) {
            throw CircularDependencyException(e.what());
        }
    
    } else {
        SetTextCell(text);
    }
}

void Cell::SetCellTo(Position pos) const {
    cash_.cells_to_.insert(pos);
}

void Cell::SetCellFrom(Position pos) const {
    cash_.cells_from_.insert(pos);
}

void Cell::SetDependentCells(const std::set<Position>& cells) const {
    cash_.cells_from_ = std::move(cells);
}

void Cell::SetValue(const CellInterface::Value& value) const {
    cash_.value_ = value;
}

void Cell::SetValidateFlag(bool is_validate) const {
    cash_.is_validate_ = is_validate;
}

CellInterface::Value Cell::GetValue() const {
    if (cash_.is_validate_) {
        return cash_.value_;
    }

    //инвалидируем кэш (устанавливаем признак валидации false) в зависимых "сверху" ячейках
    for (Position cell_pos : cash_.cells_from_) {
        const Cell* cell_ptr = GetCell(cell_pos);
        if (cell_ptr) {
            cell_ptr->SetValidateFlag(false);
        }
    }

    Value value = impl_->GetValue(sheet_);

    //валидируем кэш новым значением и устанавливаем признак валидации true
    SetValue(value);
    SetValidateFlag(true);

    return value;
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

std::set<Position> Cell::GetDependentCells() const {
    return cash_.cells_from_;
}

const Cell* Cell::GetCell(Position pos) const {
    return sheet_.GetCell(pos);
}

bool Cell::GetValidity() const {
    return cash_.is_validate_;
}

//--------------------------------------------------------

std::unique_ptr<Impl> ParseFormulaCell(std::string text) {
    try {
        return std::make_unique<FormulaImpl>(std::move(text));
    } catch (const std::exception& e) {
        std::throw_with_nested(FormulaException(e.what()));
    }
}

void DFS(std::unordered_map<Position, std::set<Position>, Cell::CellPositionHasher>& graph, std::unordered_map<Position, int, Cell::CellPositionHasher>& visited, Position v, Position pos) {
    visited[v] = 1;
    for (const Position u : graph[v]) {
        if (visited[u] == 0) {
            DFS(graph, visited, u, v);
        } else if (visited[u] == 1) {
            using namespace std::literals;
            throw CircularDependencyException("Formula has cycle"s);
        }
    }
    visited[v] = 2;
}

std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) {
    std::visit([&](const auto& x) { output << x; }, value);
    return output;
}