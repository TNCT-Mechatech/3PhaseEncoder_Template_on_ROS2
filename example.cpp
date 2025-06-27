#include "3PEncoder.hpp"
#include <iostream>
#include <thread>
#include <chrono>

// 例: DIPスイッチでPPR=2048, ギア比=30:1
AMT102VEncoder encoder(17, 27, 22, 2048, 30.0);

int main()
{

    encoder.measurePPR();

    while (true)
    {
        encoder.updateSpeed();
        std::cout << "モーター軸: "
                  << encoder.getPosition() << " count, "
                  << encoder.getSpeedRPM() << " rpm | ";

        std::cout << "出力軸: "
                  << encoder.getPositionOutput() << " 回転, "
                  << encoder.getOutputSpeedRPM() << " rpm"
                  << "\r" << std::flush;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}