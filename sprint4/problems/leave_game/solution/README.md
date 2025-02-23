## Multithreaded game web-server

### Description

#### Game mechanics

The player controls a dog whose job is to find lost items and take them to the nearest lost property office. There are several dogs on the map at the same time, controlled by other users connected to the server.\
The goal of the game is to score as many game points as possible, which are awarded for items delivered to the lost property office.\
The start screen displays a list of maps. After selecting a map, the game screen opens. This screen displays a fragment of the city map. The map consists of roads along which the players' dogs move. Lost items appear on the roads from time to time, which must be picked up before the opponents and taken to the nearest lost property office. Player-controlled dogs can carry no more than three items at a time, so you need to visit the office regularly. In addition to roads and offices, there are buildings on the map that serve a purely decorative function and do not affect the course of the game.\
The information area of ​​the game screen displays the items that the player's dog is carrying, as well as the points scored.

#### Game architecture

The game is designed as a client-server application:\
The server is responsible for storing and updating the game state, sending the state to clients, and processing dog control commands. Placing the game code on the server side puts players on an equal footing and reduces the possibility of cheating by modifying the code running on the client side.
The client receives user input and sends it to the server. The client also receives the current game state from the server and displays it to the user. The client part of the game (frontend) is executed in a web browser and uses HTML/CSS/JavaScript technologies.

#### Build

The program was tested on Ubuntu 22.04.\
To compile, your system must have gcc 11.3 or later, python 3 installed.

Installing the required packages:

```Bash
sudo apt update && apt install -y python3-pip cmake && pip3 install conan==1.*
```
Установка пакетного менеджера Conan:

```Bash
mkdir /build && cd /build && conan install .. -s compiler.libcxx=libstdc++11
```

Сборка программы:

```Bash
cd /build && cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
```

Run

```Bash
cd /build
game_server [options]
```

```Bash
Allowed options:
  -h [ --help ]                     produce help message
  -t [ --tick-period ] milliseconds set tick period
  -c [ --config-file ] file         set config file path
  -w [ --www-root ] dir             set static files root
  --randomize-spawn-points          spawn dogs at random positions
```

* Параметр --tick-period (-t) задаёт период автоматического обновления игрового состояния в миллисекундах. Если этот параметр указан, каждые N миллисекунд сервер должен обновлять координаты объектов. Если этот параметр не указан, время в игре должно управляться с помощью запроса /api/v1/game/tick к REST API.
* Параметр --config-file (-c) задаёт путь к конфигурационному JSON-файлу игры.
* Параметр --www-root (-w) задаёт путь к каталогу со статическими файлами игры.
* Параметр --randomize-spawn-points включает режим, при котором пёс игрока появляется в случайной точке случайно выбранной дороги карты.
* Параметр --help (-h) должен выводить информацию о параметрах командной строки.

Для запуска игры на стороне клиента нужно в браузере ввести:
```
<hostname>:8080
```
