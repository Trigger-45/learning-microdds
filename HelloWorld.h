// HelloWorld.h - Header-Datei für die Datenstruktur (ohne IDL-Tool)
// Definiert die Struktur und Serialisierungsfunktionen

#ifndef _HELLOWORLD_H_
#define _HELLOWORLD_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

// Datenstruktur für HelloWorld Messages
typedef struct HelloWorld
{
    uint32_t index;           // Nachrichtennummer
    char message[255];        // Text-Nachricht
} HelloWorld;

struct ucdrBuffer;

// Funktionen für Serialisierung/Deserialisierung
bool HelloWorld_serialize_topic(
        struct ucdrBuffer* writer,
        const HelloWorld* topic);

bool HelloWorld_deserialize_topic(
        struct ucdrBuffer* reader,
        HelloWorld* topic);

uint32_t HelloWorld_size_of_topic(
        const HelloWorld* topic,
        uint32_t size);

#ifdef __cplusplus
}
#endif

#endif // _HELLOWORLD_H_
