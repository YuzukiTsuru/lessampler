#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#define _PI                      3.14159265358979               // PI
#define _HALF_PI                 1.57079632679490               // PI / 2
#define _QUARTER_PI              0.78539816339745               // PI / 4
#define _TWAIN_PI                6.28318530717958               // PI * 2
#define _E                       2.71828182845905               // e
#define _LOG2E                   1.44269504088896               // log2(e)
#define _LOG10E                  0.43429448190325               // log10(e)
#define _LN2                     0.69314718055995               // ln(2)
#define _LN10                    2.30258509299405               // ln(10)
#define _SQRT_2                  1.41421356237310               // sqrt(2)
#define _SQRT_1_2                0.70710678118655               // 1/sqrt(2)


int main() {
    // 测试数据
    std::string String = "Strings";
    std::vector<double> Vector1D(10, _PI);
    std::vector<std::vector<double>> Vector2D{
            {_HALF_PI,  _QUARTER_PI},
            {_TWAIN_PI, _E},
            {_LOG2E,    _LOG10E},
            {_LN2,      _LN10},
            {_SQRT_2,   _SQRT_1_2},
            {_LOG2E,    _LOG10E},
            {_LN2,      _LN10},
            {_SQRT_2,   _SQRT_1_2}
    };

    std::ofstream ofstream("test.bin", std::ios::out | std::ios::binary);

    // 首先需要写入长度，以 char 格式保存。但是由于 String.size 是一个临时变量无法写入，所以需要赋值新变量
    std::streamsize outStringSize = String.size();
    // 将指向类型的指针转换为 <char *> 并给出类型的 sizeof 写入
    ofstream.write(reinterpret_cast<const char *>(&outStringSize), sizeof(std::streamsize));
    ofstream.write(String.c_str(), std::streamsize(outStringSize * sizeof(char)));

    // 写入一维 Vector
    std::streamsize outVector1DSize = Vector1D.size();
    // 使用了 Vector 的特性，&vector<T>[0] 是 T 类型的指针
    // 将指向类型的指针转换为 <char *> 并给出 <长度 * sizeof<T>>
    ofstream.write(reinterpret_cast<const char *>(&outVector1DSize), sizeof(std::streamsize));
    ofstream.write(reinterpret_cast<const char *>(&Vector1D[0]), Vector1D.size() * sizeof(double));

    // 写入二维 Vector
    // 二维 Vector 不能直接写入，需要一层遍历
    std::streamsize outVector2DSize = Vector2D.size();
    ofstream.write(reinterpret_cast<const char *>(&outVector2DSize), sizeof(std::streamsize));
    for (auto &item: Vector2D) {
        std::streamsize size = item.size();
        ofstream.write(reinterpret_cast<const char *>(&size), sizeof(std::streamsize));
        ofstream.write(reinterpret_cast<const char *>(&item[0]), item.size() * sizeof(double));
    }

    ofstream.close();

    // 再读回来
    std::ifstream ifstream("test.bin", std::ios::in | std::ios::binary);

    // 首先读入 StringSize
    std::streamsize inStringSize;
    ifstream.read(reinterpret_cast<char *>(&inStringSize), sizeof(std::streamsize));

    // 创建一个 buffer 来读取
    std::vector<char> temp(inStringSize);
    ifstream.read(reinterpret_cast<char *>(&temp[0]), std::streamsize(inStringSize * sizeof(char)));
    // 然后再保存加钱
    std::string inString(temp.begin(), temp.end());

    // 读取一维 Vector
    std::streamsize inVector1DSize;
    ifstream.read(reinterpret_cast<char *>(&inVector1DSize), sizeof(std::streamsize));
    std::vector<double> inVector1D(inVector1DSize);
    ifstream.read(reinterpret_cast<char *>(&inVector1D[0]), std::streamsize(inVector1DSize * sizeof(double)));

    // 读取二维 Vector
    std::streamsize inVector2DSize;
    ifstream.read(reinterpret_cast<char *>(&inVector2DSize), sizeof(std::streamsize));

    // 声明下
    std::vector<std::vector<double>> inVector2D;
    for (std::streamsize n = 0; n < inVector2DSize; ++n) {
        // 获取内层大小
        std::streamsize inVector2DInnerSize = 0;
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