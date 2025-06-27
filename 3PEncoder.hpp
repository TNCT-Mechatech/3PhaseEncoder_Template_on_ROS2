#pragma once

#include <pigpio.h>
#include <atomic>
#include <chrono>

class AMT102VEncoder
{
public:
    AMT102VEncoder(int gpioA, int gpioB, int gpioC, int ppr, double gear_ratio = 1.0);
    ~AMT102VEncoder();

    void start();
    void stop();

    // 現在のパルスカウント値
    int getPosition() const;            // モーター軸のカウント
    double getPositionOutput() const;   // 出力軸の回転数

    // 直近の回転方向（1=正転, -1=逆転, 0=静止）
    int getDirection() const;

    // 速度関連(モーター軸)
    void updateSpeed(); // 定期的に呼び出す（例：100msごと）
    double getSpeedRPS() const; // 1秒あたりの回転数[rps]
    double getSpeedRPM() const; // 1分あたりの回転数[rpm]

    // 速度関連(出力軸)
    double getOutputSpeedRPS() const;   // 出力軸のrps（ギア比補正済み）
    double getOutputSpeedRPM() const;   // 出力軸のrpm

    //ギア比の閲覧, 変更
    double getGearRatio() const { return gear_ratio_; }
    void setGearRatio(double ratio) { gear_ratio_ = ratio; }

    // 分解能測定（1回転あたりのパルス数/PPRを自動計測）
    // 計測のため、1回転させる必要がある
    int measurePPR();

private:
    int pinA, pinB, pinC;
    const int ppr_;        // 定数：1回転あたりパルス数
    double gear_ratio_; // ギア比

    std::atomic<int> position;
    std::atomic<int> lastDirection;
    std::atomic<bool> revolutionFlag;

    // pigpioコールバックID管理用
    int cbIdA, cbIdB, cbIdC;

    // 速度計算用
    int last_position;
    std::chrono::steady_clock::time_point last_time;
    double speed_rps;

    static void encoderISR(int gpio, int level, uint32_t tick, void* userdata);
    static void cPhaseISR(int gpio, int level, uint32_t tick, void* userdata);

    // pigpio初期化状態を管理（複数インスタンスでも正しく扱うために）
    static std::atomic<int> pigpioInitCount;
};
