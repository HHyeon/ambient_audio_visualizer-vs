// ambient_audio_visualizer.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <SFML/Graphics.hpp>

int main()
{
    std::cout << "Hello World!\n";

    sf::RenderWindow window(sf::VideoMode(800, 600), "My window");
    window.setVerticalSyncEnabled(true);

    sf::Event sfevent;

    while (window.isOpen())
    {
        while (window.pollEvent(sfevent))
        {
            if (sfevent.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        window.display();
    }

    return 0;
}

