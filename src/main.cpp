#include "mbed.h"
#include "DHT/DHT.h"
#include "DS1820/DS1820.h"
#include "HX711/HX711.h"

#define ANEMO_MEASURE_TIME 2.f
#define ALERT_CHECK_PERIOD 60
#define SENSORS_READ_PERIOD 600

EventQueue event_queue;
EventQueue alert_queue;

LowPowerTicker alert_ticker;
LowPowerTicker ticker;

Serial sigfox(D1, D0);

DigitalOut led(LED3);

DHT dht22(D5, DHT22);

HX711 loadcell(D3, D4);

DS1820 probe1(D12);
DS1820 probe2(D11);
DS1820 probe3(D10);
DS1820 probe4(D9);
DS1820* probes[4] = {&probe1, &probe2, &probe3, &probe4};

InterruptIn anemo(D6);
AnalogIn girouette(A0);

AnalogIn battery(A1);

volatile uint32_t ticker_callback_count = 0;

int temp;

int message1 = 0;
int message2 = 0;
int message3 = 0;  

int err=0;

int probe_init[4] = {0, 0, 0, 0};
int probes_mem[4] = {0, 0, 0, 0};     // 8 bits * 4

int dht_mem=0;      // 8 bits
int humi_mem=0;     // 8 bits
int mass_mem=0;     // 16 bits

int mass_delta=0;   // 8 bits
int wind_mem=0;     // 8 bits
int bat_mem=100;    // 8 bits
int dir_mem=0;      // 3 bits
int flags=0;        // 5 bits

LowPowerTimer   timer;

volatile bool   measuringEnabled = false;
volatile int    anemo_counter;

typedef enum Direction {N, NE, E, SE, S, SW, W, NW} Direction;

void ledBlink() {
    led = !led;
}

// couting pulse for anemometer
void onPulse(void) {
    if(measuringEnabled)
        anemo_counter++;
}
// stop couting anemometer ticks
void stopMeasuring(void) {
    measuringEnabled = false;
}

void resetMessage() {
    message1 = 0;
    message2 = 0;
    message3 = 0;
}

void buildMessage() {
    message1 += ((probes_mem[0]+20)&0xff) * 1;
    message1 += ((probes_mem[1]+20)&0xff) * 100;
    message1 += ((probes_mem[2]+20)&0xff) * 10000;
    message1 += ((probes_mem[3]+20)&0xff) * 1000000;


    message2 |= ((dht_mem+20)&0xff) << 0;
    message2 |= (humi_mem   &0xff) << 8;
    message2 |= (mass_mem & 0xffff) << 16;

    message3 |= (mass_delta & 0xff);
    message3 |= (int(wind_mem*10) & 0xff) << 8;
    message3 |= (bat_mem & 0xff) << 16;
    message3 |= (int(dir_mem) & 0x7) << 24;
    
    message3 |= (flags & 0x1F) << 27;
}


Direction lireGirou() {
    float girou = girouette.read();

    Direction dir = N; // valeur par defaut
    if (0.71 <= girou && girou < 0.74)
        dir = NW;
    else if (0.74 <= girou && girou < 0.78)
        dir = SW;
    else if (0.78 <= girou && girou < 0.8)
        dir = NE;
    else if (0.8 <= girou && girou < 0.82)
        dir = W;
    else if (0.82 <= girou && girou < 0.85)
        dir = SE;
    else if (0.85 <= girou && girou < 0.87)
        dir = N;
    else if (0.87 <= girou && girou < 0.90)
        dir = E;
    else if (0.90 <= girou && girou < 1)
        dir = S;
    return dir;
}

float readAnemometer() {
    float speed = 0.0;
    // start measuring 
    anemo_counter = 0;
    measuringEnabled = true;
    timer.reset();
    timer.start();
    
    //mesurer pendant 2 secondes
    while(measuringEnabled)
        if (timer.read() > ANEMO_MEASURE_TIME) 
            break;
    
    timer.stop();
    speed = (float)(anemo_counter/(2.f*ANEMO_MEASURE_TIME))*2.4;
    
    return speed;
}

void checkAlert() {
    flags = 0;
    
    loadcell.powerUp();
    ThisThread::sleep_for(1000);
    temp = std::max(0, int(loadcell.getGram()/10.f));   // poids à la dizaine de gramme près
    loadcell.powerDown();

    int bat = int(battery.read() * 100);
    int alert = 0;

    if (temp <= 10) {
        alert = 1;
        mass_mem = temp;
        flags |= 1;
    }
    if ( bat <= 15) {
        alert = 1;
        bat_mem = bat;
        flags |= (1 << 1);
    }
    if (probes_mem[0] < 10 && probes_mem[1] < 10 && probes_mem[2] < 10 && probes_mem[3] < 10) {
        alert = 1;
        flags |= 1<<2;
    }
    if (alert == 1) {
        resetMessage();
        buildMessage();
        sigfox.printf("AT$SF=%08X%08X%08X\n\r", message3, message2, message1);
    }
}

void readSensors() {
    flags = 0;
    alert_ticker.detach();
    long int i = 0;
    float humi = 0.f;

    err = dht22.readData();
    while (err != 0 && i < 10) {
        i++;
        ThisThread::sleep_for(100);
        err = dht22.readData();
    }
    if (err == 0) {
        temp = int(dht22.ReadTemperature(CELCIUS));
        humi = dht22.ReadHumidity();
    }
    else {
        temp = dht_mem;
        humi = humi_mem;    
    }
    if (temp != 0)
        dht_mem = temp;
    if (humi != 0)
        humi_mem = humi;

    // reinit probes that were not initialized and read it
    for (int i = 0; i < 4; ++i) {
        if (probe_init[i] == 0) {
            probes[i]->begin();
            ThisThread::sleep_for(50);
        }
        probes[i]->startConversion();
        ThisThread::sleep_for(1000);
        temp = int(probes[i]->read()); 
        if (temp != 0)
            probes_mem[i] = temp;
    }

    Direction direction =  lireGirou();
    dir_mem = direction;

    float vitesse = readAnemometer();
    wind_mem = vitesse;

    loadcell.powerUp();
    ThisThread::sleep_for(1000);
    temp = std::max(0, int(loadcell.getGram()/10.f));
    loadcell.powerDown();

    mass_delta = temp - mass_mem;
    int sign = (mass_delta < 0) ? 1 : 0;
    mass_delta = mass_delta*10 + sign;
    mass_mem = temp;

    int bat = int(battery.read()*100) - 60;
    if (bat < 0) bat = 0;
    bat_mem = bat;

    resetMessage();
    buildMessage();

    sigfox.printf("AT$SF=%08X%08X%08X\r\n", message3, message2, message1);
    alert_ticker.attach(alert_queue.event(&checkAlert), ALERT_CHECK_PERIOD);
}

int main()
{
    Thread events_thread;
    events_thread.start(callback(&event_queue, &EventQueue::dispatch_forever));

    Thread alert_thread;
    alert_thread.start(callback(&alert_queue, &EventQueue::dispatch_forever));

    LowPowerTicker led_ticker;
    led_ticker.attach(callback(&ledBlink), 0.1);

    // SYSTEM INIT
    for (int i = 0; i < 4; ++i) {
        int r = probes[i]->begin();
        int timeout = 10;
        while(r!=1) {
            ThisThread::sleep_for(50);
            r = probes[i]->begin();
            if (--timeout == 0) {
                printf("probe %d not initialized \n\r", i);
                break;
            }
        }
        if (timeout > 0)
            printf("probe %d initialized \n\r", i);
        probe_init[i] = r;
    }

    loadcell.setOffset(8445576);
    
    // assign an ISR to count anemometer pulses
    anemo.rise(callback(&onPulse)); 

    message3 |= 1 << 31;
    for (int i = 0; i < 4; ++i) {
        message3 |= (probe_init[i] & 1) << (30-i);
    }
    printf("Init state probes : %d %d %d %d\n\r", probe_init[0], probe_init[1], probe_init[2], probe_init[3]);
    printf("Init done \n\r");
    
    ThisThread::sleep_for(500);
    resetMessage();
    readSensors();
    printf("Init messag sent\n\r");

    led_ticker.detach();
    // INIT END

    led = 0;
    
    ticker.attach(event_queue.event(&readSensors), SENSORS_READ_PERIOD);
    alert_ticker.attach(alert_queue.event(&checkAlert), ALERT_CHECK_PERIOD);

    while (1) {
        ThisThread::sleep_for(1000000);
    }
}
