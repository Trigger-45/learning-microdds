// SensorData.c - Implementierung der Serialisierungsfunktionen
// Diese Funktionen serialisieren/deserialisieren die Datenstruktur
// CDR-kompatibel für FastDDS Interoperabilität

#include "SensorData.h"
#include <ucdr/microcdr.h>
#include <string.h>

// Serialisiert SensorData-Struktur in einen Buffer (CDR-Format)
bool SensorData_serialize_topic(
        ucdrBuffer* writer,
        const SensorData* topic)
{
    // Serialisiere index (uint32_t)
    (void) ucdr_serialize_uint32_t(writer, topic->index);
    
    // Serialisiere temp (float)
    (void) ucdr_serialize_float(writer, topic->temp);
    
    // Serialisiere einheit (string)
    (void) ucdr_serialize_string(writer, topic->einheit);
    
    // Serialisiere date (string)
    (void) ucdr_serialize_string(writer, topic->date);
    
    // Serialisiere daten (string)
    (void) ucdr_serialize_string(writer, topic->daten);

    return !writer->error;
}

// Deserialisiert SensorData-Struktur aus einem Buffer
bool SensorData_deserialize_topic(
        ucdrBuffer* reader,
        SensorData* topic)
{
    // Deserialisiere index (uint32_t)
    (void) ucdr_deserialize_uint32_t(reader, &topic->index);
    
    // Deserialisiere temp (float)
    (void) ucdr_deserialize_float(reader, &topic->temp);
    
    // Deserialisiere einheit (string, max 50 bytes)
    (void) ucdr_deserialize_string(reader, topic->einheit, 50);
    
    // Deserialisiere date (string, max 50 bytes)
    (void) ucdr_deserialize_string(reader, topic->date, 50);
    
    // Deserialisiere daten (string, max 255 bytes)
    (void) ucdr_deserialize_string(reader, topic->daten, 255);

    return !reader->error;
}

// Berechnet die Größe einer serialisierten SensorData-Struktur
uint32_t SensorData_size_of_topic(
        const SensorData* topic,
        uint32_t size)
{
    uint32_t previousSize = size;
    
    // Größe für uint32_t (mit 4-Byte-Alignment)
    size += (uint32_t)(ucdr_alignment(size, 4) + 4);
    
    // Größe für float (mit 4-Byte-Alignment)
    size += (uint32_t)(ucdr_alignment(size, 4) + 4);
    
    // Größe für einheit (Length prefix + String + Null-Terminator)
    size += (uint32_t)(ucdr_alignment(size, 4) + 4 + strlen(topic->einheit) + 1);
    
    // Größe für date (Length prefix + String + Null-Terminator)
    size += (uint32_t)(ucdr_alignment(size, 4) + 4 + strlen(topic->date) + 1);
    
    // Größe für daten (Length prefix + String + Null-Terminator)
    size += (uint32_t)(ucdr_alignment(size, 4) + 4 + strlen(topic->daten) + 1);

    return size - previousSize;
}
