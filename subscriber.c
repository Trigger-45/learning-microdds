// subscriber.c - Non-blocking Subscriber, der Random JSON empfängt und sofort weiterleitet

#include "HelloWorld.h"
#include <uxr/client/client.h>
#include <ucdr/microcdr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#define STREAM_HISTORY  8
#define BUFFER_SIZE     UXR_CONFIG_UDP_TRANSPORT_MTU * STREAM_HISTORY
#define MAX_QUEUE       32
#define MSG_SIZE        1024

// Queue für empfangene Nachrichten
typedef struct {
    char messages[MAX_QUEUE][MSG_SIZE];
    int head;
    int tail;
} MessageQueue;

MessageQueue queue = { .head = 0, .tail = 0 };

// Callback: nur empfangen und in Queue legen
void on_topic(uxrSession* session, uxrObjectId object_id,
              uint16_t request_id, uxrStreamId stream_id,
              struct ucdrBuffer* ub, uint16_t length, void* args)
{
    (void) session; (void) object_id; (void) request_id; (void) stream_id; (void) length;
    (void) args;

    HelloWorld topic;
    HelloWorld_deserialize_topic(ub, &topic);

    // In Queue legen, falls Platz
    int next = (queue.tail + 1) % MAX_QUEUE;
    if (next != queue.head) {
        snprintf(queue.messages[queue.tail], MSG_SIZE, "%s", topic.message);
        queue.tail = next;
    }

    printf("[Subscriber] Empfangen: \"%s\" (ID: %d)\n", topic.message, topic.index);
}

// Funktion: Queue verarbeiten und Nachrichten auf Forward-Topic senden
void publish_from_queue(uxrSession* session, uxrStreamId reliable_out, uxrObjectId forward_datawriter_id)
{
    while (queue.head != queue.tail) {
        HelloWorld topic;
        topic.index = rand() % 100000;  // optional: neue ID
        snprintf(topic.message, sizeof(topic.message), "%s", queue.messages[queue.head]);

        ucdrBuffer ub;
        uint32_t topic_size = HelloWorld_size_of_topic(&topic, 0);
        uxr_prepare_output_stream(session, reliable_out, forward_datawriter_id, &ub, topic_size);
        HelloWorld_serialize_topic(&ub, &topic);

        uxr_run_session_time(session, 0);  // nur senden, nicht blockieren

        queue.head = (queue.head + 1) % MAX_QUEUE;
    }
}

int main(int argc, char** argv)
{
    char* ip = "127.0.0.1";
    char* port = "8888";

    if (argc >= 3) {
        ip = argv[1];
        port = argv[2];
    } else {
        printf("Verwendung: %s [ip] [port]\n", argv[0]);
        printf("Standard: %s:%s\n", ip, port);
    }

    srand((unsigned int)time(NULL));

    // UDP Transport
    uxrUDPTransport transport;
    if (!uxr_init_udp_transport(&transport, UXR_IPv4, ip, port)) {
        printf("Fehler beim Erstellen des UDP Transports.\n");
        return 1;
    }

    // Session
    uxrSession session;
    uxr_init_session(&session, &transport.comm, 0xCCCCDDDD);
    if (!uxr_create_session(&session)) {
        printf("Fehler beim Erstellen der Session.\n");
        return 1;
    }

    // Streams
    uint8_t output_reliable_stream_buffer[BUFFER_SIZE];
    uxrStreamId reliable_out = uxr_create_output_reliable_stream(&session, output_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);
    uint8_t input_reliable_stream_buffer[BUFFER_SIZE];
    uxrStreamId reliable_in = uxr_create_input_reliable_stream(&session, input_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);

    // Participant
    uxrObjectId participant_id = uxr_object_id(0x01, UXR_PARTICIPANT_ID);
    const char* participant_xml =
        "<dds>"
        "  <participant>"
        "    <rtps>"
        "      <name>ForwardingSubscriber</name>"
        "    </rtps>"
        "  </participant>"
        "</dds>";
    uint16_t participant_req = uxr_buffer_create_participant_xml(&session, reliable_out, participant_id, 0, participant_xml, UXR_REPLACE);

    // Eingangs-Topic
    uxrObjectId topic_id = uxr_object_id(0x01, UXR_TOPIC_ID);
    const char* topic_xml =
        "<dds>"
        "  <topic>"
        "    <name>RandomJSONTopic</name>"
        "    <dataType>HelloWorld</dataType>"
        "  </topic>"
        "</dds>";
    uint16_t topic_req = uxr_buffer_create_topic_xml(&session, reliable_out, topic_id, participant_id, topic_xml, UXR_REPLACE);

    // Subscriber + DataReader
    uxrObjectId subscriber_id = uxr_object_id(0x01, UXR_SUBSCRIBER_ID);
    const char* subscriber_xml = "";
    uint16_t subscriber_req = uxr_buffer_create_subscriber_xml(&session, reliable_out, subscriber_id, participant_id, subscriber_xml, UXR_REPLACE);

    uxrObjectId datareader_id = uxr_object_id(0x01, UXR_DATAREADER_ID);
    const char* datareader_xml =
        "<dds>"
        "  <data_reader>"
        "    <topic>"
        "      <kind>NO_KEY</kind>"
        "      <name>RandomJSONTopic</name>"
        "      <dataType>HelloWorld</dataType>"
        "    </topic>"
        "  </data_reader>"
        "</dds>";
    uint16_t datareader_req = uxr_buffer_create_datareader_xml(&session, reliable_out, datareader_id, subscriber_id, datareader_xml, UXR_REPLACE);

    // Forwarding-Topic: Publisher + Topic + DataWriter
    uxrObjectId forward_publisher_id = uxr_object_id(0x02, UXR_PUBLISHER_ID);
    const char* forward_publisher_xml = "";
    uint16_t forward_publisher_req = uxr_buffer_create_publisher_xml(&session, reliable_out, forward_publisher_id, participant_id, forward_publisher_xml, UXR_REPLACE);

    uxrObjectId forward_topic_id = uxr_object_id(0x02, UXR_TOPIC_ID);
    const char* forward_topic_xml =
        "<dds>"
        "  <topic>"
        "    <name>ForwardedTopic</name>"
        "    <dataType>HelloWorld</dataType>"
        "  </topic>"
        "</dds>";
    uint16_t forward_topic_req = uxr_buffer_create_topic_xml(&session, reliable_out, forward_topic_id, participant_id, forward_topic_xml, UXR_REPLACE);

    uxrObjectId forward_datawriter_id = uxr_object_id(0x02, UXR_DATAWRITER_ID);
    const char* forward_datawriter_xml =
        "<dds>"
        "  <data_writer>"
        "    <topic>"
        "      <kind>NO_KEY</kind>"
        "      <name>ForwardedTopic</name>"
        "      <dataType>HelloWorld</dataType>"
        "    </topic>"
        "  </data_writer>"
        "</dds>";
    uint16_t forward_datawriter_req = uxr_buffer_create_datawriter_xml(&session, reliable_out, forward_datawriter_id, forward_publisher_id, forward_datawriter_xml, UXR_REPLACE);

    // Status prüfen
    uint8_t status[10];
    uint16_t requests[10] = {
        participant_req, topic_req, subscriber_req, datareader_req,
        forward_publisher_req, forward_topic_req, forward_datawriter_req
    };
    if (!uxr_run_session_until_all_status(&session, 1000, requests, status, 7)) {
        printf("Fehler beim Erstellen der Entities.\n");
        return 1;
    }

    // Callback setzen
    uxr_set_topic_callback(&session, on_topic, NULL);

    // Daten-Request starten
    uxrDeliveryControl delivery_control = {0};
    delivery_control.max_samples = UXR_MAX_SAMPLES_UNLIMITED;
    uxr_buffer_request_data(&session, reliable_out, datareader_id, reliable_in, &delivery_control);

    printf("Subscriber initialisiert. Warte auf Messages und leite sie weiter...\n");

    // Endlos-Loop: Session ausführen & Queue verarbeiten
    while (true) {
        // Session ausführen: empfangen
        uxr_run_session_time(&session, 100);

        // Queue abarbeiten und forwarden
        publish_from_queue(&session, reliable_out, forward_datawriter_id);

        usleep(1000);  // kurze Pause für CPU-Entlastung
    }

    uxr_delete_session(&session);
    uxr_close_udp_transport(&transport);

    return 0;
}
