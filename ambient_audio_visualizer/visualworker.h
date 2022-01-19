#pragma once

#include <iostream>
#include <queue>
#include <SFML/Graphics.hpp>
#include <array>

#include "global.h"

using namespace std;
using namespace sf;

class visualworker
{
public:
   visualworker();
   void update(queue<Int16> *audiodata_queue, Mutex *mutex);
   void validate(RenderWindow *window);
   void setSpeedDivider(int n);
private:
   VertexArray audio_L_spectrum_vertexarray;
   array<Vector2f, Window_WID> audio_L_spectrum_stream_value;
   VertexArray audio_R_spectrum_vertexarray;
   array<Vector2f, Window_WID> audio_R_spectrum_stream_value;

   Uint32 audio_level_box_size = 200;
   Uint32 buffer_level_box_size = 100;

   RectangleShape audio_level_l_visualer_box;
   RectangleShape audio_level_l_visualer;

   RectangleShape audio_level_r_visualer_box;
   RectangleShape audio_level_r_visualer;

   RectangleShape buffer_usage_level_box;
   RectangleShape buffer_usage_level;

   Font font;
   Text text_buffer_level_info;

   double audio_level_l = 0;
   double audio_level_r = 0;
   double audio_level_l_fadeoff = 0;
   double audio_level_r_fadeoff = 0;
   int speeddivider = 16;
};

