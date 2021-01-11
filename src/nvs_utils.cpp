#include "nvs_utils.h"
#include "nvs_flash.h"
#include "nvs.h"

typedef uint32_t nvs_handle_t;

bool initNVS()
{
    Serial.println("initNVS");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK)
    {
        Serial.println("NVS error: ");
        Serial.println(err);
        return false;
    }
    else
    {
        Serial.println("NVS OK");
        return true;
    }
}

bool tryReadSSIDPW(char *ssid, char *pwd)
{
    Serial.println("tryReadSSIDPW");
    nvs_handle_t storage;
    esp_err_t err = nvs_open("store", NVS_READWRITE, &storage);
    if (err != ESP_OK)
    {
        Serial.print("nvs_open: ");
        Serial.println(esp_err_to_name(err));
        return false;
    }
    else
    {
        // Read ssid
        size_t length = MAX_SSID_LENGTH;
        err = nvs_get_str(storage, "ssid", ssid, &length);
        switch (err)
        {
        case ESP_OK:
            Serial.print("SSID was: ");
            Serial.println(ssid);
            break;
        default:
            Serial.print("nvs_get_str(ssid): ");
            Serial.println(esp_err_to_name(err));
            nvs_close(storage);
            return false;
        }

        if (length > MAX_SSID_LENGTH)
        {
            Serial.println("Stored SSID too long");
            nvs_close(storage);
            return false;
        }

        // read pwd
        length = MAX_PWD_LENGTH;
        err = nvs_get_str(storage, "pwd", pwd, &length);
        switch (err)
        {
        case ESP_OK:
            Serial.print("pwd was: ");
            Serial.println(pwd);
            break;
        default:
            Serial.print("nvs_get_str(pwd): ");
            Serial.println(esp_err_to_name(err));
            nvs_close(storage);
            return false;
        }

        if (length > MAX_PWD_LENGTH)
        {
            Serial.println("Stored PWD too long");
            nvs_close(storage);
            return false;
        }

        return true;
    }
}

bool writeSSIDPW(const char *ssid, const char *pwd)
{
    Serial.println("writeSSIDPW");
    nvs_handle_t storage;
    esp_err_t err = nvs_open("store", NVS_READWRITE, &storage);
    if (err != ESP_OK)
    {
        Serial.print("nvs_open: ");
        Serial.println(esp_err_to_name(err));
        return false;
    }
    else
    {
        // Write SSID
        if (ssid == nullptr)
        {
            err = nvs_erase_key(storage, "ssid");
            if (err != ESP_OK)
            {
                Serial.print("nvs_erase_key(ssid): ");
                Serial.println(esp_err_to_name(err));
                nvs_close(storage);
                return false;
            }
        }
        else
        {
            err = nvs_set_str(storage, "ssid", ssid);
            if (err != ESP_OK)
            {
                Serial.print("nvs_set_str(ssid): ");
                Serial.println(esp_err_to_name(err));
                nvs_close(storage);
                return false;
            }
        }

        // write password
        if (pwd == nullptr)
        {
            err = nvs_erase_key(storage, "pwd");
            if (err != ESP_OK)
            {
                Serial.print("nvs_erase_key(pwd): ");
                Serial.println(esp_err_to_name(err));
                nvs_close(storage);
                return false;
            }
        }
        else
        {
            err = nvs_set_str(storage, "pwd", pwd);
            if (err != ESP_OK)
            {
                Serial.print("nvs_set_str(pwd): ");
                Serial.println(esp_err_to_name(err));
                nvs_close(storage);
                return false;
            }
        }

        // commit
        err = nvs_commit(storage);
        if (err != ESP_OK)
        {
            Serial.print("nvs_commit: ");
            Serial.println(esp_err_to_name(err));
            nvs_close(storage);
            return false;
        }

        nvs_close(storage);
        return true;
    }
}