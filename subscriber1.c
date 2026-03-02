// subscriber1_transformer.c - Transformer: HelloWorld → SensorData
// Empfängt JSON, parst es, und publiziert als strukturierte Daten

#include "HelloWorld.h"      // Für Input
#include "SensorData.h"      // Für Output
#include <uxr/client/client.h>
#include <ucdr/microcdr.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define STREAM_HISTORY  8
#define BUFFER_SIZE     UXR_CONFIG_UDP_TRANSPORT_MTU * STREAM_HISTORY

// ============================================================
// EINFACHER JSON-PARSER
// ============================================================

int json_get_int(const char* json, const char* key)
{
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":", key);
    const char* pos = strstr(json, search);
    if (pos)
    {
        pos += strlen(search);
        while (*pos == ' ') pos++;
        return atoi(pos);
    }
    return 0;
}

float json_get_float(const char* json, const char* key)
{
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":", key);
    const char* pos = strstr(json, search);
    if (pos)
    {
        pos += strlen(search);
        while (*pos == ' ') pos++;
        return (float)atof(pos);
    }
    return 0.0f;
}

void json_get_string(const char* json, const char* key, char* output, size_t max_len)
{
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":", key);
    const char* pos = strstr(json, search);
    if (pos)
    {
        pos += strlen(search);
        while (*pos == ' ') pos++;
        if (*pos == '"')
        {
            pos++;
            const char* end = strchr(pos, '"');
            if (end)
            {
                size_t len = end - pos;
                if (len >= max_len) len = max_len - 1;
                strncpy(output, pos, len);
                output[len] = '\0';
                return;
            }
        }
    }
    output[0] = '\0';
}

// ============================================================
// GLOBALE VARIABLEN FÜR PUBLISHING
// ============================================================

uxrSession* g_session = NULL;
uxrStreamId g_reliable_out;
uxrObjectId g_datawriter_id;

// ============================================================
// CALLBACK: Empfängt HelloWorld, transformiert zu SensorData
// ============================================================

void on_helloworld_received(
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
    (void) args;

    // === SCHRITT 1: HelloWorld empfangen ===
    HelloWorld hello_topic;
    HelloWorld_deserialize_topic(ub, &hello_topic);

    printf("\n[Subscriber1] Empfangen (HelloWorld):\n");
    printf("  Index: %d\n", hello_topic.index);
    printf("  JSON: %s\n", hello_topic.message);

    // === SCHRITT 2: JSON parsen ===
    SensorData sensor_topic;
    
    sensor_topic.index = (uint32_t)json_get_int(hello_topic.message, "index");
    sensor_topic.temp = json_get_float(hello_topic.message, "temp");
    json_get_string(hello_topic.message, "einheit", sensor_topic.einheit, sizeof(sensor_topic.einheit));
    json_get_string(hello_topic.message, "date", sensor_topic.date, sizeof(sensor_topic.date));
    json_get_string(hello_topic.message, "daten", sensor_topic.daten, sizeof(sensor_topic.daten));

    printf("[Subscriber1] Geparst zu SensorData:\n");
    printf("  Index: %d\n", sensor_topic.index);
    printf("  Temp: %.2f %s\n", sensor_topic.temp, sensor_topic.einheit);
    printf("  Date: %s\n", sensor_topic.date);
    printf("  Daten: %s\n", sensor_topic.daten);

    // === SCHRITT 3: Als SensorData weiterleiten ===
    ucdrBuffer out_ub;
    uint32_t topic_size = SensorData_size_of_topic(&sensor_topic, 0);
    uxr_prepare_output_stream(g_session, g_reliable_out, g_datawriter_id, &out_ub, topic_size);
    SensorData_serialize_topic(&out_ub, &sensor_topic);

    uxr_run_session_time(g_session, 0);  // Non-blocking send

    printf("[Subscriber1] ✓ Weitergeleitet als SensorData\n");
}

// ============================================================
// MAIN
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
    else
    {
        printf("Verwendung: %s [ip] [port]\n", argv[0]);
        printf("Standard: %s:%s\n", ip, port);
    }

    // UDP Transport & Session
    uxrUDPTransport transport;
    if (!uxr_init_udp_transport(&transport, UXR_IPv4, ip, port))
    {
        printf("Fehler beim Erstellen des UDP Transports.\n");
        return 1;
    }

    uxrSession session;
    uxr_init_session(&session, &transport.comm, 0xBBBBCCCC);
    uxr_set_topic_callback(&session, on_helloworld_received, NULL);

    if (!uxr_create_session(&session))
    {
        printf("Fehler beim Erstellen der Session.\n");
        return 1;
    }

    g_session = &session;

    uint8_t output_reliable_stream_buffer[BUFFER_SIZE];
    g_reliable_out = uxr_create_output_reliable_stream(
        &session, output_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);

    uint8_t input_reliable_stream_buffer[BUFFER_SIZE];
    uxrStreamId reliable_in = uxr_create_input_reliable_stream(
        &session, input_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);

    // Participant
    uxrObjectId participant_id = uxr_object_id(0x01, UXR_PARTICIPANT_ID);
    const char* participant_xml =
        "<dds>"
        "  <participant>"
        "    <rtps>"
        "      <name>Transformer_Subscriber1</name>"
        "    </rtps>"
        "  </participant>"
        "</dds>";
    uint16_t participant_req = uxr_buffer_create_participant_xml(
        &session, g_reliable_out, participant_id, 0, participant_xml, UXR_REPLACE);

    // INPUT: Topic + Subscriber + DataReader (HelloWorld)
    uxrObjectId input_topic_id = uxr_object_id(0x01, UXR_TOPIC_ID);
    const char* input_topic_xml =
        "<dds>"
        "  <topic>"
        "    <name>HelloWorldTopic</name>"
        "    <dataType>HelloWorld</dataType>"
        "  </topic>"
        "</dds>";
    uint16_t input_topic_req = uxr_buffer_create_topic_xml(
        &session, g_reliable_out, input_topic_id, participant_id, input_topic_xml, UXR_REPLACE);

    uxrObjectId subscriber_id = uxr_object_id(0x01, UXR_SUBSCRIBER_ID);
    const char* subscriber_xml = "";
    uint16_t subscriber_req = uxr_buffer_create_subscriber_xml(
        &session, g_reliable_out, subscriber_id, participant_id, subscriber_xml, UXR_REPLACE);

    uxrObjectId datareader_id = uxr_object_id(0x01, UXR_DATAREADER_ID);
    const char* datareader_xml =
        "<dds>"
        "  <data_reader>"
        "    <topic>"
        "      <kind>NO_KEY</kind>"
        "      <name>HelloWorldTopic</name>"
        "      <dataType>HelloWorld</dataType>"
        "    </topic>"
        "  </data_reader>"
        "</dds>";
    uint16_t datareader_req = uxr_buffer_create_datareader_xml(
        &session, g_reliable_out, datareader_id, subscriber_id, datareader_xml, UXR_REPLACE);

    // OUTPUT: Topic + Publisher + DataWriter (SensorData)
    uxrObjectId output_topic_id = uxr_object_id(0x02, UXR_TOPIC_ID);
    const char* output_topic_xml =
        "<dds>"
        "  <topic>"
        "    <name>SensorDataTopic</name>"
        "    <dataType>SensorData</dataType>"
        "  </topic>"
        "</dds>";
    uint16_t output_topic_req = uxr_buffer_create_topic_xml(
        &session, g_reliable_out, output_topic_id, participant_id, output_topic_xml, UXR_REPLACE);

    uxrObjectId publisher_id = uxr_object_id(0x01, UXR_PUBLISHER_ID);
    const char* publisher_xml = "";
    uint16_t publisher_req = uxr_buffer_create_publisher_xml(
        &session, g_reliable_out, publisher_id, participant_id, publisher_xml, UXR_REPLACE);

    g_datawriter_id = uxr_object_id(0x01, UXR_DATAWRITER_ID);
    const char* datawriter_xml =
        "<dds>"
        "  <data_writer>"
        "    <topic>"
        "      <kind>NO_KEY</kind>"
        "      <name>SensorDataTopic</name>"
        "      <dataType>SensorData</dataType>"
        "    </topic>"
        "  </data_writer>"
        "</dds>";
    uint16_t datawriter_req = uxr_buffer_create_datawriter_xml(
        &session, g_reliable_out, g_datawriter_id, publisher_id, datawriter_xml, UXR_REPLACE);

    // Status prüfen
    uint8_t status[7];
    uint16_t requests[7] = {
        participant_req, 
        input_topic_req, subscriber_req, datareader_req,
        output_topic_req, publisher_req, datawriter_req
    };

    if (!uxr_run_session_until_all_status(&session, 1000, requests, status, 7))
    {
        printf("Fehler beim Erstellen der Entities.\n");
        return 1;
    }

    // Daten-Request starten
    uxrDeliveryControl delivery_control = {0};
    delivery_control.max_samples = UXR_MAX_SAMPLES_UNLIMITED;
    uxr_buffer_request_data(&session, g_reliable_out, datareader_id, reliable_in, &delivery_control);

    printf("Subscriber1 (Transformer) initialisiert.\n");
    printf("  Empfängt: HelloWorldTopic (HelloWorld)\n");
    printf("  Sendet: SensorDataTopic (SensorData)\n\n");

    // Endlos-Loop
    while (true)
    {
        uxr_run_session_time(&session, 1000);
    }

    uxr_delete_session(&session);
    uxr_close_udp_transport(&transport);

    return 0;
}
