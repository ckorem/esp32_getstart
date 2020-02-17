#include "uart_tests.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include <string.h>


typedef struct uart_if_struct {
	uart_port_t	port;
	size_t		rx_buf_sz;
	size_t		tx_buf_sz;
	gpio_num_t	rx_pin;
	gpio_num_t	tx_pin;
	gpio_num_t	rts_pin;
	gpio_num_t	cts_pin;
} uart_if_t;


static const uart_if_t test_uart0 = {
	.port = UART_NUM_2,
	.rx_buf_sz = 1024,
	.tx_buf_sz = 0,
	.rx_pin = GPIO_NUM_16,
	.tx_pin = GPIO_NUM_17,
	.rts_pin = UART_PIN_NO_CHANGE, 
	.cts_pin = UART_PIN_NO_CHANGE,
};

static const uart_if_t test_uart1 = {
	.port = UART_NUM_1,
	.rx_buf_sz = 1024,
	.tx_buf_sz = 0,
	.rx_pin = GPIO_NUM_4,
	.tx_pin = GPIO_NUM_5,
	.rts_pin = UART_PIN_NO_CHANGE, 
	.cts_pin = UART_PIN_NO_CHANGE,
};


// WARNING! Installs driver!
void uart_config_apply(const uart_if_t if_cfg, const uart_config_t uart_cfg);

static void init();
static void generator_task(); // simply sends predefined message with some time interval; uses test_uart0
static void echo_task(); // receives messages and echoes em back; uses test_uart1
static void terminator_task(); // receives messages and prints em to the console; uses test_uart0


void uart_tests(void)
{
	printf("UART tests;\n");
	printf("Will use UART port %u to generate initial message and consume the result;\n", test_uart0.port);
	printf("Will use UART port %u to echo any incoming message back;\n", test_uart1.port);

	init();

	xTaskCreate(generator_task, "uart_test_generator", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
	xTaskCreate(echo_task, "uart_test_echo", 1024*2, NULL, configMAX_PRIORITIES, NULL);
	xTaskCreate(terminator_task, "uart_test_terminator", 1024*2, NULL, configMAX_PRIORITIES, NULL);
}

void uart_config_apply(const uart_if_t if_cfg, const uart_config_t uart_cfg)
{
	#if 1 // output specified configuration
	printf("UART port %u configuration: baud = %u, TX pin = %u, RX pin = %u\n",
		if_cfg.port, uart_cfg.baud_rate, if_cfg.tx_pin, if_cfg.rx_pin);
	#endif

	uart_param_config( if_cfg.port, &uart_cfg );
	uart_set_pin( if_cfg.port, if_cfg.tx_pin, if_cfg.rx_pin, if_cfg.rts_pin, if_cfg.cts_pin );
	uart_driver_install( if_cfg.port, if_cfg.rx_buf_sz, if_cfg.tx_buf_sz, 0, NULL, 0 );
}

static void init() {
	const uart_config_t uart_config = {
		.baud_rate = 115200,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
	};

	uart_config_apply( test_uart0, uart_config );
	uart_config_apply( test_uart1, uart_config );
}

int UartSendStr(const uart_if_t* if_ptr, const char* str)
{
	const size_t len = strlen(str);
	return uart_write_bytes( if_ptr->port, str, len );
}

int UartSendData(const uart_if_t* if_ptr, const void* data, size_t size)
{
	return uart_write_bytes( if_ptr->port, data, size );
}

// expects buffer of at least if_ptr->rx_buf_sz in size
int UartGetData(const uart_if_t* if_ptr, void* data, unsigned long long timeout)
{
	return uart_read_bytes( if_ptr->port, data, if_ptr->rx_buf_sz, timeout );
}

static void generator_task()
{
	printf("%s started\n", __PRETTY_FUNCTION__);
	uart_if_t const * const if_ptr = &test_uart0;

	vTaskDelay(1000 / portTICK_PERIOD_MS);

	while (1) {
		const char str[] = "Hello, modified world!";
		const int sent_cnt = UartSendStr( if_ptr, str );
		printf("Uart port %u, sent %d bytes: \"%s\"\n", if_ptr->port, sent_cnt, str);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);
}

static void echo_task()
{
	printf("%s started\n", __PRETTY_FUNCTION__);
	uart_if_t const * const if_ptr = &test_uart1;

	uint8_t* data = (uint8_t*) malloc(if_ptr->rx_buf_sz+1);
	while (1) {
		const int rxBytes = UartGetData( if_ptr, data, 100 / portTICK_RATE_MS );
		if (rxBytes > 0) {
			data[rxBytes] = 0;
			const int txBytes = UartSendData( if_ptr, data, rxBytes );
			printf("Port %u echoed %d bytes from: \"%s\"\n", if_ptr->port, txBytes, data);
		}
	}
	free(data);
	vTaskDelete(NULL);
}

static void terminator_task()
{
	printf("%s started\n", __PRETTY_FUNCTION__);
	uart_if_t const * const if_ptr = &test_uart0;

	uint8_t* data = (uint8_t*) malloc(if_ptr->rx_buf_sz+1);
	while (1) {
		const int rxBytes = UartGetData( if_ptr, data, 100 / portTICK_RATE_MS );
		if (rxBytes > 0) {
			data[rxBytes] = 0;
			printf("Port %u consumed %d bytes: \"%s\"\n\n", if_ptr->port, rxBytes, data);
		}
	}
	free(data);
	vTaskDelete(NULL);
}

