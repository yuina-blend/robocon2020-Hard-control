#include "mbed.h"
#include <HCSR04.h>

void serial_received();

class BlackMD {
  private:
    I2C md;
    int md_address;
    const int short_brake_data = 0x80;
    void i2c_send(int i2c_address, int i2c_data) {
        wait_ms(10);
        md.start();
        md.write(i2c_address);
        md.write(i2c_data);
        md.stop();
    }

  public:
    BlackMD(PinName sda, PinName scl, int address)
        : md(sda, scl), md_address(address) {
        // pass
    }

    void short_brake() { BlackMD::i2c_send(md_address, short_brake_data); }
    void forword(int strength) {
        BlackMD::i2c_send(md_address, 131 + strength);
    }
    void back(int strength) { BlackMD::i2c_send(md_address, 124 - strength); }
};

class Led_matrix {
  private:
    const int maru = 0x00;
    const int batsu = 0xff;
    I2C i2c;
    int matrix_address;
    void i2c_send(int i2c_address, int i2c_data) {
        wait_ms(10);
        i2c.start();
        i2c.write(i2c_address);
        i2c.write(i2c_data);
        i2c.stop();
    }

  public:
    Led_matrix(PinName sda, PinName scl, int address)
        : i2c(sda, scl), matrix_address(address) {
        // pass
    }

    void change_to_maru() { Led_matrix::i2c_send(matrix_address << 1, maru); }

    void change_to_batsu() { Led_matrix::i2c_send(matrix_address << 1, batsu); }
};

Serial raspi(USBTX, USBRX);

DigitalOut leds[3] = {
    DigitalOut(D3), // red
    DigitalOut(D4), // yellow
    DigitalOut(D5)  // green
};

DigitalOut power(D6); //電源のGND
DigitalIn button(D8);
DigitalIn fuck(USER_BUTTON);
//スイッチは超音波に変える(HC-sr04?)
DigitalOut led_sticks[2] = {DigitalOut(D9), DigitalOut(D10)};
HCSR04 ultrasonic_sensor(D11, D12);   //pinは現在適当
BlackMD moters[2] = {BlackMD(D14, D15, 0x14), BlackMD(D14, D15, 0x16)};
Led_matrix eye[2] = {Led_matrix(D14, D15, 0x10), Led_matrix(D14, D15, 0x12)};

char serial_received_data = 'X';

void serial_received() {
    serial_received_data = raspi.getc();
    raspi.putc(serial_received_data);
}

int main() {
    ultrasonic_sensor.start();
    leds[0] = true;
    //  leds[1] = true;
    bool state = true;
    wait_ms(100);
    power = false;
    raspi.attach(&serial_received, Serial::RxIrq);
    while(true) {
        if(serial_received_data == 'A') {
            leds[1] = true;
            for(int i = 0; i < sizeof(moters) / sizeof(moters[0]); i++) {
                moters[i].forword(100);
            }
            for(int i = 0; i < sizeof(led_sticks) / sizeof(led_sticks[0]);
                i++) {
                led_sticks[i] = true;
            }
            while(ultrasonic_sensor.get_dist_cm() < 5) {
                state = !state;
                if(state) {
                    for(int i = 0; i < sizeof(eye) / sizeof(eye[0]); i++) {
                        eye[i].change_to_maru();
                    }
                } else {
                    for(int i = 0; i < sizeof(eye) / sizeof(eye[0]); i++) {
                        eye[i].change_to_batsu();
                    }
                }
                wait_ms(500);
                raspi.printf("dist: %d\n", ultrasonic_sensor.get_dist_cm());
            }
            raspi.printf("pushed");
            leds[2] = true;
            for(int i = 0; i < sizeof(moters) / sizeof(moters[0]); i++) {
                moters[i].short_brake();
            }
            for(int i = 0; i < sizeof(led_sticks) / sizeof(led_sticks[0]);
                i++) {
                led_sticks[i] = false;
            }
        }
    }
}