#include "spi.h"
#include <stdio.h>
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "spi_read_data.pio.h"

#define PIN_SCK  18
#define PIN_CS   17
#define PIN_MOSI 16

#define BUF_SIZE 16

uint8_t buffer[BUF_SIZE];

static PIO pio = pio1;
uint read_data_sm;
int dma_chan;
dma_channel_config c;

spi_callback_t __spi_callback = NULL;

void cs_callback(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_FALL) {
        dma_channel_set_write_addr(dma_chan, buffer, false);
        dma_channel_set_trans_count(dma_chan, BUF_SIZE, false);
        dma_channel_start(dma_chan);

    }
    if (events & GPIO_IRQ_EDGE_RISE) {
        dma_channel_abort(dma_chan);
        pio_sm_clear_fifos(pio, read_data_sm);
        if (__spi_callback != NULL) {
            __spi_callback(buffer);
        }
    }

}


void setup_dma(){
    dma_chan = dma_claim_unused_channel(true);
    c = dma_channel_get_default_config(dma_chan);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, pio_get_dreq(pio, read_data_sm, false));
    dma_channel_configure(dma_chan, &c, buffer, &pio->rxf[read_data_sm], BUF_SIZE, false);
}

void setup_cs_irq(){
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_IN);
    gpio_pull_up(PIN_CS);
    gpio_set_irq_enabled_with_callback(PIN_CS, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &cs_callback);
}

void setup_pio(){
    uint read_data_offset = pio_add_program(pio, &spi_read_data_program);
    read_data_sm = pio_claim_unused_sm(pio, true);
    spi_read_data_program_init(pio, read_data_sm, read_data_offset, PIN_MOSI);
}

void spi_init(spi_callback_t spi_callback) {
    __spi_callback = spi_callback;
    setup_pio();
    setup_dma();
    setup_cs_irq();
}
