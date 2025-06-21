#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/unique_id.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#define BUTTON_GPIO 5
#define LED_PIN_RED 13

#define WIFI_SSID "XXXXXX"
#define WIFI_PASSWORD "XXXXXX"
#define SERVER_IP "192.168.1.105" // IP do servidor Python
#define SERVER_PORT 5005

float read_internal_temp() {
    adc_select_input(4); // canal do sensor interno
    uint16_t raw = adc_read();
    float voltage = raw * 3.3f / (1 << 12);
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}

int main() {
    stdio_init_all();

    gpio_init(BUTTON_GPIO);
    gpio_set_dir(BUTTON_GPIO, GPIO_IN);
    gpio_pull_up(BUTTON_GPIO); // se botão for pull-down

    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);
    gpio_put(LED_PIN_RED, 0);

    adc_init();
    adc_set_temp_sensor_enabled(true);

    if (cyw43_arch_init()) {
        printf("Erro ao iniciar Wi-Fi\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    printf("Conectando a Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Falha na conexão Wi-Fi\n");
        return -1;
    }

    printf("Conectado ao Wi-Fi!\n");

    // Espera um pouco pra estabilidade
    sleep_ms(1000);

    // Criação do socket TCP
    ip_addr_t server_addr;
    ip4addr_aton(SERVER_IP, &server_addr);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!pcb) {
        printf("Erro ao criar socket\n");
        return -1;
    }

    err_t err = tcp_connect(pcb, &server_addr, SERVER_PORT, NULL);
    if (err != ERR_OK) {
        printf("Erro ao conectar ao servidor: %d\n", err);
        return -1;
    }

    while (true) {
        bool button = !gpio_get(BUTTON_GPIO);
        float temp = read_internal_temp();

        char msg[128];
        snprintf(msg, sizeof(msg), "botao=%d;temperatura=%.2f\n", button, temp);
        printf("Enviando: %s", msg);

        struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, strlen(msg), PBUF_RAM);
        if (p) {
            memcpy(p->payload, msg, strlen(msg));
            tcp_write(pcb, p->payload, p->len, TCP_WRITE_FLAG_COPY);
            pbuf_free(p);
        }

        gpio_put(LED_PIN_RED, button); // liga LED se botão pressionado
        sleep_ms(1000);
    }

    cyw43_arch_deinit();
    return 0;
}