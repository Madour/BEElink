#include "mbed.h"
#include "DHT/DHT.h"
#include "DS1820/DS1820.h"
#include "HX711/HX711.h"

#define TEMPS_MESURE_ANEMO 2.f
#define ALERT_CHECK_PERIOD 60
#define SENSORS_READ_PERIOD 600

//EventQueue printf_queue;
EventQueue event_queue;
EventQueue alert_queue;

LowPowerTicker alert_ticker;
LowPowerTicker ticker;


mbed_stats_cpu_t stats; // MBED_CPU_STATS_ENABLED or MBED_ALL_STATS_ENABLED

Serial sigfox(D1, D0);

DigitalOut led(LED3);
DigitalOut led2(LED2);

DHT capteur1(D5, DHT11);
//DHT capteur2(D12, DHT11);

// alimenter en 5V
HX711 loadcell(D3, D4);


DS1820 sonde1(D12);
DS1820 sonde2(D11);
DS1820 sonde3(D10);
DS1820 sonde4(D9);
DS1820* sondes[4] = {&sonde1, &sonde2, &sonde3, &sonde4};

// anemo : c'est le blanc
InterruptIn anemo(D6);
AnalogIn girouette(A0);

AnalogIn batterie(A1);

volatile uint32_t ticker_callback_count = 0;

int temp;

int message1 = 0;
int message2 = 0;
int message3 = 0;  

int err=0;

int sonde_init[4] = {0, 0, 0, 0};

int sondes_mem[4]={0, 0, 0, 0};     // 8 bits * 4

int dht_mem=0;      // 8 bits
int humi_mem=0;     // 8 bits
int mass_mem=0;     // 16 bits

int mass_delta=0;   // 8 bits
int wind_mem=0;     // 8 bits
int bat_mem=100;  // 8 bits
int dir_mem=0;      // 3 bits
int flags=0;        // 5 bits

Timeout         timeout;
LowPowerTimer   timer;
volatile bool   measuringEnabled = false;
volatile int    compteurAnemo;

typedef enum Direction {N, NE, E, SE, S, SW, W, NW} Direction;


void printfCpuStats(void) {
    mbed_stats_cpu_get(&stats);
    printf("CPU %lu : up time %9llu (sleep %2llu%%, deepsleep %2llu%%)\n", ticker_callback_count, stats.uptime, (stats.sleep_time * 100) / stats.uptime, (stats.deep_sleep_time * 100) / stats.uptime);
}

void ledBlink() {
    led = !led;
}

//ISR counting pulses
void onPulse(void) {
    if(measuringEnabled)
        compteurAnemo++;
}
// ISR to stop counting
void stopMeasuring(void) {
    measuringEnabled = false;
}

void printDirection(Direction direction) {
    switch (direction) {
    case N:
        printf("Nord\n\r");
        break;
    case NE:
        printf("NordEst\n\r");
        break;
    case E:
        printf("Est\n\r");
        break;
    case SE:
        printf("SudEst\n\r");
        break;
    case S:
        printf("Sud\n\r");
        break;
    case SW:
        printf("SudWest\n\r");
        break;
    case W:
        printf("West\n\r");
        break;
    case NW:
        printf("NordWest\n\r");
        break;
    default:
        break;
    }
}

void resetMessage() {
    message1 = 0;
    message2 = 0;
    message3 = 0;
}

void buildMessage() {
    message1 += ((sondes_mem[0]+20)&0xff) * 1;
    message1 += ((sondes_mem[1]+20)&0xff) * 100;
    message1 += ((sondes_mem[2]+20)&0xff) * 10000;
    message1 += ((sondes_mem[3]+20)&0xff) * 1000000;


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
    printf("girou %f\n\r", girou);
    Direction sens = N; // valeur par defaut
    if (0.71 <= girou && girou < 0.74)
        sens = NW;
    else if (0.74 <= girou && girou < 0.78)
        sens = SW;
    else if (0.78 <= girou && girou < 0.8)
        sens = NE;
    else if (0.8 <= girou && girou < 0.82)
        sens = W;
    else if (0.82 <= girou && girou < 0.85)
        sens = SE;
    else if (0.85 <= girou && girou < 0.87)
        sens = N;
    else if (0.87 <= girou && girou < 0.90)
        sens = E;
    else if (0.90 <= girou && girou < 1)
        sens = S;
    return sens;
}

float lireAnemo() {
    float vitesse = 0.0;
    // start measuring 
    compteurAnemo = 0;
    measuringEnabled = true;
    timer.reset();
    timer.start();
    
    //mesurer pendant 2 secondes
    while(measuringEnabled)
        if (timer.read() > TEMPS_MESURE_ANEMO) 
            break;
    
    timer.stop();
    vitesse = (float)(compteurAnemo/(2.f*TEMPS_MESURE_ANEMO))*2.4;
    
    return vitesse;
}

void checkAlert() {
    flags = 0;
    
    printf("Batt : %f \n\r", batterie.read());

    loadcell.powerUp();
    ThisThread::sleep_for(1000);
    temp = std::max(0, int(loadcell.getGram()/10.f));   // poids à la dizaine de gramme près
    printf("poids : %d0 g\n\r", temp);
    loadcell.powerDown();

    int bat = int(batterie.read() * 100);
    int alert = 0;

    if (temp <= 10) {
        alert = 1;
        mass_mem = temp;
        flags |= 1;
        printf("Masse alert : masse = %d0 g", temp);
    }
    if ( bat <= 15) {
        alert = 1;
        bat_mem = bat;
        flags |= (1 << 1);
        printf("Batterie alert : Batterie =  %d % \n\r", bat);
    }
    if (sondes_mem[0] < 10 && sondes_mem[1] < 10 && sondes_mem[2] < 10 && sondes_mem[3] < 10) {
        alert = 1;
        flags |= 1<<2;
    }
    if (alert == 1) {
        resetMessage();
        buildMessage();
        printf("AT$SF=%08X%08X%08X\n\r", message3, message2, message1);
        sigfox.printf("AT$SF=%08X%08X%08X\n\r", message3, message2, message1);
    }
}

void readSensors() {
    flags = 0;
    alert_ticker.detach();
    long int i = 0;
    float humi = 0.f;
    // DHT 11
    err = capteur1.readData();
    while (err != 0 && i < 10) {
        i++;
        ThisThread::sleep_for(100);
        err = capteur1.readData();
    }
    if (err == 0) {
        temp = int(capteur1.ReadTemperature(CELCIUS));
        humi = capteur1.ReadHumidity();
    }
    else {
        printf("Capteur 1 error %d\r\n", err);
        temp = dht_mem;
        humi = humi_mem;    
    }
    if (temp != 0)
        dht_mem = temp;
    if (humi != 0)
        humi_mem = humi;

    printf("DHT : %d °C  %d humi \n\r", dht_mem+20, humi_mem);

    // tentative de réinitialisation des sondes non fonctionnelles
    for (int i = 0; i < 4; ++i) {
        if (sonde_init[i] == 0) {
            sondes[i]->begin();
        }
    }

    // SONDE 1
    sonde1.startConversion();
    ThisThread::sleep_for(1000);
    temp = int(sonde1.read()); 
    if (temp != 0)
        sondes_mem[0] = temp;
    printf("temp sonde1 : %d , sent : %d\n\r", temp, sondes_mem[0]);

    // SONDE 2
    sonde2.startConversion();
    ThisThread::sleep_for(1000);
    temp = int(sonde2.read());    
    if (temp != 0)
        sondes_mem[1] = temp;
    printf("temp sonde2 : %d , sent : %d\n\r", temp, sondes_mem[1]);
    
    // SONDE 3
    sonde3.startConversion();
    ThisThread::sleep_for(1000);
    temp = int(sonde3.read());    
    if (temp != 0)
        sondes_mem[2] = temp;
    printf("temp sonde2 : %d , sent : %d\n\r", temp, sondes_mem[2]);

    // SONDE 4
    sonde4.startConversion();
    ThisThread::sleep_for(1000);
    temp = int(sonde4.read());    
    if (temp != 0)
        sondes_mem[3] = temp;
    printf("temp sonde3 : %d , sent : %d\n\r", temp, sondes_mem[3]);

    Direction direction =  lireGirou();
    printDirection(direction);
    dir_mem = direction;

    float vitesse = lireAnemo();
    printf("compteur : %d \n\r vit = %f\n\r", compteurAnemo, vitesse);
    wind_mem = vitesse;

    loadcell.powerUp();
    ThisThread::sleep_for(1000);
    temp = std::max(0, int(loadcell.getGram()/10.f));// poids à la dizaine de gramme près
    printf("poids : %d0 g\n\r", temp);
    loadcell.powerDown();

    mass_delta = temp - mass_mem;
    int sign = (mass_delta < 0) ? 1 : 0;
    mass_delta = mass_delta*10 + sign;
    mass_mem = temp;
    printf("Delta mass : %d \n\r", mass_delta);

    int bat = int(batterie.read()*100);
    bat_mem = bat;
    printf("Batterie %d \n\r", int(batterie.read()*100));

    resetMessage();
    buildMessage();

    printf("AT$SF=%08X%08X%08X\r\n------------\n\r", message3, message2, message1);
    sigfox.printf("AT$SF=%08X%08X%08X\r\n", message3, message2, message1);

    alert_ticker.attach(alert_queue.event(&checkAlert), ALERT_CHECK_PERIOD);
}

int main()
{
    /*Thread printfThread(osPriorityLow);
    printfThread.start(callback(&printf_queue, &EventQueue::dispatch_forever));
    */
    
    Thread events_thread;
    events_thread.start(callback(&event_queue, &EventQueue::dispatch_forever));

    Thread alert_thread;
    alert_thread.start(callback(&alert_queue, &EventQueue::dispatch_forever));

    LowPowerTicker led_ticker;
    led_ticker.attach(callback(&ledBlink), 0.1);

    led2 = 0;

    #if MBED_TICKLESS
        printf("MBED_TICKLESS is enabled\n");
    #else
        printf("MBED_TICKLESS is disabled\n");
    #endif

    // SYSTEM INIT
    for (int i = 0; i < 4; ++i) {
        int r = sondes[i]->begin();
        int timeout = 40;
        while(r!=1) {
            ThisThread::sleep_for(50);
            r = sondes[i]->begin();
            if (--timeout == 0) {
                printf("Sonde %d not initialized \n\r", i);
                break;
            }
        }
        if (timeout > 0)
            printf("Sonde %d initialized \n\r", i);
        sonde_init[i] = r;
    }

    loadcell.setOffset(8445576);
    
    // assign an ISR to count pulses POUR LE COMPTEUR ANEMOMETRE
    anemo.rise(callback(&onPulse)); 

    message3 |= 1 << 31;
    for (int i = 0; i < 4; ++i) {
        message3 |= (sonde_init[i] & 1) << (30-i);
    }
    printf("Init state : pow : %d  sondes : %d %d %d %d\n\r", 
        1, sonde_init[0], sonde_init[1], sonde_init[2], sonde_init[3]);
    printf("Init done \n\r");
    
    ThisThread::sleep_for(500);
    resetMessage();
    readSensors();
    printf("Init messag sent\n\r");

    led_ticker.detach();


    led = 0;
    
    ticker.attach(event_queue.event(&readSensors), SENSORS_READ_PERIOD);

    alert_ticker.attach(alert_queue.event(&checkAlert), ALERT_CHECK_PERIOD);

    while (1) {
        ThisThread::sleep_for(110000);
        //printf_queue.call(&printfCpuStats);
    }
}
