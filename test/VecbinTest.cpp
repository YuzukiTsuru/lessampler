#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>

int main() {
    // 测试数据
    std::string String = "Strings";
    std::vector<double> Vector1D(10, M_1_PI);
    std::vector<std::vector<double>> Vector2D{{M_E,   M_LN10},
                                              {M_LN2, M_LOG2E},
                                              {M_E,   M_LN10},
                                              {M_LN2, M_LOG2E},
                                              {M_LN2, M_LOG2E},
                                              {M_E,   M_LN10},
                                              {M_LN2, M_LOG2E},
                                              {M_LN2, M_LOG2E}};

    std::ofstream ofstream("test.bin", std::ios::out | std::ios::binary);

    // 首先需要写入长度，以 char 格式保存。但是由于 String.size 是一个临时变量无法写入，所以需要赋值新变量
    uint32_t outStringSize = String.size();
    // 将指向类型的指针转换为 <char *> 并给出类型的 sizeof 写入
    ofstream.write(reinterpret_cast<const char *>(&outStringSize), sizeof(uint32_t));
    ofstream.write(String.c_str(), std::streamsize(outStringSize * sizeof(char)));

    // 写入一维 Vector
    uint32_t outVector1DSize = Vector1D.size();
    // 使用了 Vector 的特性，&vector<T>[0] 是 T 类型的指针
    // 将指向类型的指针转换为 <char *> 并给出 <长度 * sizeof<T>>
    ofstream.write(reinterpret_cast<const char *>(&outVector1DSize), sizeof(uint32_t));
    ofstream.write(reinterpret_cast<const char *>(&Vector1D[0]), std::streamsize(Vector1D.size() * sizeof(double)));

    // 写入二维 Vector
    // 二维 Vector 不能直接写入，需要一层遍历
    uint32_t outVector2DSize = Vector2D.size();
    ofstream.write(reinterpret_cast<const char *>(&outVector2DSize), sizeof(uint32_t));
    for (auto &item: Vector2D) {
        uint32_t size = item.size();
        ofstream.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));
        ofstream.write(reinterpret_cast<const char *>(&item[0]), std::streamsize(item.size() * sizeof(double)));
    }

    ofstream.close();

    // 再读回来
    std::ifstream ifstream("test.bin", std::ios::in | std::ios::binary);

    // 首先读入 StringSize
    uint32_t inStringSize;
    ifstream.read(reinterpret_cast<char *>(&inStringSize), sizeof(uint32_t));

    // 创建一个 buffer 来读取
    std::vector<char> temp(inStringSize);
    ifstream.read(reinterpret_cast<char *>(&temp[0]), std::streamsize(inStringSize * sizeof(char)));
    // 然后再保存加钱
    std::string inString(temp.begin(), temp.end());

    // 读取一维 Vector
    uint32_t inVector1DSize;
    ifstream.read(reinterpret_cast<char *>(&inVector1DSize), sizeof(uint32_t));
    std::vector<double> inVector1D(inVector1DSize);
    ifstream.read(reinterpret_cast<char *>(&inVector1D[0]), std::streamsize(inVector1DSize * sizeof(double)));

    // 读取二维 Vector
    uint32_t inVector2DSize;
    ifstream.read(reinterpret_cast<char *>(&inVector2DSize), sizeof(uint32_t));

    // 声明下
    std::vector<std::vector<double>> inVector2D;
    for (uint32_t n = 0; n < inVector2DSize; ++n) {
        // 获取内层大小
        uint32_t inVector2DInnerSize = 0;
        ifstream.read(reinterpret_cast<char *>(&inVector2DInnerSize), sizeof(inVector2DInnerSize));

        // resize vector
        inVector2D.resize(inVector2DSize, std::vector<double>(inVector2DInnerSize));

        // 读取 vector2d
        ifstream.read(reinterpret_cast<char *>(&inVector2D[n][0]), std::streamsize(inVector2DInnerSize * sizeof(double)));
    }

    ifstream.close();

    // 检验数据
    if (String == inString)
        std::cout << "String OK" << std::endl;
    else
        std::cout << __LINE__ << " ERROR" << std::endl;

    if (Vector1D.size() == inVector1D.size())
        std::cout << "Vector1D Size OK, " << Vector1D.size() << " = " << inVector1D.size() << std::endl;
    else
        std::cout << __LINE__ << "ERROR" << std::endl;

    for (int i = 0; i < Vector1D.size(); ++i) {
        if (Vector1D[i] == inVector1D[i])
            std::cout << "Vector1D " << i << " OK, " << Vector1D[i] << " = " << inVector1D[i] << std::endl;
        else
            std::cout << __LINE__ << "ERROR" << std::endl;
    }

    for (int i = 0; i < Vector2D.size(); ++i) {
        for (int j = 0; j < Vector2D[0].size(); ++j) {
            if (Vector2D[i][j] == inVector2D[i][j])
                std::cout << "Vector2D " << i << j << " OK, " << Vector2D[i][j] << " = " << inVector2D[i][j] << std::endl;
            else
                std::cout << __LINE__ << "ERROR" << std::endl;
        }
    }
    return 0;
}