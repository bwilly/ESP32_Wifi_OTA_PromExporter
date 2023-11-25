#pragma once
#include <Arduino.h>

// // Declaration for paramList
// extern std::vector<ParamMetadata> paramList;

// In ParamMetadata.h
struct ParamMetadata
{
    String name;
    String spiffsPath;
    enum Type
    {
        STRING,
        NUMBER,
        BOOLEAN
    } type;
};
