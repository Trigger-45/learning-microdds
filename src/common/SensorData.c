#include "SensorData.h"
#include <ucdr/microcdr.h>
#include <string.h>

bool SensorData_serialize_topic(
        ucdrBuffer* writer,
        const SensorData* topic)
{
    (void) ucdr_serialize_uint32_t(writer, topic->index);
    
    (void) ucdr_serialize_float(writer, topic->temp);
    
    (void) ucdr_serialize_string(writer, topic->einheit);
    
    (void) ucdr_serialize_string(writer, topic->date);
    
    (void) ucdr_serialize_string(writer, topic->daten);

    return !writer->error;
}

bool SensorData_deserialize_topic(
        ucdrBuffer* reader,
        SensorData* topic)
{
    (void) ucdr_deserialize_uint32_t(reader, &topic->index);
    
    (void) ucdr_deserialize_float(reader, &topic->temp);
    
    (void) ucdr_deserialize_string(reader, topic->einheit, 50);
    
    (void) ucdr_deserialize_string(reader, topic->date, 50);
    
    (void) ucdr_deserialize_string(reader, topic->daten, 255);

    return !reader->error;
}

uint32_t SensorData_size_of_topic(
        const SensorData* topic,
        uint32_t size)
{
    uint32_t previousSize = size;
    
    size += (uint32_t)(ucdr_alignment(size, 4) + 4);
    
    size += (uint32_t)(ucdr_alignment(size, 4) + 4);
    
    size += (uint32_t)(ucdr_alignment(size, 4) + 4 + strlen(topic->einheit) + 1);
    
    size += (uint32_t)(ucdr_alignment(size, 4) + 4 + strlen(topic->date) + 1);
    
    size += (uint32_t)(ucdr_alignment(size, 4) + 4 + strlen(topic->daten) + 1);

    return size - previousSize;
}
