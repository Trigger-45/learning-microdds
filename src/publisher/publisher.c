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

void publish_message(uxrSession* session, uxrStreamId reliable_out, uxrObjectId datawriter_id, const char* message)
{
    HelloWorld topic;
    topic.index = rand() % 1000; 
    snprintf(topic.message, sizeof(topic.message), "%s", message);

    ucdrBuffer ub;
    uint32_t topic_size = HelloWorld_size_of_topic(&topic, 0);
    uxr_prepare_output_stream(session, reliable_out, datawriter_id, &ub, topic_size);
    HelloWorld_serialize_topic(&ub, &topic);

    printf("[Publisher] Sent: %s\n", message);

    uxr_run_session_time(session, 100);
}

void generate_random_json(uxrSession* session, uxrStreamId reliable_out, uxrObjectId datawriter_id)
{
    char buffer[256];

    while (true)
    {
        int id = rand() % 1000;
        double value = ((double)rand() / RAND_MAX) * 100.0;
        int flag = rand() % 2;

        snprintf(buffer, sizeof(buffer),
                 "{ \"id\": %d, \"value\": %.2f, \"flag\": %s }",
                 id, value, flag ? "true" : "false");


        publish_message(session, reliable_out, datawriter_id, buffer);

        sleep(1);  
    }
}

int main()
{

    char* ip = "127.0.0.1";
    char* port = "8888";

    srand((unsigned int)time(NULL)); 


    uxrUDPTransport transport;
    if (!uxr_init_udp_transport(&transport, UXR_IPv4, ip, port))
    {
        printf("Error creating UDP transport.\n");
        return 1;
    }


    uxrSession session;
    uxr_init_session(&session, &transport.comm, 0xAAAABBBB);
    if (!uxr_create_session(&session))
    {
        printf("Error creating session.\n");
        return 1;
    }

    uint8_t output_reliable_stream_buffer[BUFFER_SIZE];
    uxrStreamId reliable_out = uxr_create_output_reliable_stream(
        &session, output_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);

    uint8_t input_reliable_stream_buffer[BUFFER_SIZE];
    uxr_create_input_reliable_stream(
        &session, input_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);

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


    uxrObjectId publisher_id = uxr_object_id(0x01, UXR_PUBLISHER_ID);
    const char* publisher_xml = "";
    uint16_t publisher_req = uxr_buffer_create_publisher_xml(
        &session, reliable_out, publisher_id, participant_id, publisher_xml, UXR_REPLACE);


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

    uint8_t status[4];
    uint16_t requests[4] = {participant_req, topic_req, publisher_req, datawriter_req};
    
    if (!uxr_run_session_until_all_status(&session, 1000, requests, status, 4))
    {
        printf("Error creating DDS entities:\n");
        printf("  Participant: %i\n", status[0]);
        printf("  Topic: %i\n", status[1]);
        printf("  Publisher: %i\n", status[2]);
        printf("  DataWriter: %i\n", status[3]);
        return 1;
    }

    printf("Publisher initialized. Starting continuous transmission of JSON data...\n");

    generate_random_json(&session, reliable_out, datawriter_id);

    uxr_delete_session(&session);
    uxr_close_udp_transport(&transport);

    return 0;
}
