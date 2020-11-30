#include "mbed.h"
#include "DHT/DHT.h"
#include "DS1820/DS1820.h"
#include "HX711/HX711.h"

#define TEMPS_MESURE_ANEMO 2.f

//EventQueue printf_queue;
EventQueue event_queue;
EventQueue alert_queue;


mbed_stats_cpu_t stats; // MBED_CPU_STATS_ENABLED or MBED_ALL_STATS_ENABLED

Serial sigfox(D1, D0);

DigitalOut led(LED3);
DigitalOut led2(LED2);

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

int message1 = 0;
int message2 = 0;
int message3 = 0;  

int err;

int sonde_init[4] = {0, 0, 0, 0};

int temp_mem[5];
int humi_mem;
int mass_mem=0;
int mass_delta=0;
int wind_mem;
int dir_mem;

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

void resetMessage() {
    message1 = 0;
    message2 = 0;
    message3 = 0;
    for (int i = 0; i < 4; ++i) {
        message3 |= (sonde_init[i] & 1) << (30-i);
    }
    humi = 0.f;
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

void readMass() {
    loadcell.powerUp();
    ThisThread::sleep_for(1000);
    temp = std::max(0, int(loadcell.getGram()/10.f));// poids à la dizaine de gramme près
    printf("poids : %d0 g\n\r", temp);
    loadcell.powerDown();

    if (temp <= 10) {
        printf("AT$SF=%08X%08X%08X\n\r", 0x1000000, 0, 0);
        sigfox.printf("AT$SF=%08X%08X%08X\n\r", 0x1000000, 0, 0);
    }
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
    }
    else {
        printf("Capteur 1 error %d\r\n", err);
        temp = temp_mem[0];
        humi = humi_mem;    
    }
    if (temp != 0) {
        temp_mem[0] = temp+20;            
    }
    if (humi != 0) {
        humi_mem = humi;
    }
    message2 |= (temp_mem[0]&0xff)<<0;
    message2 |= (humi_mem   &0xff)<<8;

    printf("DHT : %d °C  %d humi \n\r", temp_mem[0], humi_mem);

    // SONDE 1
    sonde1.startConversion();
    ThisThread::sleep_for(1000);
    temp = int(sonde1.read()); 
    printf("temp sonde1 : %d\n\r", temp);
    
    if (temp != 0)
        temp_mem[1] = temp+20;
    message1 += (temp_mem[1]&0xff) << 0;

    // SONDE 2
    sonde2.startConversion();
    ThisThread::sleep_for(1000);
    temp = int(sonde2.read());    
    printf("temp sonde2 : %d\n\r", temp);
    
    if (temp != 0)
        temp_mem[2] = temp+20;
    message1 += (temp_mem[2]&0xff) * 100;
    
    // SONDE 3
    sonde3.startConversion();
    ThisThread::sleep_for(1000);
    temp = int(sonde3.read());    
    printf("temp sonde3 : %d\n\r", temp);
    
    if (temp != 0)
        temp_mem[3] = temp+20;
    message1 += (temp_mem[3]&0xff) * 10000;

    // SONDE 4
    sonde4.startConversion();
    ThisThread::sleep_for(1000);
    temp = int(sonde4.read());    
    printf("temp sonde4 : %d\n\r", temp);
    
    if (temp != 0)
        temp_mem[4] = temp+20;
    message1 += (temp_mem[4]&0xff) * 1000000;
    
    printf("Sonde var : %d\n", message1);
    
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

    mass_delta = mass_mem - temp;
    int sign = (mass_delta < 0) ? 1 : 0;
    mass_delta = mass_delta*10 + sign;
    mass_mem = temp;

    message2 |= (mass_mem & 0xffff) << 16;

    message3 |= (mass_delta & 0xff) ;


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

    // initialisation de la sonde
    
    int r = sonde1.begin();
    int timeout = 20;
    while (r != 1) {
        ThisThread::sleep_for(50);
        r = sonde1.begin();
        printf("sonde1 begin : %d\n", r);
        if (--timeout == 0)
            break;
    }
    sonde_init[0] = r;

    r = sonde2.begin();
    timeout = 20;
    while (r != 1) {
        ThisThread::sleep_for(50);
        r = sonde2.begin();
        printf("sonde2 begin : %d\n", r);
        if (--timeout == 0)
            break;
    }
    sonde_init[1] = r;

    r = sonde3.begin();
    timeout = 20;
    while (r != 1) {
        ThisThread::sleep_for(50);
        r = sonde3.begin();
        printf("sonde3 begin : %d\n", r);
        if (--timeout == 0)
            break;
    }
    sonde_init[2] = r;

    r = sonde4.begin();
    timeout = 20;
    while (r != 1) {
        ThisThread::sleep_for(50);
        r = sonde4.begin();
        printf("sonde4 begin : %d\n", r);
        if (--timeout == 0)
            break;
    }
    sonde_init[3] = r;

    loadcell.tare();

    message3 |= 1 << 31;
    for (int i = 0; i < 4; ++i) {
        message3 |= (sonde_init[i] & 1) << (30-i);
    }
    printf("Init state : pow : %d  sondes : %d %d %d %d\n\r", 
        1, sonde_init[0], sonde_init[1], sonde_init[2], sonde_init[3]);
    printf("AT$SF=%08X%08X%08X\r\n", message3, message2, message1);
    sigfox.printf("AT$SF=%08X%08X%08X\r\n", message3, message2, message1);


    printf("Init done \n\r");
    led_ticker.detach();
    led = 0;

    // assign an ISR to count pulses POUR LE COMPTEUR ANEMOMETRE
    anemo.rise(callback(&onPulse)); 
    
    LowPowerTicker ticker;
    ticker.attach(event_queue.event(&readSensors), 600);

    LowPowerTicker alert_ticker;
    alert_ticker.attach(alert_queue.event(&readMass), 60);

    while (1) {
        ThisThread::sleep_for(11000);
        //printf_queue.call(&printfCpuStats);
    }
}
