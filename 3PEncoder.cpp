#include "3PEncoder.hpp"
#include <iostream>
#include <thread>
#include <chrono>

std::atomic<int> AMT102VEncoder::pigpioInitCount(0);

AMT102VEncoder::AMT102VEncoder(int gpioA, int gpioB, int gpioC, int ppr, double gear_ratio)
    : pinA(gpioA), pinB(gpioB), pinC(gpioC),
      ppr_(ppr), gear_ratio_(gear_ratio),
      position(0), lastDirection(0), revolutionFlag(false),
      cbIdA(-1), cbIdB(-1), cbIdC(-1),
      last_position(0), speed_rps(0.0)
{
    if (pigpioInitCount.fetch_add(1) == 0) {
        if (gpioInitialise() < 0) {
            throw std::runtime_error("pigpio initialization failed");
        }
    }

    gpioSetMode(pinA, PI_INPUT);
    gpioSetMode(pinB, PI_INPUT);
    gpioSetMode(pinC, PI_INPUT);
    gpioSetPullUpDown(pinA, PI_PUD_UP);
    gpioSetPullUpDown(pinB, PI_PUD_UP);
    gpioSetPullUpDown(pinC, PI_PUD_UP);

    cbIdA = gpioSetAlertFuncEx(pinA, encoderISR, this);
    cbIdB = gpioSetAlertFuncEx(pinB, encoderISR, this);
    cbIdC = gpioSetAlertFuncEx(pinC, cPhaseISR, this);

    last_time = std::chrono::steady_clock::now();
}

AMT102VEncoder::~AMT102VEncoder()
{
    // pigpioのコールバックはアンインストール不要
    if (pigpioInitCount.fetch_sub(1) == 1) {
        gpioTerminate();
    }
}

void AMT102VEncoder::start()
{
    // 必要ならここで状態初期化など（今回は特に何もしない）
}

void AMT102VEncoder::stop()
{
    // 必要ならここでコールバック解除など（今回は特に何もしない）
}

int AMT102VEncoder::getPosition() const
{
    return position.load();
}

double AMT102VEncoder::getPositionOutput() const
{
    // モーター軸のカウントから出力軸の回転数
    return (static_cast<double>(position.load()) / ppr_) / gear_ratio_;
}

int AMT102VEncoder::getDirection() const
{
    return lastDirection.load();
}

// --- 速度測定 ---

void AMT102VEncoder::updateSpeed()
{
    // 100msなど任意周期で呼び出してください
    auto now = std::chrono::steady_clock::now();
    int pos = getPosition();
    auto dt = std::chrono::duration<double>(now - last_time).count(); // 秒

    if (dt > 0.0) {
        int dp = pos - last_position;
        speed_rps = dp / dt / static_cast<double>(ppr_);
    }

    last_position = pos;
    last_time = now;
}

double AMT102VEncoder::getSpeedRPS() const
{
    return speed_rps;
}

double AMT102VEncoder::getSpeedRPM() const
{
    return speed_rps * 60.0;
}

double AMT102VEncoder::getOutputSpeedRPS() const
{
    return speed_rps / gear_ratio_;
}

double AMT102VEncoder::getOutputSpeedRPM() const
{
    return getOutputSpeedRPS() * 60.0;
}

void AMT102VEncoder::encoderISR(int gpio, int level, uint32_t tick, void* userdata)
{
    AMT102VEncoder* self = reinterpret_cast<AMT102VEncoder*>(userdata);

    static int lastA = 0, lastB = 0;
    int A = gpioRead(self->pinA);
    int B = gpioRead(self->pinB);

    int delta = 0;
    if (gpio == self->pinA && level != PI_TIMEOUT) {
        if (A != lastA) {
            if (A == B) delta = +1;
            else       delta = -1;
            lastA = A;
        }
    } else if (gpio == self->pinB && level != PI_TIMEOUT) {
        if (B != lastB) {
            if (A != B) delta = +1;
            else       delta = -1;
            lastB = B;
        }
    }
    if (delta != 0) {
        self->position += delta;
        self->lastDirection = (delta > 0) ? 1 : -1;
    }
}

void AMT102VEncoder::cPhaseISR(int gpio, int level, uint32_t tick, void* userdata)
{
    AMT102VEncoder* self = reinterpret_cast<AMT102VEncoder*>(userdata);
    if (level == 1) { // 立ち上がり
        self->revolutionFlag = true;
    }
}

int AMT102VEncoder::measurePPR()
{
    std::cout << "AMT102V 分解能（PPR）自動測定を開始します。" << std::endl;
    std::cout << "エンコーダ軸を1回転させてください。" << std::endl;

    int start_count = 0, end_count = 0;
    bool measuring = false;
    int revolutions = 0;
    int prev_direction = 0;

    while (revolutions < 2) {
        int d = getDirection();
        if (revolutionFlag.exchange(false)) {
            revolutions++;
            if (!measuring) {
                start_count = getPosition();
                measuring = true;
                prev_direction = d;
                std::cout << "[START] C相検出: 計測開始" << std::endl;
            } else {
                end_count = getPosition();
                std::cout << "[END] C相検出: 計測終了" << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    int diff = end_count - start_count;
    if (prev_direction < 0 && diff > 0) diff = -diff;
    int ppr = std::abs(diff);

    std::cout << "AMT102V 実測分解能（PPR）： " << ppr << " パルス/回転" << std::endl;
    std::cout << "DIPスイッチ設定と一致するかご確認ください。" << std::endl;

    return ppr;
}

