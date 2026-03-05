// SensorData.h - Header-Datei für die Datenstruktur (ohne IDL-Tool)
// Definiert die Struktur und Serialisierungsfunktionen

#ifndef _SENSORDATA_H_
#define _SENSORDATA_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

// Datenstruktur für Sensordaten
typedef struct SensorData
{
    uint32_t index;           // ID / Index
    float temp;               // Temperaturwert
    char einheit[50];         // Einheit (z.B. "Celsius")
    char date[50];            // Datum/Zeit
    char daten[255];          // Zusätzliche Daten (z.B. JSON)
} SensorData;

struct ucdrBuffer;

// Funktionen für Serialisierung/Deserialisierung
bool SensorData_serialize_topic(
        struct ucdrBuffer* writer,
        const SensorData* topic);

bool SensorData_deserialize_topic(
        struct ucdrBuffer* reader,
        SensorData* topic);

uint32_t SensorData_size_of_topic(
        const SensorData* topic,
        uint32_t size);

#ifdef __cplusplus
}
#endif

#endif // _SENSORDATA_H_
