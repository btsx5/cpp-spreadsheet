#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <variant>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#DIV/0!";
}

namespace {
class Formula : public FormulaInterface {
private:
    FormulaAST ast_;
public:
    explicit Formula(std::string expression) try
            : ast_(ParseFormulaAST(expression)) {
    } catch (...) {
        throw FormulaException("Invalid Formula Syntax");
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        Value result;
        CellCalculation function_execute = [&sheet](Position pos) {
            const CellInterface* cell = sheet.GetCell(pos);

            if (cell == nullptr) {
                return 0.0;
            }

            const CellInterface::Value value = cell->GetValue();

            if (std::holds_alternative<std::string>(value)) {
                std::string string_value = cell->GetText();

                if (std::get<std::string>(value).empty()) {
                    return 0.0;
                }

                try {
                    size_t stod_end;
                    double double_value = stod(string_value, &stod_end);

                    if (stod_end != string_value.size()) {
                        throw FormulaError(FormulaError::Category::Value);
                    }

                    return double_value;
                } catch (...) {
                    throw FormulaError(FormulaError::Category::Value);
                }
            } else if (std::holds_alternative<double>(value)) {
                return std::get<double>(value);
            } else {
                throw std::get<FormulaError>(value);
            }
        };

        try {
            result = ast_.Execute(function_execute);
        } catch (FormulaError& fe) {
            return fe;
        }

        return result;
    }

    std::string GetExpression() const override {
    std::ostringstream os;
    ast_.PrintFormula(os);
    return os.str();
}

    std::vector<Position> GetReferencedCells() const override {
    std::vector<Position> referenced_cells;
    for (const Position pos : ast_.GetCells()) {

        if (!std::count(referenced_cells.begin(), referenced_cells.end(), pos)) {
            referenced_cells.push_back(pos);
        }
    }

    return referenced_cells;
}
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}