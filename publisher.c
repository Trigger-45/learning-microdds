// publisher.c - Publisher für HelloWorld mit JSON-Daten

#include "HelloWorld.h"
#include <uxr/client/client.h>
#include <ucdr/microcdr.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define STREAM_HISTORY  8
#define BUFFER_SIZE     UXR_CONFIG_UDP_TRANSPORT_MTU * STREAM_HISTORY

// Funktion: Generiert JSON und sendet als HelloWorld
void publish_json_data(
    uxrSession* session, 
    uxrStreamId reliable_out, 
    uxrObjectId datawriter_id)
{
    // === Deine Variablen (die du schon hast) ===
    uint32_t index = rand() % 1000;
    float temp = 20.0f + ((float)rand() / RAND_MAX) * 15.0f;
    char* einheit = "Celsius";
    
    // Datum/Zeit
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char date[50];
    snprintf(date, sizeof(date), "%04d-%02d-%02d %02d:%02d:%02d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
    
    // Zusätzliche Daten
    float humidity = 40.0f + ((float)rand() / RAND_MAX) * 30.0f;
    int pressure = 990 + rand() % 50;
    char daten[255];
    snprintf(daten, sizeof(daten), 
             "{\"humidity\": %.1f, \"pressure\": %d}", 
             humidity, pressure);

    // === JSON-String erstellen ===
    HelloWorld topic;
    topic.index = index;
    
    snprintf(topic.message, sizeof(topic.message),
             "{"
             "\"index\": %d, "
             "\"temp\": %.2f, "
             "\"einheit\": \"%s\", "
             "\"date\": \"%s\", "
             "\"daten\": \"%s\""
             "}",
             index, temp, einheit, date, daten);

    printf("\n[Publisher] Sende HelloWorld:\n");
    printf("  Index: %d\n", topic.index);
    printf("  JSON: %s\n", topic.message);

    // Serialisieren und senden
    ucdrBuffer ub;
    uint32_t topic_size = HelloWorld_size_of_topic(&topic, 0);
    uxr_prepare_output_stream(session, reliable_out, datawriter_id, &ub, topic_size);
    HelloWorld_serialize_topic(&ub, &topic);

    uxr_run_session_time(session, 1000);
    
    printf("  ✓ Gesendet\n");
}

int main(int argc, char** argv)
{
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
        printf("Standard: %s:%s\n", ip, port);
    }

    srand((unsigned int)time(NULL));

    // UDP Transport
    uxrUDPTransport transport;
    if (!uxr_init_udp_transport(&transport, UXR_IPv4, ip, port))
    {
        printf("Fehler beim Erstellen des UDP Transports.\n");
        return 1;
    }

    // Session
    uxrSession session;
    uxr_init_session(&session, &transport.comm, 0xAAAABBBB);

    if (!uxr_create_session(&session))
    {
        printf("Fehler beim Erstellen der Session.\n");
        return 1;
    }

    // Streams
    uint8_t output_reliable_stream_buffer[BUFFER_SIZE];
    uxrStreamId reliable_out = uxr_create_output_reliable_stream(
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
        "      <name>HelloWorldPublisher</name>"
        "    </rtps>"
        "  </participant>"
        "</dds>";
    uint16_t participant_req = uxr_buffer_create_participant_xml(
        &session, reliable_out, participant_id, 0, participant_xml, UXR_REPLACE);

    // Topic
    uxrObjectId topic_id = uxr_object_id(0x01, UXR_TOPIC_ID);
    const char* topic_xml = 
        "<dds>"
        "  <topic>"
        "    <name>HelloWorldTopic</name>"
        "    <dataType>HelloWorld</dataType>"
        "  </topic>"
        "</dds>";
    uint16_t topic_req = uxr_buffer_create_topic_xml(
        &session, reliable_out, topic_id, participant_id, topic_xml, UXR_REPLACE);

    // Publisher
    uxrObjectId publisher_id = uxr_object_id(0x01, UXR_PUBLISHER_ID);
    const char* publisher_xml = "";
    uint16_t publisher_req = uxr_buffer_create_publisher_xml(
        &session, reliable_out, publisher_id, participant_id, publisher_xml, UXR_REPLACE);

    // DataWriter
    uxrObjectId datawriter_id = uxr_object_id(0x01, UXR_DATAWRITER_ID);
    const char* datawriter_xml = 
        "<dds>"
        "  <data_writer>"
        "    <topic>"
        "      <kind>NO_KEY</kind>"
        "      <name>HelloWorldTopic</name>"
        "      <dataType>HelloWorld</dataType>"
        "    </topic>"
        "  </data_writer>"
        "</dds>";
    uint16_t datawriter_req = uxr_buffer_create_datawriter_xml(
        &session, reliable_out, datawriter_id, publisher_id, datawriter_xml, UXR_REPLACE);

    // Status prüfen
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

    printf("Publisher initialisiert.\n");
    printf("Sende HelloWorld mit JSON-Daten...\n\n");

    // Haupt-Schleife
    while (true)
    {
        publish_json_data(&session, reliable_out, datawriter_id);
        sleep(2);
    }

    uxr_delete_session(&session);
    uxr_close_udp_transport(&transport);

    return 0;
}
