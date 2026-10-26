#pragma once
#include <stdint.h>
#include <esp_partition.h>
#include <ArduinoLog.h>

inline const char* getPartitionTypeName(uint8_t type) {
  switch (type) {
    case ESP_PARTITION_TYPE_DATA:
      return "data";
    case ESP_PARTITION_TYPE_APP:
      return "app";

    default:
      return "unknown";
  }
}

inline const char* getPartitionSubTypeName(uint8_t type, esp_partition_subtype_t subtype) {
  if (type == ESP_PARTITION_TYPE_APP) {
    switch (subtype) {
      case ESP_PARTITION_SUBTYPE_APP_FACTORY:
        return "factory";
      case ESP_PARTITION_SUBTYPE_APP_OTA_0:
      case ESP_PARTITION_SUBTYPE_APP_OTA_1:
      case ESP_PARTITION_SUBTYPE_APP_OTA_2:
      case ESP_PARTITION_SUBTYPE_APP_OTA_3:
      case ESP_PARTITION_SUBTYPE_APP_OTA_4:
      case ESP_PARTITION_SUBTYPE_APP_OTA_5:
      case ESP_PARTITION_SUBTYPE_APP_OTA_6:
      case ESP_PARTITION_SUBTYPE_APP_OTA_7:
      case ESP_PARTITION_SUBTYPE_APP_OTA_8:
      case ESP_PARTITION_SUBTYPE_APP_OTA_9:
      case ESP_PARTITION_SUBTYPE_APP_OTA_10:
      case ESP_PARTITION_SUBTYPE_APP_OTA_11:
      case ESP_PARTITION_SUBTYPE_APP_OTA_12:
      case ESP_PARTITION_SUBTYPE_APP_OTA_13:
      case ESP_PARTITION_SUBTYPE_APP_OTA_14:
      case ESP_PARTITION_SUBTYPE_APP_OTA_15:
        return "ota0-15";
      case ESP_PARTITION_SUBTYPE_APP_TEST:
        return "test";
      default:
        break;
    }
  }

  if (type == ESP_PARTITION_TYPE_DATA) {
    switch (subtype) {
      case ESP_PARTITION_SUBTYPE_DATA_OTA:
        return "ota";
      case ESP_PARTITION_SUBTYPE_DATA_PHY:
        return "phy";
      case ESP_PARTITION_SUBTYPE_DATA_NVS:
        return "nvs";
      case ESP_PARTITION_SUBTYPE_DATA_COREDUMP:
        return "coredump";
      case ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS:
        return "nvs_keys";
      case ESP_PARTITION_SUBTYPE_DATA_EFUSE_EM:
        return "fuse";
      case ESP_PARTITION_SUBTYPE_DATA_FAT:
        return "fat";
      case ESP_PARTITION_SUBTYPE_DATA_SPIFFS:
        return "spiffs";
      default:
        break;
    }
  }

  return "unknown";
}

inline void traceEspPartitions() {
  Log.traceln("");
  Log.traceln("Parsing partitions info:");

  auto it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, nullptr);

  while(it) {
    const auto partition = esp_partition_get(it);
    Log.traceln(
        "%s: type=%s(%u)  subtype=%s(0x%x)   addr=0x%x   size=%ukB",
        partition->label,
        getPartitionTypeName(partition->type),
        partition->type,
        getPartitionSubTypeName(partition->type, partition->subtype),
        partition->subtype,
        partition->address,
        (int) partition->size / 1024
    );

    it = esp_partition_next(it);
  }

  Log.traceln("");
}
