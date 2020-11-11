#include "mbed.h"
#include "DHT/DHT.h"


EventQueue printf_queue;
EventQueue event_queue;

mbed_stats_cpu_t stats; // MBED_CPU_STATS_ENABLED or MBED_ALL_STATS_ENABLED

DigitalOut led1(LED3);

DHT capteur1(D11, DHT11);
DHT capteur2(D12, DHT11);

volatile uint32_t ticker_callback_count = 0;

int temp;
float humi;
int humi_cnt = 0;

int message1 = 0;
int message2 = 0;
int message3 = 0;  

int err;

void ledBlink(void) {
    led1 = !led1;
    ticker_callback_count++;
}

void printfCpuStats(void) {
    mbed_stats_cpu_get(&stats);
    printf("CPU %lu : up time %9llu (sleep %2llu%%, deepsleep %2llu%%)\n", ticker_callback_count, stats.uptime, (stats.sleep_time * 100) / stats.uptime, (stats.deep_sleep_time * 100) / stats.uptime);
}

void resetMessage() {
    message1 = 0;
    message2 = 0;
    message3 = 0;
    humi = 0.f;
    humi_cnt = 0;
}

void readSensors() {
    resetMessage();

    int i = 0;

    err = capteur1.readData();
    while (err != 0 && i++ < 10)
        err = capteur1.readData();

    if (err == 0) {    
        temp = int(capteur1.ReadTemperature(CELCIUS));
        humi = capteur1.ReadHumidity();
        humi_cnt++;
        message1 |= (temp&0x1f)<<0;
    }
    else {
        printf("Capteur 1 error %d\r\n", err);
    }

    i = 0;
    err = capteur2.readData();
    while (err != 0 && i++ < 10)
        err = capteur2.readData();

    if (err == 0) {
        temp = int(capteur2.ReadTemperature(CELCIUS));
        humi += capteur2.ReadHumidity();
        humi_cnt++;
        message1 |= (temp&0x1f)<<5;
    }
    else {
        printf("Capteur 2 error %d\r\n", err);
    }

    // calc humidity
    message2 |= (int(humi/humi_cnt)&0xff)<<8;

    printf("humi : %f on %d\n\r", humi/humi_cnt, humi_cnt);

    printf("AT$SF=%08X%08X%08X\r\n", message3, message2, message1);
    //sigfox.printf("AT$SF=%08X%08X%08X\r\n", message3, message2, message1);
}

int main()
{

    Thread printfThread(osPriorityLow);
    printfThread.start(callback(&printf_queue, &EventQueue::dispatch_forever));
    
    Thread events_thread;
    events_thread.start(callback(&event_queue, &EventQueue::dispatch_forever));
    

    #if MBED_TICKLESS
        printf("MBED_TICKLESS is enabled\n");
    #else
        printf("MBED_TICKLESS is disabled\n");
    #endif

    LowPowerTicker led_ticker;
    led_ticker.attach(callback(ledBlink), 0.5);

    LowPowerTicker ticker;
    ticker.attach(event_queue.event(&readSensors), 10);

    while (1) {
        ThisThread::sleep_for(11000);
        printf_queue.call(&printfCpuStats);
    }
}
