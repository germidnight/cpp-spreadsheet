#pragma once

#include "common.h"
#include "formula.h"

#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet) : impl_(std::make_unique<EmptyImpl>()),
                        sheet_(sheet) {}
    ~Cell() = default;

    void Set(std::string text, const Position& my_pos);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    // возвращает индексы всех ячеек, которые входят в формулу
    std::vector<Position> GetReferencedCells() const override;
    bool IsDependentCellsEmpty() const;

private:
    class Impl {
    public:
        virtual ~Impl() = default;

        virtual Value GetValue(const SheetInterface&) const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const = 0;
        virtual void ImplInvalidateCache() {}
    };

    class EmptyImpl : public Impl {
    public:
        EmptyImpl() = default;

        Value GetValue(const SheetInterface&) const override {
            return 0.;
        }
        std::string GetText() const override {
            return "";
        }
        std::vector<Position> GetReferencedCells() const override {
            return {};
        }
    };

    class TextImpl : public Impl {
    public:
        explicit TextImpl(std::string text) : text_{std::move(text)} {}

        Value GetValue(const SheetInterface&) const override {
            return (text_.length() > 0 && text_[0] == ESCAPE_SIGN) ? text_.substr(1) : text_;
        }
        std::string GetText() const override {
            return text_;
        }
        std::vector<Position> GetReferencedCells() const override {
            return {};
        }

    private:
        std::string text_;
    };

    class FormulaImpl : public Impl {
    public:
        FormulaImpl(std::string text)
                : formula_{ParseFormula(std::move(text))} {}

        Value GetValue(const SheetInterface& sheet) const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;

        void ImplInvalidateCache() override;

    private:
        std::unique_ptr<FormulaInterface> formula_;
        mutable std::optional<FormulaInterface::Value> cache_; // кэш вычисленного значения
    };

    void UpdateDependencies();
    bool IsCyclic();
    bool DFS(Cell *v_from, std::unordered_map<Cell *, int> &vertices_color);
    void CellInvalidateCache();

    std::unique_ptr<Impl> impl_;
    Sheet &sheet_;
    std::unordered_set<Cell*> referring_to_cells_; /* на кого ссылается (поиск циклов) */
    std::unordered_set<Cell*> dependent_cells_;    /* кто ссылается на ячейку (инвалидация кэша) */
};