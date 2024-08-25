#include "sheet.h"

#include <algorithm>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text) {
    CheckIfInvalidPosition(pos);

    if (cells_.count(pos) == 0) {
        std::unique_ptr<Cell> temp_cell = std::make_unique<Cell>(*this);
        temp_cell->Set(std::move(text), pos);
        cells_.insert_or_assign(pos, std::move(temp_cell));
    } else {
        // если ячейка уже есть, то просто пытаемся менять её содержимое
        cells_.at(pos)->Set(std::move(text), pos);
    }
}

const CellInterface *Sheet::GetCell(Position pos) const {
    CheckIfInvalidPosition(pos);
    if (cells_.count(pos) == 0) {
        return nullptr;
    }
    return cells_.at(pos).get();
}
CellInterface *Sheet::GetCell(Position pos) {
    CheckIfInvalidPosition(pos);
    if (cells_.count(pos) == 0) {
        return nullptr;
    }
    return cells_.at(pos).get();
}

void Sheet::ClearCell(Position pos) {
    CheckIfInvalidPosition(pos);
    if (cells_.count(pos) > 0) {
        cells_[pos]->Clear();
        if (cells_[pos]->IsDependentCellsEmpty()) {
            cells_.erase(pos);
        }
    }
}

Size Sheet::GetPrintableSize() const {
    if (cells_.empty()) {
        return {0, 0};
    }
    Size sheet_size = {0, 0};
    for (auto it = cells_.begin(); it != cells_.end(); ++it) {
        if (sheet_size.rows < it->first.row) {
            sheet_size.rows = it->first.row;
        }
        if (sheet_size.cols < it->first.col) {
            sheet_size.cols = it->first.col;
        }
    }
    ++sheet_size.rows;
    ++sheet_size.cols;
    return sheet_size;
}

std::ostream &operator<<(std::ostream &output, const CellInterface::Value &val) {
    if (std::holds_alternative<std::string>(val)) {
        output << std::get<std::string>(val);
    } else if (std::holds_alternative<double>(val)) {
        output << std::get<double>(val);
    } else if (std::holds_alternative<FormulaError>(val)) {
        output << "#ARITHM!";
    }
    return output;
}

void Sheet::PrintValues(std::ostream &output) const {
    Size sheet_size = GetPrintableSize();
    for (int row = 0; row != sheet_size.rows; ++row) {
        bool first_time = true;
        for (int col = 0; col != sheet_size.cols; ++col) {
            if (!first_time) {
                output << '\t';
            } else {
                first_time = false;
            }
            if (cells_.count({row, col}) > 0) {
                output << cells_.at({row, col})->GetValue();
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream &output) const {
    Size sheet_size = GetPrintableSize();
    for (int row = 0; row != sheet_size.rows; ++row) {
        bool first_time = true;
        for (int col = 0; col != sheet_size.cols; ++col) {
            if (!first_time) {
                output << '\t';
            } else {
                first_time = false;
            }
            if (cells_.count({row, col}) > 0) {
                output << cells_.at({row, col})->GetText();
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

void Sheet::CheckIfInvalidPosition(const Position &pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid cell position");
    }
}