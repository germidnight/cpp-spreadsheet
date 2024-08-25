#include "formula.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

namespace {
    class Formula : public FormulaInterface {
    public:
        explicit Formula(std::string expression)
                        try : ast_(ParseFormulaAST(std::move(expression))) {
                            Position prev = Position::NONE;
                            for (Position &cell : ast_.GetCells()) {
                                if (!(cell == prev)) {
                                    prev = cell;
                                    referenced_cells_.push_back(std::move(cell));
                                }
                            }
                        } catch(const std::exception& e) {
                            throw FormulaException(e.what());
                        }

        Value Evaluate(const SheetInterface& sheet) const override {
            try {
                return ast_.Execute(sheet);
            } catch(const FormulaError& e) {
                return e;
            }
        }
        std::string GetExpression() const override {
            std::stringstream ss;
            ast_.PrintFormula(ss);
            return ss.str();
        }
        std::vector<Position> GetReferencedCells() const override {
            return referenced_cells_;
        }

    private :
        FormulaAST ast_;
        std::vector<Position> referenced_cells_;
    };
} // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}