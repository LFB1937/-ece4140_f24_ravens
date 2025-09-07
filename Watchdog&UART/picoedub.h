#define PICOEDUB_LED0_PIN      1
#define PICOEDUB_LED1_PIN      0
#define PICOEDUB_LED2_PIN      2
#define PICOEDUB_LED3_PIN      3
#define PICOEDUB_SW2R3_PIN     9
#define PICOEDUB_SW3R2_PIN     8
#define PICOEDUB_SW4R1_PIN     7
#define PICOEDUB_SW5R0_PIN     6
#define PICOEDUB_COL0_PIN      10
#define PICOEDUB_COL1_PIN      11
#define PICOEDUB_COL2_PIN      12
#define PICOEDUB_COL3_PIN      13

void edub_init(){
    //LED0
    gpio_init(PICOEDUB_LED0_PIN);
    gpio_set_dir(PICOEDUB_LED0_PIN, GPIO_OUT);

    //LED1
    gpio_init(PICOEDUB_LED1_PIN);
    gpio_set_dir(PICOEDUB_LED1_PIN, GPIO_OUT);

    //LED2
    gpio_init(PICOEDUB_LED2_PIN);
    gpio_set_dir(PICOEDUB_LED2_PIN, GPIO_OUT);

    //LED3
    gpio_init(PICOEDUB_LED3_PIN);
    gpio_set_dir(PICOEDUB_LED3_PIN, GPIO_OUT);

    //SW2R3
    gpio_init(PICOEDUB_SW2R3_PIN);
    gpio_set_dir(PICOEDUB_SW2R3_PIN, GPIO_IN);
    
    //SW3R2
    gpio_init(PICOEDUB_SW3R2_PIN);
    gpio_set_dir(PICOEDUB_SW3R2_PIN, GPIO_IN);

    //SW4R1
    gpio_init(PICOEDUB_SW4R1_PIN);
    gpio_set_dir(PICOEDUB_SW4R1_PIN, GPIO_IN);

    //SW5R0
    gpio_init(PICOEDUB_SW5R0_PIN);
    gpio_set_dir(PICOEDUB_SW5R0_PIN, GPIO_IN);

    //COL0
    gpio_init(PICOEDUB_COL0_PIN);
    gpio_set_dir(PICOEDUB_COL0_PIN, GPIO_IN);

    //COL1
    gpio_init(PICOEDUB_COL1_PIN);
    gpio_set_dir(PICOEDUB_COL1_PIN, GPIO_IN);
    
    //COL2
    gpio_init(PICOEDUB_COL2_PIN);
    gpio_set_dir(PICOEDUB_COL2_PIN, GPIO_IN);
    
    //COL3
    gpio_init(PICOEDUB_COL3_PIN);
    gpio_set_dir(PICOEDUB_COL3_PIN, GPIO_IN);
}

void pico_set_LED0(bool led_on) {
    // Just set the GPIO on or off
    gpio_put(PICOEDUB_LED0_PIN, led_on);
}

void pico_set_LED1(bool led_on) {
    // Just set the GPIO on or off
    gpio_put(PICOEDUB_LED1_PIN, led_on);
}

void pico_set_LED2(bool led_on) {
    // Just set the GPIO on or off
    gpio_put(PICOEDUB_LED2_PIN, led_on);
}

void pico_set_LED3(bool led_on) {
    // Just set the GPIO on or off
    gpio_put(PICOEDUB_LED3_PIN, led_on);
}


bool read_sw2r3(){
    gpio_init(PICOEDUB_SW2R3_PIN);
    gpio_set_dir(PICOEDUB_SW2R3_PIN, GPIO_OUT);
    gpio_put(PICOEDUB_SW2R3_PIN, false);
    gpio_init(PICOEDUB_SW2R3_PIN);
    gpio_set_dir(PICOEDUB_SW2R3_PIN, GPIO_IN);
    return gpio_get(PICOEDUB_SW2R3_PIN);
}

bool read_sw3r2(){
    gpio_init(PICOEDUB_SW3R2_PIN);
    gpio_set_dir(PICOEDUB_SW3R2_PIN, GPIO_OUT);
    gpio_put(PICOEDUB_SW3R2_PIN, false);
    gpio_init(PICOEDUB_SW3R2_PIN);
    gpio_set_dir(PICOEDUB_SW3R2_PIN, GPIO_IN);
    return gpio_get(PICOEDUB_SW3R2_PIN);
}

bool read_sw4r1(){
    gpio_init(PICOEDUB_SW4R1_PIN);
    gpio_set_dir(PICOEDUB_SW4R1_PIN, GPIO_OUT);
    gpio_put(PICOEDUB_SW4R1_PIN, false);
    gpio_init(PICOEDUB_SW4R1_PIN);
    gpio_set_dir(PICOEDUB_SW4R1_PIN, GPIO_IN);
    return gpio_get(PICOEDUB_SW4R1_PIN);
}

bool read_sw5r0(){
    gpio_init(PICOEDUB_SW5R0_PIN);
    gpio_set_dir(PICOEDUB_SW5R0_PIN, GPIO_OUT);
    gpio_put(PICOEDUB_SW5R0_PIN, false);
    gpio_init(PICOEDUB_SW5R0_PIN);
    gpio_set_dir(PICOEDUB_SW5R0_PIN, GPIO_IN);
    return gpio_get(PICOEDUB_SW5R0_PIN);
}

bool read_col0(){
    gpio_init(PICOEDUB_COL0_PIN);
    gpio_set_dir(PICOEDUB_COL0_PIN, GPIO_OUT);
    gpio_put(PICOEDUB_COL0_PIN, false);
    gpio_init(PICOEDUB_COL0_PIN);
    gpio_set_dir(PICOEDUB_COL0_PIN, GPIO_IN);
    return gpio_get(PICOEDUB_COL0_PIN);
}

bool read_col1(){
    gpio_init(PICOEDUB_COL1_PIN);
    gpio_set_dir(PICOEDUB_COL1_PIN, GPIO_OUT);
    gpio_put(PICOEDUB_COL1_PIN, false);
    gpio_init(PICOEDUB_COL1_PIN);
    gpio_set_dir(PICOEDUB_COL1_PIN, GPIO_IN);
    return gpio_get(PICOEDUB_COL1_PIN);
}

bool read_col2(){
    gpio_init(PICOEDUB_COL2_PIN);
    gpio_set_dir(PICOEDUB_COL2_PIN, GPIO_OUT);
    gpio_put(PICOEDUB_COL2_PIN, false);
    gpio_init(PICOEDUB_COL2_PIN);
    gpio_set_dir(PICOEDUB_COL2_PIN, GPIO_IN);
    return gpio_get(PICOEDUB_COL2_PIN);
}

bool read_col3(){
    gpio_init(PICOEDUB_COL3_PIN);
    gpio_set_dir(PICOEDUB_COL3_PIN, GPIO_OUT);
    gpio_put(PICOEDUB_COL3_PIN, false);
    gpio_init(PICOEDUB_COL3_PIN);
    gpio_set_dir(PICOEDUB_COL3_PIN, GPIO_IN);
    return gpio_get(PICOEDUB_COL3_PIN);
}
