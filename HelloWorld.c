// HelloWorld.c - Implementierung der Serialisierungsfunktionen
// Diese Funktionen serialisieren/deserialisieren die Datenstruktur

#include "HelloWorld.h"
#include <ucdr/microcdr.h>
#include <string.h>

// Serialisiert HelloWorld-Struktur in einen Buffer
bool HelloWorld_serialize_topic(
        ucdrBuffer* writer,
        const HelloWorld* topic)
{
    // Serialisiere index (uint32_t)
    (void) ucdr_serialize_uint32_t(writer, topic->index);
    
    // Serialisiere message (string)
    (void) ucdr_serialize_string(writer, topic->message);

    return !writer->error;
}

// Deserialisiert HelloWorld-Struktur aus einem Buffer
bool HelloWorld_deserialize_topic(
        ucdrBuffer* reader,
        HelloWorld* topic)
{
    // Deserialisiere index (uint32_t)
    (void) ucdr_deserialize_uint32_t(reader, &topic->index);
    
    // Deserialisiere message (string, max 255 bytes)
    (void) ucdr_deserialize_string(reader, topic->message, 255);

    return !reader->error;
}

// Berechnet die Größe einer serialisierten HelloWorld-Struktur
uint32_t HelloWorld_size_of_topic(
        const HelloWorld* topic,
        uint32_t size)
{
    uint32_t previousSize = size;
    
    // Größe für uint32_t (mit Alignment)
    size += (uint32_t)(ucdr_alignment(size, 4) + 4);
    
    // Größe für string (Length prefix + String + Null-Terminator)
    size += (uint32_t)(ucdr_alignment(size, 4) + 4 + strlen(topic->message) + 1);

    return size - previousSize;
}
