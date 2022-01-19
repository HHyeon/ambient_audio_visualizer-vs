
#include "visualworker.h"
#include <sstream>

visualworker::visualworker()
{
   for(int i=0;i<Window_WID;i++)
   {
      audio_L_spectrum_stream_value[i] = Vector2f((float)i, 0);
      audio_R_spectrum_stream_value[i] = Vector2f((float)i, 0);
   }
   audio_level_l_visualer_box.setSize(Vector2f((float)audio_level_box_size, 20));
   audio_level_l_visualer_box.setPosition(50,50);
   audio_level_l_visualer_box.setOutlineColor(Color::White);
   audio_level_l_visualer_box.setOutlineThickness(1.0);
   audio_level_l_visualer_box.setFillColor(Color::Black);
   audio_level_l_visualer.setPosition(50,50);
   audio_level_l_visualer.setFillColor(Color::White);

   audio_level_r_visualer_box.setSize(Vector2f((float)audio_level_box_size, 20));
   audio_level_r_visualer_box.setPosition(50,80);
   audio_level_r_visualer_box.setOutlineColor(Color::Yellow);
   audio_level_r_visualer_box.setOutlineThickness(1.0);
   audio_level_r_visualer_box.setFillColor(Color::Black);
   audio_level_r_visualer.setPosition(50,80);
   audio_level_r_visualer.setFillColor(Color::Yellow);

   buffer_usage_level_box.setSize(Vector2f((float)buffer_level_box_size, 20));
   buffer_usage_level_box.setPosition(50,120);
   buffer_usage_level_box.setOutlineColor(Color::White);
   buffer_usage_level_box.setOutlineThickness(1.0);
   buffer_usage_level_box.setFillColor(Color::Black);
   buffer_usage_level.setPosition(50,120);
   buffer_usage_level.setFillColor(Color::White);

   font.loadFromFile("AdobeGothicStd-Bold.otf");
   text_buffer_level_info.setFont(font);
   text_buffer_level_info.setCharacterSize(11);
   text_buffer_level_info.setFillColor(Color::White);
   text_buffer_level_info.setOutlineThickness(2.0);
   text_buffer_level_info.setOutlineColor(Color::Black);
   text_buffer_level_info.setPosition(Vector2f(90,123));

}

void visualworker::setSpeedDivider(int n)
{
   speeddivider = n;
}

void visualworker::update(queue<Int16> *audiodata_queue, Mutex* mutex)
{
   int queuesize = (int)audiodata_queue->size();
   if(queuesize >= Window_WID*2) // audiodata_queue => 2400
   {
      int speed = Window_WID/speeddivider;

      for(int i=0;i<Window_WID-1;i++)
      {
         int ylevel;
         if(i+speed < Window_WID-1) ylevel = (int)audio_L_spectrum_stream_value[i+speed].y;
         else ylevel = 0;
         audio_L_spectrum_stream_value[i] = Vector2f((float)i, (float)ylevel);
         if(i+speed < Window_WID-1) ylevel = (int)audio_R_spectrum_stream_value[i+speed].y;
         else ylevel = 0;
         audio_R_spectrum_stream_value[i] = Vector2f((float)i, (float)ylevel);
      }

      mutex->lock();

      audio_level_l = audio_level_r = 0;
      for(int i=0;i<speed;i++)
      {
         Int32 Lsample=0;
         Int32 Rsample=0;

         for(int n=0;n<speeddivider;n++)
         {
            Lsample += audiodata_queue->front();
            audiodata_queue->pop();
            Rsample += audiodata_queue->front();
            audiodata_queue->pop();
         }

         Lsample /= speeddivider;
         Rsample /= speeddivider;

         if(audio_level_l < abs(Lsample)) audio_level_l = abs(Lsample);
         if(audio_level_r < abs(Rsample)) audio_level_r = abs(Rsample);

         Int16 resampled = (Int16)((Lsample / 32768.0)*100.0);
         audio_L_spectrum_stream_value[Window_WID-speed+i-1] = Vector2f((float)Window_WID-speed+i-1, (float)resampled);
         resampled = (Int16)((Rsample / 32768.0)*100.0);
         audio_R_spectrum_stream_value[Window_WID-speed+i-1] = Vector2f((float)Window_WID-speed+i-1, (float)resampled);
      }

      mutex->unlock();

      audio_level_l /= 32768.0;
      audio_level_r /= 32768.0;

      if(audio_level_l > audio_level_l_fadeoff)
         audio_level_l_fadeoff = audio_level_l;
      if(audio_level_r > audio_level_r_fadeoff)
         audio_level_r_fadeoff = audio_level_r;
   }
   else
   {
      audio_level_l_fadeoff *= 0.9;
      audio_level_r_fadeoff *= 0.9;
   }
   audio_L_spectrum_vertexarray.clear();
   audio_L_spectrum_vertexarray.setPrimitiveType(LineStrip);
   audio_R_spectrum_vertexarray.clear();
   audio_R_spectrum_vertexarray.setPrimitiveType(LineStrip);
   for(int i=0;i<Window_WID;i++)
   {
      audio_L_spectrum_vertexarray.append(Vertex(audio_L_spectrum_stream_value[i]+Vector2f(0,300), Color::White));
      audio_R_spectrum_vertexarray.append(Vertex(audio_R_spectrum_stream_value[i]+Vector2f(0,300), Color::Yellow));
   }
   audio_level_l_visualer.setSize(Vector2f((float)audio_level_l_fadeoff*audio_level_box_size, 20));
   audio_level_r_visualer.setSize(Vector2f((float)audio_level_r_fadeoff*audio_level_box_size, 20));

   double queuelevel = (double)queuesize/44100.0;
   buffer_usage_level.setSize(Vector2f((float)queuelevel*buffer_level_box_size, 20));

   stringstream ss;
   ss << audiodata_queue->size();
   text_buffer_level_info.setString(ss.str());
}

void visualworker::validate(RenderWindow *window)
{
   window->draw(audio_R_spectrum_vertexarray);
   window->draw(audio_L_spectrum_vertexarray);
   window->draw(audio_level_l_visualer_box);
   window->draw(audio_level_r_visualer_box);
   window->draw(audio_level_l_visualer);
   window->draw(audio_level_r_visualer);
   window->draw(buffer_usage_level_box);
   window->draw(buffer_usage_level);
   window->draw(text_buffer_level_info);
}
