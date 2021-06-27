//
// Created by david on 19.10.20.
//
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_modem.h"

/**
 * @brief ESP32 Modem DTE
 *
 */
typedef struct {
    uart_port_t uart_port;                  /*!< UART port */
    uint8_t *buffer;                        /*!< Internal buffer to store response lines/data from DCE */
    QueueHandle_t event_queue;              /*!< UART event queue handle */
    esp_event_loop_handle_t event_loop_hdl; /*!< Event loop handle */
    TaskHandle_t uart_event_task_hdl;       /*!< UART event task handle */
    SemaphoreHandle_t process_sem;          /*!< Semaphore used for indicating processing status */
    modem_dte_t parent;                     /*!< DTE interface that should extend */
    esp_modem_on_receive receive_cb;        /*!< ptr to data reception */
    void *receive_cb_ctx;                   /*!< ptr to rx fn context data */
    int line_buffer_size;                   /*!< line buffer size in commnad mode */
    int pattern_queue_size;                 /*!< UART pattern queue size */
} esp_modem_dte_t;

modem_dte_t *esp_modem_dte_init(const esp_modem_dte_config_t *config)
{
    esp_err_t res;
    /* malloc memory for esp_dte object */
    esp_modem_dte_t *esp_dte = calloc(1, sizeof(esp_modem_dte_t));
    MODEM_CHECK(esp_dte, "calloc esp_dte failed", err_dte_mem);
    /* malloc memory to storing lines from modem dce */
    esp_dte->line_buffer_size = config->line_buffer_size;
    esp_dte->buffer = calloc(1, config->line_buffer_size);
    MODEM_CHECK(esp_dte->buffer, "calloc line memory failed", err_line_mem);
    /* Set attributes */
    esp_dte->uart_port = config->port_num;
    esp_dte->parent.flow_ctrl = config->flow_control;
    /* Bind methods */
    esp_dte->parent.send_cmd = esp_modem_dte_send_cmd;
    esp_dte->parent.send_data = esp_modem_dte_send_data;
    esp_dte->parent.send_wait = esp_modem_dte_send_wait;
    esp_dte->parent.change_mode = esp_modem_dte_change_mode;
    esp_dte->parent.process_cmd_done = esp_modem_dte_process_cmd_done;
    esp_dte->parent.deinit = esp_modem_dte_deinit;

    /* Config UART */
    uart_config_t uart_config = {
            .baud_rate = config->baud_rate,
            .data_bits = config->data_bits,
            .parity = config->parity,
            .stop_bits = config->stop_bits,
            .source_clk = UART_SCLK_REF_TICK,
            .flow_ctrl = (config->flow_control == MODEM_FLOW_CONTROL_HW) ? UART_HW_FLOWCTRL_CTS_RTS : UART_HW_FLOWCTRL_DISABLE
    };
    MODEM_CHECK(uart_param_config(esp_dte->uart_port, &uart_config) == ESP_OK, "config uart parameter failed", err_uart_config);
    if (config->flow_control == MODEM_FLOW_CONTROL_HW) {
        res = uart_set_pin(esp_dte->uart_port, config->tx_io_num, config->rx_io_num,
                           config->rts_io_num, config->cts_io_num);
    } else {
        res = uart_set_pin(esp_dte->uart_port, config->tx_io_num, config->rx_io_num,
                           UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }
    MODEM_CHECK(res == ESP_OK, "config uart gpio failed", err_uart_config);
    /* Set flow control threshold */
    if (config->flow_control == MODEM_FLOW_CONTROL_HW) {
        res = uart_set_hw_flow_ctrl(esp_dte->uart_port, UART_HW_FLOWCTRL_CTS_RTS, UART_FIFO_LEN - 8);
    } else if (config->flow_control == MODEM_FLOW_CONTROL_SW) {
        res = uart_set_sw_flow_ctrl(esp_dte->uart_port, true, 8, UART_FIFO_LEN - 8);
    }
    MODEM_CHECK(res == ESP_OK, "config uart flow control failed", err_uart_config);
    /* Install UART driver and get event queue used inside driver */
    res = uart_driver_install(esp_dte->uart_port, config->rx_buffer_size, config->tx_buffer_size,
                              config->event_queue_size, &(esp_dte->event_queue), 0);
    MODEM_CHECK(res == ESP_OK, "install uart driver failed", err_uart_config);
    res = uart_set_rx_timeout(esp_dte->uart_port, 1);
    MODEM_CHECK(res == ESP_OK, "set rx timeout failed", err_uart_config);

    /* Set pattern interrupt, used to detect the end of a line. */
    res = uart_enable_pattern_det_baud_intr(esp_dte->uart_port, '\n', 1, MIN_PATTERN_INTERVAL, MIN_POST_IDLE, MIN_PRE_IDLE);
    /* Set pattern queue size */
    esp_dte->pattern_queue_size = config->pattern_queue_size;
    res |= uart_pattern_queue_reset(esp_dte->uart_port, config->pattern_queue_size);
    /* Starting in command mode -> explicitly disable RX interrupt */
    uart_disable_rx_intr(esp_dte->uart_port);

    MODEM_CHECK(res == ESP_OK, "config uart pattern failed", err_uart_pattern);
    /* Create Event loop */
    esp_event_loop_args_t loop_args = {
            .queue_size = ESP_MODEM_EVENT_QUEUE_SIZE,
            .task_name = NULL
    };
    MODEM_CHECK(esp_event_loop_create(&loop_args, &esp_dte->event_loop_hdl) == ESP_OK, "create event loop failed", err_eloop);
    /* Create semaphore */
    esp_dte->process_sem = xSemaphoreCreateBinary();
    MODEM_CHECK(esp_dte->process_sem, "create process semaphore failed", err_sem);
    /* Create UART Event task */
    BaseType_t ret = xTaskCreate(uart_event_task_entry,             //Task Entry
                                 "uart_event",              //Task Name
                                 config->event_task_stack_size,           //Task Stack Size(Bytes)
                                 esp_dte,                           //Task Parameter
                                 config->event_task_priority,             //Task Priority
                                 & (esp_dte->uart_event_task_hdl)   //Task Handler
    );
    MODEM_CHECK(ret == pdTRUE, "create uart event task failed", err_tsk_create);
    return &(esp_dte->parent);
    /* Error handling */
    err_tsk_create:
    vSemaphoreDelete(esp_dte->process_sem);
    err_sem:
    esp_event_loop_delete(esp_dte->event_loop_hdl);
    err_eloop:
    uart_disable_pattern_det_intr(esp_dte->uart_port);
    err_uart_pattern:
    uart_driver_delete(esp_dte->uart_port);
    err_uart_config:
    free(esp_dte->buffer);
    err_line_mem:
    free(esp_dte);
    err_dte_mem:
    return NULL;
}
