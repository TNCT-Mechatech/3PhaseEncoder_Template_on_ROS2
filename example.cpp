#include "3PEncoder.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
    // AMT102VのA/B/C相ピン（例：GPIO17,27,22）
    AMT102VEncoder encoder(17, 27, 22);

    // PPR測定
    encoder.measurePPR();

    // 通常のエンコーダ値表示
    while (true) {
        std::string dir;
        int d = encoder.getDirection(); // 正転,逆転を受け取る関数
        if (d > 0) dir = "正転";
        else if (d < 0) dir = "逆転";
        else dir = "停止";

        std::cout << "カウント値: " << encoder.getPosition() // 座標を受け取る関数
                  << " | 方向: " << dir
                  << "\r" << std::flush;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
