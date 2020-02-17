#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "uart_tests.h"
#include "gpio_tests.h"


static void salutations(void);

void app_main()
{
	salutations();

	uart_tests();
	// gpio_tests();
}


static void salutations(void)
{
	printf("ESP-WROOM-32 code; compiled " __DATE__ " " __TIME__ "\n");
	printf("FW: \"ESP32_periph_tests\"\n");
}

