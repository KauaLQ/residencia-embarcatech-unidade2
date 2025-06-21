#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#define WIFI_SSID "XXXXXX"
#define WIFI_PASSWORD "XXXXXX"
#define SERVER_IP "192.168.1.105"
#define SERVER_PORT 5005

#define DEADZONE 400
#define ADC_CENTER 2048

int16_t get_axis_offset(uint input) {
    int16_t delta = (int16_t)input - ADC_CENTER;
    if (delta > -DEADZONE && delta < DEADZONE) return 0;
    return delta;
}

const char* get_direction(int16_t dx, int16_t dy) {
    if (dx == 0 && dy == 0) return "Centro";
    if (dx > 0 && dy == 0) return "Leste";
    if (dx < 0 && dy == 0) return "Oeste";
    if (dx == 0 && dy > 0) return "Norte";
    if (dx == 0 && dy < 0) return "Sul";
    if (dx > 0 && dy > 0) return "Nordeste";
    if (dx < 0 && dy > 0) return "Noroeste";
    if (dx > 0 && dy < 0) return "Sudeste";
    if (dx < 0 && dy < 0) return "Sudoeste";
    return "Desconhecido";
}

int main() {
    stdio_init_all();
    adc_init();

    // X = ADC0 (GPIO 26), Y = ADC1 (GPIO 27)
    adc_gpio_init(26);
    adc_gpio_init(27);

    if (cyw43_arch_init()) {
        printf("Erro ao inicializar Wi-Fi\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Erro ao conectar no Wi-Fi\n");
        return -1;
    }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);

    ip_addr_t server_ip;
    ip4addr_aton(SERVER_IP, &server_ip);
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!pcb || tcp_connect(pcb, &server_ip, SERVER_PORT, NULL) != ERR_OK) {
        printf("Erro na conexão TCP\n");
        return -1;
    }

    while (true) {
        adc_select_input(0); // eixo X
        uint16_t raw_x = adc_read();

        adc_select_input(1); // eixo Y
        uint16_t raw_y = adc_read();

        int16_t dx = get_axis_offset(raw_x);
        int16_t dy = get_axis_offset(raw_y);

        const char* dir = get_direction(dx, dy);

        char msg[100];
        snprintf(msg, sizeof(msg), "x=%d;y=%d;direcao=%s\n", dx, dy, dir);
        printf("%s", msg);

        struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, strlen(msg), PBUF_RAM);
        if (p) {
            memcpy(p->payload, msg, strlen(msg));
            tcp_write(pcb, p->payload, p->len, TCP_WRITE_FLAG_COPY);
            pbuf_free(p);
        }

        sleep_ms(300);
    }

    return 0;
}