#include "formula.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

//--------------------------FormulaError-------------------------------

FormulaError::FormulaError(Category category) 
    : category_(category) {
}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return ToString() == rhs.ToString();
}

std::string FormulaError::ToString() const {
    switch (category_) {
        case Category::Ref:
            return std::string("#REF!"s);
        case Category::Value:
            return std::string("#VALUE!"s);
        case Category::Div0:
            return std::string("#DIV/0!"s);
        default:
            // have to do this because VC++ has a buggy warning
            assert(false);
            return std::string();
    }
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

//---------------Formula---------------------------------------------------

Formula::Formula(std::string expression) 
    : ast_(ParseFormulaAST(std::move(expression))) {
}

FormulaInterface::Value Formula::Evaluate(const SheetInterface& sheet) const {
    auto cell_lookup = [&sheet](Position pos) -> double {
        if (!pos.IsValid()) {
            throw FormulaError::Category::Ref;
        }
        double result = 0.0;
        CellInterface::Value value;
        const CellInterface* cell_ptr = sheet.GetCell(pos);
        if (cell_ptr) {
            value = cell_ptr->GetValue(); 
        } else {
            value = 0.0;
        }
        if (std::holds_alternative<double>(value)) {
            result = std::get<double>(value);
        } else if (std::holds_alternative<std::string>(value)) {
            std::optional<double> numeric = IsStringDoubleNumeric(std::get<std::string>(value));
            if (numeric.has_value()) {
                result = numeric.value();
            } else {
                throw FormulaError(FormulaError::Category::Value);
            }
        } else {
            throw FormulaError(std::get<FormulaError>(value).GetCategory());
        }
        return result;
    };

    try {
        return ast_.Execute(cell_lookup);
    } catch (const FormulaError& e) {
        return FormulaError(e.GetCategory());
    } 
}

std::string Formula::GetExpression() const {
    std::ostringstream out;
    ast_.PrintFormula(out);
    return out.str();
}

std::vector<Position> Formula::GetReferencedCells() const {
    std::forward_list<Position> cells(ast_.GetReferencedCells());
    std::vector<Position> result(cells.begin(), cells.end());
    auto it = std::unique(result.begin(), result.end());
    result.erase(it, result.end());
    return result;
}

//-----------------------------------------------------------------

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    } catch (const std::exception& e) {
        throw FormulaException(e.what());
    }
}

std::ostream& operator<<(std::ostream& output, const FormulaInterface::Value& value) {
    std::visit([&](const auto& x) { output << x; }, value);
    return output;
}

std::optional<double> IsStringDoubleNumeric(const std::string &str) {
    char *p;
    std::optional<double> result = strtod(str.c_str(), &p);
    if (*p == 0) { //если указатель указывает на конец строки
        return result;
    }
    return std::optional<double>{};
}