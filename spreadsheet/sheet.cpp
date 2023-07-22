#include "sheet.h"
#include "common.h"

#include <functional>
#include <iostream>

using namespace std::literals;

Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Position not valid");
    }

    if (sheet_.size() < static_cast<size_t>(pos.row) + 1) {
        sheet_.resize(pos.row + 1);
    }

    if (sheet_[pos.row].size() < static_cast<size_t>(pos.col) + 1) {
        sheet_[pos.row].resize(pos.col + 1);
    }

    if (sheet_[pos.row][pos.col] == nullptr) {
        sheet_[pos.row][pos.col] = std::make_unique<Cell>(*this, pos);
    }

    sheet_[pos.row][pos.col]->Set(text);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Position not valid");
    }

    if (sheet_.size() > static_cast<size_t>(pos.row)) {
        if (sheet_.at(pos.row).size() > static_cast<size_t>(pos.col)) {
            return sheet_[pos.row][pos.col].get();
        }
    }

    return nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Position not valid");
    }

    if (sheet_.size() > static_cast<size_t>(pos.row)) {
        if (sheet_.at(pos.row).size() > static_cast<size_t>(pos.col)) {
            return sheet_[pos.row][pos.col].get();
        }
    }

    return nullptr;
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Position not valid");
    }

    if (sheet_.size() > static_cast<size_t>(pos.row)) {
        if (sheet_.at(pos.row).size() > static_cast<size_t>(pos.col)) {
            if (sheet_[pos.row][pos.col] != nullptr) {
                sheet_[pos.row][pos.col]->Clear();
                sheet_[pos.row][pos.col] = nullptr;
            }
        }
    }
}

Size Sheet::GetPrintableSize() const {
    if (sheet_.empty()) {
        return {0,0};
    }
    Size printable_size;
    for (size_t i = 0; i < sheet_.size(); ++i) {
        bool not_empty_row = false;

        for (size_t j = 0; j < sheet_[i].size(); ++j) {
            if (sheet_[i][j] != nullptr) {
                printable_size.cols = printable_size.cols <= std::distance(sheet_[i].begin(), sheet_[i].begin() + j) ? j + 1 : printable_size.cols;
                not_empty_row = true;
            }
        }

        if (not_empty_row) {
            printable_size.rows = i + 1;
        }
    }

    return printable_size;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int i = 0; i < GetPrintableSize().rows; ++i) {
        for (int j = 0; j < GetPrintableSize().cols; ++j) {
            if (sheet_.at(i).size() <= static_cast<size_t>(j)) {
                if (j + 1 != GetPrintableSize().cols) {
                    output << '\t';
                }
            } else if (sheet_.at(i).at(j) != nullptr) {
                auto value = sheet_.at(i).at(j)->GetValue();
                std::visit([&output](const auto& arg) { output << arg; }, value);
                if (j + 1 != GetPrintableSize().cols) {
                    output << '\t';
                }
            } else {
                if (j + 1 != GetPrintableSize().cols) {
                    output << '\t';
                }
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    for (int i = 0; i < GetPrintableSize().rows; ++i) {
        for (int j = 0; j < GetPrintableSize().cols; ++j) {
            if (sheet_.at(i).size() <= static_cast<size_t>(j)) {
                if (j + 1 != GetPrintableSize().cols) {
                    output << '\t';
                }
            } else if (sheet_.at(i).at(j) != nullptr) {
                output << sheet_.at(i).at(j)->GetText();
                if (j + 1 != GetPrintableSize().cols) {
                    output << '\t';
                }
            } else {
                if (j + 1 != GetPrintableSize().cols) {
                    output << '\t';
                }
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}