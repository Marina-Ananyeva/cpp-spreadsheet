#pragma once

#include "common.h"
#include "formula.h"

#include <set>

class Sheet;

struct Cash {
    bool is_validate_ = false;
    CellInterface::Value value_;
    std::set<Position> cells_to_;
    std::set<Position> cells_from_;
};

class Impl {
public:
    virtual ~Impl() = default;
    virtual CellInterface::Value GetValue(const SheetInterface& sheet) const = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const = 0;
};

class EmptyImpl : public Impl {
public:
    EmptyImpl();
    CellInterface::Value GetValue(const SheetInterface& sheet) const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
private:
    double zero_val_;
};

class TextImpl : public Impl {
public:
    explicit TextImpl(std::string text);
    CellInterface::Value GetValue(const SheetInterface& sheet) const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

private:
    std::string text_;
};

class FormulaImpl : public Impl {
public:
    explicit FormulaImpl(std::string text);
    CellInterface::Value GetValue(const SheetInterface& sheet) const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

private:
    std::unique_ptr<FormulaInterface> formula_;
};

class Cell : public CellInterface {
public:
    struct CellPositionHasher {
        std::size_t operator()(Position pos) const {
            uint64_t hash = (size_t)(pos.row) * 37 + (size_t)(pos.col) * 37 * 37;
            return static_cast<size_t>(hash);
        }
    };

    Cell(Sheet& sheet);
    ~Cell() = default;

    bool IsEmptyCell() const;
    void Clear();

    void BuildGraph(std::unordered_map<Position, std::set<Position>, CellPositionHasher> &graph, std::set<Position> cells, Position pos, Position new_pos) const;
    void CheckCycle(Position pos, const std::vector<Position>& cells) const;

    void SetFormulaCell(Position pos, const std::string &text);
    void SetTextCell(const std::string &text);
    void Set(Position pos, std::string text);

    void SetCellTo(Position pos) const;
    void SetCellFrom(Position pos) const;
    void SetDependentCells(const std::set<Position>& cells) const;

    void SetValue(const Value& value) const;
    void SetValidateFlag(bool is_validate) const;

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    std::set<Position> GetDependentCells() const;
    const Cell* GetCell(Position pos) const;
    bool GetValidity() const;

private:
    std::unique_ptr<Impl> impl_;
    const Sheet& sheet_;
    mutable Cash cash_;
};

std::unique_ptr<Impl> ParseFormulaCell(std::string text);

//обход графа в глубину (проверяем на ацикличность)
void DFS(std::unordered_map<Position, std::set<Position>, Cell::CellPositionHasher>& graph, std::unordered_map<Position, int, Cell::CellPositionHasher>& visited, Position v, Position pos);

std::ostream &operator<<(std::ostream &output, const CellInterface::Value &value);
