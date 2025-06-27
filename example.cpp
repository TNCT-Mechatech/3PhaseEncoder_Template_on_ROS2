#include "3PEncoder.hpp"
#include <iostream>
#include <thread>
#include <chrono>

AMT102VEncoder encoder(17, 27, 22);

int main()
{

    int ppr = encoder.measurePPR();

    while (true)
    {
        encoder.updateSpeed(); // 100msごとなどで呼び出す
        std::string dir;
        int d = encoder.getDirection();
        if (d > 0)
            dir = "正転";
        else if (d < 0)
            dir = "逆転";
        else
            dir = "停止";

        std::cout << "カウント値: " << encoder.getPosition()
                  << " | 方向: " << dir
                  << " | 速度: " << encoder.getSpeedRPS() << " [rps]"
                  << " | " << encoder.getSpeedRPM() << " [rpm]        "
                  << "\r" << std::flush;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
