#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

class Cell::Impl {
public:
    virtual void Set(std::string text) {};
    virtual void Clear() {};
    virtual Cell::Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const;
    virtual void InvalidateCache(const std::unordered_set<Cell*>& parents_cells) {};
    virtual ~Impl() = default;
};

std::vector<Position> Cell::Impl::GetReferencedCells() const {
    return {};
}

class Cell::EmptyImpl : public Impl {
public:
    Cell::Value GetValue() const override {
        return 0.0;
    }

    std::string GetText() const override {
        return "";
    }

    std::vector<Position> GetReferencedCells() const override {
        return {};
    }
    ~EmptyImpl() override = default;
};

class Cell::TextImpl : public Impl {
private:
    std::string text_;

public:
    void Set(std::string text) override {
        text_ = std::move(text);
    }

    Cell::Value GetValue() const override {
        if (text_[0] == ESCAPE_SIGN) {
            return text_.substr(1);
        }
        return text_;
    }

    std::string GetText() const override {
        return text_;
    }

    void InvalidateCache(const std::unordered_set<Cell*>& parents_cells) override {
        for (const auto parent_cell : parents_cells) {
            auto cell = dynamic_cast<Cell*>(parent_cell);

            if (cell->IsReferenced()) {
                parent_cell->impl_->InvalidateCache(cell->parents_);
            }
        }
    }

    ~TextImpl() override = default;
};

class Cell::FormulaImpl : public Impl {
private:
    SheetInterface& sheet_;
    mutable std::optional<double> cache_;
    std::unique_ptr<FormulaInterface> formula_ptr_ = nullptr;

public:
    explicit FormulaImpl(SheetInterface& sheet)
            : sheet_(sheet) {
    }

    void Set(std::string text) override {
        if (formula_ptr_ == nullptr) {
            formula_ptr_ = ParseFormula(text.substr(1));
        }

        for (const Position& pos : formula_ptr_->GetReferencedCells()) {
            if (sheet_.GetCell(pos) == nullptr && pos.IsValid()) {
                sheet_.SetCell(pos, "");
            }
        }
    }

    void Clear() override {
        formula_ptr_.reset();
    }

    Cell::Value GetValue() const override {
        if (cache_.has_value()) {
            return cache_.value();
        }
        auto value = formula_ptr_->Evaluate(sheet_);

        if (std::holds_alternative<double>(value)) {
            cache_.emplace(std::get<double>(value));
            return std::get<double>(value);
        } else {
            return std::get<FormulaError>(value);
        }
    }

    std::string GetText() const override {
        return '=' + formula_ptr_->GetExpression();
    }

    std::vector<Position> GetReferencedCells() const override {
        return formula_ptr_->GetReferencedCells();
    }

    void InvalidateCache(const std::unordered_set<Cell*>& parents_cells) override {
        this->cache_.reset();

        if (!parents_cells.empty()) {
            for (const auto parent_cell : parents_cells) {
                parent_cell->impl_->InvalidateCache(parent_cell->parents_);
            }
        }
    }

    ~FormulaImpl() override = default;
};

Cell::Cell(SheetInterface& sheet, Position& pos)
        : impl_(std::make_unique<EmptyImpl>()),
          sheet_(sheet),
          pos_(pos) {
}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    if (text[0] == '=' && text.size() != 1) {
        auto tmp = std::make_unique<FormulaImpl>(sheet_);
        tmp->Set(text);
        TraverseThroughGraph(tmp->GetReferencedCells(), this);
        impl_ = std::move(tmp);
        impl_->InvalidateCache(parents_);
        UpdateReferences();
    } else if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    } else {
        impl_ = std::make_unique<TextImpl>();
        impl_->Set(text);
        impl_->InvalidateCache(parents_);
        UpdateReferences();
    }
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

void Cell::Clear() {
    impl_->InvalidateCache(parents_);
    impl_ = std::make_unique<EmptyImpl>();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
    return !parents_.empty();
}

void Cell::UpdateReferences() {
    if (IsReferenced()) {
        for (const auto cell : children_) {
            cell->parents_.erase(this);
        }
    }

    children_.clear();

    for (const Position& pos : GetReferencedCells()) {
        dynamic_cast<Cell*>(sheet_.GetCell(pos))->parents_.insert(this);
        children_.insert(dynamic_cast<Cell*>(sheet_.GetCell(pos)));
    }
}

void Cell::TraverseThroughGraph(std::vector<Position> references, Cell* root) {
    for (const auto pos : references) {
        auto cell_at_pos = dynamic_cast<Cell*>(sheet_.GetCell(pos));

        if (cell_at_pos == root) {
            throw CircularDependencyException("Formula is circular");
        }

        cell_at_pos->TraverseThroughGraph(sheet_.GetCell(pos)->GetReferencedCells(), root);
    }
}
