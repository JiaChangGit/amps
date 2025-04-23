// g++ -std=c++17 -O2 append_rows.cpp -o append_rows
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// 從 source.txt 提取第 start_row 到 end_row 的行，接到 target.txt 後面
bool appendSelectedRowsToFile(const std::string& source_file,
                              const std::string& target_file,
                              size_t start_row,
                              size_t end_row) {
    if (start_row > end_row || start_row == 0) {
        std::cerr << "Invalid row range\n";
        return false;
    }

    std::ifstream src(source_file);
    if (!src.is_open()) {
        std::cerr << "Failed to open source file: " << source_file << "\n";
        return false;
    }

    std::ofstream tgt(target_file, std::ios::app);  // append 模式
    if (!tgt.is_open()) {
        std::cerr << "Failed to open target file: " << target_file << "\n";
        return false;
    }

    std::string line;
    size_t current_row = 0;

    while (std::getline(src, line)) {
        ++current_row;
        if (current_row >= start_row && current_row <= end_row) {
            tgt << line << '\n';  // 寫入目標檔案
        }
        if (current_row > end_row) break;  // 提前停止讀取
    }

    if (current_row < start_row) {
        std::cerr << "Start row exceeds total line count.\n";
        return false;
    }

    std::cout << "Successfully appended lines " << start_row << " to " << end_row
              << " from " << source_file << " to " << target_file << ".\n";
    return true;
}

// 主程式範例用法
int main() {
    std::string source = "source.txt";
    std::string target = "target.txt";

    size_t start_line = 3;
    size_t end_line = 7;

    if (!appendSelectedRowsToFile(source, target, start_line, end_line)) {
        std::cerr << "Operation failed.\n";
        return 1;
    }

    return 0;
}

