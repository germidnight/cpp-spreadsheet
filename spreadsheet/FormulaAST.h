#pragma once

#include "FormulaLexer.h"
#include "common.h"

#include <forward_list>
#include <functional>
#include <optional>
#include <stdexcept>

namespace ASTImpl {
class Expr;
}

class ParsingError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class FormulaAST {
public:
    explicit FormulaAST(std::unique_ptr<ASTImpl::Expr> root_expr, std::forward_list<Position> cells_pos);
    FormulaAST(FormulaAST&&) = default;
    FormulaAST& operator=(FormulaAST&&) = default;
    ~FormulaAST();

    double Execute(const SheetInterface &sheet) const;
    void Print(std::ostream& out) const;
    void PrintFormula(std::ostream& out) const;
    void PrintCells(std::ostream &out) const;

    std::forward_list<Position> &GetCells();
    const std::forward_list<Position> &GetCells() const;

private:
    std::unique_ptr<ASTImpl::Expr> root_expr_;
    /* тут хранятся все встреченные индексы ячеек при парсинге формулы в методе ParseFormulaAST */
    std::forward_list<Position> cells_pos_;
};

FormulaAST ParseFormulaAST(std::istream& in);
FormulaAST ParseFormulaAST(const std::string& in_str);