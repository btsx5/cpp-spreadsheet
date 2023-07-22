#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>
#include <set>

class Sheet;

class Cell : public CellInterface {
private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    std::unordered_set<Cell*> parents_;
    std::unordered_set<Cell*> children_;
    std::unique_ptr<Impl> impl_;
    SheetInterface& sheet_;
    Position pos_;

public:
    Cell(SheetInterface& sheet, Position& pos);
    ~Cell() override;
    void Set(std::string text);
    Value GetValue() const override;
    std::string GetText() const override;
    void Clear();
    std::vector<Position> GetReferencedCells() const override;

private:
    bool IsReferenced() const;
    void UpdateReferences();
    void TraverseThroughGraph(std::vector<Position> references, Cell* root);
};
