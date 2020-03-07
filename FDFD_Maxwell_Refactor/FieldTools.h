#pragma once

class Field;
class MainConfig;

extern double overlap(const Field& field1, const int mode1, const Field& field2, const int mode2, const MainConfig& config);
extern bool writeFields(const Field& field, const char* filename);