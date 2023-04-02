// Local IO: Buttons, LEDs, IR relay IO, warning lights
//-----------------------------------

#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"

#include "main.h"
#include "local_io.h"
#include "pindefs.h"

// Logging tag
static const char *TAG = "local_io";

// Input message queue handle pointer - passed in from main module
QueueHandle_t *input_event_queue_ptr;

// Define internal buffers that hold the 'current' state of the IO expanders
// When reading and writing to these the buffers are used to reduce I2C bus pileups

SemaphoreHandle_t output_state_buffer_mutex = NULL; // protects:
uint8_t output_state_buffer_changed_flag = 0;       // 0 unchanged, 1 changed since last output run therefore outputs need refreshed
struct Output_Buffer_Struct output_state_buffer;    // Raw state of outputs to IO expanders

SemaphoreHandle_t input_state_buffer_mutex = NULL; // protects:
struct Input_Buffer_Struct input_state_buffer;     // Raw state of inputs from IO expanders
struct Input_Buffer_Struct input_state_counts;     // Counters for debouncing
struct Input_Buffer_Struct input_debounced_buffer; // Debounced state

// I2C commmunication
// =============================================================================

static esp_err_t i2c_init(void)
{
    // Sets up I2C communication
    uint8_t i2c_master_port = I2C_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = PIN_I2C_SDA;
    conf.sda_pullup_en = 0;
    conf.scl_io_num = PIN_I2C_SCL;
    conf.scl_pullup_en = 0;
    conf.master.clk_speed = I2C_FREQ;
    conf.clk_flags = 0;
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_RX_BUFFER, I2C_TX_BUFFER, 0);
}

static void expander_write_command(uint8_t address, uint8_t module_register, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, address << 1 | I2C_WRITE_BIT, I2C_ACK_CHECK_EN);
    i2c_master_write_byte(cmd, module_register, I2C_ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data, I2C_ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_RATE_MS));
    i2c_cmd_link_delete(cmd);
}

static void expander_read_command(uint8_t address, uint8_t module_register, uint8_t *data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, address << 1 | I2C_WRITE_BIT, I2C_ACK_CHECK_EN);
    i2c_master_write_byte(cmd, module_register, I2C_ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, address << 1 | I2C_READ_BIT, I2C_ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data, I2C_NACK_CHECK_EN);
    i2c_master_stop(cmd);
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_RATE_MS));
    i2c_cmd_link_delete(cmd);
}

// Main tasks: output refresh and input debouncing
// =============================================================================

static void refresh_outputs(void)
{
    // Update button LEDs, warning lights and IR relay when required

    uint8_t out_byte_A_GP0 = 0x00;
    uint8_t out_byte_A_GP1 = 0x00;
    uint8_t out_byte_B_GP1 = 0x00;

    if (output_state_buffer_mutex == NULL)
    {
        ESP_LOGE(TAG, "Output buffer mutex NULL at at refresh_outputs");
        return;
    }

    if (xSemaphoreTake(output_state_buffer_mutex, (TickType_t) 10) == pdTRUE)
    {
        if (output_state_buffer_changed_flag == 0)
        {
            xSemaphoreGive(output_state_buffer_mutex);
            ESP_LOGD(TAG, "refresh outputs not required");
            return;
        }

        output_state_buffer_changed_flag = 0;

        out_byte_A_GP0 = 1 * ((output_state_buffer.led_panel[3] & 1) != 0) + 2 * ((output_state_buffer.led_panel[3] & 2) != 0) + 4 * ((output_state_buffer.led_panel[3] & 4) != 0) + 8 * ((output_state_buffer.led_panel[0] & 4) != 0) + 16 * ((output_state_buffer.led_panel[0] & 2) != 0) + 32 * ((output_state_buffer.led_panel[0] & 1) != 0) + 64 * ((output_state_buffer.led_panel[1] & 1) != 0) + 128 * ((output_state_buffer.led_panel[1] & 2) != 0);

        out_byte_A_GP1 = 1 * ((output_state_buffer.led_panel[1] & 4) != 0) + 2 * ((output_state_buffer.led_panel[2] & 4) != 0) + 4 * ((output_state_buffer.led_panel[2] & 2) != 0) + 8 * ((output_state_buffer.led_panel[2] & 1) != 0) + 16 * (output_state_buffer.usb_enable_a) + 32 * (output_state_buffer.usb_enable_b) + 64 * (output_state_buffer.ir_relay) + 128 * (output_state_buffer.ir_button_led);

        out_byte_B_GP1 = 64 * (output_state_buffer.ethernet_fail_led) + 128 * (output_state_buffer.relay_fail_led);

        xSemaphoreGive(output_state_buffer_mutex);
        ESP_LOGD(TAG, "Output bytes at refresh outputs:%#X, %#X, %#X", out_byte_A_GP0, out_byte_A_GP1, out_byte_B_GP1);
    }
    else
    {
        ESP_LOGW(TAG, "Output buffer mutex timeout at refresh_outputs");
        return;
    }

    expander_write_command(I2C_MODULE_A_ADDRESS, I2C_MODULE_REGISTER_GP0, out_byte_A_GP0);
    expander_write_command(I2C_MODULE_A_ADDRESS, I2C_MODULE_REGISTER_GP1, out_byte_A_GP1);
    expander_write_command(I2C_MODULE_B_ADDRESS, I2C_MODULE_REGISTER_GP1, out_byte_B_GP1);
}

static void button_debounce(uint8_t *raw_input, uint8_t *state, uint8_t *counter, uint8_t message_type, uint8_t panel_num)
{
    if (*raw_input == 0)
    {
        *counter = 0;
        if (*state != 0)
        {
            // Button has been pressed and released, send message to main logic
            struct Queued_Input_Message_Struct new_message;
            new_message.type = message_type;
            if (message_type == IN_MSG_TYP_ROUTING)
            {
                new_message.panel = panel_num;
                new_message.panel_button = *state - 1; // Note change from physical button 1-6 to array index 0-5 for reference to settings struct in main logic
            }

            if (xQueueSend(*input_event_queue_ptr, (void *)&new_message, 0) == pdTRUE)
            {
                ESP_LOGI(TAG, "Sending message from button debounce: %i,%i,%i", new_message.type, new_message.panel, new_message.panel_button);
            }
            else
            {
                ESP_LOGW(TAG, "Sending message from button debounce failed due to queue full? - %i,%i,%i", new_message.type, new_message.panel, new_message.panel_button);
            }
            *state = 0;
        }
    }
    else
    {
        *counter = *counter + 1;
    }

    if (*counter >= INPUT_DEBOUNCE_LOOP_COUNT)
    {
        *state = *raw_input;
    }
}

static void refresh_inputs(void)
{
    // Get total raw input state from buttons and relay from I2C expanders
    uint8_t in_byte_B_GP0 = 0x00;
    uint8_t in_byte_B_GP1 = 0x00;

    expander_read_command(I2C_MODULE_B_ADDRESS, I2C_MODULE_REGISTER_GP0, &in_byte_B_GP0);
    expander_read_command(I2C_MODULE_B_ADDRESS, I2C_MODULE_REGISTER_GP1, &in_byte_B_GP1);

    if (input_state_buffer_mutex == NULL)
    {
        ESP_LOGE(TAG, "Input buffer mutex NULL at at refresh_inputs");
        return;
    }

    if (xSemaphoreTake(input_state_buffer_mutex, (TickType_t)10) == pdTRUE)
    {
        input_state_buffer.button_panel[0] = 1 * ((in_byte_B_GP1 & 8) == 0) + 2 * ((in_byte_B_GP1 & 4) == 0) + 4 * ((in_byte_B_GP1 & 2) == 0);

        input_state_buffer.button_panel[1] = 1 * ((in_byte_B_GP0 & 64) == 0) + 2 * ((in_byte_B_GP0 & 128) == 0) + 4 * ((in_byte_B_GP1 & 1) == 0);

        input_state_buffer.button_panel[2] = 1 * ((in_byte_B_GP0 & 8) == 0) + 2 * ((in_byte_B_GP0 & 16) == 0) + 4 * ((in_byte_B_GP0 & 32) == 0);

        input_state_buffer.button_panel[3] = 1 * ((in_byte_B_GP0 & 4) == 0) + 2 * ((in_byte_B_GP0 & 2) == 0) + 4 * ((in_byte_B_GP0 & 1) == 0);

        input_state_buffer.ir_relay_aux = 1 * ((in_byte_B_GP1 & 16) == 0); // Inverted due to extern. pullup
        input_state_buffer.ir_button = 1 * ((in_byte_B_GP1 & 32) == 0);    // Inverted due to extern. pullup

        // Debounce raw inputs into debounced state for buttons, trigger events if required
        button_debounce(&input_state_buffer.button_panel[0], &input_debounced_buffer.button_panel[0], &input_state_counts.button_panel[0], IN_MSG_TYP_ROUTING, 0);
        button_debounce(&input_state_buffer.button_panel[1], &input_debounced_buffer.button_panel[1], &input_state_counts.button_panel[1], IN_MSG_TYP_ROUTING, 1);
        button_debounce(&input_state_buffer.button_panel[2], &input_debounced_buffer.button_panel[2], &input_state_counts.button_panel[2], IN_MSG_TYP_ROUTING, 2);
        button_debounce(&input_state_buffer.button_panel[3], &input_debounced_buffer.button_panel[3], &input_state_counts.button_panel[3], IN_MSG_TYP_ROUTING, 3);
        button_debounce(&input_state_buffer.ir_button, &input_debounced_buffer.ir_button, &input_state_counts.ir_button, IN_MSG_TYP_IR_TOGGLE, 0);

        xSemaphoreGive(input_state_buffer_mutex);
        ESP_LOGD(TAG, "Input bytes at refresh inputs:%#X, %#X", in_byte_B_GP0, in_byte_B_GP1);
    }
    else
    {
        ESP_LOGW(TAG, "Input buffer mutex timeout at refresh_inputs");
        return;
    }
}

static void input_poll_task(void)
{
    while (1)
    {
        refresh_inputs();
        refresh_outputs();
        vTaskDelay(REFRESH_LOOP_TICKS / portTICK_RATE_MS);
    }
}

static uint8_t buffer_single_read(uint8_t *buffer, char *label)
{
    // Generic function for returning a value from the buffer
    if (input_state_buffer_mutex == NULL)
    {
        ESP_LOGW(TAG, "Input buffer read mutex NULL at %s", label);
        return NULL;
    }

    if (xSemaphoreTake(input_state_buffer_mutex, (TickType_t)10) == pdTRUE)
    {
        uint8_t value = *buffer;
        xSemaphoreGive(input_state_buffer_mutex);
        ESP_LOGD(TAG, "Input buffer read at %s:%i", label, value);
        return value;
    }
    else
    {
        ESP_LOGW(TAG, "Input buffer read mutex timeout at %s", label);
        return NULL;
    }
}

static void buffer_single_write(uint8_t *buffer, uint8_t value, char *label)
{
    // Generic function for writing a value to the buffer
    if (output_state_buffer_mutex == NULL)
    {
        ESP_LOGW(TAG, "Output buffer write mutex NULL at %s", label);
        return;
    }

    if (xSemaphoreTake(output_state_buffer_mutex, (TickType_t)10) == pdTRUE)
    {
        if (*buffer != value)
        {
            *buffer = value;
            output_state_buffer_changed_flag = 1;
        }
        xSemaphoreGive(output_state_buffer_mutex);
        ESP_LOGD(TAG, "Output buffer write at %s:%i", label, value);
        return;
    }
    else
    {
        ESP_LOGW(TAG, "Output buffer write mutex timeout at %s", label);
        return;
    }
}

// Setup and zero outputs at poweron
// =============================================================================

void setup_local_io(QueueHandle_t *input_queue)
{
    // Set up mutexes for local buffer of IO state
    output_state_buffer_mutex = xSemaphoreCreateMutex();
    input_state_buffer_mutex = xSemaphoreCreateMutex();

    // Sets up IO expanders, sets default state of outputs
    ESP_ERROR_CHECK(i2c_init());

    // Module A - All outputs (Direction register all 0s)
    expander_write_command(I2C_MODULE_A_ADDRESS, I2C_MODULE_REGISTER_IODIR0, 0x00);
    expander_write_command(I2C_MODULE_A_ADDRESS, I2C_MODULE_REGISTER_IODIR1, 0x00);
    expander_write_command(I2C_MODULE_A_ADDRESS, I2C_MODULE_REGISTER_GP0, 0x00);
    expander_write_command(I2C_MODULE_A_ADDRESS, I2C_MODULE_REGISTER_GP1, 0x00);

    // Module B - 14 inputs, last 2 are outputs (Direction register 14 1s 2 0s)
    expander_write_command(I2C_MODULE_B_ADDRESS, I2C_MODULE_REGISTER_IODIR0, 0xFF);
    expander_write_command(I2C_MODULE_B_ADDRESS, I2C_MODULE_REGISTER_IODIR1, 0x3F);
    expander_write_command(I2C_MODULE_B_ADDRESS, I2C_MODULE_REGISTER_GP0, 0x00);
    expander_write_command(I2C_MODULE_B_ADDRESS, I2C_MODULE_REGISTER_GP1, 0x00);

    // Set up 'board live' input from standard pin - sees if SM desk is active
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_BOARD_LIVE);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // Set up local pointers to the event queue in the main logic
    input_event_queue_ptr = input_queue;

    xTaskCreate((TaskFunction_t)input_poll_task, "input_poll_task", 2048, NULL, 5, NULL);
}

// Main button panels (routing buttons)
// =============================================================================

uint8_t get_button_panel_state(uint8_t panel)
{
    // Returns main button panel state

    // Limit check
    if (panel > 3)
    {
        ESP_LOGE(TAG, "get_button_panel state called with invalid panel id of %i",panel);
        panel = 3;
    }
    char s[17];
    snprintf(s, 17, "Button panel %u", (panel + 1));
    return buffer_single_read(&input_debounced_buffer.button_panel[panel], s);
}

void set_button_led_state(uint8_t panel, uint8_t value)
{
    // Sets the state of the main button panel LEDs

    // Limit check
    if (panel > 3)
    {
        panel = 3;
    }
    char s[22];
    snprintf(s, 22, "Button panel %u LEDs", (panel + 1));
    buffer_single_write(&output_state_buffer.led_panel[panel], value, s);
}

// IR Button and relay
// =============================================================================

uint8_t get_ir_button_state(void)
{
    // Returns IR button state from internal buffer last read from I2C bus
    return buffer_single_read(&input_debounced_buffer.ir_button, "IR button");
}

void set_ir_button_led(uint8_t value)
{
    // Sets the state of the IR button LED
    buffer_single_write(&output_state_buffer.ir_button_led, value, "IR Button LED");
}

uint8_t get_ir_relay_state(void)
{
    // Returns IR relay state from internal buffer last read from I2C bus
    // Note that this doesn't use the debouncing fuction as it's not a button
    return buffer_single_read(&input_state_buffer.ir_relay_aux, "IR relay aux");
}

void toggle_ir_relay_state(void)
{
    // Starts a timed task to toggle the bistable relay in the patchbay
    // TODO
    buffer_single_write(&output_state_buffer.ir_relay, 1, "IR Relay coil");
    vTaskDelay(500 / portTICK_RATE_MS);
    // TODO - this write must happen to turn the coil off but not assured by mutex if failed
    // Build in a 'try again' loop? Checks every so often until it's done?
    buffer_single_write(&output_state_buffer.ir_relay, 0, "IR Relay coil");
}

// Warning lights
// =============================================================================

void set_ethernet_fail_led(uint8_t value)
{
    // Sets the state of the Ethernet sync failure LED
    buffer_single_write(&output_state_buffer.ethernet_fail_led, value, "Ethernet failure LED");
}

void set_relay_fail_led(uint8_t value)
{
    // Sets the state of the IR relay sync failure LED
    buffer_single_write(&output_state_buffer.relay_fail_led, value, "Relay failure LED");
}

// USB passthrough enables
// =============================================================================

void set_usb_enable_a(uint8_t value)
{
    // Sets the state the USB passthrough enable A
    buffer_single_write(&output_state_buffer.usb_enable_a, value, "USB enable A");
}

void set_usb_enable_b(uint8_t value)
{
    // Sets the state the USB passthrough enable B
    buffer_single_write(&output_state_buffer.usb_enable_b, value, "USB enable B");
}

uint8_t get_board_live_state(void)
{
    // Returns state of PCB power supply
    return buffer_single_read(&input_debounced_buffer.board_live, "Board live");
}