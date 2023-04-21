#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <memory>
#include <unordered_map>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void CheckSheetSize(Position pos);
    void InvalidateDependentCells(Position pos);
    void SetReferencedAndDependentCells(Position pos);
    void SetCell(Position pos, std::string text) override;

    const Cell* GetCell(Position pos) const override;
    Cell* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    bool IsSheetIncludesPos(Position pos) const;

    struct CellPositionHasher {
        std::size_t operator()(Position pos) const {
            uint64_t hash = (size_t)(pos.row) * 37 + (size_t)(pos.col) * 37 * 37;
            return static_cast<size_t>(hash);
        }
    };
private:
    Size sheet_size_;
    std::unordered_map<Position, std::unique_ptr<Cell>, CellPositionHasher> sheet_;
};

std::unique_ptr<SheetInterface> CreateSheet();