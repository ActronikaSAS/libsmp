/* libsmp
 * Copyright (C) 2020 Actronika SAS
 *     Author: Hugo Bouchard <hugo.bouchard@actronika.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/********************************************************************/
/* Includes                                                         */
/********************************************************************/
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "config.h"
#include "serial-device.h"
#include "libsmp-private.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_spp_api.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"

/********************************************************************/
/* Defines                                                          */
/********************************************************************/
/* Bluetooth */
#define SPP_SERVER_NAME			"SPP_SERVER"
#define BT_DEVICE_NAME			"UNITOUCH_PLAYER"

/* Tag for logging */
#define TAG						"BT spp"

/********************************************************************/
/* Local types definition                                           */
/********************************************************************/
typedef struct
{
    uint8_t cbuf[SMP_SERIAL_DEVICE_RX_BUFFER_SIZE];
    uint16_t rindex;
    uint16_t windex;

} rx_buffer_t;

/********************************************************************/
/* Local variables                                                  */
/********************************************************************/
/* Reception buffer for all serial devices */
rx_buffer_t g_rx_buffers;

/* Rx event semaphore to wake up waiting task */
xSemaphoreHandle g_rx_event_semaphores;

/* The connection handle */
int32_t g_handle;

/********************************************************************/
/* Private functions prototype                                      */
/********************************************************************/
static void init_bluetooth(void);
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
static void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

/********************************************************************/
/* Private functions                                                */
/********************************************************************/
static void init_bluetooth(void)
{
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    ret = esp_bt_controller_init(&bt_cfg);
    if (ESP_OK != ret) {
        ESP_LOGE(TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    if (ESP_OK != ret) {
        ESP_LOGE(TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ESP_OK != ret) {
        ESP_LOGE(TAG, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ESP_OK != ret) {
        ESP_LOGE(TAG, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_gap_register_callback(esp_bt_gap_cb);
	if (ESP_OK != ret) {
        ESP_LOGE(TAG, "%s gap register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

	ret = esp_spp_register_callback(esp_spp_cb);
    if (ESP_OK != ret) {
        ESP_LOGE(TAG, "%s spp register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_spp_init(ESP_SPP_MODE_CB);
    if (ESP_OK != ret) {
        ESP_LOGE(TAG, "%s spp init failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

#if (CONFIG_BT_SSP_ENABLED == true)
    /* Set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
#endif

    /*
     * Set default parameters for Legacy Pairing
     * Use variable pin, input pin code when pairing
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);
}

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	uint16_t i;

    switch (event) {
		case ESP_SPP_INIT_EVT:
			esp_bt_dev_set_device_name(BT_DEVICE_NAME);
			esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
			esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, SPP_SERVER_NAME);
			break;

		case ESP_SPP_DISCOVERY_COMP_EVT:
			break;

		case ESP_SPP_OPEN_EVT:
			break;

		case ESP_SPP_CLOSE_EVT:
			//g_handle = -1;
			break;

		case ESP_SPP_START_EVT:
			break;

		case ESP_SPP_CL_INIT_EVT:
			break;

		case ESP_SPP_DATA_IND_EVT:
			/* Save handle of device sending the data */
			g_handle = param->data_ind.handle;

			for (i = 0U; i < param->data_ind.len; i++) {
				/* If no more space to store data */
//				if (g_rx_buffers.rindex == g_rx_buffers.windex)
//					break;

				/* Store rx byte into buffer */
				g_rx_buffers.cbuf[g_rx_buffers.windex] = param->data_ind.data[i];

				/* Increment read index and roll over if at the end (circular buffer) */
				g_rx_buffers.windex++;
				if (g_rx_buffers.windex >= SMP_SERIAL_DEVICE_RX_BUFFER_SIZE)
					g_rx_buffers.windex = 0;
			}

            /* Wakeup Rx task */
            xSemaphoreGiveFromISR(g_rx_event_semaphores, &xHigherPriorityTaskWoken);
			break;

		case ESP_SPP_CONG_EVT:
			break;

		case ESP_SPP_WRITE_EVT:
			break;

		case ESP_SPP_SRV_OPEN_EVT:
			break;

		default:
			break;
    }

    /* If xHigherPriorityTaskWoken was set to true we should yield */
    if(pdTRUE == xHigherPriorityTaskWoken)
		portYIELD_FROM_ISR();
}

static void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
		case ESP_BT_GAP_AUTH_CMPL_EVT:
			if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
				ESP_LOGI(TAG, "authentication success: %s", param->auth_cmpl.device_name);
			}
			else {
				ESP_LOGE(TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
			}
			break;

		case ESP_BT_GAP_PIN_REQ_EVT:
			ESP_LOGI(TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
			if (param->pin_req.min_16_digit) {
				ESP_LOGI(TAG, "Input pin code: 0000 0000 0000 0000");
				esp_bt_pin_code_t pin_code = {0};
				esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
			}
			else {
				ESP_LOGI(TAG, "Input pin code: 1234");
				esp_bt_pin_code_t pin_code;
				pin_code[0] = '1';
				pin_code[1] = '2';
				pin_code[2] = '3';
				pin_code[3] = '4';
				esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
			}
			break;

#if (CONFIG_BT_SSP_ENABLED == true)
		case ESP_BT_GAP_CFM_REQ_EVT:
			ESP_LOGI(TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
			esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
			break;

		case ESP_BT_GAP_KEY_NOTIF_EVT:
			ESP_LOGI(TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
			break;

		case ESP_BT_GAP_KEY_REQ_EVT:
			ESP_LOGI(TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
			break;
#endif

		default:
			ESP_LOGI(TAG, "event: %d", event);
			break;
    }

    return;
}

/********************************************************************/
/* Public functions                                                */
/********************************************************************/
void smp_serial_device_init(SmpSerialDevice *device)
{
    device->fd = -1;
    g_handle = -1;
}

int smp_serial_device_open(SmpSerialDevice *device, const char *path)
{
    /* Create Rx event semaphore to wake up waiting task (must be created before corresponding Bluetooth is enabled) */
    vSemaphoreCreateBinary(g_rx_event_semaphores);

    /* Initialize Bluetooth driver */
    init_bluetooth();

    device->fd = 1;

    return device->fd;
}

void smp_serial_device_close(SmpSerialDevice *device)
{
	esp_spp_deinit();
	esp_bt_controller_deinit();
    device->fd = -1;
    g_handle = -1;
}

intptr_t smp_serial_device_get_fd(SmpSerialDevice *device)
{
    return (device->fd < 0) ? SMP_ERROR_BAD_FD : device->fd;
}

int smp_serial_device_set_config(SmpSerialDevice *device, SmpSerialBaudrate baudrate, SmpSerialParity parity, int flow_control)
{
    return SMP_ERROR_NOT_SUPPORTED;
}

ssize_t smp_serial_device_write(SmpSerialDevice *device, const void *buf, size_t size)
{
	esp_err_t ret = -1;

	if (0 < g_handle) {
		ret = esp_spp_write(g_handle, size, (uint8_t *)buf);
		if (ESP_OK == ret)
			ret = size;
	}

    return ret;
}

ssize_t smp_serial_device_read(SmpSerialDevice *device, void *buf, size_t size)
{
    ssize_t i;

	for (i = 0; i < size; i++) {
		/* If no more data to read */
		if (g_rx_buffers.rindex == g_rx_buffers.windex)
			break;

		/* Store rx byte into buffer */
		((uint8_t *)buf)[i] = g_rx_buffers.cbuf[g_rx_buffers.rindex];

		/* Increment read index and roll over if at the end (circular buffer) */
		g_rx_buffers.rindex++;
		if (g_rx_buffers.rindex >= SMP_SERIAL_DEVICE_RX_BUFFER_SIZE)
			g_rx_buffers.rindex = 0;
	}

    return i;
}

int smp_serial_device_wait(SmpSerialDevice *device, int timeout_ms)
{
    portTickType timeout;
    signed portBASE_TYPE ret;

	/* Convert timeout from ms to OS ticks */
	if(0 > timeout_ms)
		timeout = portMAX_DELAY;
	else
		timeout = (portTickType)timeout_ms / portTICK_RATE_MS;

	ret = xSemaphoreTake(g_rx_event_semaphores, timeout);
	if (errQUEUE_EMPTY == ret)
		return SMP_ERROR_TIMEDOUT;

    return 0;
}
