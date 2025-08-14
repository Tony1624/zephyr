#include<zephyr/kernel.h>
#include<zephyr/device.h>
#include<zephyr/drivers/gpio.h>
#include<zephyr/shell/shell.h>

#define LED0 DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0, gpios);

static int cmd_on(const struct shell *shell, size_t argc, char **argv)
{
    gpio_pin_set_dt(&led,1);
    shell_print(shell, "LED is ON");
    return 0;


}

static int cmd_off(const struct shell *shell, size_t argc, char **argv)
{
    gpio_pin_set_dt(&led,0);
    shell_print(shell, "LED is OFF");
    return 0;

}

SHELL_CMD_REGISTER(ON,NULL,"led on",cmd_on);
SHELL_CMD_REGISTER(OFF,NULL,"LED OFF",cmd_off);

int  main(void)
{
    if (!device_is_ready(led.port)) {
        printk("Error: LED device not ready\n");
        return;
    }
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
}

