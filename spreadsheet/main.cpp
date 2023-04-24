#include "cell.h"
#include "common.h"
#include "formula.h"
#include "sheet.h"
#include "test_runner_p.h"

#include <limits>

inline Position operator"" _pos(const char* str, std::size_t) {
    return Position::FromString(str);
}

int main() {
    /*Example 1
    {
        auto sheet = CreateSheet();
        sheet->SetCell("A1"_pos, "1");
        sheet->SetCell("A2"_pos, "20");
        sheet->SetCell("B2"_pos, "=A2/A1");
        sheet->PrintTexts(std::cout);
        sheet->PrintValues(std::cout);        
    }
    */
    /*Exapmle2
    {
        auto sheet = CreateSheet();
        for (int i = 0; i <= 5; ++i) {
            sheet->SetCell(Position{i, i}, std::to_string(i));
            sheet->PrintTexts(std::cout);
            sheet->PrintValues(std::cout); 

        }

        for (int i = 5; i >= 0; --i) {
            sheet->ClearCell(Position{i, i});
            sheet->PrintTexts(std::cout);
            sheet->PrintValues(std::cout); 
        }        
    }
    */
    return 0;
}