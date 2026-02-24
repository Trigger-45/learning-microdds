// publisher.c - Publisher für kontinuierliche Random JSON Daten über Micro-XRCE-DDS

#include "HelloWorld.h"
#include <uxr/client/client.h>
#include <ucdr/microcdr.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>  // für sleep()
#include <time.h>
#include <stdbool.h>

#define STREAM_HISTORY  8
#define BUFFER_SIZE     UXR_CONFIG_UDP_TRANSPORT_MTU * STREAM_HISTORY

// Funktion, die eine Nachricht über XRCE-DDS publiziert
void publish_message(uxrSession* session, uxrStreamId reliable_out, uxrObjectId datawriter_id, const char* message)
{
    HelloWorld topic;
    topic.index = rand() % 1000;  // zufällige ID
    snprintf(topic.message, sizeof(topic.message), "%s", message);

    // Nachricht serialisieren und senden
    ucdrBuffer ub;
    uint32_t topic_size = HelloWorld_size_of_topic(&topic, 0);
    uxr_prepare_output_stream(session, reliable_out, datawriter_id, &ub, topic_size);
    HelloWorld_serialize_topic(&ub, &topic);

    printf("[Publisher] Gesendet: %s\n", message);

    // Session ausführen
    uxr_run_session_time(session, 100);
}

// Funktion, die kontinuierlich zufällige JSON-Nachrichten generiert und sendet
void generate_random_json(uxrSession* session, uxrStreamId reliable_out, uxrObjectId datawriter_id)
{
    char buffer[256];

    while (true)  // Endlosschleife
    {
        int id = rand() % 1000;
        double value = ((double)rand() / RAND_MAX) * 100.0;
        int flag = rand() % 2;

        snprintf(buffer, sizeof(buffer),
                 "{ \"id\": %d, \"value\": %.2f, \"flag\": %s }",
                 id, value, flag ? "true" : "false");

        // Nachricht senden
        publish_message(session, reliable_out, datawriter_id, buffer);

        sleep(1);  // kurze Pause zwischen den Nachrichten
    }
}

int main(int argc, char** argv)
{
    // Standardwerte
    char* ip = "127.0.0.1";
    char* port = "8888";

    if (argc >= 3)
    {
        ip = argv[1];
        port = argv[2];
    }
    else
    {
        printf("Verwendung: %s [ip] [port]\n", argv[0]);
        printf("Verwende Standardwerte: %s:%s\n", ip, port);
    }

    srand((unsigned int)time(NULL)); // Zufall initialisieren

    // UDP Transport initialisieren
    uxrUDPTransport transport;
    if (!uxr_init_udp_transport(&transport, UXR_IPv4, ip, port))
    {
        printf("Fehler beim Erstellen des UDP Transports.\n");
        return 1;
    }

    // Session initialisieren
    uxrSession session;
    uxr_init_session(&session, &transport.comm, 0xAAAABBBB);
    if (!uxr_create_session(&session))
    {
        printf("Fehler beim Erstellen der Session.\n");
        return 1;
    }

    // Output/Input Streams erstellen
    uint8_t output_reliable_stream_buffer[BUFFER_SIZE];
    uxrStreamId reliable_out = uxr_create_output_reliable_stream(
        &session, output_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);

    uint8_t input_reliable_stream_buffer[BUFFER_SIZE];
    uxr_create_input_reliable_stream(
        &session, input_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);

    // Participant erstellen
    uxrObjectId participant_id = uxr_object_id(0x01, UXR_PARTICIPANT_ID);
    const char* participant_xml = 
        "<dds>"
        "  <participant>"
        "    <rtps>"
        "      <name>RandomJSON_Publisher</name>"
        "    </rtps>"
        "  </participant>"
        "</dds>";
    uint16_t participant_req = uxr_buffer_create_participant_xml(
        &session, reliable_out, participant_id, 0, participant_xml, UXR_REPLACE);

    // Topic erstellen
    uxrObjectId topic_id = uxr_object_id(0x01, UXR_TOPIC_ID);
    const char* topic_xml = 
        "<dds>"
        "  <topic>"
        "    <name>RandomJSONTopic</name>"
        "    <dataType>HelloWorld</dataType>"
        "  </topic>"
        "</dds>";
    uint16_t topic_req = uxr_buffer_create_topic_xml(
        &session, reliable_out, topic_id, participant_id, topic_xml, UXR_REPLACE);

    // Publisher erstellen
    uxrObjectId publisher_id = uxr_object_id(0x01, UXR_PUBLISHER_ID);
    const char* publisher_xml = "";
    uint16_t publisher_req = uxr_buffer_create_publisher_xml(
        &session, reliable_out, publisher_id, participant_id, publisher_xml, UXR_REPLACE);

    // DataWriter erstellen
    uxrObjectId datawriter_id = uxr_object_id(0x01, UXR_DATAWRITER_ID);
    const char* datawriter_xml = 
        "<dds>"
        "  <data_writer>"
        "    <topic>"
        "      <kind>NO_KEY</kind>"
        "      <name>RandomJSONTopic</name>"
        "      <dataType>HelloWorld</dataType>"
        "    </topic>"
        "  </data_writer>"
        "</dds>";
    uint16_t datawriter_req = uxr_buffer_create_datawriter_xml(
        &session, reliable_out, datawriter_id, publisher_id, datawriter_xml, UXR_REPLACE);

    // Auf Status-Antworten warten
    uint8_t status[4];
    uint16_t requests[4] = {participant_req, topic_req, publisher_req, datawriter_req};
    
    if (!uxr_run_session_until_all_status(&session, 1000, requests, status, 4))
    {
        printf("Fehler beim Erstellen der Entities:\n");
        printf("  Participant: %i\n", status[0]);
        printf("  Topic: %i\n", status[1]);
        printf("  Publisher: %i\n", status[2]);
        printf("  DataWriter: %i\n", status[3]);
        return 1;
    }

    printf("Publisher initialisiert. Starte kontinuierliches Senden von JSON-Daten...\n");

    // Zufällige JSON-Nachrichten kontinuierlich senden
    generate_random_json(&session, reliable_out, datawriter_id);

    // Aufräumen (dieser Punkt wird nie erreicht, da generate_random_json endlos läuft)
    uxr_delete_session(&session);
    uxr_close_udp_transport(&transport);

    return 0;
}
