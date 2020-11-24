#include "mbed.h"
#include "DHT/DHT.h"
#include "DS1820/DS1820.h"
#include "HX711/HX711.h"

#define TEMPS_MESURE_ANEMO 2.f

EventQueue printf_queue;
EventQueue event_queue;

mbed_stats_cpu_t stats; // MBED_CPU_STATS_ENABLED or MBED_ALL_STATS_ENABLED

Serial sigfox(D1, D0);


DHT capteur1(D3, DHT11);
//DHT capteur2(D12, DHT11);

// alimenter en 5V
HX711 loadcell(D4, D5);


DS1820 sonde1(D12);
DS1820 sonde2(D11);
DS1820 sonde3(D10);
DS1820 sonde4(D9);

// anemo : c'est le blanc
InterruptIn     anemo(D6);
AnalogIn        girouette(A0);

volatile uint32_t ticker_callback_count = 0;

int temp;
float humi;
int humi_cnt = 0;

int message1 = 0;
int message2 = 0;
int message3 = 0;  

int err;

Timeout         timeout;
LowPowerTimer   timer;
volatile bool   measuringEnabled = false;
volatile int    compteurAnemo;

typedef enum Direction {N, NE, E, SE, S, SW, W, NW} Direction;


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

//ISR counting pulses
void onPulse(void) {
    if(measuringEnabled)
        compteurAnemo++;
}
// ISR to stop counting
void stopMeasuring(void) {
    measuringEnabled = false;
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

void readSensors() {
    resetMessage();

    long int i = 0;

    err = capteur1.readData();
    while (err != 0 && i < 10) {
        i++;
        ThisThread::sleep_for(100);
        err = capteur1.readData();
    }

    if (err == 0) {    
        temp = int(capteur1.ReadTemperature(CELCIUS));
        humi = capteur1.ReadHumidity();
        humi_cnt++;
        message1 |= (temp&0x1f)<<0;
    }
    else {
        printf("Capteur 1 error %d\r\n", err);
    }

    /*i = 0;
    err = capteur2.readData();
    while (err != 0 && i < 10){
        i++;
        ThisThread::sleep_for(100);
        err = capteur2.readData();
    }
    if (err == 0) {
        temp = int(capteur2.ReadTemperature(CELCIUS));
        humi += capteur2.ReadHumidity();
        humi_cnt++;
        message1 |= (temp&0x1f)<<5;
    }
    else {
        printf("Capteur 2 error %d\r\n", err);
    }*/

    // calc humidity
    if (humi_cnt > 0) {
        message2 |= (int(humi/humi_cnt)&0xff)<<8;
        printf("humi : %f\n\r", humi/humi_cnt);
    }

    
    sonde1.startConversion();
    ThisThread::sleep_for(1000);
    temp = int(sonde1.read());    
    printf("temp sonde1 : %d\n\r", temp);
    message1 |= (temp&0xff)<< 10;

    sonde2.startConversion();
    ThisThread::sleep_for(1000);
    temp = int(sonde2.read());    
    printf("temp sonde2 : %d\n\r", temp);
    message1 |= (temp&0xff) << 15;
    
    sonde3.startConversion();
    ThisThread::sleep_for(1000);
    temp = int(sonde3.read());    
    printf("temp sonde3 : %d\n\r", temp);
    message1 |= (temp&0xff) << 20;

    sonde4.startConversion();
    ThisThread::sleep_for(1000);
    temp = int(sonde4.read());    
    printf("temp sonde4 : %d\n\r", temp);
    message1 |= (temp&0xff)<< 25;

    Direction direction =  lireGirou();
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
   
    float vitesse = lireAnemo();
    printf("compteur : %d \n\r vit = %f\n\r", compteurAnemo, vitesse);

    message3 |= (int(vitesse*10) & 0xff) << 8;
    message3 |= (int(direction) & 0x7) << 16 ;
   
    loadcell.powerUp();
    ThisThread::sleep_for(1000);
    temp = std::max(0, int(loadcell.getGram()/10.f));// poids à la dizaine de gramme près
    printf("poids : %d0 g\n\r", temp);
    loadcell.powerDown();
    message2 |= (temp &0xffff) << 16;

    printf("AT$SF=%08X%08X%08X\r\n------------\n\r", message3, message2, message1);
    sigfox.printf("AT$SF=%08X%08X%08X\r\n", message3, message2, message1);
}

int main()
{
    /*Thread printfThread(osPriorityLow);
    printfThread.start(callback(&printf_queue, &EventQueue::dispatch_forever));
    */
    
    Thread events_thread;
    events_thread.start(callback(&event_queue, &EventQueue::dispatch_forever));

    #if MBED_TICKLESS
        printf("MBED_TICKLESS is enabled\n");
    #else
        printf("MBED_TICKLESS is disabled\n");
    #endif

    // initialisation de la sonde
    
    int r = sonde1.begin();
    while (r != 1) {
        ThisThread::sleep_for(50);
        r = sonde1.begin();
        printf("sonde1 begin : %d\n", r);
    }

    r = sonde2.begin();
    while (r != 1) {
        ThisThread::sleep_for(50);
        r = sonde2.begin();
        printf("sonde2 begin : %d\n", r);
    }

    r = sonde3.begin();
    while (r != 1) {
        ThisThread::sleep_for(50);
        r = sonde3.begin();
        printf("sonde3 begin : %d\n", r);
    }

    r = sonde4.begin();
    while (r != 1) {
        ThisThread::sleep_for(50);
        r = sonde4.begin();
        printf("sonde4 begin : %d\n", r);
    }
    loadcell.tare();

    // assign an ISR to count pulses POUR LE COMPTEUR ANEMOMETRE
    anemo.rise(callback(&onPulse)); 
    
    LowPowerTicker ticker;
    ticker.attach(event_queue.event(&readSensors), 600);

    while (1) {
        ThisThread::sleep_for(11000);
        //printf_queue.call(&printfCpuStats);
    }
}
