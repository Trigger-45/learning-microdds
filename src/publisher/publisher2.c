// publisher2.c - Publisher mit Message Queue (einfache Version ohne Threads)

#include "HelloWorld.h"
#include <uxr/client/client.h>
#include <ucdr/microcdr.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

#define STREAM_HISTORY  8
#define BUFFER_SIZE     UXR_CONFIG_UDP_TRANSPORT_MTU * STREAM_HISTORY
#define QUEUE_SIZE      100
#define MESSAGE_LENGTH  256

// Message Queue
typedef struct {
    char messages[QUEUE_SIZE][MESSAGE_LENGTH];
    int head;
    int tail;
    int count;
} MessageQueue;

MessageQueue queue = {
    .head = 0,
    .tail = 0,
    .count = 0
};

// Globale DDS Variablen
uxrSession* global_session = NULL;
uxrStreamId global_reliable_out;
uxrObjectId global_datawriter_id;

// ============================================================
// QUEUE FUNKTIONEN
// ============================================================

// Fügt Nachricht in die Queue
void add_to_queue(const char* json_string)
{
    if (queue.count >= QUEUE_SIZE)
    {
        printf("[Queue] FEHLER: Queue ist voll! Nachricht verworfen.\n");
        return;
    }

    strncpy(queue.messages[queue.tail], json_string, MESSAGE_LENGTH - 1);
    queue.messages[queue.tail][MESSAGE_LENGTH - 1] = '\0';

    queue.tail = (queue.tail + 1) % QUEUE_SIZE;
    queue.count++;

    printf("[Queue] Nachricht hinzugefügt (Queue: %d/%d)\n", queue.count, QUEUE_SIZE);
}

// Holt nächste Nachricht aus Queue
bool get_from_queue(char* json_string)
{
    if (queue.count == 0)
    {
        return false;
    }

    strncpy(json_string, queue.messages[queue.head], MESSAGE_LENGTH - 1);
    json_string[MESSAGE_LENGTH - 1] = '\0';

    queue.head = (queue.head + 1) % QUEUE_SIZE;
    queue.count--;

    return true;
}

// ============================================================
// JSON GENERIERUNG
// ============================================================

// Generiert JSON-String und speichert in Queue
void generate_json(void)
{
    char json_string[MESSAGE_LENGTH];
    
    int id = rand() % 1000;
    double value = ((double)rand() / RAND_MAX) * 100.0;
    int flag = rand() % 2;

    // JSON formatieren
    snprintf(json_string, sizeof(json_string),
             "{ \"id\": %d, \"value\": %.2f, \"flag\": %s }",
             id, value, flag ? "true" : "false");

    printf("[Generate] JSON erstellt: %s\n", json_string);

    // In Queue speichern
    add_to_queue(json_string);
}

// ============================================================
// DDS PUBLISHING
// ============================================================

// Publiziert eine Nachricht über DDS
void publish_json(const char* json_string)
{
    HelloWorld topic;
    topic.index = rand() % 1000;
    snprintf(topic.message, sizeof(topic.message), "%s", json_string);

    // Serialisieren
    ucdrBuffer ub;
    uint32_t topic_size = HelloWorld_size_of_topic(&topic, 0);
    uxr_prepare_output_stream(global_session, global_reliable_out, global_datawriter_id, &ub, topic_size);
    HelloWorld_serialize_topic(&ub, &topic);

    printf("[Publish] Gesendet: %s\n", json_string);

    // ============================================================
    // Session ausführen - OPTIONEN:
    // ============================================================
    
    // OPTION 1: Mit festem Timeout (Standard, blockiert 100ms)
    // Wartet auf Bestätigungen, garantiert Zustellung
    uxr_run_session_time(global_session, 100);
    
    // OPTION 2: Nur Output-Streams flushen (schnell, keine Garantie)
    // Sendet sofort ohne auf Antwort zu warten
    // uxr_flash_output_streams(global_session);
    
    // OPTION 3: Bis Timeout mit Verarbeitung (flexibler)
    // Verarbeitet Ein-/Ausgehende Nachrichten bis Timeout
    // uxr_run_session_until_timeout(global_session, 100);
    
    // OPTION 4: Nur Stream flushen + kurzes Run (Hybrid)
    // Schneller aber mit minimaler Bestätigung
    // uxr_flash_output_streams(global_session);
    // uxr_run_session_time(global_session, 10);
    
    // OPTION 5: Gar nichts (Session läuft in Main-Loop)
    // Batch-Processing: Alle Messages erst sammeln, dann einmal in Main senden
    // (Siehe Main-Loop für batch_send Beispiel)
}

// ============================================================
// MAIN FUNKTION
// ============================================================

int main(int argc, char** argv)
{
    char* ip = "127.0.0.1";
    char* port = "8888";

    if (argc >= 3)
    {
        ip = argv[1];
        port = argv[2];
    }

    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║     Publisher2 mit Message Queue                           ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("DDS: %s:%s\n\n", ip, port);

    srand((unsigned int)time(NULL));

    // ============================================================
    // DDS INITIALISIERUNG
    // ============================================================

    // UDP Transport
    uxrUDPTransport transport;
    if (!uxr_init_udp_transport(&transport, UXR_IPv4, ip, port))
    {
        printf("Fehler: UDP Transport konnte nicht initialisiert werden.\n");
        return 1;
    }

    // Session
    uxrSession session;
    uxr_init_session(&session, &transport.comm, 0xAAAABBBB);
    if (!uxr_create_session(&session))
    {
        printf("Fehler: Session konnte nicht erstellt werden.\n");
        return 1;
    }

    global_session = &session;

    // Streams
    uint8_t output_reliable_stream_buffer[BUFFER_SIZE];
    global_reliable_out = uxr_create_output_reliable_stream(
        &session, output_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);

    uint8_t input_reliable_stream_buffer[BUFFER_SIZE];
    uxr_create_input_reliable_stream(
        &session, input_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);

    // Participant
    uxrObjectId participant_id = uxr_object_id(0x01, UXR_PARTICIPANT_ID);
    const char* participant_xml = 
        "<dds>"
        "  <participant>"
        "    <rtps>"
        "      <name>JsonPublisher</name>"
        "    </rtps>"
        "  </participant>"
        "</dds>";
    uint16_t participant_req = uxr_buffer_create_participant_xml(
        &session, global_reliable_out, participant_id, 0, participant_xml, UXR_REPLACE);

    // Topic
    uxrObjectId topic_id = uxr_object_id(0x01, UXR_TOPIC_ID);
    const char* topic_xml = 
        "<dds>"
        "  <topic>"
        "    <name>JsonTopic</name>"
        "    <dataType>HelloWorld</dataType>"
        "  </topic>"
        "</dds>";
    uint16_t topic_req = uxr_buffer_create_topic_xml(
        &session, global_reliable_out, topic_id, participant_id, topic_xml, UXR_REPLACE);

    // Publisher
    uxrObjectId publisher_id = uxr_object_id(0x01, UXR_PUBLISHER_ID);
    const char* publisher_xml = "";
    uint16_t publisher_req = uxr_buffer_create_publisher_xml(
        &session, global_reliable_out, publisher_id, participant_id, publisher_xml, UXR_REPLACE);

    // DataWriter
    global_datawriter_id = uxr_object_id(0x01, UXR_DATAWRITER_ID);
    const char* datawriter_xml = 
        "<dds>"
        "  <data_writer>"
        "    <topic>"
        "      <kind>NO_KEY</kind>"
        "      <name>JsonTopic</name>"
        "      <dataType>HelloWorld</dataType>"
        "    </topic>"
        "  </data_writer>"
        "</dds>";
    uint16_t datawriter_req = uxr_buffer_create_datawriter_xml(
        &session, global_reliable_out, global_datawriter_id, publisher_id, datawriter_xml, UXR_REPLACE);

    // Warte auf Responses
    uint8_t status[4];
    uint16_t requests[4] = {participant_req, topic_req, publisher_req, datawriter_req};
    
    if (!uxr_run_session_until_all_status(&session, 1000, requests, status, 4))
    {
        printf("Fehler beim Erstellen der DDS Entities:\n");
        printf("  Participant: %i\n", status[0]);
        printf("  Topic: %i\n", status[1]);
        printf("  Publisher: %i\n", status[2]);
        printf("  DataWriter: %i\n", status[3]);
        return 1;
    }

    printf("DDS initialisiert.\n");
    printf("Queue läuft mit Größe: %d\n\n", QUEUE_SIZE);
    printf("Starte Haupt-Schleife...\n");
    printf("(Drücke CTRL+C zum Beenden)\n");
    printf("════════════════════════════════════════════════════════════\n\n");

    // ============================================================
    // HAUPT-SCHLEIFE
    // ============================================================

    while (1)
    {
        // 1. JSON generieren und in Queue speichern
        generate_json();

        // 2. Alle Nachrichten aus Queue publishen
        char json_message[MESSAGE_LENGTH];
        while (get_from_queue(json_message))
        {
            publish_json(json_message);
            usleep(100000);  // 100ms Pause zwischen Nachrichten
        }
        
        // ALTERNATIVE für OPTION 5 (Batch-Send):
        // Wenn publish_json() KEINE Session-Run Funktion aufruft,
        // kannst du hier einmal pro Batch die Session ausführen:
        // uxr_run_session_time(global_session, 100);

        sleep(1);  // 1 Sekunde Pause vor nächster JSON-Generierung
    }

    // Aufräumen
    uxr_delete_session(&session);
    uxr_close_udp_transport(&transport);

    return 0;
}
