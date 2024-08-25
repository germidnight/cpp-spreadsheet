#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <optional>
#include <string>
#include <variant>

CellInterface::Value Cell::FormulaImpl::GetValue(const SheetInterface &sheet) const {
    if (!cache_.has_value()) {
        cache_ = formula_->Evaluate(sheet);
    }
    if (std::holds_alternative<double>(cache_.value())) {
        return std::get<double>(cache_.value());
    }
    return std::get<FormulaError>(cache_.value());
}

std::string Cell::FormulaImpl::GetText() const {
    return "=" + formula_->GetExpression();
}

// возвращает индексы всех ячеек, которые входят в формулу
std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_->GetReferencedCells();
}

void Cell::CellInvalidateCache() {
    for (Cell *cell : dependent_cells_) {
        cell->impl_->ImplInvalidateCache();
    }
}

void Cell::FormulaImpl::ImplInvalidateCache() {
    cache_.reset();
}

void Cell::Set(std::string text, const Position& my_pos) {
    if (text == impl_->GetText()) { // ячейка не изменилась
        return;
    }
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    } else if (text.length() > 1 && text[0] == FORMULA_SIGN) { // в ячейке формула
        /* когда пользователь задаёт текст формульной ячейки в методе Cell::Set(),
         * лексический анализатор возвращает std::forward_list<Position> cells_ с индексами ячеек, входящих в формулу.  */
        std::unique_ptr<FormulaImpl> impl_temp = std::make_unique<FormulaImpl>(std::move(text.substr(1)));

        // сохранить предыдущее состояние
        std::unordered_set<Cell*> last_ref_to_cells;
        if (!referring_to_cells_.empty()) {
            last_ref_to_cells = referring_to_cells_;
        }

        bool itself_ref = false;
        /* перенос ячеек, входящих в формулу, проверяем наличие ячеек, на которые ссылается формула, если ячейки нет, то создать пустую */
        for (Position cell_pos : impl_temp->GetReferencedCells()) {
            if (my_pos == cell_pos) { // ячейка ссылается на саму себя
                itself_ref = true;
                break;
            }
            Cell *cell_addr = (Cell *)(sheet_.GetCell(cell_pos));
            if (cell_addr == nullptr) {
                sheet_.SetCell(cell_pos, {});
                cell_addr = (Cell *)(sheet_.GetCell(cell_pos));
            }
            referring_to_cells_.insert(cell_addr);
        }

        if (itself_ref || IsCyclic()) { // определяем наличие циклов для формул
            referring_to_cells_.clear();
            if (!last_ref_to_cells.empty()) {
                referring_to_cells_ = last_ref_to_cells;
            }
            throw CircularDependencyException("Found circular dependency in formula");
            return;
        }
        impl_ = std::move(impl_temp);
        UpdateDependencies();  // обновить перечень зависимых ячеек
        CellInvalidateCache(); // пройтись по зависимым ячейкам и очистить их кэш
    } else {
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }
}

void Cell::Clear() {
    // очистить кэш по всем dependent_cells_
    CellInvalidateCache();
    // очистить referring_to_cells_
    referring_to_cells_.clear();

    /* вызвать деструктор (автоматически для unique_ptr) и сбросить указатель на impl_ */
    impl_.reset(nullptr);
    /* но если есть другие ячейки, зависящие от этой, то должна остаться пустая ячейка*/
    if (!dependent_cells_.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    }
}

Cell::Value Cell::GetValue() const {
    assert(impl_ != nullptr);
    return impl_->GetValue(sheet_);
}
std::string Cell::GetText() const {
    assert(impl_ != nullptr);
    return impl_->GetText();
}
std::vector<Position> Cell::GetReferencedCells() const {
    assert(impl_ != nullptr);
    return impl_->GetReferencedCells();
}

bool Cell::IsDependentCellsEmpty() const {
    if (dependent_cells_.empty()) {
        return true;
    }
    return false;
}

bool Cell::DFS(Cell *v_from, std::unordered_map<Cell *, int>& vertices_color) {
    vertices_color[v_from] = 1;
    for (Cell *v : v_from->referring_to_cells_) {
        if (vertices_color.count(v) == 0) {
            vertices_color[v] = 0;
        }
    }
    for (Cell *v_to : v_from->referring_to_cells_) {
        if (vertices_color[v_to] == 0) {
            if (DFS(v_to, vertices_color)) {
                return true;
            }
        } else if (vertices_color[v_to] == 1) {
            return true;
        }
    }
    vertices_color[v_from] = 2;
    return false;
}

bool Cell::IsCyclic() {
    if (referring_to_cells_.empty()) {
        return false;
    }
    /* Цвет вершин:
     * 0 - не посещённая вершина
     * 1 - в вершину вошли, но ещё не вышли
     * 2 - вышли из вершины */
    std::unordered_map<Cell *, int> vertices_color;

    for (Cell *v : referring_to_cells_) {
        vertices_color[v] = 0;
    }

    for (auto [v, color] : vertices_color) {
        if (color != 2) {
            if (DFS(v, vertices_color)) {
                return true;
            }
        }
    }
    return false;
}

void Cell::UpdateDependencies() {
    for (Cell* cell : referring_to_cells_) {
        if (cell->dependent_cells_.count(this) == 0) {
            cell->dependent_cells_.insert(this);
        }
    }
}