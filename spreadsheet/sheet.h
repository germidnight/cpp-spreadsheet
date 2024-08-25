#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <map>
#include <unordered_map>

struct PositionHasher {
    size_t operator()(const Position &pos) const {
        const size_t trivial_number = 16411;
        return static_cast<size_t>(pos.row) + trivial_number * static_cast<size_t>(pos.col);
    }
};

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface *GetCell(Position pos) const override;
    CellInterface *GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream &output) const override;
    void PrintTexts(std::ostream &output) const override;

private:
    void CheckIfInvalidPosition(const Position& pos) const;

    std::unordered_map<Position, std::unique_ptr<Cell>, PositionHasher> cells_;
};