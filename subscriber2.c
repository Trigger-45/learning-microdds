// subscriber2.c - Subscriber für SensorData
// Empfängt transformierte Daten von Subscriber1

#include "SensorData.h"
#include <uxr/client/client.h>
#include <ucdr/microcdr.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define STREAM_HISTORY  8
#define BUFFER_SIZE     UXR_CONFIG_UDP_TRANSPORT_MTU * STREAM_HISTORY

// Callback-Funktion bei empfangenen Messages
void on_topic(
        uxrSession* session,
        uxrObjectId object_id,
        uint16_t request_id,
        uxrStreamId stream_id,
        struct ucdrBuffer* ub,
        uint16_t length,
        void* args)
{
    (void) session;
    (void) object_id;
    (void) request_id;
    (void) stream_id;
    (void) length;

    // Deserialisiere empfangene Message
    SensorData topic;
    SensorData_deserialize_topic(ub, &topic);

    printf("\n[Subscriber2] Empfangen (SensorData):\n");
    printf("  Index: %d\n", topic.index);
    printf("  Temperatur: %.2f %s\n", topic.temp, topic.einheit);
    printf("  Datum: %s\n", topic.date);
    printf("  Zusatzdaten: %s\n", topic.daten);

    // Zähler erhöhen
    uint32_t* count_ptr = (uint32_t*) args;
    (*count_ptr)++;
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
        printf("Standardwerte: %s:%s\n", ip, port);
    }

    uint32_t count = 0;

    // UDP Transport initialisieren
    uxrUDPTransport transport;
    if (!uxr_init_udp_transport(&transport, UXR_IPv4, ip, port))
    {
        printf("Fehler beim Erstellen des UDP Transports.\n");
        return 1;
    }

    // Session initialisieren
    uxrSession session;
    uxr_init_session(&session, &transport.comm, 0xCCCCDDDD);
    uxr_set_topic_callback(&session, on_topic, &count);

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
    uxrStreamId reliable_in = uxr_create_input_reliable_stream(
        &session, input_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);

    // Participant erstellen
    uxrObjectId participant_id = uxr_object_id(0x01, UXR_PARTICIPANT_ID);
    const char* participant_xml =
        "<dds>"
        "  <participant>"
        "    <rtps>"
        "      <name>SensorSubscriber2</name>"
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
        "    <name>SensorDataTopic</name>"
        "    <dataType>SensorData</dataType>"
        "  </topic>"
        "</dds>";
    uint16_t topic_req = uxr_buffer_create_topic_xml(
        &session, reliable_out, topic_id, participant_id, topic_xml, UXR_REPLACE);

    // Subscriber erstellen
    uxrObjectId subscriber_id = uxr_object_id(0x01, UXR_SUBSCRIBER_ID);
    const char* subscriber_xml = "";
    uint16_t subscriber_req = uxr_buffer_create_subscriber_xml(
        &session, reliable_out, subscriber_id, participant_id, subscriber_xml, UXR_REPLACE);

    // DataReader erstellen
    uxrObjectId datareader_id = uxr_object_id(0x01, UXR_DATAREADER_ID);
    const char* datareader_xml =
        "<dds>"
        "  <data_reader>"
        "    <topic>"
        "      <kind>NO_KEY</kind>"
        "      <name>SensorDataTopic</name>"
        "      <dataType>SensorData</dataType>"
        "    </topic>"
        "  </data_reader>"
        "</dds>";
    uint16_t datareader_req = uxr_buffer_create_datareader_xml(
        &session, reliable_out, datareader_id, subscriber_id, datareader_xml, UXR_REPLACE);

    // Auf Status-Antworten warten
    uint8_t status[4];
    uint16_t requests[4] = {participant_req, topic_req, subscriber_req, datareader_req};

    if (!uxr_run_session_until_all_status(&session, 1000, requests, status, 4))
    {
        printf("Fehler beim Erstellen der Entities:\n");
        printf("  Participant: %i\n", status[0]);
        printf("  Topic: %i\n", status[1]);
        printf("  Subscriber: %i\n", status[2]);
        printf("  DataReader: %i\n", status[3]);
        return 1;
    }

    // Daten-Request starten
    uxrDeliveryControl delivery_control = {0};
    delivery_control.max_samples = UXR_MAX_SAMPLES_UNLIMITED;
    uxr_buffer_request_data(&session, reliable_out, datareader_id, reliable_in, &delivery_control);

    printf("Subscriber2 initialisiert.\n");
    printf("Empfängt SensorData von SensorDataTopic...\n\n");

    // Endlos-Loop
    while (true)
    {
        uxr_run_session_time(&session, 1000);
    }

    // Aufräumen
    uxr_delete_session(&session);
    uxr_close_udp_transport(&transport);

    return 0;
}
