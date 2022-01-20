// ambient_audio_visualizer.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <sstream>
#include <queue>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include <complex>
#include <valarray>
#include <math.h>
#define PI 3.141592f

#include "visualworker.h"
#include "global.h"

#include <Windows.h>

using namespace sf;
using namespace std;

bool Terminate = false;

Mutex mutex;

queue<Int16> audiodata_queue;
queue<Int16> audiodata_queue_fft;
int speeddivider = 16;

#define fft_one_sample_size 2048
typedef complex<double> Complex;
typedef valarray<Complex> fft_one_sample;
fft_one_sample fftsample_l;
fft_one_sample fftsample_r;
valarray<double> bars_fadeoff_effect_ch_l;
valarray<double> bars_fadeoff_effect_ch_r;

void fft(fft_one_sample& binx)
{
    const int N = (int)binx.size();
    if (N <= 1) return;

    fft_one_sample even = binx[slice(0, N / 2, 2)];
    fft_one_sample  odd = binx[slice(1, N / 2, 2)];

    fft(even);
    fft(odd);

    for (int k = 0; k < N / 2; k++)
    {
        Complex t = polar((double)(1.0), (double)(-2 * PI * k / N)) * odd[k];
        binx[k] = even[k] + t;
        binx[k + N / 2] = even[k] - t;
    }
}

class SampleCaptureReCorder : public SoundRecorder
{
    virtual bool onStart()
    {
        return true;
    }

    virtual bool onProcessSamples(const Int16* samples, std::size_t sampleCount)
    {
        mutex.lock();
        if (audiodata_queue.size() + sampleCount < 44100)
            for (int i = 0; i < (int)sampleCount; i++)
                audiodata_queue.push(samples[i]);
        else
            cout << "dropped " << sampleCount << endl;

        if (audiodata_queue_fft.size() + sampleCount < 44100)
            for (int i = 0; i < (int)sampleCount; i++)
                audiodata_queue_fft.push(samples[i]);
        else
            cout << "fftqueuedropped " << sampleCount << endl;

        mutex.unlock();
        return true; // false to stop it
    }

    virtual void onStop()
    {
    }
};

char* UTF8ToANSI(string pszCode)
{
    BSTR    bstrWide;
    char* pszAnsi;
    int     nLength;

    wchar_t wtext[1024];

    size_t ConvertedChars = 0;
    mbstowcs_s(&ConvertedChars, wtext, pszCode.c_str(), pszCode.length());//includes null
    LPWSTR ptr = wtext;

    nLength = MultiByteToWideChar(CP_UTF8, 0, pszCode.c_str(), lstrlen(ptr) + 1, NULL, NULL);
    bstrWide = SysAllocStringLen(NULL, nLength);

    MultiByteToWideChar(CP_UTF8, 0, pszCode.c_str(), lstrlen(ptr) + 1, bstrWide, nLength);

    nLength = WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, NULL, 0, NULL, NULL);
    pszAnsi = new char[nLength];

    WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, pszAnsi, nLength, NULL, NULL);
    SysFreeString(bstrWide);

    return pszAnsi;
}

int main()
{
    RenderWindow window(VideoMode(Window_WID, Window_HEI), "ambient_record", Style::None | Style::Titlebar);
    window.setFramerateLimit(60);

    string selected_device;
    vector<string> availableDevices = SoundRecorder::getAvailableDevices();

    if (availableDevices.size() == 0)
    {
        cout << "Device unAvailable\n";
        return 1;
    }

    for (int i = 0; i < (int)availableDevices.size(); i++)
    {
        stringstream ss;
        ss << i << ". " << availableDevices.at(i) << endl;

        cout << UTF8ToANSI(ss.str());
        //Output(ss.str());
    }

    selected_device = availableDevices.at(0);

    cout << UTF8ToANSI(selected_device) << endl;

    SampleCaptureReCorder recorder;
    if (!recorder.setDevice(selected_device))
    {
        cout << "record Device select failed\n";
        return 1;
    }
    recorder.setChannelCount(2);
    recorder.start();

    visualworker visualcharge;
    fftsample_l.resize(fft_one_sample_size);
    fftsample_r.resize(fft_one_sample_size);

    bars_fadeoff_effect_ch_l.resize(1000);
    bars_fadeoff_effect_ch_r.resize(1000);

    Font ft;
    Text txt;
    ft.loadFromFile("AdobeGothicStd-Bold.otf");
    txt.setFont(ft);
    txt.setCharacterSize(11);
    txt.setFillColor(Color::White);
    txt.setOutlineThickness(1.0);
    txt.setOutlineColor(Color::Black);
    txt.setPosition(Vector2f(90, 403));

    RectangleShape levelbox, levelbox_level;
    levelbox.setSize(Vector2f(100, 20));
    levelbox.setPosition(50, 400);
    levelbox.setOutlineColor(Color::White);
    levelbox.setOutlineThickness(1.0);
    levelbox.setFillColor(Color::Black);
    levelbox_level.setPosition(50, 400);
    levelbox_level.setFillColor(Color::White);

    VertexArray spectrum_data_ch_l;
    VertexArray spectrum_data_ch_r;
    spectrum_data_ch_l.setPrimitiveType(Lines);
    spectrum_data_ch_r.setPrimitiveType(Lines);

    Clock clock;
    Event event;
    while (window.isOpen())
    {
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                window.close();
            }
            else if (event.type == Event::KeyPressed)
            {
                if (event.key.code == Keyboard::Q)
                    window.close();
            }
            else if (event.type == Event::MouseWheelScrolled)
            {
                if (event.mouseWheelScroll.delta < 0)
                    if (speeddivider < 64) speeddivider++; else;
                else
                    if (speeddivider > 2) speeddivider--; else;

                visualcharge.setSpeedDivider(speeddivider);
            }
        }

        visualcharge.update(&audiodata_queue, &mutex);




        if (audiodata_queue_fft.size() > fft_one_sample_size * 2)
        {
            mutex.lock();
            for (int i = 0; i < fft_one_sample_size; i++)
            {
                fftsample_l[i] = Complex(audiodata_queue_fft.front(), 0);
                audiodata_queue_fft.pop();
                fftsample_r[i] = Complex(audiodata_queue_fft.front(), 0);
                audiodata_queue_fft.pop();

            }
            mutex.unlock();

            int queuesize = (int)audiodata_queue_fft.size();
            double queuelevel = (double)queuesize / 44100.0;
            levelbox_level.setSize(Vector2f((float)(queuelevel * 100), 20));

            stringstream ss;
            ss << queuesize;
            txt.setString(ss.str());

            fft(fftsample_l);
            fft(fftsample_r);

            spectrum_data_ch_l.clear();
            spectrum_data_ch_l.setPrimitiveType(Lines);
            spectrum_data_ch_r.clear();
            spectrum_data_ch_r.setPrimitiveType(Lines);

            float max = 100000000;
            int yheightmax = 100;
            double multiplier = 5.0;
            int xpush = 100;
            int ypush = 500;
            int SecuredSamples = 1000; // about 1000 samples is Suitable counts at about 1000 samples a channel

            int barxpos = 0;
            int visiblesamples_from = 0;
            int visiblesamples_to = 100;
            int visiblesamples_cnt = (visiblesamples_to - visiblesamples_from);

            if (visiblesamples_cnt > SecuredSamples) visiblesamples_cnt = SecuredSamples;
            int barinterval = SecuredSamples / visiblesamples_cnt;

            for (int i = visiblesamples_from; i < visiblesamples_to; i++)
            {
                int sample_l = (int)((abs(fftsample_l[i]) / max * multiplier) * yheightmax);
                if (sample_l > yheightmax) sample_l = yheightmax;
                if (sample_l <= 5) sample_l = 0;
                if (sample_l > bars_fadeoff_effect_ch_l[barxpos]) bars_fadeoff_effect_ch_l[barxpos] = sample_l;
                else bars_fadeoff_effect_ch_l[barxpos] *= 0.5;
                spectrum_data_ch_l.append(Vertex(Vector2f((float)((barxpos * barinterval) + xpush), (float)(ypush + bars_fadeoff_effect_ch_l[barxpos])), Color::White));
                spectrum_data_ch_l.append(Vertex(Vector2f((float)((barxpos * barinterval) + xpush), (float)(ypush - bars_fadeoff_effect_ch_l[barxpos])), Color::White));

                int sample_r = (int)((abs(fftsample_r[i]) / max * multiplier) * yheightmax);
                if (sample_r > yheightmax) sample_r = yheightmax;
                if (sample_r <= 5) sample_r = 0;
                if (sample_r > bars_fadeoff_effect_ch_r[barxpos]) bars_fadeoff_effect_ch_r[barxpos] = sample_r;
                else bars_fadeoff_effect_ch_r[barxpos] *= 0.5;
                spectrum_data_ch_r.append(Vertex(Vector2f((float)((barxpos * barinterval) + xpush), (float)(ypush + bars_fadeoff_effect_ch_r[barxpos])), Color::Yellow));
                spectrum_data_ch_r.append(Vertex(Vector2f((float)((barxpos * barinterval) + xpush), (float)(ypush - bars_fadeoff_effect_ch_r[barxpos])), Color::Yellow));



                if (barxpos % 100 == 0)
                {
                    spectrum_data_ch_r.append(Vertex(Vector2f((float)(barxpos * barinterval) + xpush, (float)ypush - 30), Color::Green));
                    spectrum_data_ch_r.append(Vertex(Vector2f((float)(barxpos * barinterval) + xpush, (float)ypush + 30), Color::Green));
                }
                else
                {
                    if (barxpos % 10 == 0)
                    {
                        spectrum_data_ch_r.append(Vertex(Vector2f((float)(barxpos * barinterval) + xpush, (float)ypush - 20), Color::Red));
                        spectrum_data_ch_r.append(Vertex(Vector2f((float)(barxpos * barinterval) + xpush, (float)ypush + 20), Color::Red));
                    }
                }
                barxpos++;
            }

            spectrum_data_ch_r.append(Vertex(Vector2f((float)(barxpos * barinterval) + xpush, (float)ypush - 30), Color::Green));
            spectrum_data_ch_r.append(Vertex(Vector2f((float)(barxpos * barinterval) + xpush, (float)ypush + 30), Color::Green));
        }

        window.clear();
        visualcharge.validate(&window);
        window.draw(spectrum_data_ch_r);
        window.draw(spectrum_data_ch_l);
        window.draw(levelbox);
        window.draw(levelbox_level);
        window.draw(txt);
        window.display();
    }

    recorder.stop();

    return 0;
}

