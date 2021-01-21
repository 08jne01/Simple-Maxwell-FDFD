#pragma once

class Field;
class MainConfig;

extern double overlap(const Field& field1, const int mode1, const Field& field2, const int mode2, const MainConfig& config);
extern bool outputFields(const Field& field, const int mode, const int width, const int height, const char* filename);